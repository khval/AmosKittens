
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#include <amoskittens.h>
#endif

#ifdef __linux__
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include "os/linux/stuff.h"
#endif

#include "debug.h"
#include "stack.h"
#include "commands.h"
#include "commandsData.h"
#include "var_helper.h"
#include "kittyErrors.h"
#include "engine.h"
#include "label.h"
#include "amosString.h"

extern void clear_local_vars( int proc );

bool every_on = true;
unsigned int every_timer = 0;
char *on_every_gosub_location = NULL;
char *on_every_proc_location = NULL;
struct timeval every_before, every_after;

extern unsigned int var_count[2];

extern char *_file_pos_ ;

int timer_offset = 0;

extern int _last_var_index;		// we need to know what index was to keep it.
extern int _set_var_index;		// we need to resore index 

static struct timeval timer_before, timer_after;

extern std::vector<struct label> labels;
extern std::vector<int> engineCmdQue;
extern struct globalVar proc_main_data;

extern struct kittyData *getVar(uint16_t ref);

extern struct globalVar globalVars[];
extern int tokenMode;
extern int tokenlength;
extern int bobDoUpdate ;
extern int bobUpdateNextWait ;
extern int findVarPublic( char *name, int type );
extern int ReferenceByteLength(char *ptr);

extern struct stackFrame *currentFrame;

using namespace std;

void	input_mode( char *tokenBuffer );
char *dupRef( struct reference *ref );
char *_setVar( struct glueCommands *data, int nextToken );

// dummy not used, see code in cmdNext
char *_exit( struct glueCommands *data, int nextToken )
{
	return NULL;
}

// dummy not used, see code in cmdNext
char *_for( struct glueCommands *data, int nextToken )
{
	return NULL;
}

// dummy not used, we need to know what to do on "else if" and "else", "If" and "else if" does not know where "end if" is.

char *_ifSuccess( struct glueCommands *data, int nextToken ) 
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	setError(22,data -> tokenBuffer);	// shoud not be executed
	return NULL;
}

char *_ifNotSuccess( struct glueCommands *data, int nextToken ) 
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	setError(22,data -> tokenBuffer);	// shoud not be executed
	return NULL;
}


char *_ifThenSuccess( struct glueCommands *data, int nextToken ) 
{
	return NULL;
}

char *_ifThenNotSuccess( struct glueCommands *data, int nextToken ) 
{
	return NULL;
}


char *_procedure( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	// Executed after "End Proc"
	popStack(instance.stack - data->stack);
	return  data -> tokenBuffer ;
}

void free_local_var(struct kittyData *var);
void setup_local_var(struct kittyData *var);

void setup_local_vars()
{
	unsigned int n;
	struct globalVar *var;
	struct kittyData *lvar;

	for (n=0;n<var_count[0];n++)
	{
		var = globalVars +n;

		if (var -> proc == currentFrame -> id)
		{
			lvar = currentFrame -> localVarData + var -> localIndex;
			bzero( lvar , sizeof(struct kittyData) );
			lvar -> type = var -> var.type;
		}
	}
}

void free_local_vars()
{
	struct kittyData *var;

	for ( var =currentFrame -> localVarData; var < currentFrame -> localVarDataNext; var++)
	{
		free_local_var(var);
	}
}

void dump_local_vars()
{
	struct kittyData *var;

	for ( var =currentFrame -> localVarData; var < currentFrame -> localVarDataNext; var++)
	{
		unsigned int localIndex = (unsigned int) (var - currentFrame -> localVarData);
		
		switch (var -> type)
		{
			case type_int:
				printf("%d:%d:%s %s=%d\n",
					currentFrame -> id, 
					localIndex,
					"Local",
					"unkown",
					var -> integer.value );
				break;

			case type_float:
				printf("%d:%d:%s %s=%0.2lf\n",
					currentFrame -> id, 
					localIndex,
					"Local",
					"unkown",
 					var -> decimal.value );
				break;

			case type_string:
				printf("%d:%d:%s %s=%c%s%c\n",
					currentFrame -> id, 
					localIndex,
					"Local",
					"unkown",
					 34, var->str ? &(var -> str -> ptr) : "NULL", 34 );
				break;
		}
	}
}

struct stackFrame *find_stackframe(int proc)
{
	int n;
	for (n=proc_stack_frame;n>0;n--)
	{
		if (procStcakFrame[n].id == proc)
		{
			return procStcakFrame + n;
		}
	}
	return NULL;
}

void stack_frame_up(int varIndex)
{
	struct globalVar *proc = globalVars + varIndex;
	struct stackFrame *lastFrame;

	dprintf("stack frame up on name %s\n",proc -> varName );
	dprintf("size %d\n",proc -> localIndexSize);

	lastFrame = procStcakFrame +proc_stack_frame;

	proc_stack_frame++;		// move stack frame up.

	// setup new stack frame;

	currentFrame = procStcakFrame +proc_stack_frame;
	currentFrame -> id = proc -> proc;

	if ( proc -> procDataPointer )
	{
		currentFrame -> dataPointer  = proc -> procDataPointer;
	}
	else currentFrame -> dataPointer = lastFrame -> dataPointer;

	currentFrame -> localVarData = lastFrame -> localVarDataNext ; 
	currentFrame -> localVarDataNext = lastFrame -> localVarDataNext + proc -> localIndexSize; 

	setup_local_vars();
}

void __stack_frame_down()	// so this where we should take care of local vars and so on.
{
//	dump_local_vars();
	free_local_vars();
	proc_stack_frame--;	
	currentFrame = procStcakFrame +proc_stack_frame;
}

char *stack_frame_down()		// should only be used in end_proc, (pop proc calles end_proc)
{
	__stack_frame_down();
	return cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack],0);
}


char *_if( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	char *ptr;
	int args = instance.stack - data -> stack + 1;

	if (args > 1) 
	{
		setError(22,data -> tokenBuffer);
	}

	if (kittyStack[instance.stack].integer.value == 0)	// 0 is FALSE always -1 or 1 can be TRUE
	{
		int offset = *((unsigned short *) data -> tokenBuffer);

		if (offset) 
		{
			ptr = data->tokenBuffer+(offset*2) ;
			stackIfNotSuccess();
			return ptr;
		}
	}
	else 	stackIfSuccess();
	return NULL;
}


char *_while( struct glueCommands *data, int nextToken )	// jumps back to the token.
{
	return data -> tokenBuffer-2;
}

char *_whileCheck( struct glueCommands *data, int nextToken )		
{
	int offset = 0;

	proc_names_printf("'%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	// two command should be stacked, while loop, and while check.
	// while loop is removed from stack, if check is false
	// and we jump over the wend

	if (kittyStack[data->stack].integer.value == 0)
	{
		if (instance.cmdStack) if (cmdTmp[instance.cmdStack-1].cmd == _while ) 
		{
			instance.cmdStack --;
			offset = *((unsigned short *) cmdTmp[instance.cmdStack].tokenBuffer);
			if (offset) 
			{
				proc_names_printf("Jump over\n");

				return data->tokenBuffer+(offset*2);
			}
		}
	}
	return NULL;
}




char *_do( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	return data -> tokenBuffer-2;
}

char *_repeat( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	// jump back to repeat, until true
	if (kittyStack[instance.stack].integer.value == 0) return data -> tokenBuffer;
	return 0;
}

char *set_tokenBuffer;

const char *traced_vars[]={"JUMP","FALL","Y","DEAD",NULL};

void trace_vars( struct kittyData *var ,const char **array )
{
	const char **item;

	for (item = array ; *item ; item++)
	{
		if (var_has_name(var,*item))
		{
			getLineFromPointer( set_tokenBuffer );
			printf("line %d,  %s=%d\n",lineFromPtr.line, *item, var -> integer.value);
			getchar();
			return;
		}
	}
}


BOOL setVarInt( struct kittyData *var )
{
	switch (kittyStack[instance.stack].type)
	{
		case type_int:
			var->integer.value = kittyStack[instance.stack].integer.value;

//			trace_vars( var , traced_vars );

			return TRUE;

		case type_float:
			var->integer.value = (int) kittyStack[instance.stack].decimal.value;
			return TRUE;
	}

	return FALSE;
}

BOOL setVarDecimal( struct kittyData *var )
{
	switch (kittyStack[instance.stack].type)
	{
		case type_int:
			var->decimal.value =  (double) kittyStack[instance.stack].integer.value;
			return TRUE;

		case type_float:
			var->decimal.value =  kittyStack[instance.stack].decimal.value;
			return TRUE;
	}

	return FALSE;
}

BOOL setVarString( struct kittyData *var, kittyData *s )
{
	if (var == NULL)
	{
		setError(22,NULL);
		return NULL;
	}

	switch (s -> type)
	{
		case type_string:
			if (s -> str)
			{
				if (var->str) freeString(var->str);
				var->str = amos_strdup(s -> str);
			}
			else
			{
				if (var->str)
				{
					var->str->ptr = 0;
					var->str->size = 0;
				}
			}

			return TRUE;
	}


	return FALSE;
}

BOOL setVarIntArray( struct kittyData *var, char *tokenBuffer )
{
	if ((_set_var_index>-1)&&(_set_var_index<var->count))
	{
		var -> index = _set_var_index;

		switch (kittyStack[instance.stack].type)
		{
			case type_int:
				(&(var->int_array -> ptr) + var -> index) -> value = kittyStack[instance.stack].integer.value;
				return TRUE;

			case type_float:
				(&(var->int_array -> ptr) + var -> index) -> value = (int) kittyStack[instance.stack].decimal.value;
				return TRUE;
		}
	}
	else setError(25,tokenBuffer);

	return FALSE;
}

BOOL setVarDecimalArray( struct kittyData *var, char *tokenBuffer )
{
	if ((_set_var_index>-1)&&(_set_var_index<var->count))
	{
		var -> index = _set_var_index;

		switch (kittyStack[instance.stack].type)
		{
			case type_int:
				(&(var->float_array -> ptr) + var -> index) -> value = (double) kittyStack[instance.stack].integer.value;
				return TRUE;

			case type_float:
				(&(var->float_array -> ptr) + var -> index) -> value = kittyStack[instance.stack].decimal.value;
				return TRUE;
		}
	}
	else setError(25,tokenBuffer);

	return FALSE;
}

BOOL setVarStringArray( struct kittyData *var, char *tokenBuffer )
{
	if ((_set_var_index>-1)&&(_set_var_index<var->count))
	{
		var -> index = _set_var_index;

		switch (kittyStack[instance.stack].type)
		{
			case type_string:
				{
					struct stringData **str_item = &(var->str_array -> ptr) + var -> index;

					if (*str_item) freeString(*str_item);
					*str_item = amos_strdup(kittyStack[instance.stack].str);	

				}
				return TRUE;
		}
	}
	else setError(25,tokenBuffer);

	return FALSE;
}

char *(*_do_set) ( struct glueCommands *data, int nextToken ) = _setVar;

extern const char *type_names[];

char *_setVar( struct glueCommands *data, int nextToken )
{
	BOOL success = FALSE;
	struct kittyData *var;

	proc_names_printf("%s:%d -- set %s var %d\n",__FUNCTION__,__LINE__, (data -> lastVar & 0x8000 ? "local" : ""),  (data -> lastVar & 0x7FFF) - 1);

	var = getVar(data -> lastVar);

	if (var == NULL)
	{
		printf("%s:%s:%d -- set %s var %d\n",__FILE__,__FUNCTION__,__LINE__);
		setError(ERROR_OBJECT_NOT_FOUND,data -> tokenBuffer);
		return NULL;
	}

	success = FALSE;

	set_tokenBuffer = data -> tokenBuffer;

	switch (var->type)
	{
		case type_int:
			success = setVarInt( var );
			break;
		case type_float:
			success = setVarDecimal( var );
			break;
		case type_string:
			success = setVarString( var , &kittyStack[instance.stack] );
			break;
		case type_int | type_array:
			success = setVarIntArray( var, data -> tokenBuffer );
			break;
		case type_float | type_array:
			success = setVarDecimalArray( var, data -> tokenBuffer );
			break;
		case type_string | type_array:
			success = setVarStringArray( var, data -> tokenBuffer );
			break;
	}

	if (success == FALSE)
	{
		if ( kittyStack[instance.stack].type !=  (var -> type & 15))
		{
			printf("kittyStack[%d].type= %s, (var -> type & 15)=%s\n",
				instance.stack, 
				type_names[kittyStack[instance.stack].type & 15], 
				type_names[var -> type & 15]);
			setError(ERROR_Type_mismatch,data->tokenBuffer);
		}

		if (var -> type & type_array)
		{
			if (var -> count == 0) setError(27,data -> tokenBuffer);
		}
	}

	return NULL;
}

char *_setVarReverse( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d data: lastVar %d\n",__FUNCTION__,__LINE__, data -> lastVar);

	// if variable is string it will have stored its data on the stack.

	if (kittyStack[instance.stack].str) freeString(kittyStack[instance.stack].str);
	kittyStack[instance.stack].str = NULL;
	instance.stack --;

	data->lastVar = last_var;	// we did know then, but now we know,
	return _setVar( data, 0 );
}

//--------------------------------------------------------

int parenthesis[MAX_PARENTHESIS_COUNT];

char *nextArg(struct nativeCommand *cmd, char *tokenBuffer)
{
	flushCmdParaStack(0);
	if (do_input[instance.parenthesis_count]) do_input[instance.parenthesis_count]( cmd, tokenBuffer );	// read from keyboad or disk.
	return tokenBuffer;
}

extern const char *TokenName( unsigned short token );
extern struct nativeCommand nativeCommands[];
extern int nativeCommandsSize;

char *skip_next_cmd( char * ptr, unsigned short token)
{
	bool _exit = false;
	struct nativeCommand *cmd;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id )
		{
/*
			getLineFromPointer(ptr);
			printf("Line %08d offset %08x token %04x - size %d - name %s\n",
						lineFromPtr.line,
						ptr-_file_start_,
						token, 
						cmd -> size,
						TokenName(token));
*/

			switch (token)
			{
				case 0x0006:	ptr += ReferenceByteLength(ptr);		break;
				case 0x000c:	ptr += ReferenceByteLength(ptr);		break;
				case 0x0012:	ptr += ReferenceByteLength(ptr);		break;
				case 0x0018:	ptr += ReferenceByteLength(ptr);		break;
				case 0x0026:	ptr += QuoteByteLength(ptr);			break;
				case 0x002E:	ptr += QuoteByteLength(ptr);			break;
				case 0x064A:	ptr += QuoteByteLength(ptr);			break;
				case 0x0652:	ptr += QuoteByteLength(ptr);			break;
			}

			return ptr + cmd -> size;
		}
	}

	getLineFromPointer(ptr);
	printf("TOKEN NOT FOUND: %04x AS LINE %d!!!\n",token,lineFromPtr.line);
	return NULL;
}

unsigned short token_after_array( char * ptr)
{
	int count = 0;
	unsigned short token;

//	ptr += sizeof(struct reference)+ref->length;
	token = *((short *) (ptr));

	for(;;)
	{
		switch (token)
		{
				case 0x0054:   
				case 0x0000:	printf("unexpected exit\n");
							return 0;
	
				case 0x0074:	count ++; break;
				case 0x007C:	count --;
							if (count == 0) return *((unsigned short *) (ptr + 2));
		}

		ptr = skip_next_cmd( ptr +2 , token);

		if (ptr==NULL)
		{
			printf("BAD EXIT value\n");
			getchar();
			return 0;
		}		

		token = *((short *) (ptr));
	}

	return 0;
}

char *parenthesisStart(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short last_token;
	unsigned short next_token;

	proc_names_printf("%s:%s:%d stack is %d cmd stack is %d state %d\n",__FILE__,__FUNCTION__,__LINE__, instance.stack, instance.cmdStack, kittyStack[instance.stack].state);

	last_token = getLastProgStackToken();
	next_token = token_after_array( tokenBuffer-2 );

	dprintf("correct_order( %04x, %04x)\n ", last_token ,  next_token );

	if (last_token == 0x0074)	// index command..
	{
		last_token = getLastLastProgStackToken();

		dprintf("**correct_order( %04x, %04x)\n ", last_token ,  next_token );

		if ( correct_order( last_token,  next_token ) == false )
		{
			dprintf(" hidden ( condition. opt 1\n");
			setStackHiddenCondition();
			cmdTmp[__cmdStack-1].stack ++;	// fixing index start...
		}
	}
	else	if ( correct_order( last_token,  next_token ) == false )
	{
		dprintf(" hidden ( condition. opt 2\n");
		setStackHiddenCondition();
		setStackNone();
	}

	parenthesis[instance.parenthesis_count] =instance.stack;
	instance.parenthesis_count++;

	setStackParenthesis();
	instance.stack++;
	setStackNone();

	return tokenBuffer;
}

char *parenthesisEnd(struct nativeCommand *cmd, char *tokenBuffer)
{
	char *ret;
	int nextToken = *((unsigned short *) tokenBuffer);

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	_file_pos_ = tokenBuffer;		// needed by "Fn", need to return End Bracket after Fn call.

	ret = flushCmdParaStack(0);
	if (ret) return ret;

	if (instance.parenthesis_count)
	{
		remove_parenthesis( parenthesis[instance.parenthesis_count -1] );

		parenthesis[instance.parenthesis_count -1] = 255;
		do_input[instance.parenthesis_count] = do_std_next_arg;
		instance.parenthesis_count--;

		if (instance.cmdStack) 
		{
			struct glueCommands *sub = &cmdTmp[instance.cmdStack-1];

			if ((sub->parenthesis_count == instance.parenthesis_count ) && (sub -> flag == cmd_index )) 	// only if we at the right place !!!
			{
				sub -> cmd( sub, nextToken);
				instance.cmdStack --;
			}
		}
	}

	ret = flushCmdParaStack(nextToken);
	if (ret) return ret;

	return tokenBuffer;
}

char *breakData(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (instance.cmdStack) if (cmdTmp[instance.cmdStack-1].flag & (cmd_index | cmd_onBreak) ) cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack],0);

	if (do_breakdata) do_breakdata( cmd, tokenBuffer );
	return tokenBuffer;
}

char *setVar(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (instance.cmdStack) if (cmdTmp[instance.cmdStack-1].flag == cmd_index ) cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack], 0);

	if (tokenMode == mode_logical)
	{
		stackCmdMathOperator(_equalData, tokenBuffer, token_equal );
		instance.stack++;
		setStackNum(0);	// prevent random data from being on the stack.
	}
	else
	{
		stackCmdNormal( _do_set, tokenBuffer);
		_set_var_index = _last_var_index;		// (var->index will be overwritten by index reads)

		switch (kittyStack[instance.stack].type)
		{
			case type_int : setStackNum(0); break;
			case type_float : setStackDecimal(0); break;
		}

		if (tokenMode == mode_standard) tokenMode = mode_logical;		// first equal is set, next equal is logical
	}

	return tokenBuffer;
}

char *cmdIf(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d \n",__FUNCTION__,__LINE__);

	instance.token_is_fresh = false;

	setStackNum(0);	// stack reset.
	stackCmdNormal(_if, tokenBuffer);

	tokenMode = mode_logical;
	return tokenBuffer;
}

char *cmdThen(struct nativeCommand *cmd, char *tokenBuffer)
{
	char *ret = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	
	// empty the stack for what ever is inside the IF.

	instance.token_is_fresh = true;

	while (instance.cmdStack)
	{
		if (cmdTmp[instance.cmdStack-1].cmd == _if ) break;
		cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack],0);
	}

	if (instance.cmdStack) if (cmdTmp[instance.cmdStack-1].cmd == _if ) ret=cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack],0);

	if (instance.cmdStack)
	{
		if (cmdTmp[instance.cmdStack-1].cmd == _ifSuccess) 
		{
			cmdTmp[instance.cmdStack-1].cmd = _ifThenSuccess;
			cmdTmp[instance.cmdStack-1].flag = cmd_onEol | cmd_true;			// should run at end of line
		}
		else	if (cmdTmp[instance.cmdStack-1].cmd == _ifNotSuccess) 
		{
			cmdTmp[instance.cmdStack-1].cmd = _ifThenNotSuccess;
			cmdTmp[instance.cmdStack-1].flag = cmd_onEol | cmd_false;			// should run at end of line
		}
	}

	if (ret) tokenBuffer = ret -2;	// on exit +2 token
	tokenMode = mode_standard;

	return tokenBuffer;
}

char *nextCmd(nativeCommand *cmd, char *ptr);

char *cmdElse(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	instance.token_is_fresh = true;

	char *retTokenBuffer = nextCmd(NULL, tokenBuffer);
	if (retTokenBuffer != tokenBuffer) tokenBuffer = retTokenBuffer + 2;	// nextCmd() should expect +2 token

	if (instance.cmdStack)
	{
		if (cmdTmp[instance.cmdStack-1].flag & cmd_true ) 		// if success jump over else
		{
			char *ptr;
			int offset = *((unsigned short *) tokenBuffer);

			if (offset) 
			{
				ptr = tokenBuffer+(offset*2);
//				printf("0x%08x - 0x%08x -- jump +0x%02x\n", tokenBuffer, ptr, offset*2);
				return ptr-4; 	// on exit +2 token +2 data
			}
		}
	}

	return tokenBuffer;
}

char *_else_if( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	char *ptr;

	if (kittyStack[data->stack].integer.value == 0)	// 0 is FALSE always -1 or 1 can be TRUE
	{
		int offset = *((unsigned short *) data -> tokenBuffer);

		if (offset) 
		{
			ptr = data->tokenBuffer+(offset*2) ;
//			printf("0x%08x - 0x%08x -- jump +0x%02x\n", data->tokenBuffer, ptr, offset*2);
			return ptr ;	// we don't know what command thats going to execute this code, do not subtract tokens!
		}
	}
	else 	
	{
		if (instance.cmdStack) if (cmdTmp[instance.cmdStack-1].cmd == _ifNotSuccess) instance.cmdStack--; 
		stackIfSuccess();
	}

	return NULL;
}

char *cmdElseIf(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (instance.cmdStack)
	{
		if (cmdTmp[instance.cmdStack-1].cmd == _ifSuccess)		// if success jump over else if
		{
			char *ptr;
			int offset = *((unsigned short *) tokenBuffer);

			if (offset) 
			{
				ptr = tokenBuffer+(offset*2)-4;	// on exit +2 token +2 data	
				return ptr;
			}
		}
	}

	setStackNum(0);	// stack reset.
	stackCmdNormal(_else_if, tokenBuffer);
	tokenMode = mode_logical;

	return tokenBuffer;
}

char *cmdEndIf(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (instance.cmdStack)
	{
		if ( cmdTmp[instance.cmdStack-1].flag & (cmd_true | cmd_false) )
		{
			instance.cmdStack--;
		}
		else setError(23,tokenBuffer);
	}

	return tokenBuffer;
}

char *_gosub_return( struct glueCommands *data, int nextToken )
{
	return data -> tokenBuffer;	// jumpBack.
}

char *_gosub( struct glueCommands *data, int nextToken )
{
	int args = instance.stack - data -> stack + 1;
	int ref_num = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args != 1) setError(22,data -> tokenBuffer);

	switch (kittyStack[instance.stack].type)
	{
		case type_int:	
				{
					char num[50];
					sprintf(num,"%d", kittyStack[instance.stack].integer.value );
					ref_num = findLabelRef( num , procStcakFrame[proc_stack_frame].id  );
				}
				break;

		case type_string:
				{
					struct stringData *str = getStackString(instance.stack);
					ref_num = findLabelRef( &str -> ptr, procStcakFrame[proc_stack_frame].id );
				}
				break;

		default:
				setError(22,data -> tokenBuffer);
	}

	if (ref_num)
	{
		char *return_tokenBuffer = data -> tokenBuffer;
		int token_size;
		uint16_t token;

		dprintf("next token %04x\n", nextToken);

		token =   *((unsigned short *) return_tokenBuffer );

		while ( token != nextToken  ) 
		{
			return_tokenBuffer += 2;

			dprintf("%04x\n", token);
			token_size = 0;
			switch ( token )
			{
				case 0006:	token_size = sizeof(struct reference) + ReferenceByteLength(return_tokenBuffer); break;
			}
			dprintf("token_size: %d\n",token_size);
			return_tokenBuffer += token_size ;	// skip token data, we have skiped token before
			token =   *((unsigned short *) return_tokenBuffer );
		}

		return_tokenBuffer += 2;

		stackCmdLoop( _gosub_return, return_tokenBuffer );
		return labels[ref_num-1].tokenLocation;
	}
	else
	{
		if (kittyStack[instance.stack].type == type_string)
		{
			printf("gosub can't find string '%s'\n",kittyStack[instance.stack].str);
		}

		setError(22,data -> tokenBuffer);
	}

	return NULL ;
}



char *_goto( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	int args = instance.stack - data -> stack + 1;
	int ref_num = 0;
	struct label *label;

	if (args != 1) setError(22,data -> tokenBuffer);

	switch (kittyStack[instance.stack].type)
	{
		case type_int:	
				{
					char num[50];
					sprintf(num,"%d", kittyStack[instance.stack].integer.value );
					ref_num = findLabelRef( num, procStcakFrame[proc_stack_frame].id );
				}
				break;

		case type_string:
				{
					struct stringData *str = getStackString(instance.stack);
					ref_num = findLabelRef( &str -> ptr, procStcakFrame[proc_stack_frame].id );
				}
				break;

		default:
				setError(22,data -> tokenBuffer);
	}

	if (ref_num)
	{
		label = &labels[ref_num-1];

		dropProgStackAllFlag( cmd_true | cmd_false );	// just kill the if condition, if any.

		if (label)
		{
			return label -> tokenLocation;
		}
	}
	else
	{
		setError(22,data -> tokenBuffer);
	}

	return NULL ;
}


char *cmdGoto(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (next_token)
	{
		case 0x003E:	// number.
		case 0x0026:	// text
					stackCmdNormal( _goto, tokenBuffer );
					break;

		case 0x0018:	// label
		case 0x0006: 	// variable

					switch ( var_type_is( (struct reference *) (tokenBuffer+2), 0x7 ))
					{
						case type_int:		// jump to label with same name as var.
								{
									dropProgStackAllFlag( cmd_true | cmd_false );	// just kill the if condition, if any.

									struct label *label = var_JumpToName( (struct reference *) (tokenBuffer+2) );		// after function, amos kittens try access next token and adds +2 (+0 data)

									if (label)
									{
										tokenBuffer = label -> tokenLocation-2;
									}
								}
								break;

						case type_string:	// jump to string.
								stackCmdNormal( _goto, tokenBuffer );
								break;

						case type_float:
								setError(22,tokenBuffer);
								break;

						default:
								stackCmdNormal( _goto, tokenBuffer );
					}
					break;

		default:

				printf("bad token: %04x\n", next_token);
				setError(22,tokenBuffer);
	}

	return tokenBuffer;
}

char *cmdGosub(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);
	char *return_tokenBuffer;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (next_token)
	{
		case 0x0026:	// text
					stackCmdNormal( _gosub, tokenBuffer );
					break;

		case 0x0074:  // data
					stackCmdNormal( _gosub,tokenBuffer );
					break;

		case 0x003E:	// number
					stackCmdNormal( _gosub,tokenBuffer );
					break;

		case 0x0018:	// label
		case 0x0006: 	// variable

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
					switch ( var_type_is( (struct reference *) (tokenBuffer + 2), 0x7 ))
					{
						case type_int:		// jump to label with same name as var.

								{
									struct label *label;

									label = var_JumpToName( (struct reference *) (tokenBuffer+2) ); 			// after function, amos kittens try access next token and adds +2 (+0 data)
									if (label)
									{
										if (label -> tokenLocation)
										{ 
											return_tokenBuffer = tokenBuffer + 4 + sizeof(struct reference ) + ReferenceByteLength(tokenBuffer + 2);
											stackCmdLoop( _gosub_return, return_tokenBuffer );
											return label -> tokenLocation -2;
										}
										else
										{
											printf("location not found\n");
										}
									}
									else printf("label not found\n");

									setError(22,tokenBuffer);
								}
								break;

						case type_string:	// jump to string.

								printf("%s:%d\n",__FUNCTION__,__LINE__);
								stackCmdNormal( _gosub, tokenBuffer );
								break;

						case type_float:

								setError(22, tokenBuffer);
								break;
					}
				
					break;

		default:
				printf("bad token: %04x\n", next_token);
				setError(22, tokenBuffer);
	}

	return tokenBuffer;
}

char *cmdDo(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdLoop( _do, tokenBuffer );
	return tokenBuffer;
}

char *cmdRepeat(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdLoop( _repeat, tokenBuffer );
	return tokenBuffer;
}

char *cmdLoop(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (instance.cmdStack) 
	{
		if (cmdTmp[instance.cmdStack-1].cmd == _do )
		{
			tokenBuffer=cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack],0);
		}
		else 	if (cmdTmp[instance.cmdStack-1].cmd == _exit)
		{
			instance.cmdStack--;
		}
		else	setError(23,tokenBuffer);
	}
	else	setError(23,tokenBuffer);

	return tokenBuffer;
}

char *cmdWhile(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	tokenMode = mode_logical;
	stackCmdLoop( _while, tokenBuffer );
	stackCmdNormal( _whileCheck, tokenBuffer );
	return tokenBuffer;
}

char *cmdWend(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (instance.cmdStack)
	{
		if (cmdTmp[instance.cmdStack-1].cmd == _while ) 
		{
			tokenBuffer=cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack],0);
		}
		else 	if (cmdTmp[instance.cmdStack-1].cmd == _exit)
		{
			instance.cmdStack--;
		}
		else	setError(23,tokenBuffer);
	}
	else	setError(23,tokenBuffer);

	return tokenBuffer;
}

char *cmdUntil(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	// we are changin the stack from loop to normal, so when get to end of line or next command, it be executed after the logical tests.

	instance.token_is_fresh = false;
	tokenMode = mode_logical;
	if (instance.cmdStack)
	{
		if (cmdTmp[instance.cmdStack-1].cmd == _repeat )
		{
			cmdTmp[instance.cmdStack-1].flag = cmd_normal | cmd_onNextCmd | cmd_onEol;
		}
		else if (cmdTmp[instance.cmdStack-1].cmd == _exit)
		{
			instance.cmdStack--;
		}
		else	setError(23,tokenBuffer);
	}
	else	setError(23,tokenBuffer);

	return tokenBuffer;
}

char *cmdTrue(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackNum( ~0 );
	flushCmdParaStack(NEXT_TOKEN(tokenBuffer));
	return tokenBuffer;
}

char *cmdFalse(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackNum(0);
	flushCmdParaStack(NEXT_TOKEN(tokenBuffer));
	return tokenBuffer;
}

char *do_for_to( struct nativeCommand *cmd, char *tokenBuffer);

char *cmdFor(struct nativeCommand *cmd, char *tokenBuffer )
{
	kittyData *var = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (NEXT_TOKEN( (tokenBuffer+2) ) == 0x0006 )	// Next var
	{
		struct reference *ref = (struct reference *) (tokenBuffer + 4);
		var = getVar(ref -> ref);

		stackCmdNormal( _for, tokenBuffer );

		switch (var -> type)
		{
			case type_int:
					{
						cmdTmp[instance.cmdStack-1].optionsType = glue_option_for_int;
						cmdTmp[instance.cmdStack-1].optionsInt.step = 1.0;
					}
					break;
			case type_float:
					{
						cmdTmp[instance.cmdStack-1].optionsType = glue_option_for_float;
						cmdTmp[instance.cmdStack-1].optionsFloat.step = 1.0f;
					}
					break;
		}
	}

	do_to[instance.parenthesis_count] = do_for_to;

	return tokenBuffer;
}

char *do_for_to( struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (instance.cmdStack) if (cmdTmp[instance.cmdStack-1].flag == cmd_index ) cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack],0);

	if (instance.cmdStack) if ( cmdTmp[instance.cmdStack-1].cmd == _setVar ) cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack],0);

	if (instance.cmdStack) 
	{
		// We loop back to "TO" not "FOR", we are not reseting COUNTER var.

		if (( cmdTmp[instance.cmdStack-1].cmd == _for ) && (cmdTmp[instance.cmdStack-1].flag & cmd_normal ))
		{
			cmdTmp[instance.cmdStack-1].FOR_NUM_TOKENBUFFER = tokenBuffer ;
			cmdTmp[instance.cmdStack-1].cmd_type = cmd_loop;
			popStack( instance.stack - cmdTmp[instance.cmdStack-1].stack );
		}
	}
	return NULL;
}

char *do_to_default( struct nativeCommand *, char * )
{
	instance.stack ++;
	return NULL;
}

char *cmdTo(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (do_to[instance.parenthesis_count])
	{
		char *ret = do_to[instance.parenthesis_count]( cmd, tokenBuffer );	
		if (ret) tokenBuffer = ret;
	}

	return tokenBuffer;
}

char *_step( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	struct glueCommands *gcmd = &cmdTmp[instance.cmdStack-1];
	
	if (( gcmd -> cmd == _for ) && ( gcmd -> flag & cmd_loop ))
	{
		struct kittyData *var = &kittyStack[instance.stack];
		switch (gcmd -> optionsType)
		{
			case glue_option_for_int:
					gcmd -> optionsInt.step = (var -> type == type_int) ? var -> integer.value :  (int) var -> decimal.value ;
					break;
			case glue_option_for_float:
					gcmd -> optionsFloat.step = (var -> type == type_int) ? (double) var -> integer.value : var -> decimal.value ;
					break;
		}
	}
	popStack(instance.stack - data->stack);

	return NULL;
}

char *cmdStep(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _step, tokenBuffer );	// we need to store the step counter.

	if (NEXT_TOKEN(tokenBuffer) == 0xFFCA)
	{
		setStackNum(0);
		instance.stack++;
		setStackNum(0);
	}

	return tokenBuffer;
}

extern char *executeToken( char *ptr, unsigned short token );

#undef NEXT_INT

extern char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ );


void FOR_NEXT_VALUE_ON_STACK( char *tokenBuffer , char **new_ptr )
{
	unsigned short token;
	char *ptr = tokenBuffer;
	int cmdStack_start = instance.cmdStack;

	token = *( (unsigned short *) ptr);
	ptr +=2;

	// exit on End of line, or exit on Step or next command.
	while  ((token != 0) && (token != 0x0356 ) && (token != 0x0054))
	{
		ptr = executeToken( ptr, token );
		
		if (ptr == NULL) 
		{
			proc_names_printf("NULL\n");
			break;
		}

		token = *( (short *) ptr);
		ptr += 2;
	};

	if (token == 0x0356 )	// skip Step...
	{
		ptr = FinderTokenInBuffer( ptr-2, token , 0x0000, 0x0054, _file_end_ )+2;
	}

	*new_ptr = ptr - 2;

	// forcefully flush all cmds

	while ( instance.cmdStack > cmdStack_start ) 
	{
		cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack], 0);
	}
}


char *cmdNext(struct nativeCommand *cmd, char *tokenBuffer )
{
	char *ptr = tokenBuffer ;
	char *new_ptr = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);


	if ( cmdTmp[instance.cmdStack-1].cmd == _for )
	{
		kittyData *var = NULL;

		if (NEXT_TOKEN(ptr) == 0x0006 )	// Next var
		{
			struct reference *ref = (struct reference *) (ptr + 2);
			var = getVar(ref -> ref);
		}
		else 	// For var=
		{
			if (( cmdTmp[instance.cmdStack-1].cmd == _for ) && (cmdTmp[instance.cmdStack-1].flag == cmd_loop ))
			{
				char *ptr = cmdTmp[instance.cmdStack-1].tokenBuffer + 2  ;	// first short is JMP address, next after is token.
	
				if (NEXT_TOKEN(ptr) == 0x0006 )	// next is variable
				{
					struct reference *ref = (struct reference *) (ptr + 2);
					var = getVar(ref -> ref);
				}
			}
			else
			{
				setError( 22,  tokenBuffer );
				return tokenBuffer;
			}
		}
		
		if (var)
		{
			int next_num=0;
			double next_float=0.0;
			struct glueCommands *gcmd;

			gcmd = &cmdTmp[instance.cmdStack-1];

			ptr = cmdTmp[instance.cmdStack-1].FOR_NUM_TOKENBUFFER;

			FOR_NEXT_VALUE_ON_STACK(ptr, &new_ptr);

			switch ( var -> type )
			{
				case type_int:	

						switch ( kittyStack[instance.stack].type )
						{
							case type_int:		next_num = kittyStack[instance.stack].integer.value;	break;
							case type_float:	next_num = (int) kittyStack[instance.stack].decimal.value;	break;
						}
						break;

				case type_float:

						switch ( kittyStack[instance.stack].type )
						{
							case type_int:		next_float = (double) kittyStack[instance.stack].integer.value;	break;
							case type_float:	next_float = kittyStack[instance.stack].decimal.value;	break;
						}
						break;
			}

			switch (gcmd->optionsType)
			{
				case glue_option_for_int:

					var -> integer.value += gcmd->optionsInt.step; 

					if (gcmd->optionsInt.step > 0)  {
						if (var -> integer.value <= next_num )	{
							tokenBuffer = new_ptr;
						} else instance.cmdStack--;
					} else if (gcmd->optionsInt.step < 0) {
						if (var -> integer.value >= next_num  )	{
							tokenBuffer = new_ptr;
						} else instance.cmdStack--;
					} else setError(23,tokenBuffer);
					break;

				case glue_option_for_float:

					var -> decimal.value += gcmd->optionsFloat.step; 

					if (gcmd->optionsFloat.step > 0.0)  {

						printf("%0.2lf + %0.2lf <= %0.2lf \n",var -> decimal.value, gcmd->optionsFloat.step, next_float );

						if (var -> decimal.value <= next_float )	{
							tokenBuffer = new_ptr;
						} else instance.cmdStack--;
					} else if (gcmd->optionsFloat.step < 0.0) {
						if (var -> decimal.value >= next_float  )	{
							tokenBuffer = new_ptr;
						} else instance.cmdStack--;
					} else setError(23,tokenBuffer);
					break;
			}
		}
		else
		{
			setError(22,tokenBuffer);	
			return tokenBuffer;
		}
	}
	else	if (cmdTmp[instance.cmdStack-1].cmd == _exit )
	{
		instance.cmdStack--;
	}
	else
	{
		setError(22,tokenBuffer);
	}

	return tokenBuffer;
}

char *cmdEnd(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	return NULL;
}

char *cmdReturn(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	char *(*scmd) ( struct glueCommands *data, int nextToken ) = NULL;

	// exit if's until we found a none if state.

	while (instance.cmdStack)
	{
		scmd = cmdTmp[instance.cmdStack-1].cmd;
		if ( (scmd==_ifSuccess) ||  (scmd==_ifNotSuccess) ||  (scmd==_ifThenSuccess) ||  (scmd==_ifThenNotSuccess) )
		{
			instance.cmdStack --;
		}
		else break;
	}

	if (instance.cmdStack) if (cmdTmp[instance.cmdStack-1].cmd == _gosub_return ) 
	{
		char *ptr = cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack],0);
//		printf("0x%08x - 0x%08x -- jump +0x%02x\n", tokenBuffer, ptr, (int) tokenBuffer - (int) ptr);
		return ptr-2;		// after cmdReturn +2 token
	}

	// should find _gosub_return, if not some thing is broken.
	setError(22,tokenBuffer);
	return NULL;
}

char *cmdProcedure(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	struct procedure *proc = (struct procedure *) tokenBuffer;

	return proc -> EndOfProc - sizeof(struct procedure);
}

int get_proc_num_from_ref(int ref)
{
	if (ref)
	{
		return globalVars[ref-1].proc;
	}
	return 0;
}

char *_procAndArgs( struct glueCommands *data, int nextToken )
{
	int oldStack;
	struct reference *ref = (struct reference *) (data->tokenBuffer);

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (ref -> ref)
	{
		int idx = ref->ref-1;

		if (data -> tokenBuffer2)	// has arguments.
		{
			switch (globalVars[idx].var.type & 7)
			{
				case type_proc:

					oldStack = data -> stack;

					stackCmdProc( _procedure, data -> tokenBuffer2);  

					cmdTmp[instance.cmdStack-1].stack = oldStack;	// carry stack.

					dgetLineFromPointer(globalVars[idx].var.tokenBufferPos );
					dprintf("Goto %08x -- line %d\n", globalVars[idx].var.tokenBufferPos, lineFromPtr.line );

					tokenMode = mode_store;
					stack_frame_up( idx );
					return globalVars[idx].var.tokenBufferPos  ;
			}
		}
		else 	// no arguments
		{
			switch (globalVars[idx].var.type & 7)
			{
				case type_proc:

					oldStack = data -> stack;

					stackCmdProc( _procedure, data->tokenBuffer+sizeof(struct reference)+ref->length ) ;

					cmdTmp[instance.cmdStack-1].stack = oldStack;	// carry stack.

					dgetLineFromPointer(globalVars[idx].var.tokenBufferPos );
					dprintf("Goto %08x -- line %d\n", globalVars[idx].var.tokenBufferPos, lineFromPtr.line );

					tokenMode = mode_store;
					stack_frame_up( idx);
					return globalVars[idx].var.tokenBufferPos  ;
			}
		}
	}

	setError(22,data -> tokenBuffer);

	return  NULL ;
}

char *cmdProcAndArgs(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	struct reference *ref = (struct reference *) (tokenBuffer);

	stackCmdNormal( _procAndArgs, tokenBuffer );
	setStackNum(0);

	cmdTmp[instance.cmdStack-1].tokenBuffer2  = NULL;	// must be reset, is used
	tokenMode = mode_logical;					// parmiters should be handled logicaly, not as store.

	tokenBuffer += ref -> length ;

	return tokenBuffer;
}

char *cmdProc(struct nativeCommand *cmd, char *tokenBuffer )
{
// this is dummy does not do anything, silly thing 
	return tokenBuffer;
}

void _set_return_param( struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (kittyStack[instance.stack].type)
	{
		case type_int:
			var_param_num = kittyStack[instance.stack].integer.value;
			break;
		case type_float:
			var_param_decimal = kittyStack[instance.stack].decimal.value;
			break;
		case type_string:
			if (var_param_str) freeString(var_param_str);
			var_param_str = amos_strdup(kittyStack[instance.stack].str);
			break;
	}
}

char *_endProc( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	_set_return_param( NULL, NULL );
	do_input[instance.parenthesis_count] = do_std_next_arg;	// restore normal operations.
	__stack_frame_down();

	return  data -> tokenBuffer ;
}

char *cmdEndProc(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (instance.cmdStack)
	{
		while (cmdTmp[instance.cmdStack-1].cmd != _procedure )
		{
			instance.cmdStack --;
			if (instance.cmdStack == 0) break;
		}
	}

	if (instance.cmdStack)
	{
		if (cmdTmp[instance.cmdStack-1].cmd == _procedure )
		{
			if (proc_stack_frame)
			{
				if (NEXT_TOKEN(tokenBuffer) == 0x0084 )	//  End Proc[ return value ]
				{
					cmdTmp[instance.cmdStack-1].cmd = _endProc;
					do_input[instance.parenthesis_count] = _set_return_param;
				}
				else 	// End Proc
				{
					tokenBuffer=stack_frame_down();
				}
			}
			else
			{
				printf("bad stack frame\n");
				setError(22,tokenBuffer);
			}
		}
		else
		{
			dump_prog_stack();
			setError(23,tokenBuffer);
		}
	}

	return tokenBuffer;
}

char *read_kitty_args(char *tokenBuffer, int read_stack, unsigned short end_token )
{
	struct glueCommands data;
	char *ptr = tokenBuffer ;
	int args = 0;
	int read_args = 1;
	unsigned short token;
	unsigned int old_stack = read_stack;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	args = instance.stack - read_stack +1;
	instance.token_is_fresh = false;

	// the idea, stack to be read is stored first,

	instance.stack ++;					// prevent read stack form being trached.
	token = *((unsigned short *) ptr);
	for (ptr = tokenBuffer; (token != 0x0000) && (token != 0x0054) && (read_args<=args) ;)
	{
		if (token == end_token)
		{
			// use stack offset
			instance.stack = read_stack;

			// set var
			data.lastVar = last_var;
			_setVar( &data,0 );

			// restore stack
			instance.stack = old_stack;

			break;
		}

		ptr+=2;	// skip token
		ptr = executeToken( ptr, token );

		if ((token == 0x005C) || (token == 0x0054) || (token == 0x0000 ))
		{
			// save stack
			int tmp_stack = instance.stack;
			instance.stack = read_stack;

			// set var
			data.lastVar = last_var;
			_setVar( &data,0 );

			// restore stack
			instance.stack = tmp_stack;

			read_args ++;
			read_stack ++;
		}

		token = *((unsigned short *) ptr);
	}

	if (read_args==args)
	{
		// save stack
		instance.stack = read_stack;

		// set var
		data.lastVar = last_var;
		_setVar( &data,0 );

		read_args ++;
		read_stack ++;
	}

	popStack( instance.stack - old_stack );

	return ptr;
}

char *cmdBracket(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (*(( unsigned short *) tokenBuffer)==0x008C)		// no args
	{
		unsigned int flags;

		while (instance.cmdStack)	// empty prog stack if needed, should not be needed...
		{
			flags = cmdTmp[instance.cmdStack-1].flag;

			if  ( flags & (cmd_loop | cmd_never | cmd_onEol | cmd_proc | cmd_normal)) break;
			cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack], 0);
		}

		return tokenBuffer+2;	// skip end bracket 
	}
	else
	{
		char *(*scmd )( struct glueCommands *data, int nextToken ) = NULL;

		if (instance.cmdStack) scmd = cmdTmp[instance.cmdStack-1].cmd ;

		if ( scmd == _procedure )		// this is at "procedure name[ .... ]"
		{
			return read_kitty_args(tokenBuffer, cmdTmp[instance.cmdStack-1].stack, 0x0000 );
		}
	}

	return tokenBuffer;
}

char *cmdBracketEnd(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned int flags;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	while (instance.cmdStack)
	{
		flags = cmdTmp[instance.cmdStack-1].flag;

		if  ( flags & (cmd_loop | cmd_never | cmd_onEol | cmd_proc | cmd_normal)) break;
		cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack], 0);
	}

	if (instance.cmdStack) if (cmdTmp[instance.cmdStack-1].cmd == _procAndArgs )
	{
		cmdTmp[instance.cmdStack-1].tokenBuffer2 = tokenBuffer;
	}

	if (instance.cmdStack) if (cmdTmp[instance.cmdStack-1].cmd == _endProc ) return cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack],0);

	return tokenBuffer;
}

char *cmdShared(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned short token =*((short *) tokenBuffer);

	for (;;)
	{
		switch (token)
		{
			case 0x0006:	// var
					tokenBuffer +=2;
					tokenBuffer += sizeof(struct reference) + ReferenceByteLength( tokenBuffer );
					break;

			case 0x005C:	// ,
					tokenBuffer +=2;
					break;

			case 0x0074:	// (
					tokenBuffer +=2;
					break;

			case 0x007C:	// )
					tokenBuffer +=2;
					break;

			case 0x0054:
			case 0x0000:
					goto exit_for;

			default:
					printf("bad exit on token %4x\n",token);
					setError(22,tokenBuffer);
					goto exit_for;
		}

		token = *((unsigned short *) (tokenBuffer));
	}

exit_for:

	return tokenBuffer;
}

char *main_cmdGlobal( char *tokenBuffer )
{
	unsigned short token =*((short *) tokenBuffer);

	for (;;)
	{
		switch (token)
		{
			case 0x0006:	tokenBuffer +=2 + sizeof(struct reference) + ReferenceByteLength( tokenBuffer +2 );	break;
			case 0x005C:	tokenBuffer +=2;	break;
			case 0x0074:	tokenBuffer +=2;	break;
			case 0x007C:	tokenBuffer +=2;	break;
			case 0x0054:
			case 0x0000:	goto exit_for;

			default:
					printf("bad exit on token %4x\n",token);
					setError(22,tokenBuffer);
					goto exit_for;
		}

		token = *((unsigned short *) (tokenBuffer));
	}

exit_for:
	return tokenBuffer;
}

int findVar( char *name, bool is_first_token, int type, int _proc );

char *local_cmdGlobal( char *tokenBuffer )
{
	unsigned short token =*((short *) tokenBuffer);
	struct reference *ref;

	struct kittyData *localVar,*mainVar;

	int localVar_ref;
	int mainVar_ref;

	char *str;

	for (;;)
	{
		switch (token)
		{
			case 0x0006:	// var
					tokenBuffer +=2;

					printf("%s\n", tokenBuffer + sizeof(struct reference) );

					ref = (struct reference *) tokenBuffer;
					str = (char *) malloc( ref -> length+1 );

					localVar_ref = findVar( tokenBuffer + sizeof(struct reference) , false , ref -> flags, procStcakFrame[proc_stack_frame].id );
					mainVar_ref = findVar( tokenBuffer + sizeof(struct reference) , false , ref -> flags, 0 );

					if ((mainVar_ref)&&(localVar_ref))
					{
						mainVar = getVar(mainVar_ref);
						localVar = getVar(localVar_ref);

						if ((mainVar)&&(localVar))
						{
							switch ( ref -> flags )
							{
								case type_int:
									localVar -> integer = mainVar -> integer;
									break;
								case type_float:
									localVar -> decimal = mainVar -> decimal;
									break;
								case type_string:
									if (localVar -> str) free(mainVar -> str);
									localVar -> str = amos_strdup(mainVar -> str);
									break;
								default:
									setError(22,tokenBuffer);
							}
						}
					}

					tokenBuffer += sizeof(struct reference) + ReferenceByteLength( tokenBuffer );
					break;

			case 0x005C:	// ,
					tokenBuffer +=2;
					break;

			case 0x0074:	// (
					tokenBuffer +=2;
					break;

			case 0x007C:	// )
					tokenBuffer +=2;
					break;

			case 0x0054:
			case 0x0000:
					goto exit_for;

			default:
					printf("bad exit on token %4x\n",token);
					setError(22,tokenBuffer);
					goto exit_for;
		}

		token = *((unsigned short *) (tokenBuffer));
	}

exit_for:
	return tokenBuffer;
}


char *cmdGlobal(struct nativeCommand *cmd, char *tokenBuffer )
{
	if ( procStcakFrame[proc_stack_frame].id )
	{
		return local_cmdGlobal( tokenBuffer );
	}
	else
	{
		return main_cmdGlobal( tokenBuffer );
	}
}

char *cmdParamStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	if (var_param_str)
	{
		setStackStrDup( var_param_str );
	}
	else
	{
		struct stringData tmp;
		tmp.size = 0;
		tmp.ptr = 0;
		setStackStrDup( &tmp );
	}

	return tokenBuffer;
}

char *cmdParamFloat(struct nativeCommand *cmd, char *tokenBuffer )
{
	setStackDecimal( var_param_decimal );
	return tokenBuffer;
}

char *cmdParam(struct nativeCommand *cmd, char *tokenBuffer )
{
	setStackNum( var_param_num );
	return tokenBuffer;
}

char *cmdPopProc(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	// flush loops, all other stuff

	if (instance.cmdStack)
	{
		while (cmdTmp[instance.cmdStack-1].cmd != _procedure ) 
		{
			instance.cmdStack--;
			if (instance.cmdStack==0) break;
		}
	}

	return cmdEndProc( cmd, tokenBuffer );
}


extern char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ );

char *_cmdRestore( struct glueCommands *data, int nextToken )
{
	char *ptr = NULL;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (kittyStack[instance_stack].type)
	{
		case type_string:
			{
				struct stringData *name = kittyStack[instance_stack].str;
				if (name)	
				{
					struct label *label = findLabel( &name -> ptr, currentFrame -> id );
					ptr = label ? label -> tokenLocation : NULL;
				}
			}
			break;
		case type_int:
			{
				char tmp[ 30 ];
				sprintf( tmp, "%d", kittyStack[instance_stack].integer.value );
				struct label *label = findLabel( tmp, currentFrame -> id );
				ptr = label ? label -> tokenLocation : NULL;
			}
			break;
	}

	popStack( instance.stack - data->stack  );

	if (ptr)
	{
		ptr = FinderTokenInBuffer( ptr-2, 0x0404 , -1, -1, _file_end_ );
		currentFrame -> dataPointer = ptr;
	}
	else
	{
		setError( 40, data->tokenBuffer );
	}

	return NULL;
}

char *cmdRestore(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned short next_token = NEXT_TOKEN(tokenBuffer);
	struct reference *ref = NULL;
	uint16_t varType = 0x0;
	char *name;

	proc_names_printf("%s:%d\n", __FUNCTION__,__LINE__);

	switch (next_token)
	{
		case 0x0006:	// var
			{
				struct kittyData *var;
				ref = (struct reference *) (tokenBuffer + 2);

				var = getVar(ref -> ref);
				varType = var -> type;
			}
			break;

		case 0x0018:	// label
			{
				ref = (struct reference *) (tokenBuffer + 2);

				if (ref -> ref)
				{

					unsigned int idx = (unsigned int) ref->ref - 1;
					varType = globalVars[idx].var.type;
				}
			}
			break;
	}

	if (ref)
	{
		switch ( varType & 7 )
		{
			case type_int:
			case type_proc:

				if (name = dupRef( ref ))
				{
					struct label *label = findLabel(name, procStcakFrame[proc_stack_frame].id);
					char *ptr = label -> tokenLocation;
					free(name);

					if (ptr) 
					{
						printf("%s:%d\n", __FUNCTION__,__LINE__);

						ptr = FinderTokenInBuffer( ptr-2, 0x0404 , -1, -1, _file_end_ );
						currentFrame -> dataPointer = ptr;
					}
					else 	setError( 40, tokenBuffer );
				}
				return tokenBuffer + 2 + sizeof(struct reference) + ref -> length;

				break;
		}
	}
		
	// if we are here, then we did not use name of var as label name.
	stackCmdNormal( _cmdRestore, tokenBuffer );

	return tokenBuffer;
}

char *cmdRestoreNoArgs(struct nativeCommand *cmd, char *tokenBuffer )
{
	if (proc_stack_frame)
	{
		struct globalVar *this_proc = findProcPtrById( currentFrame -> id );
		currentFrame ->  dataPointer = this_proc -> procDataPointer;
	}
	else
	{
		currentFrame ->  dataPointer = proc_main_data.procDataPointer;
	}
	return tokenBuffer;
}


char *cmdData(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned short jump = *((unsigned short *) tokenBuffer) * 2;
	return tokenBuffer + jump - 4;	// next token (size 2) + data (size 2) will be added on exit.
}


char *_cmdExit(struct glueCommands *data, int nextToken )
{
	int args = instance.stack - data -> stack +1;
	int exit_loops = 1;
	unsigned short token;
	char *ptr;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1) exit_loops = getStackNum(instance.stack);
	popStack( instance.stack - data -> stack  );

	while (exit_loops>1)
	{
		if (dropProgStackToFlag( cmd_loop )) instance.cmdStack--;
		exit_loops--;
	}

	if (dropProgStackToFlag( cmd_loop ))
	{
		ptr = cmdTmp[instance.cmdStack-1].tokenBuffer;
		token = *((unsigned short *) (ptr - 2)) ;

		switch (token)
		{
			case 0x023C:	// For
			case 0x0250:	// Repeat
			case 0x0268:	// While
			case 0x027E:	// DO

				cmdTmp[instance.cmdStack-1].cmd = _exit;
				cmdTmp[instance.cmdStack-1].flag = cmd_never ;
				return ptr + ( *((unsigned short *) ptr) * 2 )-2;
				break;

			default:
				dump_prog_stack();
				proc_names_printf("token was %08x\n", token);
		}
	}

	return NULL;
}

char *cmdExit(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdExit, tokenBuffer );
	setStackNum(1);	// set default exit loops
	return tokenBuffer;
}

char *_cmdExitIf(struct glueCommands *data, int nextToken)
{
	int exit_loops = 1;
	bool is_true = false;
	unsigned short token;
	char *ptr;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = instance.stack - data -> stack +1;

	switch (args)
	{
		case 1:
			is_true = getStackNum(instance.stack);
			break;
		case 2:
			is_true = getStackNum(instance.stack-1);
			exit_loops = getStackNum(instance.stack);
			break;
	}

	popStack( instance.stack - data -> stack  );

	if (is_true == false) return NULL;

	while (exit_loops>1)
	{
		if (dropProgStackToFlag( cmd_loop )) instance.cmdStack--;
		exit_loops--;
	}

	if (dropProgStackToFlag( cmd_loop ))
	{
		ptr = cmdTmp[instance.cmdStack-1].tokenBuffer;
		token = *((unsigned short *) (ptr - 2)) ;

		switch (token)
		{
			case 0x023C:	// For
			case 0x0250:	// Repeat
			case 0x0268:	// While
			case 0x027E:	// DO

				cmdTmp[instance.cmdStack-1].cmd = _exit;
				cmdTmp[instance.cmdStack-1].flag = cmd_never ;
				ptr =  ptr + ( *((unsigned short *) ptr) * 2 )-2   ;
				return ptr;
				break;

			default:
				dump_prog_stack();
				proc_names_printf("token was %08x\n", token);
				getchar();
		}
	}

	return NULL;
}

char *cmdExitIf(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdExitIf, tokenBuffer );
	tokenMode = mode_logical;

	return tokenBuffer;
}


char *_cmdEvery( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	NYI(__FUNCTION__);

	return  NULL;
}

char *cmdEveryOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	every_on = true;
	return tokenBuffer;
}

char *cmdEveryOff(struct nativeCommand *cmd, char *tokenBuffer )
{
	every_on = false;
	return tokenBuffer;
}

char *cmdEvery(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	on_every_proc_location = NULL;
	on_every_gosub_location = NULL;

	if (NEXT_TOKEN(tokenBuffer) == 0x003E )	// next is variable
	{
		every_timer = *((int *) (tokenBuffer + 2));
		tokenBuffer += 6;

		switch (NEXT_TOKEN(tokenBuffer))
		{
			// gosub
			case 0x02B2:
					tokenBuffer += 2;

					if (NEXT_TOKEN(tokenBuffer ) == 0x006)	// label
					{
						char *name;
						struct reference *ref = (struct reference *) (tokenBuffer + 2);
						name = strndup( tokenBuffer + 2 + sizeof(struct reference), ref->length );	

						if (name)
						{
							struct label *label = findLabel(name, procStcakFrame[proc_stack_frame].id);
							on_every_gosub_location = label -> tokenLocation;
							every_on = true;
							free(name);
						}

						tokenBuffer += (2 + sizeof(struct reference) + ref -> length) ;					
					}

					break;
			// proc
			case 0x0386:
					tokenBuffer += 2;

					switch (NEXT_TOKEN(tokenBuffer ))
					{
						case 0x0012:
						case 0x0006:

							char *name;
							struct reference *ref = (struct reference *) (tokenBuffer + 2);
							name = strndup( tokenBuffer + 2 + sizeof(struct reference), ref->length );

							if (name)
							{
								int found = findVarPublic(name,ref -> flags);
								if (found)
								{
									on_every_proc_location = globalVars[found -1].var.tokenBufferPos;
									every_on = true;
								}

								free(name);
							}

							tokenBuffer += (2 + sizeof(struct reference) + ref -> length) ;	
					}
	
					break;
		}

//		dprintf("every timer: %d\n",every_timer);

	}

	gettimeofday(&every_before, NULL);	// reset diff.

	stackCmdNormal( _cmdEvery, tokenBuffer );

	return tokenBuffer;
}

extern struct amos_selected _selected_;
extern int getMenuEvent();		// is atomic
extern bool onMenuEnabled;

char *_cmdWait( struct glueCommands *data, int nextToken )
{
	int w = getStackNum(instance.stack);

	engine_lock();
	if (bobUpdateNextWait)
	{
		bobDoUpdate = 1;
		bobUpdateNextWait = 0;
	}
	engine_unlock();

	// delay 	1 x tick = 20ms,  (20ms * 50 = 1000 ms)
	// wait 50 = 1 sec, see AMOS manual.

	Delay(w);

	return  NULL ;
}

extern char *onMenuTokenBuffer ;
extern uint16_t onMenuToken ;
extern char *execute_on( int num, char *tokenBuffer, char *returnTokenBuffer, unsigned short token );

char *cmdWait(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (onMenuEnabled)
	{
		if (getMenuEvent())
		{
			char *ret;
			ret  = execute_on( _selected_.menu +1, onMenuTokenBuffer , tokenBuffer, onMenuToken );
			if (ret) tokenBuffer = ret - 2;		// +2 will be added on exit.
		}
	}

	stackCmdNormal( _cmdWait, tokenBuffer );

	return tokenBuffer;
}


char *_set_timer( struct glueCommands *data, int nextToken )
{
	timer_offset = getStackNum( instance.stack );
	gettimeofday(&timer_before, NULL);	// reset diff.
	_do_set = _setVar;
	return NULL;
}


char *cmdTimer(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned short next_token = *((short *) tokenBuffer);
	unsigned int ms_before;
	unsigned int ms_after;
	unsigned int timer_diff;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( (instance.token_is_fresh) && (NEXT_TOKEN(tokenBuffer) == 0xFFA2 ))
	{
		tokenMode = mode_store;
		_do_set = _set_timer;
	}
	else
	{
		if ( correct_order( getLastProgStackToken(),  next_token ) == false )
		{
			setStackHiddenCondition();
		}
	}	

	gettimeofday(&timer_after, NULL);	// reset diff.

	ms_before = (timer_before.tv_sec * 1000) + (timer_before.tv_usec/1000);
	ms_after = (timer_after.tv_sec * 1000) + (timer_after.tv_usec/1000);

	timer_diff = (ms_after - ms_before) / 33 ;

	if (timer_diff) timer_before = timer_after ;

	timer_offset += timer_diff;

	setStackNum( timer_offset );		// 1/50 sec = every 20 ms
	kittyStack[instance.stack].state = state_none;
	flushCmdParaStack( next_token );

	return tokenBuffer;
}

char *cmdBreakOff(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdBreakOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdCloseWorkbench(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdCloseEditor(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdKillEditor(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdAmosToBack(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	engine_lock();
	engineCmdQue.push_back(kitty_to_back);
	engine_unlock();
	return tokenBuffer;
}

char *cmdAmosToFront(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	engine_lock();
	engineCmdQue.push_back(kitty_to_front);
	engine_unlock();
	return tokenBuffer;
}

char *cmdAmosLock(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdAmosUnlock(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *_cmdNot( struct glueCommands *data, int nextToken )
{
	int res;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	res = getStackNum( instance.stack );
	popStack( instance.stack - cmdTmp[instance.cmdStack-1].stack  );
	setStackNum(~res);

	return  NULL ;
}

char *cmdNot(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( getLastProgStackFn() == _if )
	{
		int nextToken = *((unsigned short *) tokenBuffer);

		if (nextToken != 0x0074)
		{
			stackCmdFlags( _cmdNot, tokenBuffer, cmd_onNextCmd );
			return tokenBuffer;
		}
	}

	stackCmdParm( _cmdNot, tokenBuffer );
	return tokenBuffer;
}

char *_cmdSetBuffers( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	popStack( instance.stack - data->stack  );
	return NULL;
}

char *cmdSetBuffers(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdSetBuffers, tokenBuffer );
	return tokenBuffer;
}

char *cmdMultiWait(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (onMenuEnabled)
	{
		if (getMenuEvent())
		{
			char *ret;
			ret  = execute_on( _selected_.menu +1, onMenuTokenBuffer , tokenBuffer, onMenuToken );
			if (ret) tokenBuffer = ret - 2;		// +2 will be added on exit.
		}
	}

	Delay(1);

	return tokenBuffer;
}

char *cmdEdit(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;	// quit
}

char *cmdDirect(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;	// quit
}

char *cmdPop(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}


char *cmdFlushStack ( struct glueCommands *data, int nextToken )
{
	popStack( instance.stack - data -> stack );
	return NULL;
}

char *cmdExtension( struct nativeCommand *cmd, char *tokenBuffer )
{
	struct extension *ext = (struct extension *) tokenBuffer;
	char *(*ext_cmd) EXT_CMD_ARGS = NULL;
	struct extension_lib *_this = NULL;

	_this = &kitty_extensions[ ext-> ext ];
	if (_this) 
	{
		if (_this -> lookup)
		{
			ext_cmd = (char* (*) EXT_CMD_ARGS) *((void **) (kitty_extensions[ext-> ext].lookup + ext -> token));
		}
	}

	if (ext_cmd)
	{
		instance.current_extension = ext-> ext;
		return ext_cmd( &instance, cmd, tokenBuffer);
	}
	else
	{
		getLineFromPointer( tokenBuffer );
		printf("*** warning extensions not yet supported, extention %d, token %04x at line %d ****\n", ext-> ext, ext-> token, lineFromPtr.line );
		stackCmdNormal( cmdFlushStack, tokenBuffer );
	}

	return tokenBuffer;
}

char *cmdChipFree( struct nativeCommand *cmd, char *tokenBuffer )
{
#ifdef __amigaos4__
	setStackNum( AvailMem(MEMF_CHIP) );
#endif

#ifdef __linux__
	setStackNum( sys_memavail_gfxmem() );
#endif

	return tokenBuffer;
}

char *cmdFastFree( struct nativeCommand *cmd, char *tokenBuffer )
{

#ifdef __amigaos4__
	setStackNum( AvailMem(MEMF_FAST) );
#endif

#ifdef __linux__
	setStackNum( sys_memavail_sysmem() );
#endif

	return tokenBuffer;
}

char *cmdWaitKey(struct nativeCommand *cmd, char *tokenBuffer );

char *cmdStop( struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("** command stop **\n");
	dump_prog_stack();
	dump_stack();
	dump_global();
	dump_680x0_regs();
	dump_screens();
	printf("** press a key to quit **\n");
	cmdWaitKey(cmd, tokenBuffer );
	return NULL;
}

char *cmdCommandLineStr( struct nativeCommand *cmd, char *tokenBuffer )
{
	struct stringData tmp;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	tmp.size = 0;
	tmp.ptr = 0;

	setStackStrDup(&tmp);
	return tokenBuffer;
}

char *_cmdExec( struct glueCommands *data, int nextToken )
{
	int args = instance.stack - data -> stack + 1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	struct stringData *cmd;

	switch (args)
	{
		case 1:	cmd = getStackString(instance.stack);

#ifdef __amigaos4__
				if (cmd) SystemTags( &cmd -> ptr, 
							SYS_Asynch, FALSE, 
							TAG_END);
#endif

#ifdef __linux__
				if (cmd) system( cmd );
#endif
				break;

		default:
				setError(22,data->tokenBuffer);
	}

	return NULL;
}

char *cmdExec( struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _cmdExec, tokenBuffer );
	return tokenBuffer;
}

char *cmdSetAccessory( struct nativeCommand *cmd, char *tokenBuffer )
{
	return tokenBuffer;
}

char *cmdPrgUnder( struct nativeCommand *cmd, char *tokenBuffer )
{
	setStackNum(1);
	return tokenBuffer;
}

char *includeNOP(nativeCommand *cmd,char *ptr)
{
	setError(22,ptr);
	return ptr;
}


