
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <vector>
#include <math.h>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "commandsString.h"
#include "commandsMath.h"
#include "commandsBanks.h"
#include "commandsDisc.h"
#include "commandsErrors.h"
#include "debug.h"
#include "errors.h"
#include "pass1.h"
#include "init.h"
#include "cleanup.h"

char *var_param_str = NULL;
int var_param_num;
double var_param_decimal;

char *_file_start_ = NULL;
char *_file_end_ = NULL;

int cmdStack = 0;
int procStackCount = 0;
unsigned short last_token = 0;
int last_var = 0;
int tokenlength;

char *data_read_pointer = NULL;

char *(*jump_mode) (struct reference *ref, char *ptr) = NULL;
void (*do_input) ( struct nativeCommand *, char * ) = NULL;
void (*do_breakdata) ( struct nativeCommand *, char * ) = NULL;

int tokenMode = mode_standard;
void _num( int num );

struct proc procStack[1000];	// 0 is not used.
struct globalVar globalVars[1000];	// 0 is not used.
struct kittyBank kittyBanks[16];
struct kittyFile kittyFiles[10];

int globalVarsSize = sizeof(globalVars)/sizeof(struct globalVar);

std::vector<struct label> labels;	// 0 is not used.

int global_var_count = 0;
int labels_count = 0;

struct glueCommands cmdTmp[100];	
struct glueCommands input_cmd_context;

extern char *nextToken_pass1( char *ptr, unsigned short token );

char *cmdRem(nativeCommand *cmd, char *ptr)
{
	int length = *((short *) ptr);
	return ptr + length;
}


char *nextCmd(nativeCommand *cmd, char *ptr)
{
	char *ret = NULL;

	// we should empty stack, until first/normal command is not a parm command.

	while ((cmdStack) && (cmdTmp[cmdStack-1].flag != cmd_loop ))
	{
		ret = cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

		if (cmdTmp[cmdStack].flag == cmd_first) break;
		if (ret) break;
	}

	tokenMode = mode_standard;

	if (ret) ptr = ret - 2;
	return ptr;
}

char *cmdNewLine(nativeCommand *cmd, char *ptr)
{
	char *ret = NULL;

	while ((cmdStack) && (cmdTmp[cmdStack-1].flag != cmd_loop ))
	{
		ret = cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
		if (ret) break;
	}

	tokenMode = mode_standard;
	currentLine ++;

	if (ret) ptr = ret - 2;

	return ptr;
}


char *_array_index_var( glueCommands *self )
{
	int tmp_cells;
	int varNum;
	int n = 0;
	int mul;
	int index;
	struct kittyData *var = NULL;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	tmp_cells = stack - self -> stack;

	varNum = self -> lastVar;

	if (varNum == 0) return NULL;

	last_var = varNum;		// this is used when a array is set. array[var]=0, it restores last_var to array, not var

	if (varNum) var = &globalVars[varNum-1].var;

	if (var)
	{
		if ( (var -> type & type_array)  == 0)
		{
			popStack(tmp_cells);
			setError(27);		// var is not a array
			return 0;
		}

		index = 0; mul  = 1;
		for (n = self -> stack+1;n<=stack; n++ )
		{
			index += (mul * kittyStack[n].value);
			mul *= var -> sizeTab[n- self -> stack -1];
		}

		var -> index = index;

		popStack(tmp_cells);

		if ((index >= 0)  && (index<var->count))
		{
			// we over write it stack.
			if (kittyStack[n].str) free(kittyStack[n].str);
			kittyStack[n].str = NULL;

			// change stack
			switch (var -> type & 7)
			{
				case type_int:

					kittyStack[stack].type = (var -> type & 7);
					kittyStack[stack].value = var -> int_array[index];
					break;

				case type_float:

					kittyStack[stack].type = (var -> type & 7);
					kittyStack[stack].decimal = var -> float_array[index];
					break;

				case type_string:

					char *str = var -> str_array[index];
					kittyStack[stack].type = (var -> type & 7);
					kittyStack[stack].str = str ? strdup( str ) : strdup("") ;
					kittyStack[stack].len = str ? strlen( str ) : 0 ;
					break;
			}
		}
	}

	return NULL;
}

char *_alloc_mode_off( glueCommands *self )
{
	int size = 0;
	int n;
	int varNum;
	int count;
	struct kittyData *var;

	tokenMode = mode_standard;	

	varNum = *((unsigned short *) (self -> tokenBuffer + 2));

	if (varNum == 0) return NULL;

	var = &globalVars[varNum-1].var;

	var -> cells = stack - self -> stack;
	var -> sizeTab = (int *) malloc( sizeof(int) * var -> cells );

	for (n= 0; n<var -> cells; n++ ) 
	{
		var -> sizeTab[n] = kittyStack[self -> stack + n].value + 1;
	}

	var -> count =  kittyStack[stack].value +1 ;
	for (n= 1; n<var -> cells;n++) var -> count *= var -> sizeTab[n];

	switch (var -> type)
	{
		case type_int:
				size = var -> count * sizeof(int);
				var -> int_array = (int *) malloc( size ) ;
				break;
		case type_float:
				size = var -> count * sizeof(double);
				var -> float_array = (double *) malloc( size ) ;
				break;
		case type_string:
				size = var -> count * sizeof(char *);
				var -> str_array = (char **) malloc( size ) ;
				break;
	}

	memset( var -> str, 0, size );	// str is a union :-)

//	printf("name %s, cells %d, size %d, sizeTab %08x\n", globalVars[varNum].varName, 	var -> cells,var -> count,var ->sizeTab );

	var -> type |= type_array; 	

	stack -=  var -> cells;	// should use garbage collector here ;-) memory leaks works to for now.

	dump_global();

	return NULL;
}

char *cmdDim(nativeCommand *cmd, char *ptr)
{
	tokenMode = mode_alloc;
	stackCmdNormal( _alloc_mode_off, ptr );
	return ptr;
}

// nothing to do here, just jump over label name.
char *cmdLabelOnLine(nativeCommand *cmd, char *ptr)
{
	struct reference *ref = (struct reference *) ptr;
	return ptr + ref -> length ;
}

extern char *findLabel( char *name );

char *jump_mode_goto (struct reference *ref, char *ptr) 
{
	if (ref -> flags == 0)
	{
		char *name = strndup( ptr + sizeof(struct reference), ref->length );
		char *newLocation;

		if (name)
		{
			newLocation = findLabel(name);
			if (newLocation)
			{
				if (ref->length>=4)
				{
					ref -> flags = 255;
					*((char **) (ptr + sizeof(struct reference))) = newLocation - sizeof(struct reference)  ;
				}

				free(name);
				return newLocation - sizeof(struct reference)  ;
			}
			free(name);
		}
	} else if ( ref -> flags == 255 )	// accelerated, we don't give fuck about the name of variable
	{
		return *((char **) (ptr + sizeof(struct reference))) ;
	}

	stack++;

	return ptr;
}

char *jump_mode_gosub (struct reference *ref, char *ptr) 
{
	stackCmdLoop( _gosub, ptr );

	if (ref -> flags == 0)
	{
		char *name = strndup( ptr + sizeof(struct reference), ref->length );
		char *newLocation;

		if (name)
		{
			newLocation = findLabel(name);
			if (newLocation)
			{
				if (ref->length>=4)
				{
					ref -> flags = 255;
					*((char **) (ptr + sizeof(struct reference))) = newLocation - sizeof(struct reference)  ;
				}

				free(name);
				return newLocation - sizeof(struct reference)  ;
			}
			free(name);
		}
	} else if ( ref -> flags == 255 )	// accelerated, we don't give fuck about the name of variable
	{
		return *((char **) (ptr + sizeof(struct reference))) ;
	}

	stack++;

	return ptr;
}


char *cmdVar(nativeCommand *cmd, char *ptr)
{
	struct reference *ref = (struct reference *) ptr;
	unsigned short next_token = *((short *) (ptr+sizeof(struct reference)+ref->length));
	struct kittyData *var;
	
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ( correct_order( last_token,  next_token ) == false )
	{
		printf("---hidden ( symbol \n");

		// hidden ( condition.
		kittyStack[stack].str = NULL;
		kittyStack[stack].value = 0;
		kittyStack[stack].state = state_hidden_subData;
		stack++;
	}

	if (jump_mode)
	{
		printf("jump_mode\n");
		return jump_mode( ref, ptr );
	}

	last_var = ref -> ref;


	printf("----- stack %d\n",stack);

	if (next_token == 0x0074)	// ( symbol
	{
		printf("#going this path\n");

		if (tokenMode != mode_alloc)
		{
			stackCmdIndex( _array_index_var, ptr );
		}
	}
	else
	{
		if (ref -> ref)
		{
			int idx = ref->ref-1;

			switch (globalVars[idx].var.type & 7)
			{
				case type_int:
					_num(globalVars[idx].var.value);
					break;
				case type_float:
					setStackDecimal(globalVars[idx].var.decimal);
					break;
				case type_string:
					setStackStrDup(globalVars[idx].var.str);		// always copy.
					break;
				case type_proc:
					stackCmdLoop( _procedure, ptr+sizeof(struct reference)+ref->length ) ;
					return globalVars[idx].var.tokenBufferPos ;					
			}
		}
	}


	if (cmdStack) if (stack)
	{
		char *newTokenLoc = NULL;

		if (kittyStack[stack-1].state == state_none) if (cmdTmp[cmdStack-1].flag == cmd_para ) newTokenLoc =cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
		if (newTokenLoc) return newTokenLoc - sizeof(struct reference) - 2;
	}

	return ptr + ref -> length ;
}

char *cmdQuote(nativeCommand *cmd, char *ptr)
{
	unsigned short length = *((unsigned short *) ptr);
	unsigned short length2 = length;
	unsigned short next_token;
	char *txt;

	// check if - or + comes before *, / or ; symbols

	length2 += (length & 1);		// align to 2 bytes

	next_token = *((short *) (ptr+2+length2) );

	if ( correct_order( last_token,  next_token ) == false )
	{
		printf("---hidden ( symbol \n");

		// hidden ( condition.
		kittyStack[stack].str = NULL;
		kittyStack[stack].value = 0;
		kittyStack[stack].state = state_hidden_subData;
		stack++;
	}

	if (kittyStack[stack].str) free(kittyStack[stack].str);
	kittyStack[stack].str = strndup( ptr + 2, length );

	printf("%s::ALLOC %08x\n",__FUNCTION__, kittyStack[stack].str);

	kittyStack[stack].len = strlen( kittyStack[stack].str );
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = 2;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (stack)
	{
		 if (kittyStack[stack-1].state == state_none) if (cmdTmp[cmdStack-1].flag == cmd_para ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	}

	return ptr + length2;
}


char *cmdNumber(nativeCommand *cmd, char *ptr)
{
	unsigned short next_token = *((short *) (ptr+4) );

	// check if - or + comes before *, / or ; symbols

	if ( correct_order( last_token,  next_token ) == false )
	{
		printf("---hidden ( symbol \n");

		// hidden ( condition.
		kittyStack[stack].str = NULL;
		kittyStack[stack].value = 0;
		kittyStack[stack].state = state_hidden_subData;
		stack++;
	}

	kittyStack[stack].value = *((int *) ptr);
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = type_int;

	flushCmdParaStack();

	return ptr;
}

char *cmdFloat(nativeCommand *cmd,char *ptr)
{
	unsigned int data = *((unsigned int *) ptr);
	unsigned int number1 = data >> 8;

	int e = (data & 31) ;

	if (data & 32) e |= 0xFFFFFFE0;

	int n;
	double f = 0.0f;

	for (n=23;n>-1;n--)
	{
		if ((1<<n)&number1)
		{
			f += 1.0f / (double) (1<<(23-n));
		}
	}

	printf("%f E %d ", f, e );

	if (e>0)	f *= 1 <<e-1;
	if (e==0)	f /= 2;
	if (e<0)	f /= 1<<(-e+1);

	setStackDecimal( round( f *1000 ) / 1000.0f  );

	return ptr;
}


struct nativeCommand nativeCommands[]=
{
	{0x0000,	"", 2,	cmdNewLine},
	{0x0006, "", sizeof(struct reference),cmdVar},
	{0x000C, "", sizeof(struct reference),cmdLabelOnLine },		// no code to execute
	{0x0012, "procedure with args",sizeof(struct reference),cmdProcAndArgs },

	{0x0018, "", sizeof(struct reference),cmdVar},		// being a dick here its proc not a var

	{0x0026, "\"",2, cmdQuote },
	{0x003E, "",4,cmdNumber },
	{0x0046, "",4,cmdFloat },
	{0x0054, ":", 0, nextCmd },
	{0x005C, ",", 0, nextArg },
	{0x0064, ";", 0, breakData },
	{0x0074, "(", 0, subCalc },
	{0x007C, ")", 0, subCalcEnd },
	{0x0084, "[", 0, cmdBracket },
	{0x008C, "]", 0, cmdBracketEnd },
	{0x0094, "To",0,cmdTo },
	{0x023C, "For",2,cmdFor },
	{0x0246, "Next",0,cmdNext },
	{0x0356, "Step",0,cmdStep },
	{0x0250, "Repeat", 2, cmdRepeat},
	{0x025C, "Until",0,cmdUntil },
	{0x027E, "Do",2,cmdDo },
	{0x0286, "Loop",0,cmdLoop },
	{0x02A8, "Goto",0,cmdGoto },
	{0x02B2, "Gosub",0,cmdGosub },
	{0x0268, "While",2,cmdWhile },
	{0x0274, "Wend",0,cmdWend },
	{0x02BE, "If",2, cmdIf },
	{0x02C6, "Then",0,cmdThen },
	{0x02D0, "Else",2,cmdElse },
	{0x02DA, "End If",0,cmdEndIf },
	{0x033C, "Pop Proc",0,cmdPopProc },
	{0x0360, "Return",0,cmdReturn },
	{0x0376, "Procedure", sizeof(struct procedure), cmdProcedure },
	{0x0386, "Proc",0, cmdProc },	
	{0x0390, "End Proc", 0, cmdEndProc },
	{0x039E, "Shared", 0, cmdShared },
	{0x03AA, "Global", 0, cmdGlobal },
	{0x03B6, "End",0,cmdEnd },

	{0x03CA, "Param#",0,cmdParamFloat },
	{0x03D6, "Param$",0,cmdParamStr },
	{0x03E2, "Param",0,cmdParam },

	{0x0404,"data", 2, cmdData },
	{0x040E,"read",0,cmdRead },

	{0x0444, "Inc",0,incMath },
	{0x044E, "Dec",0,decMath },
	{0x0458, "Add",0,addMath },

	{0x046A, "Print #",0,cmdPrintOut },
	{0x0476, "Print",0,cmdPrint },

	{0x04B2, "Input #",0,cmdInputIn },
	{0x04D0, "Input",0,cmdInput },

	{0x04BE, "Line Input #",0,cmdLineInputFile },

	{0x050E, "Mid$",0,cmdMid },
	{0x0528, "Left$",0,cmdLeft },
	{0x0536, "Right$",0,cmdRight },

	{0x05C4, "Hex$",0,cmdHex },
	{0x05AE, "Bin$",0,cmdBin },
	{0x05A4, "Val",0, cmdVal },
	{0x0598, "Str$",0, cmdStr },

	{0x0658,"Sort",0,cmdSort },

	{0x01dc, "Aac",0, cmdAsc },
	{0x0546, "Flip$",0, cmdFlip },
	{0x0552, "Chr$",0, cmdChr },
	{0x055E, "Space$",0, cmdSpace },
	{0x056C, "String$", 0, cmdString },
	{0x057C, "Upper$",0, cmdUpper },
	{0x058A, "Lower$",0, cmdLower },
	{0x05DA, "Len",0, cmdLen },

	{0x05E4, "Instr",0, cmdInstr },
	{0x0640, "Dim",0, cmdDim },
	{0x064A, "Rem",2, cmdRem },

	{0x0662, "match",0,cmdMatch },

	{0x123E,"TRUE",0, cmdTrue },
	{0x1248,"FALSE",0, cmdFalse },

	{0x049C,"Input$(f,n)", 0, cmdInputStrFile },
	{0x16F2,"Set input", 0, cmdSetInput },
	{0x187C,"Lof(f)", 0, cmdLof },
	{0x1886,"Eof(f)", 0, cmdEof },
	{0x1890,"Pof(f)", 0, cmdPof },

	{0x18A8,"open random f,name", 0, cmdOpenRandom },
	{0x1948,"Field f,size as nane$,...", 0, cmdField },
	{0x23B8,"Get f,n", 0, cmdGet },
	{0x23AC,"Put f,n", 0, cmdPut },
	{0x01E6,"At",0,cmdAt },

	{0x175A,"Dir$",0,cmdDirStr },
	{0x17AE,"Dir",0,cmdDir },
	{0x17C4,"Set Dir",0,cmdSetDir },
	{0x1864,"Dfree",0,cmdDfree },
	{0x18BC,"Open In",0,cmdOpenIn },
	{0x18CC,"Open Out",0,cmdOpenOut },
	{0x18F0,"Append",0,cmdAppend },

	{0x190C,"Close",0,cmdClose },
	{0x1914,"Parent",0,cmdParent },
	{0x1930,"Dfree",0,cmdKill },
	{0x1920,"Rename",0,cmdRename },

	{0x196C,"Fsel$",0,cmdFselStr },
	{0x174E,"Exist",0,cmdExist },
	{0x172C,"Dir First$",0,cmdDirFirstStr },
	{0x173E,"Dir Next$",0,cmdDirNextStr },

	{0xFF4C,"or",0, orData },
	{0xFF58,"or",0, andData },

	{0xFF7A,"<=",0,lessOrEqualData },
	{0xFF84,"<=",0,lessOrEqualData },

	{0xFF8E,">=",0,moreOrEqualData },
	{0xFF98,">=",0,moreOrEqualData },

	{0xFFAC,"<",0, lessData },
	{0xFFB6,">",0, moreData },

	{0xFFC0,"+",0, addData },
	{0xFFCA,"-", 0, subData },
	{0xFFA2,"=", 0, setVar },
	{0xFFE2,"*", 0, mulData },
	{0xFFEC,"/", 0, divData },
	{0xFFF6,"^", 0, powerData },
	{0xFF66,"not equal",0,cmdNotEqual },

	{0x20F2,"",0,cmdReserveAsWork },
	{0x210A,"",0,cmdReserveAsChipWork },
	{0x2128,"",0, cmdReserveAsData },
	{0x2140,"", 0, cmdReserveAsChipData },
	{0x216A,"", 0, cmdListBank },

	{0x215E, "", 0, cmdErase },
	{0x0140, "Start", 0, cmdStart },
	{0x014C, "Length", 0, cmdLength },
	{0x180C, "Bload",0,cmdBload },
	{0x181A, "Bsave", 0, cmdBsave },

	{0x02E6, "on error", 0, cmdOnError },
	{0x031E, "Resume Label", 0, cmdResumeLabel },
	{0x03EE, "Error", 0, cmdError },
	{0x0316, "On", 4, cmdOn }

};

int nativeCommandsSize = sizeof(nativeCommands)/sizeof(struct nativeCommand);

char *executeToken( char *ptr, unsigned short token )
{
	struct nativeCommand *cmd;
	char *ret;

//	dump_stack();

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id ) 
		{

			printf("%08X %20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
						ptr,__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token);

			ret = cmd -> fn( cmd, ptr ) ;
			if (ret) ret += cmd -> size;
			return ret;
		}
	}

	printf("%08X %20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
					ptr,__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token);	

	return NULL;
}

char *token_reader( char *start, char *ptr, unsigned short lastToken, unsigned short token, int tokenlength )
{
	ptr = executeToken( ptr, token );

	if ( ( (long long int) ptr - (long long int) start)  >= tokenlength ) return NULL;

	return ptr;
}

void code_reader( char *start, int tokenlength )
{
	char *ptr;
	int token = 0;
	last_token = 0;
	
	currentLine = 0;
	ptr = start;
	while ( ptr = token_reader(  start, ptr,  last_token, token, tokenlength ) )
	{
		// this basic for now, need to handel "on error " commands as well.

		ptr = onError( ptr );
		if (ptr == NULL) break;

		last_token = token;
		token = *((short *) ptr);
		ptr += 2;	// next token.		
	}
}

int main()
{
	BOOL runtime = FALSE;
	FILE *fd;
	int amos_filesize;
	char amosid[17];
	char *data;
	int n;

	amosid[16] = 0;	// /0 string.

	stack = 0;
	cmdStack = 0;

	onError = onErrorBreak;


	memset(globalVars,0,sizeof(globalVars));

	if (init())
	{

//	fd = fopen("amos-test/var.amos","r");
//	fd = fopen("amos-test/var_num.amos","r");
//	fd = fopen("amos-test/math.amos","r");
//	fd = fopen("amos-test/dim2.amos","r");
//	fd = fopen("amos-test/input.amos","r");
//	fd = fopen("amos-test/goto.amos","r");
//	fd = fopen("amos-test/if.amos","r");
//	fd = fopen("amos-test/goto2.amos","r");
//	fd = fopen("amos-test/do-loop.amos","r");
//	fd = fopen("amos-test/repeat-until.amos","r");
//	fd = fopen("amos-test/legal-ilegal-if.amos","r");
//	fd = fopen("amos-test/while-wend.amos","r");
//	fd = fopen("amos-test/for-to-step-next.amos","r");
//	fd = fopen("amos-test/for-to-next.amos","r");
//	fd = fopen("amos-test/for-to-next2.amos","r");
//	fd = fopen("amos-test/gosub-return.amos","r");
//	fd = fopen("amos-test/left-mid-right.amos","r");
//	fd = fopen("amos-test/instr.amos","r");
//	fd = fopen("amos-test/upper-lower-flip-spaces.amos","r");
//	fd = fopen("amos-test/str-chr-asc-len.amos","r");
//	fd = fopen("amos-test/hex-bin-val-str.amos","r");
//	fd = fopen("amos-test/casting_int_float.amos","r");
//	fd = fopen("amos-test/arithmetic.amos","r");
//	fd = fopen("amos-test/inc-dec-add.amos","r");
//	fd = fopen("amos-test/compare-strings.amos","r");
//	fd = fopen("amos-test/procedure.amos","r");
//	fd = fopen("amos-test/procedure2.amos","r");
//	fd = fopen("amos-test/procedure_with_paramiters_x.amos","r");
//	fd = fopen("amos-test/procedure-shared.amos","r");
//	fd = fopen("amos-test/procedure-global.amos","r");
//	fd = fopen("amos-test/procedure_return_value.amos","r");
//	fd = fopen("amos-test/procedure_all_params.amos","r");
//	fd = fopen("amos-test/procedure_pop_proc.amos","r");
//	fd = fopen("amos-test/reserve.amos","r");
//	fd = fopen("amos-test/erase-start-length-bsave-bload.amos","r");
//	fd = fopen("amos-test/sort.amos","r");
//	fd = fopen("amos-test/or.amos","r");
//	fd = fopen("amos-test/logical1.amos","r");
//	fd = fopen("amos-test/match.amos","r");
//	fd = fopen("amos-test/dir.amos","r");
//	fd = fopen("amos-test/dir_str.amos","r");
//	fd = fopen("amos-test/parent-set-dir.amos","r");
//	fd = fopen("amos-test/fsel_exits_dir_first_dir_next.amos","r");
//	fd = fopen("amos-test/open-out.amos","r");
//	fd = fopen("amos-test/open-in.amos","r");
//	fd = fopen("amos-test/line_input_file.amos","r");
	fd = fopen("amos-test/line_input.amos","r");
//	fd = fopen("amos-test/set-input-input-eof-pof.amos","r");
//	fd = fopen("amos-test/open_random.amos","r");
//	fd = fopen("amos-test/dir_first_dir_next.amos","r");
//	fd = fopen("amos-test/on_error_goto.amos","r");
//	fd = fopen("amos-test/on_error_proc.amos","r");
//	fd = fopen("amos-test/on_gosub.amos","r");
//	fd = fopen("amos-test/input_two_args.amos","r");
	if (fd)
	{
		fseek(fd, 0, SEEK_END);
		amos_filesize = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		fread( amosid, 16, 1, fd );
		fread( &tokenlength, 4, 1, fd );

		data = (char *) malloc(amos_filesize);
		if (data)
		{
			fread(data,amos_filesize,1,fd);

			// snifff the tokens find labels, vars, functions and so on.
			pass1_reader( data, tokenlength );

			if (kittyError.code == 0)
			{
				runtime = TRUE;

				_file_start_ = data;
				_file_end_ = data + tokenlength;

				//  execute the code.
				code_reader( data, tokenlength );
			}

			if (kittyError.newError)
			{
				printError( &kittyError, runtime ? errorsRunTime : errorsTestTime );
			}
		}

		fclose(fd);

		if (kittyError.newError == false)
		{
			dump_end_of_program();
		}
	}
	else
	{
		printf("AMOS file not open/can't find it\n");
	}

		clean_up_vars();
		clean_up_stack();
		clean_up_files();
		clean_up_special();	// we add other stuff to this one.

		closedown();
	}
	

	return 0;
}
