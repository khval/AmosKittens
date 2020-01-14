
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

extern char *asl();
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
#include "commandsErrors.h"
#include "commandsScreens.h"
#include "commandsBanks.h"

#include "debug.h"
#include "kittyErrors.h"
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
#include "ext_music.h"

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
uint32_t tokenFileLength;

char *tokenBufferResume =NULL;

bool startup = false;

unsigned int amiga_joystick_dir[4];
unsigned int amiga_joystick_button[4];

struct extension_lib	kitty_extensions[32];
unsigned int regs[16];

unsigned short token_not_found = 0xFFFF;	// so we know its not a token, token 0 exists.

struct stackFrame procStcakFrame[PROC_STACK_SIZE];

char *_get_var_index( glueCommands *self, int nextToken);

char *(*do_var_index) ( glueCommands *self, int nextToken ) = _get_var_index;
char *(**do_to) ( struct nativeCommand *, char * ) ;
void (**do_input) ( struct nativeCommand *, char * ) ;
// void (**do_parenthesisEnd) ( struct nativeCommand *, char * ) ;
void (*do_breakdata) ( struct nativeCommand *, char * ) = NULL;

extern char *_errTrap( struct glueCommands *data, int nextToken );

int tokenMode = mode_standard;

struct retroSprite *icons = NULL;
struct retroSprite *sprite = NULL;
struct retroSprite *patterns = NULL;



//struct proc procStack[1000];	// 0 is not used.
struct globalVar globalVars[VAR_BUFFERS];	// 0 is not used.

struct kittyFile kittyFiles[10];
struct zone *zones = NULL;
int zones_allocated = 0;

int globalVarsSize = sizeof(globalVars)/sizeof(struct globalVar);

extern int nativeCommandsSize;

ChannelTableClass *channels = NULL;

std::vector<struct label> labels;	// 0 is not used.
std::vector<struct lineAddr> linesAddress;
std::vector<struct defFn> defFns;
std::vector<struct kittyBank> kittyBankList;
std::vector<struct kittyDevice> deviceList;
std::vector<struct kittyLib> libsList;
std::vector<struct fileContext *> files;
std::vector<struct retroSpriteObject *> bobs;

int global_var_count = 0;
int labels_count = 0;

struct glueCommands cmdTmp[100];	
struct glueCommands input_cmd_context;

extern char *nextToken_pass1( char *ptr, unsigned short token );

extern struct nativeCommand nativeCommands[];

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

struct kittyVideoInfo KittyBaseVideoInfo;
struct kittyInfo KittyBaseInfo;

extern void make_wave_noice();
extern void make_wave_bell();

void free_file(struct fileContext *file);
struct fileContext *newFile( char *name );

bool alloc_video()
{

#ifdef __amigaos4__
	video = retroAllocVideo( 640,480 );
#endif

#ifdef __linux__
	video = retroAllocVideo();
#endif

	if (video)
	{
		KittyBaseVideoInfo.videoWidth = video -> width;
		KittyBaseVideoInfo.videoHeight = video -> height;
		KittyBaseVideoInfo.display_x = 128;
		KittyBaseVideoInfo.display_y = 50;
	}

	KittyBaseInfo.video = &KittyBaseVideoInfo;
	
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
		getLineFromPointer( ptr );
		printf("line %d, int var: [%s]=%d\n",lineFromPtr.line, var_name, globalVars[ref-1].var.integer.value);
		return true;
	}
	else
	{
		ref = findVar( var_name, false, type_string, proc );

		if (ref)
		{
			getLineFromPointer( ptr );
			printf("line %d, string var: [%s]=%s\n",lineFromPtr.line, var_name, globalVars[ref-1].var.str);
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
				dump_screens();
			}
			else if (strncmp(txt,str_dump_banks,strlen(str_dump_banks))==0)
			{
				dump_banks();
			}
			else if (strncmp(txt,str_hint,strlen(str_hint))==0)
			{
				getLineFromPointer( ptr );
				printf("stack %d at line %d, hint: %s\n",stack, lineFromPtr.line, txt+strlen(str_hint));
			}
			else if (strncmp(txt,str_dump_stack,strlen(str_dump_stack))==0)
			{
				getLineFromPointer( ptr );
				printf("stack %d at line %d\n",stack, lineFromPtr.line);
			}
			else if (strncmp(txt,str_dump_prog_stack,strlen(str_dump_stack))==0)
			{
				dump_prog_stack();
				printf("<press enter to continue>\n");
				getchar();
			}
			else if (strncmp(txt,str_pause,strlen(str_pause))==0)
			{
				getLineFromPointer( ptr );
				printf("line %d -- <press enter to continue>\n", lineFromPtr.line);
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
						getLineFromPointer( ptr );
						printf("line %d, var: [%s] not found\n", lineFromPtr.line, var_name);
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

	tokenBufferResume = ptr;

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

	tokenBufferResume = ptr;

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
		getLineFromPointer( ptr );
		printf("breakpoint at line %d - <press enter for next line>\n", lineFromPtr.line );
		getchar();
	}

	return ptr;
}


int _last_var_index;		// we need to know what index was to keep it.
int _set_var_index;		// we need to resore index 

char *_get_var_index( glueCommands *self , int nextToken )
{
	uint32_t varNum;
	int n = 0;
	int mul;
	struct kittyData *var = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__ );

	varNum = self -> lastVar;
	if (varNum == 0) 
	{
		setError(11, self -> tokenBuffer);
		return NULL;
	}

	last_var = varNum;		// this is used when a array is set. array[var]=0, it restores last_var to array, not var
	var = &globalVars[varNum-1].var;

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

			switch (var -> type & 7)		// array type is 8, so mask is 7 		(while its unlikley you have array of file of proc, none we do not wont confuse this)
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
						setStackStr( alloc_amos_string(0) );
					}
					else
					{
						setStackStrDup( str );
					}
					break;
			}

			flushCmdParaStack(nextToken);
		}
		else
		{
			printf("varname %s(%d of max %d)\n",globalVars[varNum-1].varName, _last_var_index, var->count);
			setError( 23, self -> tokenBuffer  );
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
			struct kittyData *var = &globalVars[ref->ref-1].var;

			switch (var -> type & 7)
			{
				case type_int:
					setStackNum(var -> integer.value);
					break;
				case type_float:
					setStackDecimal(var -> decimal.value);
					break;
				case type_string:
					setStackStrDup(var -> str);		// always copy.
					break;
				case type_proc:
					stackCmdProc( _procedure, ptr+sizeof(struct reference)+ref->length ) ;
					stack_frame_up(ref->ref); 

					// size of ref is added on exit, then +2 next token
					return var -> tokenBufferPos - sizeof(struct reference) -2;	
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
double _exp[0x80];

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

char *includeNOP(nativeCommand *cmd,char *ptr)
{
	setError(22,ptr);
	return ptr;
}


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

/*
char *skipToken( char *ptr, unsigned short token )
{
	struct nativeCommand *cmd;
	char *ret = ptr;

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
*/

#ifdef enable_fast_execution_no

char *executeToken( char *ptr, unsigned short token )
{
	struct nativeCommand *cmd;
	char *ret;

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id ) 
		{

#ifdef show_token_numbers_yes

			getLineFromPointer(ptr);

			printf("%08d   %08X %20s:%08d stack is %d cmd stack is %d flag %d token %04x -- name %s\n",
					lineFromPtr.line, (unsigned int) ptr,__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token , TokenName(token));	
#endif
			ret = cmd -> fn( cmd, ptr ) ;
			if (ret) ret += cmd -> size;

			return ret;
		}
	}

	token_not_found = token;
	setError(23, ptr);
	getLineFromPointer( ptr);
	printf("Addr %08x, token not found %04X at line %d\n", 
				(unsigned int) ptr, 
				(unsigned int) token_not_found, 
				lineFromPtr.line);

	return NULL;
}

#endif

#ifdef enable_fast_execution_yes

char fast_lookup[0xFFFF];

void init_fast_lookup()
{
	int token;
	struct nativeCommand *cmd;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		token = (int) ((unsigned short) cmd->id) ;

		*((void **) (fast_lookup + token)) = (void *) cmd -> fn;
		*((uint16_t *) (fast_lookup + token + sizeof(void *))) = (uint16_t) cmd -> size;
	}
}

bool validate_fast_lookup()
{
	int token;
	struct nativeCommand *cmd;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		token = (int) ((unsigned short) cmd->id) ;

		if (*((void **) (fast_lookup + token)) != (void *) cmd -> fn)
		{
			printf("token %04x is corrupt, function pointer is wrong (is %08x should be %08x)\n",
				token, 
				*((void **) (fast_lookup + token)) ,
				cmd -> fn);
			return false;
		}

		if (*((uint16_t *) (fast_lookup + token + sizeof(void *))) != (uint16_t) cmd -> size)
		{
			printf("token %d is corrupt, size is wrong (is %d should be %d)\n", 
					token,
					*((uint16_t *) (fast_lookup + token + sizeof(void *))),
					cmd -> size);
			return false;
		}
	}
	return true;
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
		getLineFromPointer(ptr);
		printf("dog fart, stinky fart at line %d, stack is %d\n", lineFromPtr.line ,stack);
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

	if (start == NULL) return NULL;

	interpreter_running = true;
	ptr = start +2;
	token = *((short *) start);

	while ( ptr = token_reader( start, ptr, token, tokenlength ) )
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

extern struct retroRGB DefaultPalette[256];

int main(int args, char **arg)
{
	BOOL runtime = FALSE;
	struct fileContext *file;
	char amosid[17];
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


	if (init())
	{
		startup = true;

		switch (args)
		{
			case 2:	filename = strdup(arg[1]);
					break;
#if defined(__amigaos4__)
			case 0:
					filename = asl();
					break;	
#endif
		}
	}

	amosid[16] = 0;	// /0 string.

#ifdef enable_fast_execution_yes

	init_fast_lookup();

	if (validate_fast_lookup() == false)
	{
		getchar();
		startup = false;
	}

#endif

	stack = 0;
	cmdStack = 0;
	onError = onErrorBreak;

	memset(globalVars,0,sizeof(struct globalVar) * VAR_BUFFERS);


#ifdef __amigaos4__
	sig_main_vbl = AllocSignal(-1);
#endif

#ifdef __linux__
	sig_main_vbl = SIGUSR1;
#endif

	make_float_lookup();

	channels = new ChannelTableClass();

	if ( (startup) && (channels) )
	{
		bool init_error = false;

		alloc_video();

#ifdef __amigaos4__
		regs[3] = (unsigned int) &KittyBaseInfo;	// set D3 to kittyInfo.
#endif

		for (n=0;n<8;n++)
		{
			KittyBaseInfo.rgb[n] = (DefaultPalette[n].r << 4 & 0xF00) | (DefaultPalette[n].g & 0xF0) | (DefaultPalette[n].b >> 4);
		}

		__load_bank__( (char *) "AmosPro_System:APSystem/AMOSPro_Default_Resource.Abk",-2);
		__load_bank__( (char *) "Amos-the-creator:AMOS_System/mouse.abk",-3);

		// set up a fake extention lookup

		// set default values.
		memset( kitty_extensions , 0, sizeof(struct extension_lib) *32 );

		// alloc tabels for 2 fake extentions
		kitty_extensions[1].lookup = (char *) malloc( 0xFFFF );	// music.lib
		kitty_extensions[2].lookup = (char *) malloc( 0xFFFF );	// compat.lib
		kitty_extensions[12].lookup = (char *) malloc( 0xFFFF );	// turbo extention.

		// init default values for fake extentions
		for (n=0;n<32;n++) if (kitty_extensions[n].lookup) memset(kitty_extensions[n].lookup,0,0xFFFF);

		// function table init.

		if (kitty_extensions[1].lookup)
		{	
			*((void **) (kitty_extensions[1].lookup + 0x0074)) = (void *) ext_cmd_boom;
			*((void **) (kitty_extensions[1].lookup + 0x008A)) = (void *) ext_cmd_sam_bank;
			*((void **) (kitty_extensions[1].lookup + 0x007E)) = (void *) ext_cmd_shoot;	

			*((void **) (kitty_extensions[1].lookup + 0x009A)) = (void *) ext_cmd_sam_loop_on;
			*((void **) (kitty_extensions[1].lookup + 0x00B4)) = (void *) ext_cmd_sam_loop_off;

			*((void **) (kitty_extensions[1].lookup + 0x00DE)) = (void *) ext_cmd_sam_play;
			*((void **) (kitty_extensions[1].lookup + 0x00EE)) = (void *) ext_cmd_sam_play;
			*((void **) (kitty_extensions[1].lookup + 0x00CE)) = (void *) ext_cmd_sample;
			*((void **) (kitty_extensions[1].lookup + 0x0118)) = (void *) ext_cmd_bell;
			*((void **) (kitty_extensions[1].lookup + 0x0170)) = (void *) ext_cmd_del_wave;
			*((void **) (kitty_extensions[1].lookup + 0x0180)) = (void *) ext_cmd_set_envel;
			*((void **) (kitty_extensions[1].lookup + 0x01A4)) = (void *) ext_cmd_volume;
			*((void **) (kitty_extensions[1].lookup + 0x0104)) = (void *) ext_cmd_sam_raw;
			*((void **) (kitty_extensions[1].lookup + 0x0144)) = (void *) ext_cmd_play;
			*((void **) (kitty_extensions[1].lookup + 0x015E)) = (void *) ext_cmd_set_wave;
			*((void **) (kitty_extensions[1].lookup + 0x01BC)) = (void *) ext_cmd_wave;

			*((void **) (kitty_extensions[1].lookup + 0x01CA)) = (void *) ext_cmd_led_on;
			*((void **) (kitty_extensions[1].lookup + 0x01D6)) = (void *) ext_cmd_led_off;

			*((void **) (kitty_extensions[1].lookup + 0x0256)) = (void *) ext_cmd_sam_stop;
			*((void **) (kitty_extensions[1].lookup + 0x0324)) = (void *) ext_cmd_ssave;
		}

		if (kitty_extensions[2].lookup)
		{	
			*((void **) (kitty_extensions[2].lookup + 0x0006)) = (void *) ext_cmd_pack;
			*((void **) (kitty_extensions[2].lookup + 0x0014)) = (void *) ext_cmd_pack;
			*((void **) (kitty_extensions[2].lookup + 0x0026)) = (void *) ext_cmd_spack;
			*((void **) (kitty_extensions[2].lookup + 0x0036)) = (void *) ext_cmd_spack;
			*((void **) (kitty_extensions[2].lookup + 0x0048)) = (void *) ext_cmd_unpack;
			*((void **) (kitty_extensions[2].lookup + 0x0056)) = (void *) ext_cmd_unpack;
			*((void **) (kitty_extensions[2].lookup + 0x0060)) = (void *) ext_cmd_unpack;
		}

		if (kitty_extensions[12].lookup)
		{
			*((void **) (kitty_extensions[12].lookup + 0x0A08)) = (void *) ext_cmd_range;
		}

		for(n=0;n<32;n++)
		{
			if (kitty_extensions[n].lookup) kitty_extensions[n].crc = mem_crc( kitty_extensions[12].lookup, 0xFFFF ) ;
		}

		make_wave_noice();
		make_wave_bell();

		apply_wave(1, 15);


		do_input = (void (**)(nativeCommand*, char*)) malloc( sizeof(void *) * MAX_PARENTHESIS_COUNT );
		do_to = (char *(**)(nativeCommand*, char*)) malloc( sizeof(void *) * MAX_PARENTHESIS_COUNT );

		for (n=0;n<MAX_PARENTHESIS_COUNT;n++) 
		{
			if (do_input) do_input[n] = do_std_next_arg;
			if (do_to) do_to[n] = do_to_default;
		}

		if (video) start_engine();

		file = newFile( filename );

		if ((file)&&(video)&&(init_error == false))
		{
			if (file -> start)
			{
				_file_start_ = (char *) file -> start ;
				_file_end_ = (char *) file -> end;

				// snifff the tokens find labels, vars, functions and so on.

				pass1_reader( (char *) file -> start , _file_end_ );

				if (kittyError.code == 0)
				{
					runtime = TRUE;

					if (file ->bank) init_banks( (char *) file -> bank, file -> bankSize );

					gfxDefault(NULL, NULL);

#ifdef run_program_yes
					code_reader( (char *) file -> start , file -> tokenLength );
#endif
				}

				if (kittyError.newError)
				{
					printError( &kittyError, runtime ? errorsRunTime : errorsTestTime );
				}
			}

			 free_file(file);

//			if (kittyError.newError == true)
			{
				dump_end_of_program();
			}
		}
		else
		{
			if (!file) printf("AMOS file not open/can't find it\n");
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
	}
	else
	{
		running = false;
		wait_spawns();
	}

	free_video();
	clean_up_vars();
	clean_up_stack();
	clean_up_files();
	clean_up_special();	// we add other stuff to this one.
	closedown();

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

