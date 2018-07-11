
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <vector>
#include <math.h>
#include <libraries/retroMode.h>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "commandsString.h"
#include "commandsMath.h"
#include "commandsBanks.h"
#include "commandsDisc.h"
#include "commandsErrors.h"
#include "commandsMachine.h"
#include "commandsGfx.h"
#include "commandsScreens.h"
#include "commandsText.h"
#include "commandsKeyboard.h"
#include "commandsObjectControl.h"
#include "commandsSound.h"
#include "commandsHardwareSprite.h"
#include "commandsBlitterObject.h"
#include "debug.h"
#include "errors.h"
#include "pass1.h"
#include "init.h"
#include "cleanup.h"
#include "engine.h"

bool running = true;
bool interpreter_running = false;

int sig_main_vbl = 0;

char *var_param_str = NULL;
int var_param_num;
double var_param_decimal;

char *_file_start_ = NULL;
char *_file_pos_  = NULL;		// the problem of not knowing when stacked commands are executed.
char *_file_end_ = NULL;

int parenthesis_count = 0;
int cmdStack = 0;
int procStackCount = 0;
unsigned short last_token = 0;
int last_var = 0;
int tokenlength;


unsigned short token_not_found = 0xFFFF;	// so we know its not a token, token 0 exists.

char *data_read_pointer = NULL;

char *_get_var_index( glueCommands *self, int nextToken);

void do_to_default( struct nativeCommand *cmd, char *tokenbuffer );

char *(*do_var_index) ( glueCommands *self, int nextToken ) = _get_var_index;
void (*do_to) ( struct nativeCommand *, char * ) = do_to_default;
void (*do_input) ( struct nativeCommand *, char * ) = NULL;
void (*do_breakdata) ( struct nativeCommand *, char * ) = NULL;

int tokenMode = mode_standard;

struct retroSprite *sprite = NULL;
struct retroSpriteObject bobs[64];
struct proc procStack[1000];	// 0 is not used.
struct globalVar globalVars[1000];	// 0 is not used.
struct kittyBank kittyBanks[16];
struct kittyFile kittyFiles[10];
struct zone *zones = NULL;
int zones_allocated = 0;

int globalVarsSize = sizeof(globalVars)/sizeof(struct globalVar);

std::vector<struct label> labels;	// 0 is not used.
std::vector<struct lineAddr> linesAddress;
std::vector<struct defFn> defFns;

int global_var_count = 0;
int labels_count = 0;

struct glueCommands cmdTmp[100];	
struct glueCommands input_cmd_context;

extern char *nextToken_pass1( char *ptr, unsigned short token );

bool breakpoint = false;

const char *str_dump_stack = "dump stack";
const char *str_breakpoint_on = "breakpoint on";
const char *str_breakpoint_off = "breakpoint off";
const char *str_warning = "warning";

char *cmdRem(nativeCommand *cmd, char *ptr)
{
	int length = *((short *) ptr);

	if (length>4)
	{
		char *txt = strndup( ptr + 3, length );
		if (txt)
		{
			if (strncmp(txt,str_dump_stack,strlen(str_dump_stack))==0)
			{
				printf("stack %d at line %d\n",stack, getLineFromPointer( ptr ));
			}

			if (strncmp(txt,str_breakpoint_on,strlen(str_breakpoint_on))==0)
			{
				breakpoint = true;
			}

			if (strncmp(txt,str_breakpoint_off,strlen(str_breakpoint_off))==0)
			{
				breakpoint = false;
			}

			if (strncmp(txt,str_warning,strlen(str_warning))==0)
			{
				printf("**********************************\n");
				printf(" %s\n",txt+strlen(str_warning));
				printf("**********************************\n");
			}

			free(txt);
		}
	}
	
	return ptr + length;
}


char *nextCmd(nativeCommand *cmd, char *ptr)
{
	char *ret = NULL;
	unsigned int type;

	// we should empty stack, until first/normal command is not a parm command.

	while (cmdStack)
	{
		type = cmdTmp[cmdStack-1].flag;
		if  ( ( type == cmd_loop ) || ( type  == cmd_never ) || (type == cmd_eol) ) break;
	
		ret = cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack], 0);

		if (cmdTmp[cmdStack].flag == cmd_first) break;
		if (ret) break;
	}

	do_to = do_to_default;
	tokenMode = mode_standard;
	dprintf("setTokenMode = mode_standard\n");

	if (ret) return ret -2;		// when exit +2 token 

	return ptr;
}

char *cmdNewLine(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__ );

	if (cmdStack)
	{
		char *ret = NULL;
		unsigned int type;
		do 
		{
			type = cmdTmp[cmdStack-1].flag;
			if  ( (type == cmd_proc) || ( type == cmd_loop ) || ( type  == cmd_never ) ) break;

			ret = cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
			if (ret) return ret -4;		// when exit +2 token +2 data

		} while (cmdStack);
	}

	do_to = do_to_default;
	tokenMode = mode_standard;
	dprintf("setTokenMode = mode_standard\n");

	if (breakpoint)
	{
		dump_stack();
		dump_global();
		getchar();
	}

	return ptr;
}


int _last_var_index;		// we need to know what index was to keep it.
int _set_var_index;		// we need to resore index 

char *_get_var_index( glueCommands *self , int nextToken )
{
	int varNum;
	int n = 0;
	int mul;
	struct kittyData *var = NULL;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__ );

	varNum = self -> lastVar;

	if (varNum == 0) return NULL;

	last_var = varNum;		// this is used when a array is set. array[var]=0, it restores last_var to array, not var

	if (varNum) 
	{
		var = &globalVars[varNum-1].var;
	}

	if (var)
	{
		if ( (var -> type & type_array)  == 0)
		{
			popStack(stack - self -> stack);
			setError(27, self -> tokenBuffer);		// var is not a array
			return 0;
		}

		_last_var_index = 0; 
		mul  = 1;
		for (n = self -> stack;n<=stack; n++ )
		{
			_last_var_index += (mul * kittyStack[n].value);
			mul *= var -> sizeTab[n- self -> stack -1];
		}

		dprintf("varname: %s(%d)\n",globalVars[varNum-1].varName, _last_var_index);

		var -> index = _last_var_index;
		popStack(stack - self -> stack);

		if ((_last_var_index >= 0)  && (_last_var_index<var->count))
		{
			if ( correct_order( self -> lastToken,  nextToken ) == false )
			{
				dprintf("---hidden ( symbol \n");

				// hidden ( condition.
				kittyStack[stack].str = NULL;
				kittyStack[stack].value = 0;
				kittyStack[stack].state = state_hidden_subData;
				stack++;
			}

			switch (var -> type & 3)
			{
				case type_int: 
					setStackNum( var -> int_array[_last_var_index] );	break;

				case type_float:
					setStackDecimal( var -> float_array[_last_var_index] );	
					break;

				case type_string:	
					char *str = var -> str_array[_last_var_index];
					setStackStrDup( str ? str : "" );
					break;
			}

			flushCmdParaStack(nextToken);
		}
	}

	return NULL;
}

char *_alloc_mode_off( glueCommands *self, int nextToken )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	do_input = NULL;
	do_var_index = _get_var_index;

	return NULL;
}

char *do_var_index_alloc( glueCommands *cmd, int nextToken)
{
	int args = stack - cmd -> stack + 1;
	int size = 0;
	int n;
	int varNum;
	int count;
	struct kittyData *var;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	varNum = cmd -> lastVar;	
	if (varNum == 0) return NULL;

	var = &globalVars[varNum-1].var;
	var -> cells = stack - cmd -> stack +1;

	if (var -> sizeTab) free( var -> sizeTab);
	var -> sizeTab = (int *) malloc( sizeof(int) * var -> cells );

	var -> count = 1;
	for (n= 0; n<var -> cells; n++ ) 
	{
		var -> sizeTab[n] = kittyStack[cmd -> stack + n].value + 1;
		var -> count *= var -> sizeTab[n];
	}

	switch (var -> type)
	{
		case type_int | type_array:
				size = var -> count * sizeof(int);
				var -> int_array = (int *) malloc( size ) ;
				break;
		case type_float | type_array:
				size = var -> count * sizeof(double);
				var -> float_array = (double *) malloc( size ) ;
				break;
		case type_string | type_array:
				size = var -> count * sizeof(char *);
				var -> str_array = (char **) malloc( size ) ;
				break;

		default: setError(22, cmd -> tokenBuffer);
	}

	if (var -> str) memset( var -> str, 0, size );	// str is a union :-)

	popStack(stack - cmd -> stack);

	printf("cmd stack loc %d, satck is %d\n",cmd -> stack, stack);

	return NULL;
}

void do_dim_next_arg(nativeCommand *cmd, char *ptr)
{
	if (parenthesis_count == 0)
	{
		if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
	}
}

char *cmdDim(nativeCommand *cmd, char *ptr)
{
	do_input = do_dim_next_arg;
	do_var_index = do_var_index_alloc;

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
	
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	last_var = ref -> ref;

	if (next_token == 0x0074)	// ( symbol
	{
		if (do_var_index) stackCmdIndex( do_var_index, ptr );
	}
	else
	{
		if ( correct_order( last_token,  next_token ) == false )
		{
			dprintf("---hidden ( symbol \n");

			// hidden ( condition.
			kittyStack[stack].str = NULL;
			kittyStack[stack].value = 0;
			kittyStack[stack].state = state_hidden_subData;
			stack++;
		}

		if (ref -> ref)
		{
			int idx = ref->ref-1;

//			printf("varname: %s\n",globalVars[idx].varName);

			switch (globalVars[idx].var.type & 7)
			{
				case type_int:
					setStackNum(globalVars[idx].var.value);
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
		flushCmdParaStack(next_token);
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
		// hidden ( condition.
		kittyStack[stack].str = NULL;
		kittyStack[stack].value = 0;
		kittyStack[stack].state = state_hidden_subData;
		stack++;
	}

	if (kittyStack[stack].str) free(kittyStack[stack].str);
	kittyStack[stack].str = strndup( ptr + 2, length );
	kittyStack[stack].len = strlen( kittyStack[stack].str );
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = 2;

	flushCmdParaStack( (int) next_token );

	return ptr + length2;
}


char *cmdNumber(nativeCommand *cmd, char *ptr)
{
	unsigned short next_token = *((short *) (ptr+4) );
	proc_names_printf("%s:%d \n",__FUNCTION__,__LINE__);

	// check if - or + comes before *, / or ; symbols

	if ( correct_order( last_token,  next_token ) == false )
	{
		dprintf("---hidden ( symbol \n");

		// hidden ( condition.
		kittyStack[stack].str = NULL;
		kittyStack[stack].value = 0;
		kittyStack[stack].state = state_hidden_subData;
		stack++;
	}

	setStackNum( *((int *) ptr) );

	kittyStack[stack].state = state_none;
	flushCmdParaStack( next_token );

	return ptr;
}

char *cmdFloat(nativeCommand *cmd,char *ptr)
{
	unsigned int data = *((unsigned int *) ptr);
	unsigned int number1 = data >> 8;
	int e = (data & 0x3F) ;
	int n;
	double f = 0.0f;

	if ( (data & 0x40)  == 0)	e = -(65 - e);

	for (n=23;n>-1;n--)
	{
		if ((1<<n)&number1) f += 1.0f / (double) (1<<(23-n));
	}

	if (e>0) { while (--e) { f *= 2.0; } }
	if (e<0) { while (e) {  f /= 2.0f; e++; } }

	setStackDecimal( f );

	return ptr;
}


struct nativeCommand nativeCommands[]=
{
	{0x0000,	"<EOL>", 2,	cmdNewLine},
	{0x0006, "<var>", sizeof(struct reference),cmdVar},
	{0x000C, "", sizeof(struct reference),cmdLabelOnLine },		// no code to execute
	{0x0012, "procedure with args",sizeof(struct reference),cmdProcAndArgs },
	{0x0018, "<label>", sizeof(struct reference),cmdVar},	
	{0x001E, "<Bin>",4,cmdNumber },		// binrary
	{0x0026, "<Text>",2, cmdQuote },
	{0x0036, "<Hex>",4,cmdNumber },		// hex
	{0x003E, "<number>",4,cmdNumber },
	{0x0046, "<float>",4,cmdFloat },
	{0x004E, "Extension",4,cmdExtension },		// extention command 
	{0x0054, ":", 0, nextCmd },
	{0x005C, ",", 0, nextArg },
	{0x0064, ";", 0, breakData },
	{0x0074, "(", 0, parenthesisStart },
	{0x007C, ")", 0, parenthesisEnd },
	{0x0084, "[", 0, cmdBracket },
	{0x008C, "]", 0, cmdBracketEnd },
	{0x0094, "To",0,cmdTo },
	{0x009C, "Not",0,cmdNot },
	{0x00A6,"Swap", 0, mathSwap},
	{0x00b0, "Def fn", 0, mathDefFn },
	{0x00bc, "Fn", 0, mathFn },
	{0x00F2,"Inkey$",0,cmdInkey },
	{0x00FE,"Repeat$",0, cmdRepeatStr },
	{0x010E,"Zone$", 0, ocZoneStr },
	{0x011C,"Border$", 0, textBorderStr },			// needs more work.
	{0x012C,"Double Buffer",0,gfxDoubleBuffer },
	{0x0140, "Start", 0, cmdStart },
	{0x014C, "Length", 0, cmdLength },
	{0x015A,"Doke",0,machineDoke},
	{0x019C, "Every On", 0, cmdEveryOn },
	{0x01AA, "Every Off", 0, cmdEveryOff },
	{0x01dc, "Asc",0, cmdAsc },
	{0x01E6,"At",0,cmdAt },
	{0x01EE,"Call",0,machineCall},	
	{0x01F8,"EXECALL",0,machineEXECALL},
	{0x0214,"DOSCALL",0,machineDOSCALL},
	{0x023C, "For",2,cmdFor },
	{0x0246, "Next",0,cmdNext },
	{0x0250, "Repeat", 2, cmdRepeat},
	{0x025C, "Until",0,cmdUntil },
	{0x0268, "While",2,cmdWhile },
	{0x0274, "Wend",0,cmdWend },
	{0x027E, "Do",2,cmdDo },
	{0x0286, "Loop",0,cmdLoop },
	{0x0290, "Exit If",4,cmdExitIf },
	{0x029E, "Exit", 4, cmdExit },
	{0x02A8, "Goto",0,cmdGoto },
	{0x02B2, "Gosub",0,cmdGosub },
	{0x02BE, "If",2, cmdIf },
	{0x02C6, "Then",0,cmdThen },
	{0x02D0, "Else",2,cmdElse },
	{0x02DA, "End If",0,cmdEndIf },
	{0x02E6, "on error", 0, cmdOnError },
	{0x0316, "On", 4, cmdOn },
	{0x031E, "Resume Label", 0, cmdResumeLabel },
	{0x033C, "Pop Proc",0,cmdPopProc },
	{0x034A, "Every ...",  0, cmdEvery },
	{0x0356, "Step",0,cmdStep },
	{0x0360, "Return",0,cmdReturn },
	{0x036C, "Pop",0,cmdPop },
	{0x0376, "Procedure", sizeof(struct procedure), cmdProcedure },
	{0x0386, "Proc",0, cmdProc },	
	{0x0390, "End Proc", 0, cmdEndProc },
	{0x039E, "Shared", 0, cmdShared },
	{0x03AA, "Global", 0, cmdGlobal },
	{0x03B6, "End",0,cmdEnd },
	{0x03CA, "Param#",0,cmdParamFloat },
	{0x03D6, "Param$",0,cmdParamStr },
	{0x03E2, "Param",0,cmdParam },
	{0x03EE, "Error", 0, cmdError },
	{0x0404,"data", 2, cmdData },		
	{0x040E,"read",0,cmdRead },
	{0x0418,"Restore", 0, cmdRestore },
	{0x0426, "Break Off", 0, cmdBreakOff },
	{0x0436, "Break On", 0, cmdBreakOn },
	{0x0444, "Inc",0,mathInc },
	{0x044E, "Dec",0,mathDec },
	{0x0458, "Add",0,mathAdd },
	{0x0462, "Add var,n,f TO t", 0, mathAdd },
	{0x046A, "Print #",0,cmdPrintOut },
	{0x0476, "Print",0,textPrint },
	{0x048E,"Input$(n)",0,cmdInputStrN },
	{0x049C,"Input$(f,n)", 0, cmdInputStrFile },
	{0x04B2, "Input #",0,cmdInputIn },
	{0x04BE, "Line Input #",0,cmdLineInputFile },
	{0x04D0, "Input",0,cmdInput },
	{0x04DC, "Line Input", 0, cmdLineInput },
	{0x04FE, "Set Buffers", 0, cmdSetBuffers },
	{0x050E, "Mid$",0,cmdMid },
	{0x051E, "Mid$(a$,start)",0, cmdMid },
	{0x0528, "Left$",0,cmdLeft },
	{0x0536, "Right$",0,cmdRight },
	{0x0546, "Flip$",0, cmdFlip },
	{0x0552, "Chr$",0, cmdChr },
	{0x055E, "Space$",0, cmdSpace },
	{0x056C, "String$", 0, cmdString },
	{0x057C, "Upper$",0, cmdUpper },
	{0x058A, "Lower$",0, cmdLower },
	{0x0598, "Str$",0, cmdStr },
	{0x05A4, "Val",0, cmdVal },
	{0x05AE, "Bin$",0,cmdBin },
	{0x05BA, "Bin$(num,chars)", 0, cmdBin },
	{0x05C4, "Hex$",0,cmdHex },
	{0x05D0, "Hex$(num,chars)",0,cmdHex },
	{0x05DA, "Len",0, cmdLen },
	{0x05E4, "Instr",0, cmdInstr },
	{0x0600,"Tab$",0,cmdTabStr },
	{0x0614,"Varptr",0,machineVarPtr},
	{0x0620,"Remember X",0,textRememberX },
	{0x0640, "Dim",0, cmdDim },
	{0x064A, "Rem",2, cmdRem },
	{0x0652, "mark rem",2, cmdRem },
	{0x0658,"Sort",0,cmdSort },
	{0x0662, "match",0,cmdMatch },
	{0x0670,"Edit",0,cmdEdit },
	{0x067A,"Direct",0,cmdDirect },
	{0x0686,"Rnd",0,mathRnd},
	{0x0690,"Randomize",0,mathRandomize},
	{0x06A0,"Sgn",0,mathSgn},
	{0x06AA,"Abs",0,mathAbs},
	{0x06B4,"Int",0,mathInt},
	{0x06BE,"Radian",0,mathRadian},
	{0x06CA,"Degree",0,mathDegree},
	{0x06D6,"Pi#",0,mathPi},
	{0x06E0,"Fix",0,mathFix},
	{0x06EA,"Min",0,mathMin},
	{0x06F6,"Max",0,mathMax},
	{0x0702,"Sin",0,mathSin},
	{0x070C,"Cos",0,mathCos},
	{0x0716,"Tan",0,mathTan},
	{0x0720,"Asin",0,mathAsin},
	{0x072C,"Acos",0,mathAcos},
	{0x0738,"Atan",0,mathAtan},
	{0x0744,"Hsin",0,mathHsin},
	{0x0750,"Hcos",0,mathHcos},
	{0x075C,"Htan",0,mathHtan},
	{0x0768,"Sqr",0,mathSqr},
	{0x0772,"Log",0,mathLog},
	{0x077C,"Ln",0,mathLn},
	{0x0786,"Exp",0,mathExp},
	{0x0986,"Screen Copy",0,gfxScreenCopy },
	{0x09A8,"Screen Copy",0,gfxScreenCopy },
	{0x09D6,"Screen Clone",0,gfxScreenClone },
	{0x09EA,"Screen Open",0,gfxScreenOpen },
	{0x0A04,"Screen Close",0,gfxScreenClose },
	{0x0A18,"Screen Display",0,gfxScreenDisplay },
	{0x0A36,"Screen Offset",0,gfxScreenOffset },
	{0x0A5E,"Screen Colour", 0, gfxScreenColour },
	{0x0A72,"Screen To Front",0,gfxScreenToFront },
	{0x0A88,"Screen To Front",0,gfxScreenToFront },
	{0x0A90,"Screen To Back",0,gfxScreenToBack },
	{0x0AA6,"Screen To Back",0,gfxScreenToBack },
	{0x0AAE,"Screen Hide",0,gfxScreenHide },
	{0x0AC0,"Screen Hide",0,gfxScreenHide },
	{0x0ADA,"Screen Show",0,gfxScreenShow },
	{0x0AE2,"Screen Swap",0,gfxScreenSwap },	
	{0x0AFC,"Save Iff",0,gfxSaveIff },
	{0x0B16,"View",0,ocView },
	{0x0B20,"Auto View Off", 0, ocAutoViewOff },
	{0x0B34,"Auto View On", 0, ocAutoViewOn },
	{0x0B58,"Screen Width", 0, gfxScreenWidth },
	{0x0B74,"Screen Height", 0, gfxScreenHeight },
	{0x0B90,"Get Palette",0,gfxGetPalette },
	{0x0BAE,"Cls",0,gfxCls},
	{0x0BB8,"Cls color",0,gfxCls},
	{0x0BC0,"Cls color,x,y,w,h to d,x,y",0,gfxCls},
	{0x0BD0,"Def Scroll",0,gfxDefScroll },
	{0x0C1E,"=XScreen",0,gfxXScreen },
	{0x0C38,"=YScreen",0,gfxYScreen },
	{0x0C52,"=X Text",0,textXText },
	{0x0C60,"=Y Text",0,textYText },
	{0x0C6E,"Screen",0,gfxScreen },
	{0x0C7C,"=Screen",0,gfxGetScreen },
	{0x0C84,"Hires",0,gfxHires },
	{0x0C90,"Lowres",0,gfxLowres },
	{0x0CCA,"Wait Vbl", 0,gfxWaitVbl },
	{0x0CD8,"Default Palette",0,gfxDefaultPalette },
	{0x0CEE,"Default",0,gfxDefault },
	{0x0CFC,"Palette",0,gfxPalette },
	{0x0D0A,"Colour Back",0,gfxColourBack },		// dummy function
	{0x0D1C,"Colour",0,gfxColour },
	{0x0D2C,"=Colour(n)", 0, gfxGetColour },
	{0x0D34,"Flash Off",0,gfxFlashOff },
	{0x0D44,"Flash",0,gfxFlash },
	{0x0D52,"Shift Off",0,gfxShiftOff },
	{0x0D62,"Shift Up",0,gfxShiftUp },
	{0x0D90,"Set Rainbow",0,gfxSetRainbow },
	{0x0DDC,"Rainbow",0,gfxRainbow },
	{0x0DF0,"Rain",0,gfxRain },
	{0x0DFE,"Fade",0,gfxFade },
	{0x0E2C, "Autoback", 0, gfxAutoback },
	{0x0E3C,"Plot",0,gfxPlot },
	{0x0E4A,"Plot x,y,c",0,gfxPlot },
	{0x0E56,"Point",0,gfxPoint },
	{0x0E64,"Draw",0,gfxDraw },
	{0x0E74,"Draw",0,gfxDraw },
	{0x0E86,"Ellipse",0,gfxEllipse },
	{0x0E9A,"Circle",0,gfxCircle },
	{0x0EAC,"Polyline",0,gfxPolyline },
	{0x0EBA,"Polygon",0,gfxPolygon },
	{0x0EC8,"Bar",0,gfxBar },
	{0x0ED8,"Box",0,gfxBox },
	{0x0EE8, "Paint",0, gfxPaint },
	{0x0F04,"Gr Locate",0,gfxGrLocate },
	{0x0F4A,"Text",0,gfxText },	
	{0x0F6A,"Set Paint",0,gfxSetPaint },			// dummy function
	{0x1022,"Set Pattern",0,gfxSetPattern },
	{0x1034,"Set Line",0,gfxSetLine },
	{0x1044,"Ink",0,gfxInk },
	{0x1050,"Ink",0,gfxInk },
	{0x105A,"Ink n,n,n",0,gfxInk },
	{0x1066,"Gr Writing",0,gfxGrWriting },			// needs more work.
	{0x1078,"Clip",0,gfxClip },					// dummy function
	{0x1084,"Clip",0,gfxClip },					// dummy function
	{0x10AC,"Set Tempras",0,gfxSetTempras },		// dummy function
	{0x10B6,"Appear",0,gfxAppear },
	{0x10D6,"zoom",0,gfxZoom },
	{0x11D8,"Key State",0,cmdKeyState },
	{0x11E8,"Key Shift",0,cmdKeyShift },
	{0x123E,"TRUE",0, cmdTrue },
	{0x1248,"FALSE",0, cmdFalse },
	{0x1254,"Put Key",0,cmdPutKey },
	{0x1262,"Scancode",0,cmdScancode },
	{0x1280,"Clear Key",0,cmdClearKey },
	{0x1290,"Wait Key",0,cmdWaitKey },
	{0x129E, "Wait", 0, cmdWait },
	{0x12CE, "Timer", 0, cmdTimer },
	{0x1378,"Locate",0, textLocate },
	{0x1388,"Clw",0,textClw },
	{0x1392,"Home",0,textHome },
	{0x139C,"Curs Pen",0,textCursPen },
	{0x13AC,"Pen$(n)",0,textPenStr },
	{0x13B8,"Paper$(n)",0,textPaperStr },
	{0x13C6,"At(x,y)",0,textAt },
	{0x13D2,"Pen",0,textPen },
	{0x13DC,"Paper",0,textPaper },
	{0x13E8,"Centre",0,textCentre },
	{0x1408,"Writing",0,textWriting },
	{0x1446,"Curs Off",0,textCursOff },
	{0x1454,"Curs On",0,textCursOn },
	{0x1462,"textInverseOff",0,textInverseOff },
	{0x1474,"Inverse On",0,textInverseOn },
	{0x1484,"Under Off",0,textUnderOff },
	{0x1494,"Under On",0,textUnderOn },
	{0x14A2,"Shade Off",0,textShadeOff },
	{0x14B2,"Shade On",0,textShadeOn },
	{0x14E0,"Scroll",0,gfxScroll },
	{0x1504,"Cleft$",0,textCLeftStr },
	{0x151E,"CUp",0,textCUp },
	{0x1528,"CDown",0,textCDown },
	{0x1534,"CLeft",0,textCLeft },
	{0x1540,"CRight",0,textCRight },
	{0x154C,"Memorize X",0,textMemorizeX },
	{0x155C,"Memorize Y",0,textMemorizeY },
	{0x157C,"CMove",0,textCMove },
	{0x159E,"Hscroll",0,textHscroll },	
	{0x158A,"Cline",0,textCline },			
	{0x15AC,"Vscroll",0,textVscroll },		// dummy command.
	{0x15BA,"Set Tab",0,textSetTab },
	{0x15C8,"Set Curs",0,textSetCurs },
	{0x1632,"Reserve Zone", 0, ocReserveZone },
	{0x1646,"Reserve Zone", 0, ocReserveZone },
	{0x164E,"Reset Zone", 0, ocResetZone },
	{0x1668,"Set Zone",0,ocSetZone },
	{0x16B6,"Scin(x,y)",0,gfxScin },
	{0x16E2,"Mouse Zone",0,ocMouseZone },
	{0x16F2,"Set input", 0, cmdSetInput },
	{0x1704, "Close Workbench", 0, cmdCloseWorkbench },
	{0x171A, "Close Editor", 0, cmdCloseEditor },
	{0x172C,"Dir First$",0,cmdDirFirstStr },
	{0x173E,"Dir Next$",0,cmdDirNextStr },
	{0x174E,"Exist",0,cmdExist },
	{0x175A,"Dir$",0,cmdDirStr },
	{0x17AE,"Dir",0,cmdDir },
	{0x17C4,"Set Dir",0,cmdSetDir },
	{0x17D4,"Load iff",0, gfxLoadIff },
	{0x17E4,"Load Iff",0, gfxLoadIff },
	{0x180C, "Bload",0,cmdBload },
	{0x181A, "Bsave", 0, cmdBsave },
	{0x182A,"PLoad",0,machinePload},
	{0x184E,"Load",0,cmdLoad },
	{0x1864,"Dfree",0,cmdDfree },
	{0x187C,"Lof(f)", 0, cmdLof },
	{0x1886,"Eof(f)", 0, cmdEof },
	{0x1890,"Pof(f)", 0, cmdPof },
	{0x18A8,"open random f,name", 0, cmdOpenRandom },
	{0x18BC,"Open In",0,cmdOpenIn },
	{0x18CC,"Open Out",0,cmdOpenOut },
	{0x18F0,"Append",0,cmdAppend },
	{0x190C,"Close",0,cmdClose },
	{0x1914,"Parent",0,cmdParent },
	{0x1920,"Rename",0,cmdRename },
	{0x1930,"Dfree",0,cmdKill },
	{0x1948,"Field f,size as nane$,...", 0, cmdField },
	{0x196C,"Fsel$",0,cmdFselStr },
	{0x1AA8,"Bob Off",0,boBobOff },
	{0x1ABE,"Bob Update Off",0,boBobUpdateOff },
	{0x1B5C,"Limit Bob",0,boLimitBob },
	{0x1B8A,"Set Bob",0,boSetBob },
	{0x1B9E,"Bob",0,boBob },
	{0x1BAE,"Get Sprite Palette",0,hsGetSpritePalette },
	{0x1BFC,"Get Bob",0,boGetBob },
	{0x1CF0,"Put Bob",0,boPutBob },
	{0x1CFE,"Paste Bob",0,boPasteBob },
	{0x1DAE,"Priority On",0,ocPriorityOn },
	{0x1DC0,"Priority Off",0,ocPriorityOff },
	{0x1DD2,"Hide On",0,ocHideOn },
	{0x1D40,"No Mask",0,boNoMask },
	{0x1D90,"Hot Spot",0,boHotSpot },
	{0x1DE0, "Hide", 0, ocHide },						// hide mouse, (only dummy).
	{0x1DEA, "Show On",0,ocShowOn },
	{0x1E02,"Change Mouse",0,ocChangeMouse },		// dummy
	{0x1E16,"X Mouse",0,ocXMouse },
	{0x1E24,"Y Mouse",0,ocYMouse },
	{0x1E32,"Mouse Key",0,ocMouseKey },
	{0x1E42,"Mouse Click",0,ocMouseClick },
	{0x1E6E,"Limit Mouse",0,ocMouseLimit },
	{0x20BA,"X Bob",0,boXBob},
	{0x20C6,"Y Bob",0,boYBob},
	{0x20F2,"",0,cmdReserveAsWork },
	{0x210A,"",0,cmdReserveAsChipWork },
	{0x2128,"",0, cmdReserveAsData },
	{0x2140,"", 0, cmdReserveAsChipData },
	{0x215E, "", 0, cmdErase },
	{0x216A,"", 0, cmdListBank },
	{0x219A,"Fill",0,machineFill},
	{0x21AA,"copy",0,machineCopy},
	{0x21BA,"Hunt",0,machineHunt},
	{0x21CA,"Poke",0,machinePoke},
	{0x21D8,"Loke",0,machineLoke},
	{0x21E6,"Peek",0,machinePeek},
	{0x21F2,"Deek",0,machineDeek},
	{0x21FE,"Leek",0,machineLeek},
	{0x220A,"Bset",0,machineBset},	
	{0x2218,"Bclr",0,machineBclr},
	{0x2226,"Bchg",0,machineBchg},
	{0x2234,"Bbtst",0,machineBtst},
	{0x2242,"ror.b",0,machineRorB},	
	{0x2250,"ror.w",0,machineRorW},
	{0x225E,"ror.l",0,machineRorL},
	{0x226C,"rol.b",0,machineRolB},
	{0x227A,"rol.w",0,machineRolW},
	{0x2288,"rol.l",0,machineRolL},
	{0x2296,"AREG",0,machineAREG},
	{0x22A2,"DREG",0,machineDREG},
	{0x23AC,"Put f,n", 0, cmdPut },
	{0x23B8,"Get f,n", 0, cmdGet },
	{0x23D0,"Multi Wait",0,cmdMultiWait },		// dummy function.
	{0x23FC,"Priority Reverse On",0,ocPriorityReverseOn },
	{0x2416,"Priority Reverse Off",0,ocPriorityReverseOff },
	{0x2476,"Hrev(n)",0,boHrev},
	{0x2482,"Vrev(n)",0,boVrev},
	{0x248e,"Rev(n)",0,boRev},
	{0x24AA,"Amos To Front",0,cmdAmosToFront},
	{0x24BE,"Amos To Back",0,cmdAmosToBack},
	{0x2516, "Ntsc", 0, gfxNtsc },		// only reports false.
	{0x25A4, "Else If", 2, cmdElseIf },
	{0xFF4C,"or",0, orData },
	{0xFF58,"or",0, andData },
	{0xFF66,"not equal",0,cmdNotEqual },
	{0xFF7A,"<=",0,lessOrEqualData },
	{0xFF84,"<=",0,lessOrEqualData },
	{0xFF8E,">=",0,moreOrEqualData },
	{0xFF98,">=",0,moreOrEqualData },
	{0xFFA2,"=", 0, setVar },
	{0xFFAC,"<",0, lessData },
	{0xFFB6,">",0, moreData },
	{0xFFC0,"+",0, addData },
	{0xFFCA,"-", 0, subData },
	{0xFFD4,"mod",0,modData },
	{0xFFE2,"*", 0, mulData },
	{0xFFEC,"/", 0, divData },
	{0xFFF6,"^", 0, powerData },
//	{0x0000,"GFXCALL",0,machineGFXCALL},	
//	{0x0000,"INTCALL",0,machineINTCALL},	
	{0x161E,"xgr",0,gfxXGR },
	{0x1628,"ygr",0,gfxYGR },
};

int nativeCommandsSize = sizeof(nativeCommands)/sizeof(struct nativeCommand);

const char *noName = "<not found>";

const char *TokenName( unsigned short token )
{
	struct nativeCommand *cmd;
	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id ) 
		{
			return cmd-> name;
		}
	}
	return noName;
}

char *executeToken( char *ptr, unsigned short token )
{
	struct nativeCommand *cmd;
	char *ret;

	currentLine = getLineFromPointer( ptr );	// maybe slow!!!

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id ) 
		{
#ifdef show_token_numbers_yes
			printf("%08d   %08X %20s:%08d stack is %d cmd stack is %d flag %d token %04x -- name %s\n",
					getLineFromPointer(ptr), ptr,__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token , TokenName(token));	
#endif
			ret = cmd -> fn( cmd, ptr ) ;
			if (ret) ret += cmd -> size;
			return ret;
		}
	}

#ifdef show_token_numbers_yes
	printf("%08d   %08X %20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
					getLineFromPointer(ptr),
					ptr,__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token);	
#endif


	token_not_found = token;
	currentLine = getLineFromPointer( ptr );
	setError(23, ptr);
	printf("Addr %08x, token not found %04X\n", ptr, token_not_found);

	return NULL;
}

char *token_reader( char *start, char *ptr, unsigned short lastToken, unsigned short token, int tokenlength )
{
	ptr = executeToken( ptr, token );

	if (stack<0)
	{
		printf("dog fart, stinky fart at line %d, stack is %d\n",getLineFromPointer(ptr),stack);
		return NULL;
	}

	if ( ( (long long int) ptr - (long long int) start)  >= tokenlength ) return NULL;

	return ptr;
}

void code_reader( char *start, int tokenlength )
{
	char *ptr;
	int token = 0;
	last_token = 0;
	
	interpreter_running = true;

	currentLine = 0;
	ptr = start;
	while ( ptr = token_reader(  start, ptr,  last_token, token, tokenlength ) )
	{
		// this basic for now, need to handel "on error " commands as well.

		ptr = onError( ptr );
		if (ptr == NULL) break;

		if (every_on)
		{
			gettimeofday(&every_after, NULL);	// reset diff.
			unsigned int ms_before = (every_before.tv_sec * 1000) + (every_before.tv_usec/1000);
			unsigned int ms_after = (every_after.tv_sec * 1000) + (every_after.tv_usec/1000);

			if (((ms_after - ms_before) / 20) >= every_timer)
			{
				every_before = every_after;
				
				if (on_every_gosub_location)
				{
					stackCmdLoop( _gosub, ptr+2 );
					ptr = on_every_gosub_location;

					every_on = false;
				}

				if (on_every_proc_location)
				{
					stackCmdLoop( _procedure, ptr+2 );
					ptr = on_every_proc_location;

					every_on = false;
				}
			}
		}

		last_token = token;
		token = *((short *) ptr);
		ptr += 2;	// next token.		

		if (running == false) break;
	}

	interpreter_running = false;
}

char *filename = NULL;

void set_default_filename()
{
	if (filename) return;

//	filename = strdup("amos-test/var.amos");
//	filename = strdup("amos-test/var_num.amos");
//	filename = strdup("amos-test/math.amos");
//	filename = strdup("amos-test/dim2.amos");
//	filename = strdup("amos-test/input.amos");
//	filename = strdup("amos-test/goto.amos");
//	filename = strdup("amos-test/if.amos");
//	filename = strdup("amos-test/goto2.amos");
//	filename = strdup("amos-test/do-loop.amos");
//	filename = strdup("amos-test/repeat-until.amos");
//	filename = strdup("amos-test/legal-ilegal-if.amos");
//	filename = strdup("amos-test/while-wend.amos");
//	filename = strdup("amos-test/for-to-step-next.amos");
//	filename = strdup("amos-test/for-to-next.amos");
//	filename = strdup("amos-test/for-to-next2.amos");
//	filename = strdup("amos-test/gosub-return.amos");
//	filename = strdup("amos-test/left-mid-right.amos");
//	filename = strdup("amos-test/instr.amos");
//	filename = strdup("amos-test/upper-lower-flip-spaces.amos");
//	filename = strdup("amos-test/str-chr-asc-len.amos");
//	filename = strdup("amos-test/hex-bin-val-str.amos");
//	filename = strdup("amos-test/casting_int_float.amos");
//	filename = strdup("amos-test/arithmetic.amos");
//	filename = strdup("amos-test/inc-dec-add.amos");
//	filename = strdup("amos-test/compare-strings.amos");
//	filename = strdup("amos-test/procedure.amos");
//	filename = strdup("amos-test/procedure2.amos");
//	filename = strdup("amos-test/procedure_with_paramiters_x.amos");
//	filename = strdup("amos-test/procedure-shared.amos");
//	filename = strdup("amos-test/procedure-global.amos");
//	filename = strdup("amos-test/procedure_return_value.amos");
//	filename = strdup("amos-test/procedure_all_params.amos");
//	filename = strdup("amos-test/procedure_pop_proc.amos");
//	filename = strdup("amos-test/reserve.amos");
//	filename = strdup("amos-test/erase-start-length-bsave-bload.amos");
//	filename = strdup("amos-test/sort.amos");
//	filename = strdup("amos-test/or.amos");
//	filename = strdup("amos-test/logical1.amos");
//	filename = strdup("amos-test/match.amos");
//	filename = strdup("amos-test/dir.amos");
//	filename = strdup("amos-test/dir_str.amos");
//	filename = strdup("amos-test/parent-set-dir.amos");
//	filename = strdup("amos-test/fsel_exits_dir_first_dir_next.amos");
//	filename = strdup("amos-test/open-out.amos");
//	filename = strdup("amos-test/open-in.amos");
//	filename = strdup("amos-test/line_input_file.amos");
//	filename = strdup("amos-test/line-input.amos");
//	filename = strdup("amos-test/set-input-input-eof-pof.amos");
//	filename = strdup("amos-test/open_random.amos");
//	filename = strdup("amos-test/dir_first_dir_next.amos");
//	filename = strdup("amos-test/on_error_goto.amos");
//	filename = strdup("amos-test/on_error_proc.amos");
//	filename = strdup("amos-test/on_gosub.amos");
//	filename = strdup("amos-test/input_two_args.amos");
//	filename = strdup("amos-test/exit.amos");
//	filename = strdup("amos-test/exit2.amos");
//	filename = strdup("amos-test/exit-if.amos");
//	filename = strdup("amos-test/every.amos");
//	filename = strdup("amos-test/timer.amos");
//	filename = strdup("amos-test/string_compare.amos");
//	filename = strdup("amos-test/close-wb-editor-break.amos");
//	filename = strdup("amos-test/if-then-else-if-end-if.amos");
//	filename = strdup("amos-test/sin.amos");
//	filename = strdup("amos-test/m.amos");
//	filename = strdup("amos-test/varptr.amos");
//	filename = strdup("amos-test/fill.amos");
//	filename = strdup("amos-test/hunt.amos");
//	filename = strdup("amos-test/rol-ror.amos");
//	filename = strdup("amos-test/bit.amos");
//	filename = strdup("amos-test/asm.amos");
//	filename = strdup("amos-test/doscall.amos");
//	filename = strdup("amos-test/execall.amos");
//	filename = strdup("amos-test/pload.amos");
//	filename = strdup("amos-test/def-fn.amos");
//	filename = strdup("amos-test/swap.amos");
//	filename = strdup("amos-test/data.amos");
//	filename = strdup("amos-test/restore.amos");
	filename = strdup("amos-test/not.amos");
}


int main(char args, char **arg)
{
	BOOL runtime = FALSE;
	FILE *fd;
	int amos_filesize;
	char amosid[17];
	char *data;
	int n;

	if (args == 2)
	{
		filename = strdup(arg[1]);
	}

	if (filename == NULL)
	{
		set_default_filename();
	}

	amosid[16] = 0;	// /0 string.

	stack = 0;
	cmdStack = 0;
	onError = onErrorBreak;

	memset(globalVars,0,sizeof(globalVars));

	sig_main_vbl = AllocSignal(-1);

	for (n=0;n<64;n++)
	{
		bobs[n].image = -1;
	}


	if (init())
	{
		start_engine();

		fd = fopen(filename,"r");

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
				_file_start_ = data;
				_file_end_ = data + tokenlength;

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

//			if (kittyError.newError == true)
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
		if (zones) free(zones);

//		dumpLineAddress();

		running = false;
		wait_engine();

		closedown();
	}

	if (sig_main_vbl) FreeSignal(sig_main_vbl);
	
	if (filename) free(filename);


	return 0;
}

