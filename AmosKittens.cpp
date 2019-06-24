
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <math.h>

#include <signal.h>

#include "config.h"

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <libraries/retroMode.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#include <byteswap.h>
#endif

#ifdef __LITTLE_ENDIAN__
#warning "This is a little endien CPU, Amos was made for big endien CPU's"
#include "os/littleendian/littleendian.h"
#endif

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
#include "commandsBackgroundGraphics.h"
#include "commandsAmal.h"
#include "commandsMenu.h"
#include "commandsFonts.h"
#include "commandsgui.h"
#include "debug.h"
#include "errors.h"
#include "pass1.h"
#include "init.h"
#include "cleanup.h"
#include "engine.h"
#include "AmalCompiler.h"
#include "channel.h"
#include "spawn.h"
#include "label.h"
#include "amosstring.h"

#include "ext_compact.h"
#include "ext_turbo.h"

bool running = true;
bool interpreter_running = false;

int sig_main_vbl = 0;
int proc_stack_frame = 0;

bool ext_crc();

#ifdef enable_vars_crc_yes
unsigned int _vars_crc = 0;
unsigned int str_crc( char *name );
unsigned int vars_crc();
#endif 

#ifdef enable_bank_crc_yes
uint32_t bank_crc = 0;
#endif

struct stringData *var_param_str = NULL;
int var_param_num;
double var_param_decimal;

char *_file_start_ = NULL;
char *_file_pos_  = NULL;		// the problem of not knowing when stacked commands are executed.
char *_file_end_ = NULL;
uint32_t _file_bank_size = 0;

struct retroVideo *video = NULL;
struct retroScreen *screens[8] ;

int parenthesis_count = 0;
int cmdStack = 0;
int procStackCount = 0;
int last_var = 0;
int32_t tokenlength;

unsigned int amiga_joystick_dir[4];
unsigned int amiga_joystick_button[4];

struct extension_lib	kitty_extensions[32];

unsigned short token_not_found = 0xFFFF;	// so we know its not a token, token 0 exists.

struct stackFrame procStcakFrame[PROC_STACK_SIZE];

char *_get_var_index( glueCommands *self, int nextToken);

char *(*do_var_index) ( glueCommands *self, int nextToken ) = _get_var_index;
char *(**do_to) ( struct nativeCommand *, char * ) ;
void (**do_input) ( struct nativeCommand *, char * ) ;
void (*do_breakdata) ( struct nativeCommand *, char * ) = NULL;

extern char *_errTrap( struct glueCommands *data, int nextToken );

int tokenMode = mode_standard;

struct retroSprite *icons = NULL;
struct retroSprite *sprite = NULL;
struct retroSprite *patterns = NULL;

struct retroSpriteObject bobs[64];

//struct proc procStack[1000];	// 0 is not used.
struct globalVar globalVars[VAR_BUFFERS];	// 0 is not used.

struct kittyFile kittyFiles[10];
struct zone *zones = NULL;
int zones_allocated = 0;

int globalVarsSize = sizeof(globalVars)/sizeof(struct globalVar);

ChannelTableClass *channels = NULL;

std::vector<struct label> labels;	// 0 is not used.
std::vector<struct lineAddr> linesAddress;
std::vector<struct defFn> defFns;
std::vector<struct kittyBank> kittyBankList;

int global_var_count = 0;
int labels_count = 0;

struct glueCommands cmdTmp[100];	
struct glueCommands input_cmd_context;

extern char *nextToken_pass1( char *ptr, unsigned short token );

bool breakpoint = false;
bool token_is_fresh = true;

const char *str_dump_stack = "dump stack";
const char *str_dump_prog_stack = "dump prog stack";
const char *str_dump_vars = "dump vars";
const char *str_dump_banks = "dump banks";
const char *str_dump_screen_info = "dump screen info"; 

const char *str_breakpoint_on = "breakpoint on";
const char *str_breakpoint_off = "breakpoint off";

const char *str_warning = "warning";
const char *str_pause = "pause";
const char *str_hint = "hint ";
const char *str_show_var = "show var ";

const char *str_time_start = "time start";
const char *str_time_end = "time end";

int findVar( char *name, bool  is_first_token, int type, int _proc );

extern void dumpScreenInfo();

bool alloc_video()
{

#ifdef __amigaos4__
	video = retroAllocVideo( 640,480 );
#endif

#ifdef __linux__
	video = retroAllocVideo();
#endif

	retroAllocSpriteObjects(video,64);
	return true;
}

void free_video()
{
	if (video)
	{
		uint32_t n;

		for (n=0; n<8;n++)
		{
			if (screens[n]) retroCloseScreen(&screens[n]);
		}

		retroFreeVideo(video);
	}
}

struct timeval debug_time_start,debug_time_end;

bool show_var ( char *ptr, char *var_name, int proc )
{
	int ref = findVar( var_name, false, type_int, proc );

	if (ref)
	{
		printf("line %d, int var: [%s]=%d\n",getLineFromPointer( ptr ), var_name, globalVars[ref-1].var.integer.value);
		return true;
	}
	else
	{
		ref = findVar( var_name, false, type_string, proc );

		if (ref)
		{
			printf("line %d, string var: [%s]=%s\n",getLineFromPointer( ptr ), var_name, globalVars[ref-1].var.str);
			return true;
		}
	}

	return false;
}

char *cmdRem(nativeCommand *cmd, char *ptr)
{
	int length = *((short *) ptr);
	int length_in_bytes = length + (length&1);		// round up to 2.

	if (length>4)
	{
		char *txt = strndup( ptr + 3, length );
		if (txt)
		{
			if (strncmp(txt,str_dump_vars,strlen(str_dump_vars))==0)
			{
				dump_global();
			}
			else if (strncmp(txt,str_dump_screen_info,strlen(str_dump_screen_info))==0)
			{
				dumpScreenInfo();
			}
			else if (strncmp(txt,str_dump_banks,strlen(str_dump_banks))==0)
			{
				dump_banks();
			}
			else if (strncmp(txt,str_hint,strlen(str_hint))==0)
			{
				printf("stack %d at line %d, hint: %s\n",stack, getLineFromPointer( ptr ), txt+strlen(str_hint));
			}
			else if (strncmp(txt,str_dump_stack,strlen(str_dump_stack))==0)
			{
				printf("stack %d at line %d\n",stack, getLineFromPointer( ptr ));
			}
			else if (strncmp(txt,str_dump_prog_stack,strlen(str_dump_stack))==0)
			{
				dump_prog_stack();
				printf("<press enter to continue>\n");
				getchar();
			}
			else if (strncmp(txt,str_pause,strlen(str_pause))==0)
			{
				printf("line %d -- <press enter to continue>\n", getLineFromPointer( ptr ));
				getchar();
			}
			else if (strncmp(txt,str_show_var,strlen(str_show_var))==0)
			{
				char *var_name = txt +strlen(str_show_var);
				char *c;

				for (c=var_name;*c;c++) if (*c==' ') *c = 0;

				if (show_var( ptr, var_name, procStcakFrame[ proc_stack_frame].id ) == false )
				{
					if (show_var( ptr, var_name, 0) == false )
					{
						printf("line %d, var: [%s] not found\n", getLineFromPointer( ptr ), var_name);
					}
				}
				getchar();
			}
			else if (strncmp(txt,str_breakpoint_on,strlen(str_breakpoint_on))==0)
			{
				breakpoint = true;
			}
			else if (strncmp(txt,str_breakpoint_off,strlen(str_breakpoint_off))==0)
			{
				breakpoint = false;
			}
			else if (strncmp(txt,str_warning,strlen(str_warning))==0)
			{
				printf("**********************************\n");
				printf(" %s\n",txt+strlen(str_warning));
				printf("**********************************\n");
			}
			else if (strncmp(txt,str_time_start,strlen(str_time_start))==0)
			{
				printf("time recorded\n");
				gettimeofday(&debug_time_start, NULL);
			}
			else if (strncmp(txt,str_time_end,strlen(str_time_end))==0)
			{
				gettimeofday(&debug_time_end, NULL);
				printf("total time %f seconds\n",	(double) (debug_time_end.tv_usec - debug_time_start.tv_usec) / 1000000 +
											(double) (debug_time_end.tv_sec - debug_time_start.tv_sec)  );
			}

			free(txt);
		}
	}
	
	setStackNum(0);

	return ptr + length_in_bytes;
}


char *nextCmd(nativeCommand *cmd, char *ptr)
{
	char *ret = NULL;
	unsigned int flags;

	// we should empty stack, until first/normal command is not a parm command.

	while (cmdStack)
	{
		flags = cmdTmp[cmdStack-1].flag;

		if  ( ! (flags & cmd_onNextCmd) ) break;		// needs to be include tags, (if commands be excuted on endOfLine or Next command)
		ret = cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack], 0);

		if (cmdTmp[cmdStack].flag & cmd_normal)
		{
			if (!cmdStack) break;
			if (cmdTmp[cmdStack-1].cmd != _errTrap ) break;
		}
		if (ret) break;
	}

	do_to[parenthesis_count] = do_to_default;
	tokenMode = mode_standard;
	token_is_fresh = true;

	if (ret) return ret -2;		// when exit +2 token 

	return ptr;
}

char *cmdNewLine(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__ );

	if (cmdStack)
	{
		char *ret = NULL;
		unsigned int flag;
		do 
		{
			flag = cmdTmp[cmdStack-1].flag;
			if  ( flag & ( cmd_proc | cmd_loop | cmd_never ) ) break;

			ret = cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
			if (ret) return ret -4;		// when exit +2 token +2 data

		} while (cmdStack);
	}

	do_to[parenthesis_count] = do_to_default;
	tokenMode = mode_standard;
	token_is_fresh = true;

	if (breakpoint)
	{
		printf("breakpoint at line %d - <press enter for next line>\n",getLineFromPointer( ptr ) );
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

		if (var -> sizeTab == NULL)
		{
			printf("varname: %s\n",globalVars[varNum-1].varName);
			printf("var num: %d\n", varNum-1);

			setError(25, self -> tokenBuffer );
			return NULL;
		}

		_last_var_index = 0; 
		mul  = 1;
		for (n = self -> stack;n<=stack; n++ )
		{
			_last_var_index += (mul * kittyStack[n].integer.value);
			mul *= var -> sizeTab[n- self -> stack];
		}

		var -> index = _last_var_index;
		popStack(stack - self -> stack);

		if ((_last_var_index >= 0)  && (_last_var_index<var->count))
		{
			if ( correct_order( getLastProgStackToken(),  nextToken ) == false )
			{
				dprintf("---hidden ( symbol \n");
				setStackHiddenCondition();
			}

			switch (var -> type & 3)
			{
				case type_int: 
					setStackNum( (&(var -> int_array -> ptr) + _last_var_index) -> value );
					break;

				case type_float:
					setStackDecimal( (&(var -> float_array -> ptr) + _last_var_index) -> value );	
					break;

				case type_string:	
					struct stringData *str = *(&(var -> str_array -> ptr) + _last_var_index);

					if (str == NULL) 
					{
						setStackStr( toAmosString( "", 0) );
					}
					else
					{
						setStackStrDup( str );
					}
					break;
			}

			flushCmdParaStack(nextToken);
		}
	}

	return NULL;
}

char *_alloc_mode_off( glueCommands *self, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	do_input[parenthesis_count] = do_std_next_arg;
	do_var_index = _get_var_index;

	return NULL;
}

char *do_var_index_alloc( glueCommands *cmd, int nextToken)
{
	int size = 0;
	int n;
	int varNum;
	struct kittyData *var;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	varNum = cmd -> lastVar;	
	if (varNum == 0) return NULL;

	var = &globalVars[varNum-1].var;
	var -> cells = stack - cmd -> stack +1;

	if (var -> sizeTab) free( var -> sizeTab);

	var -> sizeTab = (int *) malloc( sizeof(int) * var -> cells );

	if (var -> sizeTab)
	{
		var -> count = 1;
		for (n= 0; n<var -> cells; n++ ) 
		{
			var -> sizeTab[n] = kittyStack[cmd -> stack + n].integer.value + 1;
			var -> count *= var -> sizeTab[n];
		}

		switch (var -> type)
		{
			case type_int | type_array:

				size = var -> count * sizeof(valueData);

				var -> int_array = (struct valueArrayData *) malloc( sizeof(struct valueArrayData) + size ) ;
				var -> int_array -> type = type_int | type_array;
				var -> int_array -> size = var -> count;
				memset( &var -> int_array -> ptr, 0, size );
				break;

			case type_float | type_array:

				size = var -> count * sizeof(desimalData);

				var -> float_array = (struct desimalArrayData *) malloc( sizeof(struct desimalArrayData) + size ) ;
				var -> float_array -> type = type_float | type_array;
				var -> float_array -> size = var -> count;
				memset( &var -> float_array -> ptr, 0, size );
				break;

			case type_string | type_array:

				size = var -> count * sizeof(struct stringData *);

				var -> str_array = (struct stringArrayData *) malloc( sizeof(struct stringArrayData) + size ) ;
				var -> str_array -> type = type_string | type_array;
				var -> str_array -> size = var -> count;
				memset( &var -> str_array -> ptr, 0, size );
				break;

			default: setError(22, cmd -> tokenBuffer);
		}
	}

 

	popStack(stack - cmd -> stack);

	printf("cmd stack loc %d, satck is %d\n",cmd -> stack, stack);

	return NULL;
}

void do_std_next_arg(nativeCommand *cmd, char *ptr)
{
	stack++;
	setStackNone();
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
	do_input[parenthesis_count] = do_dim_next_arg;
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


char *cmdVar(nativeCommand *cmd, char *ptr)
{
	struct reference *ref = (struct reference *) ptr;
	unsigned short next_token = *((short *) (ptr+sizeof(struct reference)+ref->length));
	
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	token_is_fresh = false;

	last_var = ref -> ref;

	if (next_token == 0x0074)	// ( symbol
	{
		if (do_var_index) stackCmdIndex( do_var_index, ptr );
	}
	else
	{
		if ( correct_order( getLastProgStackToken(),  next_token ) == false )
		{
			dprintf(" hidden ( condition.\n");
			setStackHiddenCondition();
		}

		if (ref -> ref)
		{
			int idx = ref->ref-1;

			switch (globalVars[idx].var.type & 7)
			{
				case type_int:
					setStackNum(globalVars[idx].var.integer.value);
					break;
				case type_float:
					setStackDecimal(globalVars[idx].var.decimal.value);
					break;
				case type_string:
					setStackStrDup(globalVars[idx].var.str);		// always copy.
					break;
				case type_proc:
					stackCmdLoop( _procedure, ptr+sizeof(struct reference)+ref->length ) ;

					// size of ref is added on exit, then +2 next token

					return globalVars[idx].var.tokenBufferPos - sizeof(struct reference) -2;	
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

	// check if - or + comes before *, / or ; symbols
	
	length2 += (length & 1);		// align to 2 bytes
	next_token = *((short *) (ptr+2+length2) );

	if ( correct_order( getLastProgStackToken(),  next_token ) == false )
	{
		// hidden ( condition.
		setStackHiddenCondition();
	}

	setStackStr( toAmosString( ptr + 2, length ) );
	kittyStack[stack].state = state_none;
	flushCmdParaStack( (int) next_token );

	return ptr + length2;
}


char *cmdNumber(nativeCommand *cmd, char *ptr)
{
	unsigned short next_token = *((short *) (ptr+4) );
	proc_names_printf("%s:%s:%d \n",__FILE__,__FUNCTION__,__LINE__);

	// check if - or + comes before *, / or ; symbols

	if ( correct_order( getLastProgStackToken(),  next_token ) == false )
	{
		dprintf("---hidden ( symbol \n");
		setStackHiddenCondition();
	}

	setStackNum( *((int *) ptr) );
	kittyStack[stack].state = state_none;
	flushCmdParaStack( next_token );

	return ptr;
}


#define fast_float_yes
#define fast_exp_yes

#ifdef fast_float_yes

double _float[256];
double _exp[0x7F];

void make_float_lookup()
{
	unsigned int number1;
	int n = 0;
	int e;
	int ee;
	double f;

	proc_names_printf("%s:%s:%d \n",__FILE__,__FUNCTION__,__LINE__);

	for (number1=0;number1<256;number1++)
	{
		f = 0.0f;
		for (n=7;n>-1;n--)
		{
			if ((1<<n)&number1) f += 1.0f / (double) (1<<(7-n));
		}
		_float[number1]=f;
	}


	for (n=0;n<=0x7F;n++)
	{
		f = 1.0f;
		e = n & 0x3F;
		if ( ( n & 0x40)  == 0)	e = -(65 - e);

		ee = e;

		if (e==0) { f/=2.0; }
		else if (e<0) { while (e) {  f /= 2.0f; e++; } }
		else if (e>0) { while (--e) { f *= 2.0; } }

		_exp[n]=f;

	}
}

#endif

char *cmdFloat(nativeCommand *cmd,char *ptr)
{
	double f = 0.0f;
	unsigned short next_token = *((short *) (ptr+4 ));
	proc_names_printf("%s:%d \n",__FUNCTION__,__LINE__);

	// check if - or + comes before *, / or ; symbols

	if ( correct_order( getLastProgStackToken(),  next_token ) == false )
	{
		dprintf("---hidden ( symbol \n");
		setStackHiddenCondition();
	}

	{
		unsigned int data = *((unsigned int *) ptr);
		unsigned int number1 = data >> 8;

#ifdef fast_exp_no
		int e = (data & 0x3F) ;
		if ( (data & 0x40)  == 0)	e = -(65 - e);
#endif

#ifdef fast_float_yes
		f = _float[ (number1 & 0xFF0000) >> 16 ] ;
		f += (_float[ (number1  & 0xFF00) >> 8 ] ) / (double) (1<<8);
		f += _float[ (number1  & 0xFF) ]  / (double) (1<<16);
#else

		for (int n=23;n>-1;n--)
		{
			if ((1<<n) & number1) f += 1.0f / (double) (1<<(23-n));
		}
#endif

#if defined(fast_float_yes) && defined(fast_exp_yes)
		f *= _exp[ data & 0x7F ];
#else
		if (e==0) { f/=2.0; }
		else if (e<0) { while (e) {  f /= 2.0f; e++; } }
		else if (e>0) { while (--e) { f *= 2.0; } }
#endif

	}

	setStackDecimal( f );

	kittyStack[stack].state = state_none;
	flushCmdParaStack( next_token );

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
	{0x00E0,"Resume Next",0,errResumeNext },
	{0x00F2,"Inkey$",0,cmdInkey },
	{0x00FE,"Repeat$",0, cmdRepeatStr },
	{0x010E,"Zone$", 0, ocZoneStr },
	{0x011C,"Border$", 0, textBorderStr },			// needs more work.
	{0x012C,"Double Buffer",0,gfxDoubleBuffer },
	{0x0140, "Start", 0, bankStart },
	{0x014C, "Length", 0, bankLength },
	{0x015A,"Doke",0,machineDoke},
	{0x019C, "Every On", 0, cmdEveryOn },
	{0x01AA, "Every Off", 0, cmdEveryOff },
	{0x01C8,"Logic",0,gfxLogic },	// current screen
	{0x01D4,"Logic",0,gfxLogic },	// =Logic(screen)
	{0x01dc, "Asc",0, cmdAsc },
	{0x01E6,"At",0,textAt },
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
	{0x02E6, "on error", 0, errOnError },
	{0x0316, "On", 4, cmdOn },
	{0x031E, "Resume Label", 0, errResumeLabel },
	{0x0330, "Resume", 0, errResume }, 
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
	{0x03C0, "Stop", 0, cmdStop },
	{0x03CA, "Param#",0,cmdParamFloat },
	{0x03D6, "Param$",0,cmdParamStr },
	{0x03E2, "Param",0,cmdParam },
	{0x03EE, "Error", 0, errError },
	{0x03FA, "Errn",0,errErrn },
	{0x0404,"data", 2, cmdData },		
	{0x040E,"read",0,cmdRead },
	{0x0418,"Restore", 0, cmdRestore },
	{0x0426, "Break Off", 0, cmdBreakOff },
	{0x0436, "Break On", 0, cmdBreakOn },
	{0x0444, "Inc",0,mathInc },
	{0x044E, "Dec",0,mathDec },
	{0x0458, "Add",0,mathAdd },
	{0x0462, "Add var,n,f TO t", 0, mathAdd },
	{0x046A, "Print #",0,discPrintOut },
	{0x0476, "Print",0,textPrint },
	{0x0482, "LPrint",0,textPrint },
	{0x048E,"Input$(n)",0,cmdInputStrN },
	{0x049C,"Input$(f,n)", 0, discInputStrFile },
	{0x04A6,"Using",0,textPrintUsing },
	{0x04B2, "Input #",0, discInputIn },
	{0x04BE, "Line Input #",0,discLineInputFile },
	{0x04D0, "Input",0,cmdInput },
	{0x04DC, "Line Input", 0, cmdLineInput },
//	{0x04F6, "Run", 0, cmdRun },
	{0x04F6, "Run",0,cmdEnd },
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
	{0x05F4, "Instr",0, cmdInstr },	// POS=Instr(ITEM$,"@",CHARNUM)
	{0x0600,"Tab$",0,cmdTabStr },
	{0x060A,"Free",0,machineFree },
	{0x0614,"Varptr",0,machineVarPtr},
	{0x0620,"Remember X",0,textRememberX },
	{0x0630,"Remember Y",0,textRememberY },
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
	{0x0790,"Menu To Bank",0,menuMenuToBank },
	{0x07B8,"Menu On",0,menuMenuOn },
	{0x07C6,"Menu Off",0,menuMenuOff },
	{0x07D4,"Menu Calc",0,menuMenuCalc },
	{0x081E,"Set Menu",0,menuSetMenu },
	{0x0832,"Menu X",0,menuMenuX },
	{0x0840,"Menu Y",0,menuMenuY },
	{0x08EA,"Menu Active",0,menuMenuActive },
	{0x08FC,"Menu Inactive",0,menuMenuInactive },
	{0x0956,"Menu Del",0,menuMenuDel },
	{0x0964,"Menu$",0,menuMenuStr },
	{0x0970,"Choice",0,menuChoice },
	{0x097E,"Choice",0,menuChoice },
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
	{0x0AC8,"Screen Show",0,gfxScreenShow },
	{0x0ADA,"Screen Show",0,gfxScreenShow },
	{0x0AE2,"Screen Swap",0,gfxScreenSwap },	// screen swap
	{0x0AF4,"Screen Swap",0,gfxScreenSwap },	// screen swap <screen num>
	{0x0AFC,"Save Iff",0,gfxSaveIff },
	{0x0B16,"View",0,ocView },
	{0x0B20,"Auto View Off", 0, ocAutoViewOff },
	{0x0B34,"Auto View On", 0, ocAutoViewOn },
	{0x0B46,"Screen Base", 0, gfxScreenBase },
	{0x0B58,"Screen Width", 0, gfxScreenWidth },
	{0x0B88,"Screen Height", 0, gfxScreenHeight },		// =Screen Height(screen nr)
	{0x0B74,"Screen Height", 0, gfxScreenHeight },		// =Screen Height
	{0x0B90,"Get Palette",0,gfxGetPalette },
	{0x0BAE,"Cls",0,gfxCls},
	{0x0BB8,"Cls color",0,gfxCls},
	{0x0BC0,"Cls color,x,y,w,h to d,x,y",0,gfxCls},
	{0x0BD0,"Def Scroll",0,gfxDefScroll },
	{0x0BEE,"=X Hard",0,gfxXHard },
	{0x0BFC,"=X Hard(s,x)",0,gfxXHard },
	{0x0C06,"=Y Hard",0,gfxYHard },
	{0x0C14,"=Y Hard(s,y)",0,gfxYHard }, 
	{0x0C1E,"=XScreen",0,gfxXScreen },
	{0x0C2E,"=XScreen(n,n)",0,gfxXScreen },	// =XScreen(0,X Mouse)
	{0x0C38,"=YScreen",0,gfxYScreen },
	{0x0C48,"=YScreen(n,n)",0,gfxYScreen },	// =YScreen(0,Y Mouse)
	{0x0C52,"=X Text",0,textXText },
	{0x0C60,"=Y Text",0,textYText },
	{0x0C6E,"Screen",0,gfxScreen },
	{0x0C7C,"=Screen",0,gfxGetScreen },
	{0x0C84,"Hires",0,gfxHires },
	{0x0C90,"Lowres",0,gfxLowres },
	{0x0C9C,"Dual Playfield",0,gfxDualPlayfield },	// not suppoted, just report error!
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
	{0x0D78,"Shift Down",0,gfxShiftDown },
	{0x0D90,"Set Rainbow",0,gfxSetRainbow },
	{0x0DC2,"Tempo",0,soundTempo },
	{0x0DD4,"Rainbow Del",0,gfxRainbowDel },
	{0x0DDC,"Rainbow",0,gfxRainbow },
	{0x0DF0,"Rain",0,gfxRain },
	{0x0DFE,"Fade",0,gfxFade },
	{0x0E16,"Physic",0,gfxPhysic },
	{0x0E24,"Physic",0,gfxPhysic },
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
	{0x0EE8,"Paint",0, gfxPaint },
	{0x0EF8,"Paint",0,gfxPaint },		// Paint n,n,n
	{0x0F04,"Gr Locate",0,gfxGrLocate },
	{0x0F16,"Text Length",0,textTextLength },
	{0x0F28,"Text Styles",0,textTextStyles },
	{0x0F3A,"Text Base",0,textTextBase },
	{0x0F4A,"Text",0,textText },
	{0x0F5A,"Set Text",0,textSetText },
	{0x0F6A,"Set Paint",0,gfxSetPaint },			// dummy function
	{0x0F7A,"Get Fonts",0,fontsGetAllFonts },
	{0x0F8A,"Get Disc Fonts",0,fontsGetDiscFonts },
	{0x0F9E,"Get Rom Fonts",0,fontsGetRomFonts },
	{0x0FB2,"Set Font",0,fontsSetFont },
	{0x0FC2,"Fonts$",0,fontsFontsStr },
	{0x0FCE,"Hslider",0,gfxHslider },
	{0x0FE8,"Vslider",0,gfsVslider },
	{0x1002,"Set Slider",0,gfxSetSlider },
	{0x1022,"Set Pattern",0,gfxSetPattern },
	{0x1034,"Set Line",0,gfxSetLine },
	{0x1044,"Ink",0,gfxInk },
	{0x1050,"Ink",0,gfxInk },
	{0x105A,"Ink n,n,n",0,gfxInk },
	{0x1066,"Gr Writing",0,textGrWriting },			// needs more work.
	{0x1078,"Clip",0,gfxClip },					// dummy function
	{0x1084,"Clip",0,gfxClip },					// dummy function
	{0x10AC,"Set Tempras",0,gfxSetTempras },		// dummy function
	{0x10B6,"Appear",0,gfxAppear },
	{0x10D6,"zoom",0,gfxZoom },
	{0x10F4,"Get CBlock",0,bgGetCBlock },
	{0x1146,"Get Block",0,bgGetBlock },
	{0x1160,"Get Block [n,x,y,w,h,?] ",0,bgGetBlock },
	{0x1172,"Put Block",0,bgPutBlock },	// Put Block (Num)
	{0x1184,"Put Block",0,bgPutBlock },
	{0x11AE,"Del Block",0,bgDelBlock },	// Del Block (no args)
	{0x11BE,"Del Block",0,bgDelBlock },
	{0x11C6,"Key Speed",0,cmdKeySpeed },
	{0x11D8,"Key State",0,cmdKeyState },
	{0x11E8,"Key Shift",0,cmdKeyShift },
	{0x11F8,"Joy",0,ocJoy },
	{0x1202,"JUp",0,ocJUp },
	{0x120C,"JDown",0,ocJDown },
	{0x1218,"JLeft",0,ocJLeft },
	{0x1224,"JRight",0,ocJRight },
	{0x1232,"Fire",0,ocFire },
	{0x123E,"TRUE",0, cmdTrue },
	{0x1248,"FALSE",0, cmdFalse },
	{0x1254,"Put Key",0,cmdPutKey },
	{0x1262,"Scancode",0,cmdScancode },
	{0x1270,"Scanshift",0,cmdScanshift },
	{0x1280,"Clear Key",0,cmdClearKey },
	{0x1290,"Wait Key",0,cmdWaitKey },
	{0x129E, "Wait", 0, cmdWait },
	{0x12AA,"Key$",0, cmdKeyStr },
	{0x12CE, "Timer", 0, cmdTimer },
	{0x12DA,"Wind Open",0,textWindOpen },
	{0x12F4,"Wind Open",0,textWindOpen },	// Wind Open n,x,y,w,h,border
	{0x131A,"Wind Close",0,textWindClose },	// Wind Close
	{0X132A,"Wind Save",0,textWindSave },	// Wind Save
	{0x133A,"Wind Move",0,textWindMove },	// Wind Move x,y
	{0x134C,"Wind Size",0,textWindSize },
	{0x136C,"=Windon",0,textWindon },					// Windon returns current window number.
	{0x135E,"Window",0,textWindow },
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
	{0x13F6,"Border",0,textBorder },
	{0x1408,"Writing",0,textWriting },
	{0x1418,"Writing",0,textWriting },
	{0x1422,"Title Top",0,textTitleTop },
	{0x1432,"Title Bottom",0,textTitleBottom },
	{0x1446,"Curs Off",0,textCursOff },
	{0x1454,"Curs On",0,textCursOn },
	{0x1462,"textInverseOff",0,textInverseOff },
	{0x1474,"Inverse On",0,textInverseOn },
	{0x1484,"Under Off",0,textUnderOff },
	{0x1494,"Under On",0,textUnderOn },
	{0x14A2,"Shade Off",0,textShadeOff },
	{0x14B2,"Shade On",0,textShadeOn },
	{0x14C0,"Scroll Off",0,gfxScrollOff },
	{0x14E0,"Scroll",0,gfxScroll },
	{0x1504,"Cleft$",0,textCLeftStr },
	{0x151E,"CUp",0,textCUp },
	{0x1528,"CDown",0,textCDown },
	{0x1534,"CLeft",0,textCLeft },
	{0x1540,"CRight",0,textCRight },
	{0x154C,"Memorize X",0,textMemorizeX },
	{0x155C,"Memorize Y",0,textMemorizeY },
	{0x157C,"CMove",0,textCMove },
	{0x1596,"Cline",0,textCline },
	{0x159E,"Hscroll",0,textHscroll },	
	{0x158A,"Cline",0,textCline },			
	{0x15AC,"Vscroll",0,textVscroll },		// dummy command.
	{0x15BA,"Set Tab",0,textSetTab },
	{0x15C8,"Set Curs",0,textSetCurs },
	{0x15E6,"X Curs",0,textXCurs },
	{0x15F2,"Y Curs",0,textYCurs },
	{0x15FE,"X Graphic",0,textXGraphic },
	{0x160E,"Y Graphic",0,textYGraphic },
	{0x161E,"xgr",0,gfxXGR },
	{0x1628,"ygr",0,gfxYGR },
	{0x1632,"Reserve Zone", 0, ocReserveZone },
	{0x1646,"Reserve Zone", 0, ocReserveZone },
	{0x164E,"Reset Zone", 0, ocResetZone },	// Reset Zone
	{0x1660,"Reset Zone", 0, ocResetZone },	// Reset Zone <n>
	{0x1668,"Set Zone",0,ocSetZone },
	{0x1680,"Zone",0,ocZone },			// Zone(x,y)
	{0x168E,"Zone",0,ocZone },			// Zone(screen,x,y)
	{0x16AA,"HZone",0,ocHZone },
	{0x16B6,"Scin(x,y)",0,gfxScin },
	{0x16D0,"Mouse Screen",0,ocMouseScreen },
	{0x16E2,"Mouse Zone",0,ocMouseZone },
	{0x16F2,"Set input", 0, discSetInput },
	{0x1704, "Close Workbench", 0, cmdCloseWorkbench },
	{0x171A, "Close Editor", 0, cmdCloseEditor },
	{0x172C,"Dir First$",0,discDirFirstStr },
	{0x173E,"Dir Next$",0,discDirNextStr },
	{0x174E,"Exist",0,discExist },
	{0x175A,"Dir$",0,discDirStr },
	{0x17A4,"Dir",0,discDir },		// no argument
	{0x17AE,"Dir",0,discDir },
	{0x17B6,"Set Dir",0,discSetDir },
	{0x17C4,"Set Dir",0,discSetDir },
	{0x17D4,"Load iff",0, gfxLoadIff },
	{0x17E4,"Load Iff",0, gfxLoadIff },
	{0x180C, "Bload",0,bankBload },
	{0x181A, "Bsave", 0, bankBsave },
	{0x182A,"PLoad",0,machinePload},
	{0x1838,"Save",0,bankSave },
	{0x1844,"Save",0,bankSave },	// save name,bank
	{0x184E,"Load",0,bankLoad },
	{0x185A,"Load",0,bankLoad },
	{0x1864,"Dfree",0,discDfree },
	{0x1870,"Makedir",0,discMakedir },
	{0x187C,"Lof(f)", 0, discLof },
	{0x1886,"Eof(f)", 0, discEof },
	{0x1890,"Pof(f)", 0, discPof },
	{0x18A8,"open random f,name", 0, discOpenRandom },
	{0x18BC,"Open In",0,discOpenIn },
	{0x18CC,"Open Out",0,discOpenOut },
	{0x18F0,"Append",0,discAppend },
	{0x1900,"Close",0,discClose },	// token used in Help_69.Amos
	{0x190C,"Close",0,discClose },
	{0x1914,"Parent",0,discParent },
	{0x1920,"Rename",0,discRename },
	{0x1930,"Dfree",0,discKill },
	{0x1948,"Field f,size as nane$,...", 0, discField },
	{0x1954,"Fsel$",0,discFselStr },		// found in Help_72.amos
	{0x196C,"Fsel$",0,discFselStr },
	{0x1978,"Fsel$",0,discFselStr },
	{0x1986,"Set Sprite Buffers",0,hsSetSpriteBuffer },
	{0x199E,"Sprite Off",0,hsSpriteOff },
	{0x1A72,"Sprite Base",0,hsSpriteBase },
	{0x1A84,"Icon Base",0,bgIconBase },
	{0x1A94,"Sprite",0,hsSprite },
	{0x1AA8,"Bob Off",0,boBobOff },
	{0x1AB6,"Bob Off [number]",0,boBobOff },
	{0x1ABE,"Bob Update Off",0,boBobUpdateOff },
	{0x1AD2,"Bob Update On",0,boBobUpdateOn },
	{0x1AE6,"Bob Update",0,boBobUpdate },
	{0x1AF6,"Bob Clear",0,boBobClear },
	{0x1B06,"Bob Draw",0,boBobDraw },
	{0x1B36,"Bob Col",0,boBobCol },
	{0x1B46,"Bob Col",0,boBobCol },
	{0x1B52,"Col",0,boCol },
	{0x1B5C,"Limit Bob",0,boLimitBob },
	{0x1B8A,"Set Bob",0,boSetBob },
	{0x1B9E,"Bob",0,boBob },
	{0x1BAE,"Get Sprite Palette",0,hsGetSpritePalette },
	{0x1BD0,"Get Sprite",0,boGetBob },	//  GetBob and GetSprite is the same.
	{0x1BFC,"Get Bob",0,boGetBob },
	{0x1C14,"Get Bob",0,boGetBob },	// get bob 0,0,0,0 to 0,0
	{0x1C42,"Del Bob",0,boDelBob },
	{0x1C5C,"Del Icon",0,bgDelIcon },
	{0x1C6C,"Del Icon",0,bgDelIcon },
	{0x1CA6,"Get Icon Palette", 0, bgGetIconPalette },
	{0x1CC6,"Get Icon", 0, bgGetIcon },
	{0x1CF0,"Put Bob",0,boPutBob },
	{0x1CFE,"Paste Bob",0,boPasteBob },
	{0x1D12,"Paste Icon", 0, bgPasteIcon },
	{0x1D28,"Make Mask", 0, ocMakeMask },
	{0x1D56,"Make Icon Mask", 0, bgMakeIconMask },
	{0x1D6C,"Make Icon Mask", 0, bgMakeIconMask },
	{0x1DA2,"Hot Spot", 0, boHotSpot },
	{0x1DAE,"Priority On",0,ocPriorityOn },
	{0x1DC0,"Priority Off",0,ocPriorityOff },
	{0x1DD2,"Hide On",0,ocHideOn },
	{0x1D40,"No Mask",0,boNoMask },
	{0x1D90,"Hot Spot",0,boHotSpot },
	{0x1DE0, "Hide", 0, ocHide },						// hide mouse, (only dummy).
	{0x1DEA, "Show On",0,ocShowOn },
	{0x1DF8, "Show",0,ocShow },
	{0x1E02,"Change Mouse",0,ocChangeMouse },		// dummy
	{0x1E16,"X Mouse",0,ocXMouse },
	{0x1E24,"Y Mouse",0,ocYMouse },
	{0x1E32,"Mouse Key",0,ocMouseKey },
	{0x1E42,"Mouse Click",0,ocMouseClick },
	{0x1E54,"Limit Mouse",0,ocMouseLimit },
	{0x1E6E,"Limit Mouse",0,ocMouseLimit },
	{0x1E8A,"Move X",0,amalMoveX },
	{0x1EA6,"Move Y",0,amalMoveY },
	{0x1EDA,"Move On",0,amalMoveOn },
	{0x1F22,"Anim On",0,amalAnimOn },
	{0x1F52,"Anim",0,amalAnim },
	{0x1F94,"Channel",0,amalChannel },
	{0x1FA2,"Amreg",0,amalAmReg },
	{0x1FBC,"Amal On",0,amalAmalOn },
	{0x1F30,"Amal On",0,amalAmalOn },
	{0x1FCA,"Amal On",0,amalAmalOn },
	{0x1FE2,"Amal Off",0,amalAmalOff },
	{0x1FD2,"Amal Off",0,amalAmalOff },
	{0x1FEA,"Amal Freeze",0,amalAmalFreeze },
	{0x1FFC,"Amal Freeze",0,amalAmalFreeze },
	{0x2004,"AmalErr",0,amalAmalErr },
	{0x2012,"Amal",0,amalAmal },
	{0x204A,"Synchro On",0,ocSynchroOn },
	{0x205A,"Synchro Off",0,ocSynchroOff },
	{0x206C,"Synchro",0,ocSynchro },
	{0x207A,"Update Off",0,ocUpdateOff },
	{0x208A,"Update On",0,ocUpdateOn },
	{0x209A,"Update Every",0,ocUpdateEvery },
	{0x20AE,"Update",0,ocUpdate },
	{0x20BA,"X Bob",0,boXBob},
	{0x20C6,"Y Bob",0,boYBob},
	{0x20F2,"Reserve As Work",0,bankReserveAsWork },
	{0x210A,"Reserve As Chip Work",0,bankReserveAsChipWork },
	{0x2128,"Reserve As Data",0, bankReserveAsData },
	{0x2140,"Reserve As Chip Data", 0, bankReserveAsChipData },
	{0x215E, "Erase", 0, bankErase },
	{0x216A,"List Bank", 0, bankListBank },
	{0x217A,"Chip Free",0,cmdChipFree },
	{0x218A,"Fast Free",0,cmdFastFree },
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
	{0x23A0,"BGrab", 0, bankBGrab },
	{0x23AC,"Put f,n", 0, discPut },
	{0x23B8,"Get f,n", 0, discGet },
	{0x23D0,"Multi Wait",0,cmdMultiWait },		// dummy function.
	{0x23C4,"System",0,cmdEnd },
	{0x23FC,"Priority Reverse On",0,ocPriorityReverseOn },
	{0x2416,"Priority Reverse Off",0,ocPriorityReverseOff },
	{0x2430,"Dev First$",0,discDevFirstStr },
	{0x2442,"Dev Next$",0,discDevNextStr },
	{0x2452,"Hrev Block",0,bgHrevBlock },
	{0x2464,"Vrev Block",0,bgVrevBlock },
	{0x2476,"Hrev(n)",0,boHrev},
	{0x2482,"Vrev(n)",0,boVrev},
	{0x248e,"Rev(n)",0,boRev},
	{0x2498,"Bank Swap",0,bankBankSwap},	// AmosPro?
	{0x24AA,"Amos To Front",0,cmdAmosToFront},
	{0x24BE,"Amos To Back",0,cmdAmosToBack},
	{0x2516,"Ntsc", 0, gfxNtsc },		// only reports false.
	{0x2520,"Laced",0, gfxLaced },
	{0x253C,"Command Line$", 0, cmdCommandLineStr },
	{0x2578,"Set Accessory",0, cmdSetAccessory },
	{0x259A,"Trap", 0, errTrap },
	{0x25A4,"Else If", 2, cmdElseIf },
	{0x25C0,"Array",0,machineArray },
	{0x260C,"Iff Anim",0,gfxIffAnim },
	{0x2694,"Call Editor", 0, cmdCallEditor },
	{0x26D8,"Erase All", 0, bankEraseAll },
	{0x26E8,"Dialog Box",0,guiDialogBox },		// d=Dialog box(a$)
	{0x2704,"Dialog Box",0,guiDialogBox },		// d=Dialog box(a$,value,b$)
	{0x2710,"Dialog Box",0,guiDialogBox },		// d=Dialog Box(a$,value,b$,n,n)
	{0x2720,"Dialog Open",0,guiDialogOpen },		// d=Dialog Open n,a$
	{0x2742,"Dialog Open",0,guiDialogOpen },
	{0x2750,"Dialog Close",0,guiDialogClose },
	{0x2764,"Dialog Close",0,guiDialogClose },	// Dialog Close <NR>
	{0x276C,"Dialog Run", 0, guiDialogRun },
	{0x277E,"Dialog Run", 0, guiDialogRun },
	{0x2796,"Dialog",0,guiDialog },
	{0x27A4,"Vdialog",0,guiVdialog },
	{0x27B6,"Vdialog$",0,guiVdialogStr },
	{0x27E6,"Dialog$",0,guiDialogStr },
	{0x2804,"EDialog",0,guiEDialog },
	{0x2812,"Dialog Clr",0,guiDialogClr },
	{0x2866,"Dialog Freeze",0,guiDialogFreeze },
	{0x2882,"Dialog Unfreeze",0,guiDialogUnfreeze },
	{0x28A0,"Poke$",0,machinePokeStr },	// Poke$(adr, string)
	{0x28AE,"Peek$",0,machinePeekStr },	// Peek$(adr, length)
	{0x28BE,"Peek$",0,machinePeekStr },	// Peek$(adr, termChar) // returns string
	{0x28CA,"Resource Bank", 0,bankResourceBank }, // AmosPro Command.
	{0x28DE,"Resource$",0,bankResourceStr }, // Resource$
	{0x28EE,"Resource Screen Open",0,guiResourceScreenOpen},
	{0x2946,"Err$" ,0, errErrStr },
	{0x2952,"Assign",0,discAssign },
	{0x2962,"Errtrap",0,errErrTrap },	// AmosPro command
	{0x2A4A,"Lvo",6,machineLvo },	// AmosPro command. (should look up string in pass1 says docs), maybe 16bit BOOL, 32bit offset
	{0x2A74,"Bstart",0,bankBstart },
	{0x2A82,"Blength",0,bankBlength },
	{0x2A90,"Bsend",0,bankBsend}, 
	{0x2A9C,"Bank Shrink",0, bankBankShrink },
	{0x2AB0,"Prg Under",0,cmdPrgUnder },
	{0x2B3E,"Exec",0,cmdExec },
	{0x2B58,"Screen Mode",0,gfxScreenMode },
	{0x2B72,"Kill Editor",0,cmdKillEditor },
	{0x2BAE,"Get Bob Palette",0,hsGetSpritePalette },
	{0xFF4C,"or",0, orData },
	{0xFF3E,"xor",0,xorData },
	{0xFF58,"and",0, andData },
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


char *skipToken( char *ptr, unsigned short token )
{
	struct nativeCommand *cmd;
	char *ret;

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id ) 
		{
			if (ret) ret += cmd -> size;
			return ret;
		}
	}

	return NULL;
}

#ifdef enable_fast_execution_no

char *executeToken( char *ptr, unsigned short token )
{
	struct nativeCommand *cmd;
	char *ret;

	dump_stack();

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id ) 
		{
#ifdef show_token_numbers_yes
			printf("%08d   %08X %20s:%08d stack is %d cmd stack is %d flag %d token %04x -- name %s\n",
					getLineFromPointer(ptr), (unsigned int) ptr,__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token , TokenName(token));	
#endif
			ret = cmd -> fn( cmd, ptr ) ;
			if (ret) ret += cmd -> size;
			return ret;
		}
	}

	token_not_found = token;
	currentLine = getLineFromPointer( ptr );
	setError(23, ptr);
	printf("Addr %08x, token not found %04X at line %d\n", 
				(unsigned int) ptr, 
				(unsigned int) token_not_found, 
				getLineFromPointer( ptr));

	return NULL;
}

#endif

#ifdef enable_fast_execution_yes

char fast_lookup[0xFFFF];

void init_fast_lookup()
{
	int token;
	struct nativeCommand *cmd;

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		token = (int) ((unsigned short) cmd->id) ;

		*((void **) (fast_lookup + token)) = (void *) cmd -> fn;
		*((uint16_t *) (fast_lookup + token + sizeof(void *))) = (uint16_t) cmd -> size;
	}
}

// should be faster, draw back, no token name can be printed.

struct nativeCommand _cmd;

char *executeToken( char *ptr, unsigned short token )
{
	char *(*fn) (struct nativeCommand *cmd, char *tokenBuffer);
	uint16_t size;
	char *ret;

	fn = (char* (*)(nativeCommand*, char*)) *((void **) (fast_lookup + token)) ;
	size = *((uint16_t *) (fast_lookup + token + sizeof(void *)));

	if (fn)
	{
		_cmd.id = token;
		_cmd.size = size;
		ret = fn( &_cmd, ptr ) ;
		if (ret) ret += size;

		return ret;
	}

	token_not_found = token;
	currentLine = getLineFromPointer( ptr );
	setError(23, ptr);
	printf("Addr %08x, token not found %04X at line %d\n", 
				(unsigned int) ptr, 
				(unsigned int) token_not_found, 
				getLineFromPointer( ptr));

	return NULL;
}
#endif


char *_for( struct glueCommands *data, int nextToken );

char *token_reader( char *start, char *ptr, unsigned short token, int tokenlength )
{
	ptr = executeToken( ptr, token );

	if (stack<0)
	{
		printf("dog fart, stinky fart at line %d, stack is %d\n",getLineFromPointer(ptr),stack);
		return NULL;
	}

#ifdef enable_ext_crc_yes

	if (ext_crc()) setError(23,ptr);

#endif

#ifdef enable_vars_crc_yes
	if (_vars_crc != vars_crc())
	{
		printf("vars are corrupted at line: %d\n", getLineFromPointer(ptr));
		setError(22,ptr);
	}
#endif

	if ( ( (long long int) ptr - (long long int) start)  >= tokenlength ) return NULL;

	return ptr;
}

char *code_reader( char *start, int tokenlength )
{
	char *ptr;
	int token = 0;
	
	interpreter_running = true;

	currentLine = 0;
	ptr = start;
	while ( ptr = token_reader(  start, ptr,  token, tokenlength ) )
	{
		// this basic for now, need to handel "on error " commands as well.

		if (kittyError.newError)
		{
			ptr = onError( ptr );
			if (ptr == NULL) break;
		}

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

		token = *((short *) ptr);
		ptr += 2;	// next token.		

		if (running == false) break;
	}

	interpreter_running = false;

	return ptr;
}

char *filename = NULL;

#define DLINE printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

bool ext_crc()
{
	int n;

	for(n=0;n<32;n++)
	{
		if (kitty_extensions[n].lookup)
		{
			kitty_extensions[n].crc != mem_crc( kitty_extensions[12].lookup, 0xFFFF );
			return false;
		}
	}
	return true;
}

#ifdef __linux__

static void ctrl_c_handler(int signum)
{
	printf("CTRL C\n");
}

#endif

#ifdef __amigaos4__

ULONG exceptCode ( struct ExecBase *SysBase, ULONG signals, ULONG exceptData)
{
	if (exceptData & SIGBREAKF_CTRL_C)
	{
		SetSignal( 0L, SIGBREAKF_CTRL_C);
		Printf("CTRL C\n");
	}
}

#endif

int main(int args, char **arg)
{
	BOOL runtime = FALSE;
	FILE *fd;
	int32_t amos_filesize;
	char amosid[17];
	char *data;
	int n;

#ifdef __amigaos__
	struct Task *me;
	APTR oldException;
	ULONG oldSigExcept;

	me = FindTask(NULL);	// don't need forbid, not looking for name.
	oldException = me -> tc_ExceptCode;
	me -> tc_ExceptCode = (APTR) exceptCode;

	oldSigExcept = SetExcept(0,0 );
	SetExcept( SIGBREAKF_CTRL_C, SIGBREAKF_CTRL_C );
#endif

#ifdef  __linux__
	struct sigaction sa;

	sa.sa_handler = ctrl_c_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGINT, &sa, NULL) ==-1)
	{
		printf("CTRL C signal handler not working\n");
	}

#endif

	if (args == 2)
	{
		filename = strdup(arg[1]);
	}

	amosid[16] = 0;	// /0 string.

#ifdef enable_fast_execution_yes
	init_fast_lookup();
#endif

	stack = 0;
	cmdStack = 0;
	onError = onErrorBreak;

	memset(globalVars,0,sizeof(globalVars));

#ifdef __amigaos4__
	sig_main_vbl = AllocSignal(-1);
#endif

#ifdef __linux__
	sig_main_vbl = SIGUSR1;
#endif

	for (n=0;n<64;n++)
	{
		bobs[n].screen_id = -1;
		bobs[n].image = -1;
	}

	make_float_lookup();

	channels = new ChannelTableClass();

	if (init() && channels)
	{
		bool init_error = false;

		__load_bank__( (char *) "AmosPro_System:APSystem/AMOSPro_Default_Resource.Abk",-2);
		__load_bank__( (char *) "Amos-the-creator:AMOS_System/mouse.abk",-3);

		// set up a fake extention lookup

		// set default values.
		memset( kitty_extensions , 0, sizeof(struct extension_lib) *32 );

		// alloc tabels for 2 fake extentions
		kitty_extensions[2].lookup = (char *) malloc( 0xFFFF );		// compat.lib
		kitty_extensions[12].lookup = (char *) malloc( 0xFFFF );	// turbo extention.

		// init default values for fake extentions
		for (n=0;n<32;n++) if (kitty_extensions[n].lookup) memset(kitty_extensions[n].lookup,0,0xFFFF);

		// function table init.
		if (kitty_extensions[2].lookup)
		{	
			*((void **) (kitty_extensions[2].lookup + 0x0048)) = (void *) ext_cmd_unpack;
			*((void **) (kitty_extensions[2].lookup + 0x0056)) = (void *) ext_cmd_unpack;
		}

		// function table init.
		if (kitty_extensions[12].lookup)
		{
			*((void **) (kitty_extensions[12].lookup + 0x0A08)) = (void *) ext_cmd_range;
		}

		for(n=0;n<32;n++)
		{
			if (kitty_extensions[n].lookup) kitty_extensions[n].crc = mem_crc( kitty_extensions[12].lookup, 0xFFFF ) ;
		}

		do_input = (void (**)(nativeCommand*, char*)) malloc( sizeof(void *) * MAX_PARENTHESIS_COUNT );
		do_to = (char *(**)(nativeCommand*, char*)) malloc( sizeof(void *) * MAX_PARENTHESIS_COUNT );

		for (n=0;n<MAX_PARENTHESIS_COUNT;n++) 
		{
			if (do_input) do_input[n] = do_std_next_arg;
			if (do_to) do_to[n] = do_to_default;
		}

		alloc_video();

		if (video) start_engine();

		fd = filename ? fopen(filename,"r") : NULL;
		if ((fd)&&(video)&&(init_error == false))
		{
			fseek(fd, 0, SEEK_END);
			amos_filesize = ftell(fd);
			fseek(fd, 0, SEEK_SET);

			fread( amosid, 16, 1, fd );
			fread( &tokenlength, 4, 1, fd );

#ifdef __LITTLE_ENDIAN__
			tokenlength = __bswap_32(tokenlength);
#endif
			data = (char *) malloc(amos_filesize);
			if (data)
			{
				int _file_code_start_ = ftell(fd);

				_file_start_ = data;
				_file_end_ = data + tokenlength;

				fread(data,amos_filesize - _file_code_start_ ,1,fd);

#ifdef enable_bank_crc
				bank_crc = mem_crc( _file_end_ , amos_filesize - tokenlength - _file_code_start_  );
#endif 


#ifdef __LITTLE_ENDIAN__
				token_littleendian_fixer( data, _file_end_ );
#endif
				// snifff the tokens find labels, vars, functions and so on.
				pass1_reader( data, _file_end_ );

				if (kittyError.code == 0)
				{
#ifdef enable_vars_crc
					_vars_crc = vars_crc();
#endif

					runtime = TRUE;

					_file_start_ = data;
					_file_end_ = data + tokenlength;

					// init banks

					_file_bank_size = amos_filesize - tokenlength - _file_code_start_;

					init_banks( _file_end_ ,  _file_bank_size );

#ifdef run_program_yes
					gfxDefault(NULL, NULL);
					code_reader( data, tokenlength );
#endif
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
			if (!fd) printf("AMOS file not open/can't find it\n");
			if (!video) printf("technical problems\n");
		}

		running = false;
		wait_spawns();

		if (do_input)
		{
			free( (void *) do_input );
			do_input = NULL;
		}

		if (do_to)
		{
			free( (void *) do_to );
			do_to = NULL;
		}

		printf("-- close & free video --\n");

		free_video();

		printf("-- clean up vars --\n");

		clean_up_vars();

		printf("-- clean up stack --\n");

		clean_up_stack();
		clean_up_files();
		clean_up_special();	// we add other stuff to this one.

		closedown();
	}

	if (sig_main_vbl) 
	{
		#ifdef __amigaos4__
		FreeSignal(sig_main_vbl);
		#endif
		sig_main_vbl = 0;
	}
	
	if (filename) free(filename);

#ifdef __amigaos__
	if ( me )
	{
		SetExcept( oldSigExcept,oldSigExcept | SIGBREAKF_CTRL_C );
		me -> tc_ExceptCode = oldException;
		Printf("Old exception handler restored\n");
	}
#endif

	return 0;
}

