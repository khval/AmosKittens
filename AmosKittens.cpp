
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "amosKittens.h"
#include "commands.h"
#include "debug.h"
#include <vector>
#include "errors.h"
#include "pass1.h"

int stack = 0;
int cmdStack = 0;
unsigned short last_token = 0;
int last_var = 0;

int tokenMode = mode_standard;

void _str(const char *str);
void _num( int num );
void _decimal( double decimal );

struct kittyData kittyStack[100];

struct globalVar globalVars[1000];	// 0 is not used.
int globalVarsSize = sizeof(globalVars)/sizeof(struct globalVar);

std::vector<struct label> labels;	// 0 is not used.

int global_var_count = 0;
int labels_count = 0;

 struct glueCommands cmdTmp[100];	

extern char *nextToken_pass1( char *ptr, unsigned short token );

char *cmdRem(nativeCommand *cmd, char *ptr)
{
	int length = *((short *) ptr);
	return ptr + length;
}


char *nextCmd(nativeCommand *cmd, char *ptr)
{
	char *ret = NULL;

	if (cmdStack) ret = cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	tokenMode = mode_standard;

	if (ret) ptr = ret - 2;
	return ptr;
}

char *cmdNewLine(nativeCommand *cmd, char *ptr)
{
	char *ret = NULL;

	dump_prog_stack();

	while ((cmdStack) && (cmdTmp[cmdStack-1].flag != cmd_loop ))
	{
		ret = cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
		if (ret) break;
	}

	tokenMode = mode_standard;

	if (ret) ptr = ret - 2;
	
	printf("-- ENTER FOR NEXT AMOS LINE --\n");
	getchar();

	return ptr;
}

char *cmdPrint(nativeCommand *cmd, char *ptr)
{
	stackCmdNormal( _print, ptr );
	return ptr;
}

char *_array_index_var( glueCommands *self )
{
	int tmp_cells;
	int varNum;
	int n = 0;
	int mul;
	int index;

	struct kittyData *var;

	tmp_cells = stack - self -> stack;

	varNum = self -> lastVar;

//	varNum = *((unsigned short *) (self -> tokenBuffer + 2));

	dump_stack();

	printf("%s: %08x, varNum %04x\n",__FUNCTION__, self -> tokenBuffer, varNum);

	var = &globalVars[varNum].var;

	index = 0; mul  = 1;
	for (n = self -> stack+1;n<=stack; n++ )
	{
		index += (mul * kittyStack[n].value);
		mul *= var -> sizeTab[n- self -> stack -1];
	}

	var -> index = index;
	stack -=  tmp_cells;		// should use garbage collector here ;-) memory leaks works to for now.

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
				kittyStack[stack].type = (var -> type & 7);
				kittyStack[stack].str = strdup(var -> str_array[index]);
				kittyStack[stack].len = strlen(kittyStack[stack].str);
				break;
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
	var = &globalVars[varNum].var;

	var -> cells = stack - self -> stack;
	var -> sizeTab = (int *) malloc( sizeof(int) * var -> cells );

	for (n= 0; n<var -> cells; n++ ) 
	{
		var -> sizeTab[n] = kittyStack[self -> stack + 1 +  n].value + 1;
	}

	var -> count = 1 ;
	for (n= 0; n<var -> cells;n++) var -> count *= var -> sizeTab[n];

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

char *cmdVar(nativeCommand *cmd, char *ptr)
{
	struct reference *ref = (struct reference *) ptr;
	unsigned short next_token = *((short *) (ptr+sizeof(struct reference)+ref->length));
	struct kittyData *var;

	if (tokenMode == mode_goto)
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

//		cmdNormal( _goto, ptr );	
		stack++;
	}

	last_var = ref -> ref;

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

			switch (ref -> flags & 3)
			{
				case 0:
					_num(globalVars[idx].var.value);
					break;
				case 1:
					_decimal(globalVars[idx].var.value);
					break;
				case 2:
					_str(globalVars[idx].var.str);
					break;
			}
		}
	}

	if (cmdStack) if (stack)
	{
		char *newTokenLoc = NULL;
		if (kittyStack[stack-1].state == state_none) newTokenLoc =cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
		if (newTokenLoc) return newTokenLoc - sizeof(struct reference) - 2;
	}

	return ptr + ref -> length ;
}



char *cmdQuote(nativeCommand *cmd, char *ptr)
{
	unsigned short length = *((unsigned short *) ptr);
	unsigned short length2 = length;
	char *txt;

	length2 += (length & 1);		// align to 2 bytes


	printf("length %d\n",length2);

	kittyStack[stack].str = strndup( ptr + 2, length );
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

	// check if - or + comes before * or /

	if (
		((last_token==0xFFC0) || (last_token==0xFFCA))
	&&
		((next_token==0xFFE2) || (next_token == 0xFFEC))
	) {

		printf("---hidden ( symbol \n");

		// hidden ( condition.
		kittyStack[stack].str = NULL;
		kittyStack[stack].state = state_hidden_subData;
		stack++;
	}

	kittyStack[stack].value = *((int *) ptr);
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = 0;

	// check it last command was * or /. and next command is not a * or /

	if (cmdStack) if (stack)
	{
		 if (kittyStack[stack-1].state == state_none) if (cmdTmp[cmdStack-1].flag == cmd_para ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	}

	if (
		((last_token==0xFFE2) || (last_token == 0xFFEC))
	&&
		((next_token!=0xFFE2) && (next_token != 0xFFEC))
	) {

		printf("---hidden ( symbol maybe ? \n");

		// it maybe a hidden ) condition.
		if (stack > 0)
		{
			if (kittyStack[stack-1].state == state_hidden_subData)
			{
				printf("---hidden ) symbol yes it is\n");

				kittyStack[stack-1] = kittyStack[stack];
				stack --;
				if (cmdStack) if (stack) if (kittyStack[stack-1].state == state_none) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
			}
		}
	}

	return ptr;
}


struct nativeCommand nativeCommands[]=
{
	{0x0000,	"", 2,	cmdNewLine},
	{0x0006, "", sizeof(struct reference),cmdVar},
	{0x000C, "", sizeof(struct reference),cmdLabelOnLine },		// no code to execute
	{0x0026,"\"",2, cmdQuote },
	{0x003E,"",4,cmdNumber },
	{0x0054,":", 0, nextCmd },
	{0x005C,",", 0, nextArg},
	{0x0064,";", 0, breakData},
	{0x0074,"(", 0, subCalc},
	{0x007C,")", 0, subCalcEnd},
	{0x0084,"[", 0, NULL },
	{0x008C,"]", 0, NULL },
	{0x0094,"To",0,cmdTo },
	{0x023C,"For",2,cmdFor },
	{0x0246,"Next",0,cmdNext },
	{0x0356,"Step",0,cmdStep },
	{0x0250,"Repeat", 2, cmdRepeat},
	{0x025C,"Until",0,cmdUntil },
	{0x027E,"Do",2,cmdDo },
	{0x0286,"Loop",0,cmdLoop },
	{0x02a8,"Goto",0,cmdGoto },
	{0x0268,"While",2,cmdWhile },
	{0x0274, "Wend",0,cmdWend },
	{0x02BE,"If",2, cmdIf },
	{0x02C6,"Then",0,cmdThen },
	{0x02D0,"Else",2,cmdElse },
	{0x02DA,"End If",0,cmdEndIf },
	{0x0476, "Print",0,cmdPrint },
	{0x04D0, "Input",0,cmdInput },
	{0x0640, "Dim",0,cmdDim },
	{0x064A, "Rem",2,cmdRem },

	{0x123E,"TRUE",0,cmdTrue },
	{0x1248,"FALSE",0,cmdFalse },

	{0xFFAC,"<",0,cmdLess },
	{0xFFAC,">",0,cmdMore },
	{0xFFC0,"+",0, addData },
	{0xFFCA,"-", 0, subData },
	{0xFFA2,"=", 0, setVar },
	{0xFFE2,"*", 0, mulData },
	{0xFFEC,"/", 0, divData },
	{0xFF66,"not equal",0,cmdNotEqual }
};

int nativeCommandsSize = sizeof(nativeCommands)/sizeof(struct nativeCommand);

char *executeToken( char *ptr, unsigned short token )
{
	struct nativeCommand *cmd;
	char *ret;

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

void _num( int num )
{
	if (kittyStack[stack].str) free(kittyStack[stack].str);	// we should always set ptr to NULL, if not its not freed.

	kittyStack[stack].str = NULL;
	kittyStack[stack].value = num;
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = type_int;
}

void _decimal( double decimal )
{
	if (kittyStack[stack].str) free(kittyStack[stack].str);	// we should always set ptr to NULL, if not its not freed.

	kittyStack[stack].str = NULL;
	kittyStack[stack].decimal = decimal;
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = type_float;
}


void _str(const char *str)
{
								printf("\n'%20s:%08d stack is %d cmd stack is %d flag %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (kittyStack[stack].str) free(kittyStack[stack].str);	// we should always set ptr to NULL, if not its not freed.

	kittyStack[stack].str = strdup( str );
	kittyStack[stack].len = strlen( kittyStack[stack].str );
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = type_string;
}

void _castNumToStr( int num )
{
	char tmp[100];
								printf("'%20s:%08d stack is %d cmd stack is %d flag %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	sprintf(tmp,"%d",num);
	kittyStack[stack].str = strdup( tmp );
	kittyStack[stack].len = strlen( kittyStack[stack].str );
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = 2;

	if (cmdStack) if (stack)
	{
		 if (kittyStack[stack-1].state == state_none) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	}
}

void paramiter_testing()
{
	_str("Amos");
	executeToken( NULL, 0x0064);
	_str(" Profsonal");
	executeToken( NULL, 0x0064 );
	_str(" can run on X");

	executeToken( NULL, 0x0064 );

	_castNumToStr( 1000 );
	executeToken( NULL, 0xFFCA );
	_str("fsonal");

	executeToken( NULL, 0xFFCA );	// -
//	executeToken( NULL, 0xFFC0 );	// +

	executeToken( NULL, 0x0074 );	// (
	_str("X");						// "X"

	executeToken( NULL, 0xFFC0 );	// +
	_str("A1000");					// "A1000"

	executeToken( NULL, 0xFFCA );	// -
	_str("A");						// "A"

	executeToken( NULL, 0x007C );	// )

	printf("--------------------------\n");

	if (kittyStack[stack].str)
	{
		printf("'%s' stack is %d cmd stack is %d\n", kittyStack[stack].str, stack, cmdStack);
	}
	else
	{
		printf("nothing\n");
	}
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
	
	ptr = start;
	while ( ptr = token_reader(  start, ptr,  last_token, token, tokenlength ) )
	{
		if (ptr == NULL) break;

		last_token = token;
		token = *((short *) ptr);
		ptr += 2;	// next token.		
	}
}

int main()
{
	int tokenlength;
	FILE *fd;
	int amos_filesize;
	char amosid[17];
	char *data;
	int n;

	amosid[16] = 0;	// /0 string.

	stack = 0;
	cmdStack = 0;

	memset(globalVars,0,sizeof(globalVars));

//	fd = fopen("amos-test/var.amos","r");
//	fd = fopen("amos-test/var_num.amos","r");
//	fd = fopen("amos-test/math.amos","r");
//	fd = fopen("amos-test/dim.amos","r");
//	fd = fopen("amos-test/input.amos","r");
//	fd = fopen("amos-test/goto.amos","r");
//	fd = fopen("amos-test/if.amos","r");
//	fd = fopen("amos-test/goto2.amos","r");
//	fd = fopen("amos-test/do-loop.amos","r");
//	fd = fopen("amos-test/repeat-until.amos","r");
//	fd = fopen("amos-test/legal-ilegal-if.amos","r");
//	fd = fopen("amos-test/while-wend.amos","r");
	fd = fopen("amos-test/for-to-step-next.amos","r");
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
				//  execute the code.
				code_reader( data, tokenlength );

				if (kittyError.code != 0) printError( &kittyError, errorsRunTime );
			}
			else
			{
				printError( &kittyError, errorsTestTime );
			}
		}

		fclose(fd);
	}

	if (kittyError.code == 0)
	{
		printf("--- End of program status ---\n");

		printf("\n--- var dump ---\n");
		dump_global();

		printf("\n--- value stack dump ---\n");
		dump_stack();

		printf("\n--- program stack dump ---\n");
		dump_prog_stack();

		printf("\n--- label dump ---\n");
		dumpLabels();
	}

	return 0;
}
