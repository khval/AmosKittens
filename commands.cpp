
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#endif

#ifdef __linux__
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include "os/linux/stuff.h"
#endif

#include "debug.h"
#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "var_helper.h"
#include "errors.h"
#include "engine.h"

extern void clear_local_vars( int proc );

bool every_on = true;
unsigned int every_timer = 0;
char *on_every_gosub_location = NULL;
char *on_every_proc_location = NULL;
struct timeval every_before, every_after;

extern char *_file_pos_ ;

int timer_offset = 0;

extern int _last_var_index;		// we need to know what index was to keep it.
extern int _set_var_index;		// we need to resore index 

static struct timeval timer_before, timer_after;

extern std::vector<struct label> labels;

extern int last_var;
extern struct globalVar globalVars[];
extern int tokenMode;
extern int tokenlength;
extern void setStackStr( char *str );
extern void setStackStrDup( const char *str );
extern int findVarPublic( char *name, int type );
extern int ReferenceByteLength(char *ptr);
extern int findLabelRef( char *name );

using namespace std;

extern char *findLabel( char *name );

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
	return  data -> tokenBuffer ;
}



void stack_frame_up(struct kittyData *var)
{
	proc_stack_frame++;		// move stack frame up.
	if ( var -> procDataPointer )
	{
		data_read_pointers[proc_stack_frame]  = var -> procDataPointer;
	}
	else data_read_pointers[proc_stack_frame] = data_read_pointers[proc_stack_frame-1];
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

					cmdTmp[cmdStack-1].stack = oldStack;	// carry stack.
					dprintf("Goto %08x -- line %d\n", globalVars[idx].var.tokenBufferPos, getLineFromPointer(globalVars[idx].var.tokenBufferPos ) );

					tokenMode = mode_store;
					stack_frame_up(&globalVars[idx].var);
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

					cmdTmp[cmdStack-1].stack = oldStack;	// carry stack.
					dprintf("Goto %08x -- line %d\n", globalVars[idx].var.tokenBufferPos, getLineFromPointer(globalVars[idx].var.tokenBufferPos ) );

					tokenMode = mode_store;
					stack_frame_up(&globalVars[idx].var);
					return globalVars[idx].var.tokenBufferPos  ;
			}
		}
	}

	setError(22,data -> tokenBuffer);

	return  NULL ;
}


char *_if( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	char *ptr;
	int args = stack - data -> stack + 1;

	if (args > 1) 
	{
		dump_stack();
		dump_prog_stack();
		setError(22,data -> tokenBuffer);
	}

	if (kittyStack[stack].value == 0)	// 0 is FALSE always -1 or 1 can be TRUE
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

	proc_names_printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	// two command should be stacked, while loop, and while check.
	// while loop is removed from stack, if check is false
	// and we jump over the wend

	if (kittyStack[data->stack].value == 0)
	{
		if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _while ) 
		{
			cmdStack --;
			offset = *((unsigned short *) cmdTmp[cmdStack].tokenBuffer);
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
	if (kittyStack[stack].value == 0) return data -> tokenBuffer;
	return 0;
}

BOOL setVarInt( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_int:
			var->value = kittyStack[stack].value;
			return TRUE;

		case type_float:
			var->value = (int) kittyStack[stack].decimal;
			return TRUE;
	}

	return FALSE;
}

BOOL setVarDecimal( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_int:
			var->decimal = (double) kittyStack[stack].value;
			return TRUE;

		case type_float:
			var->decimal = kittyStack[stack].decimal;
			return TRUE;
	}

	return FALSE;
}

BOOL setVarString( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_string:
			if (var->str) free(var->str);

			if (kittyStack[stack].str)
			{
				var->str = strdup(kittyStack[stack].str);
				var->len = kittyStack[stack].len;
			}
			else
			{
				var->str = NULL;
				var->len = 0;
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

		switch (kittyStack[stack].type)
		{
			case type_int:
				var->int_array[var -> index] = kittyStack[stack].value;
				return TRUE;

			case type_float:
				var->int_array[var -> index] = (int) kittyStack[stack].decimal;
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

		switch (kittyStack[stack].type)
		{
			case type_int:
				var->float_array[var -> index] = (double) kittyStack[stack].value;
				return TRUE;

			case type_float:
				var->float_array[var -> index] = kittyStack[stack].decimal;
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

		switch (kittyStack[stack].type)
		{
			case type_string:
				if (var->str_array[var -> index] ) free(var->str_array[var->index]);
				var->str_array[var -> index] = strdup(kittyStack[stack].str);	
				return TRUE;
		}
	}
	else setError(25,tokenBuffer);

	return FALSE;
}

char *(*_do_set) ( struct glueCommands *data, int nextToken ) = _setVar;


char *_setVar( struct glueCommands *data, int nextToken )
{
	BOOL success;
	struct kittyData *var;

	proc_names_printf("%s:%d -- set var %d\n",__FUNCTION__,__LINE__, data -> lastVar-1);

	var = &globalVars[data -> lastVar-1].var;

	#ifdef show_debug_printf_yes
		printf("SET var %s",globalVars[ data->lastVar-1].varName);
		if (var -> type & type_array)	printf("(%d)=",_set_var_index);
		printf("\n");
	#endif

	success = FALSE;

	switch (var->type)
	{
		case type_int:
			success = setVarInt( var );
			break;
		case type_float:
			success = setVarDecimal( var );
			break;
		case type_string:
			success = setVarString( var );
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
		if ( kittyStack[stack].type !=  (var -> type & 7))
		{
			proc_names_printf("kittyStack[%d].type= %d, (globalVars[%d].var.type & 7)=%d\n",
				stack, 
				kittyStack[stack].type, 
				data -> lastVar, 
				var -> type & 7);
			dump_stack();
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

	if (kittyStack[stack].str) free(kittyStack[stack].str);
	kittyStack[stack].str = NULL;
	stack --;

	data->lastVar = last_var;	// we did know then, but now we know,
	return _setVar( data, 0 );
}

//--------------------------------------------------------

int parenthesis[100];
extern int parenthesis_count ;

char *nextArg(struct nativeCommand *cmd, char *tokenBuffer)
{
	flushCmdParaStack(0);
	
	if (do_input[parenthesis_count]) do_input[parenthesis_count]( cmd, tokenBuffer );	// read from keyboad or disk.
	return tokenBuffer;
}


char *parenthesisStart(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	parenthesis[parenthesis_count] =stack;
	parenthesis_count++;

	setStackParenthesis();
	stack++;
	setStackNone();

	return tokenBuffer;
}

extern int last_var;

char *parenthesisEnd(struct nativeCommand *cmd, char *tokenBuffer)
{
	char *ret;
	int nextToken = *((unsigned short *) tokenBuffer);
	int lastToken;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	_file_pos_ = tokenBuffer;		// needed by "Fn", need to return End Bracket after Fn call.

	ret = flushCmdParaStack(nextToken);
	if (ret) return ret;

	if (parenthesis_count)
	{
		remove_parenthesis( parenthesis[parenthesis_count -1] );
		parenthesis[parenthesis_count -1] = 255;
		do_input[parenthesis_count] = do_std_next_arg;
		parenthesis_count--;

		if (cmdStack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack], nextToken);
	}

	lastToken = last_tokens[parenthesis_count];

	if ( correct_order( lastToken ,  nextToken ) == false )
	{
		// hidden ( condition.
		kittyStack[stack].str = NULL;
		kittyStack[stack].value = 0;
		kittyStack[stack].state = state_hidden_subData;
		stack++;
	}

	ret = flushCmdParaStack(nextToken);
	if (ret) return ret;

	return tokenBuffer;
}

char *breakData(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (cmdTmp[cmdStack-1].flag & (cmd_index | cmd_onBreak) ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);

	if (do_breakdata) do_breakdata( cmd, tokenBuffer );
	return tokenBuffer;
}

char *setVar(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack], 0);

	if (tokenMode == mode_logical)
	{
		stackCmdParm(_equalData, tokenBuffer);
		stack++;
		setStackNum(0);	// prevent random data from being on the stack.
	}
	else
	{
		stackCmdNormal( _do_set, tokenBuffer);
		_set_var_index = _last_var_index;		// (var->index will be overwritten by index reads)

		switch (kittyStack[stack].type)
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
	proc_names_printf("%s:%d - line %d\n",__FUNCTION__,__LINE__, getLineFromPointer( tokenBuffer ));

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

	while (cmdStack)
	{
		if (cmdTmp[cmdStack-1].cmd == _if ) break;
		cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
	}

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _if ) ret=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);

	if (cmdStack)
	{
		if (cmdTmp[cmdStack-1].cmd == _ifSuccess) 
		{
			cmdTmp[cmdStack-1].cmd = _ifThenSuccess;
			cmdTmp[cmdStack-1].flag = cmd_onEol;			// should run at end of line
		}
		else	if (cmdTmp[cmdStack-1].cmd == _ifNotSuccess) 
		{
			cmdTmp[cmdStack-1].cmd = _ifThenNotSuccess;
			cmdTmp[cmdStack-1].flag = cmd_onEol;			// should run at end of line
		}
	}

	if (ret) tokenBuffer = ret -2;	// on exit +2 token
	tokenMode = mode_standard;

	return tokenBuffer;
}

char *nextCmd(nativeCommand *cmd, char *ptr);

char *cmdElse(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d - line %d\n",__FUNCTION__,__LINE__, getLineFromPointer( tokenBuffer ));

	char *retTokenBuffer = nextCmd(NULL, tokenBuffer);
	if (retTokenBuffer != tokenBuffer) tokenBuffer = retTokenBuffer + 2;	// nextCmd() should expect +2 token

	if (cmdStack)
	{
		if ((cmdTmp[cmdStack-1].cmd == _ifSuccess) || (cmdTmp[cmdStack-1].cmd == _ifThenSuccess)) 		// if success jump over else
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

	if (kittyStack[data->stack].value == 0)	// 0 is FALSE always -1 or 1 can be TRUE
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
		if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _ifNotSuccess) cmdStack--; 
		stackIfSuccess();
	}

	return NULL;
}

char *cmdElseIf(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d - line %d\n",__FUNCTION__,__LINE__, getLineFromPointer( tokenBuffer ));

	if (cmdStack)
	{
		if (cmdTmp[cmdStack-1].cmd == _ifSuccess)		// if success jump over else if
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
	if (cmdStack)
	{
		if ( (cmdTmp[cmdStack-1].cmd == _ifSuccess) || (cmdTmp[cmdStack-1].cmd == _ifNotSuccess) )
		{
			cmdStack--;
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
	int args = stack - data -> stack + 1;
	int ref_num = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args != 1) setError(22,data -> tokenBuffer);

	switch (kittyStack[stack].type)
	{
		case type_int:	
				{
					char num[50];
					sprintf(num,"%d", kittyStack[stack].value );
					ref_num = findLabelRef( num );
				}
				break;

		case type_string:
				{
					char *txt = getStackString( stack );
					ref_num = findLabelRef( txt );
				}
				break;

		default:
				setError(22,data -> tokenBuffer);
	}

	if (ref_num)
	{
		char *return_tokenBuffer = data -> tokenBuffer;

//		printf("jump to %08x\n",labels[ref_num-1].tokenLocation);

		while ( *((unsigned short *) return_tokenBuffer) != nextToken  ) return_tokenBuffer += 2;
		stackCmdLoop( _gosub_return, return_tokenBuffer );
		return labels[ref_num-1].tokenLocation;
	}
	else
	{
		if (kittyStack[stack].type == type_string)
		{
			printf("gosub can't find string '%s'\n",kittyStack[stack].str);
		}

		setError(22,data -> tokenBuffer);
	}

	return NULL ;
}



char *_goto( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	int args = stack - data -> stack + 1;
	int ref_num = 0;

	if (args != 1) setError(22,data -> tokenBuffer);

	switch (kittyStack[stack].type)
	{
		case type_int:	
				{
					char num[50];
					sprintf(num,"%d", kittyStack[stack].value );
					ref_num = findLabelRef( num );
				}
				break;

		case type_string:
				{
					char *txt = getStackString( stack );
					ref_num = findLabelRef( txt );
				}
				break;

		default:
				setError(22,data -> tokenBuffer);
	}

	if (ref_num)
	{
		return labels[ref_num-1].tokenLocation;
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
	char *ptr;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (next_token)
	{
		case 0x0026:	// text

					stackCmdNormal( _goto, tokenBuffer );
					break;

		case 0x0018:	// label
		case 0x0006: 	// variable

					switch ( var_type_is( (struct reference *) (tokenBuffer+2), 0x7 ))
					{
						case type_int:		// jump to label with same name as var.
								tokenBuffer = var_JumpToName( (struct reference *) (tokenBuffer+2) ) -2;		// after function, amos kittens try access next token and adds +2 (+0 data)
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
	char *ptr;
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

		case 0x0018:	// label
		case 0x0006: 	// variable

					switch ( var_type_is( (struct reference *) (tokenBuffer+2), 0x7 ))
					{
						case type_int:		// jump to label with same name as var.

								// [next token][ref][data], 

								return_tokenBuffer = tokenBuffer + 4 + sizeof(struct reference ) + ReferenceByteLength(tokenBuffer + 2);
								tokenBuffer = var_JumpToName( (struct reference *) (tokenBuffer+2) ) - 2; 			// after function, amos kittens try access next token and adds +2 (+0 data)

								if (tokenBuffer) stackCmdLoop( _gosub_return, return_tokenBuffer );
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

	if (cmdStack) 
	{
		if (cmdTmp[cmdStack-1].cmd == _do )
		{
			tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
		}
		else 	if (cmdTmp[cmdStack-1].cmd == _exit)
		{
			cmdStack--;
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

	if (cmdStack)
	{
		if (cmdTmp[cmdStack-1].cmd == _while ) 
		{
			tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
		}
		else 	if (cmdTmp[cmdStack-1].cmd == _exit)
		{
			cmdStack--;
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

	tokenMode = mode_logical;
	if (cmdStack)
	{
		if (cmdTmp[cmdStack-1].cmd == _repeat )
		{
			cmdTmp[cmdStack-1].flag = cmd_normal;
		}
		else if (cmdTmp[cmdStack-1].cmd == _exit)
		{
			cmdStack--;
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _for, tokenBuffer );
	cmdTmp[cmdStack-1].step = 1;		// set default counter step
	do_to[parenthesis_count] = do_for_to;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	return tokenBuffer;
}

char *do_for_to( struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);

	if (cmdStack) if ( cmdTmp[cmdStack-1].cmd == _setVar ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);

	if (cmdStack) 
	{
		// We loop back to "TO" not "FOR", we are not reseting COUNTER var.

		if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag & cmd_normal ))
		{
			cmdTmp[cmdStack-1].tokenBuffer2 = tokenBuffer ;
			cmdTmp[cmdStack-1].cmd_type = cmd_loop;
			popStack( stack - cmdTmp[cmdStack-1].stack );
		}
	}
	return NULL;
}

char *do_to_default( struct nativeCommand *, char * )
{
	stack ++;
	return NULL;
}

char *cmdTo(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	if (do_to[parenthesis_count])
	{
		char *ret = do_to[parenthesis_count]( cmd, tokenBuffer );	
		if (ret) tokenBuffer = ret;
	}

	return tokenBuffer;
}

char *_step( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%d--------\n",__FUNCTION__,__LINE__);

	if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag == cmd_loop ))
	{
		cmdTmp[cmdStack-1].step = kittyStack[stack].value;
	}
	popStack(stack - data->stack);

	return NULL;
}

char *cmdStep(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _step, tokenBuffer );	// we need to store the step counter.

	if (NEXT_TOKEN(tokenBuffer) == 0xFFCA)
	{
		setStackNum(0);
		stack++;
		setStackNum(0);
	}

	return tokenBuffer;
}

extern char *executeToken( char *ptr, unsigned short token );

#undef NEXT_INT

int FOR_NEXT_INT( char *tokenBuffer , char **new_ptr )
{
	unsigned short token;
	char *ptr = tokenBuffer;
	int cmdStack_start = cmdStack;

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

		last_tokens[parenthesis_count] = token;
		token = *( (short *) ptr);
		ptr += 2;
	};

	*new_ptr = ptr - 2;

	// forcefully flush all cmds

	while ( cmdStack > cmdStack_start ) 
	{
		cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack], 0);
	}

	return getStackNum(stack);
}

char *cmdNext(struct nativeCommand *cmd, char *tokenBuffer )
{
	char *ptr = tokenBuffer ;
	char *new_ptr = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( cmdTmp[cmdStack-1].cmd == _for )
	{
		int idx_var = -1;

		if (NEXT_TOKEN(ptr) == 0x0006 )	// Next var
		{
			struct reference *ref = (struct reference *) (ptr + 2);
			idx_var = ref -> ref -1;
		}
		else 	// For var=
		{
			if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag == cmd_loop ))
			{
				char *ptr = cmdTmp[cmdStack-1].tokenBuffer + 2  ;	// first short is JMP address, next after is token.
	
				if (NEXT_TOKEN(ptr) == 0x0006 )	// next is variable
				{
					struct reference *ref = (struct reference *) (ptr + 2);
					idx_var = ref -> ref -1;
				}
			}
		}
		
		if (idx_var>-1)
		{
			unsigned short next_num;

			ptr = cmdTmp[cmdStack-1].FOR_NUM_TOKENBUFFER;
			globalVars[idx_var].var.value +=cmdTmp[cmdStack-1].step; 
			next_num = FOR_NEXT_INT(ptr, &new_ptr);

			if (cmdTmp[cmdStack-1].step > 0)
			{
				if (globalVars[idx_var].var.value <= next_num )
				{
					tokenBuffer = new_ptr;
				}
				else
				{
					cmdStack--;
				}
			}
			else	if (cmdTmp[cmdStack-1].step < 0)
			{
				if (globalVars[idx_var].var.value >= next_num  )
				{
					tokenBuffer = new_ptr;
				}
				else
				{
					cmdStack--;
				}
			}
			else setError(23,tokenBuffer);
		}
		else
		{
			setError(22,tokenBuffer);	
		}
	}
	else	if (cmdTmp[cmdStack-1].cmd == _exit )
	{
		cmdStack --;
	}
	else
	{
		dump_prog_stack();	// wtf unexpected....
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

	while (cmdStack)
	{
		scmd = cmdTmp[cmdStack-1].cmd;
		if ( (scmd==_ifSuccess) ||  (scmd==_ifNotSuccess) ||  (scmd==_ifThenSuccess) ||  (scmd==_ifThenNotSuccess) )
		{
			cmdStack --;
		}
		else break;
	}

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _gosub_return ) 
	{
		char *ptr = cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
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

	printf("Goto %08x -- line %d\n",proc -> EndOfProc, getLineFromPointer(proc -> EndOfProc ));

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

char *cmdProcAndArgs(struct nativeCommand *cmd, char *tokenBuffer )
{
	int proc;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	struct reference *ref = (struct reference *) (tokenBuffer);

	stackCmdNormal( _procAndArgs, tokenBuffer );
	cmdTmp[cmdStack-1].tokenBuffer2  = NULL;	// must be reset, is used

	tokenBuffer += ref -> length ;

	proc = get_proc_num_from_ref(ref->ref);

	if (proc)
	{
		printf("proc num %d\n",proc);
		clear_local_vars( proc );
	}

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

	switch (kittyStack[stack].type)
	{
		case type_int:
			var_param_num = kittyStack[stack].value;
			break;
		case type_float:
			var_param_decimal = kittyStack[stack].decimal;
			break;
		case type_string:
			if (var_param_str) free(var_param_str);
			var_param_str = strdup(kittyStack[stack].str);
			break;
	}
}

char *_endProc( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	_set_return_param( NULL, NULL );
	do_input[parenthesis_count] = do_std_next_arg;	// restore normal operations.
	proc_stack_frame--;		// move stack frame down.

	return  data -> tokenBuffer ;
}

char *cmdEndProc(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (cmdStack)
	{
		printf("next token is %04x\n",NEXT_TOKEN(tokenBuffer));

		if (NEXT_TOKEN(tokenBuffer) == 0x0084 )	//  End Proc[ return value ]
		{
			proc_names_printf("yes we are here\n");

			// changes function pointer only so that ']' don't think its end of proc by accident.
			// we aslo push result of stack into Param.

			if (cmdTmp[cmdStack-1].cmd == _procedure ) cmdTmp[cmdStack-1].cmd = _endProc;

			// _endProc -- calls "proc_stack_frame--"

			do_input[parenthesis_count] = _set_return_param;

		}
		else 	// End Proc
		{
			if (cmdTmp[cmdStack-1].cmd == _procedure )
			{
				tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
				proc_stack_frame--;		// move stack frame down.
			}
			else
			{
				dump_prog_stack();
				setError(23,tokenBuffer);
			}
		}
	}

	return tokenBuffer;
}

char *read_kitty_args(char *tokenBuffer, struct glueCommands *sdata)
{
	struct reference *ref;
	struct glueCommands data;
	char *ptr = tokenBuffer ;
	int args = 0;
	int read_args ;
	int read_stack;
	unsigned short token;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	args = stack - sdata->stack +1;

	// the idea, stack to be read is stored first,

	read_stack = sdata -> stack;
	stack ++;					// prevent read stack form being trached.

	read_args = 1;
	token = *((unsigned short *) ptr);

	for (ptr = tokenBuffer; (token != 0x0000) && (token != 0x0054) && (read_args<=args) ;)
	{
		ptr+=2;	// skip token
		ptr = executeToken( ptr, token );

		if ((token == 0x005C) || (token == 0x0054) || (token == 0x0000 ))
		{
			// save stack
			int tmp_stack = stack;
			stack = read_stack;

			// set var
			data.lastVar = last_var;
			_setVar( &data,0 );

			// restore stack
			stack = tmp_stack;

			read_args ++;
			read_stack ++;
		}

		token = *((unsigned short *) ptr);
	}

	if (read_args==args)
	{
		// save stack
		int tmp_stack = stack;
		stack = read_stack;

		// set var
		data.lastVar = last_var;
		_setVar( &data,0 );

		// restore stack
		stack = tmp_stack;

		read_args ++;
		read_stack ++;
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );
	return ptr;
}

char *read_kitty_args_old(char *tokenBuffer, struct glueCommands *sdata)
{
	struct reference *ref;
	struct glueCommands data;
	char *ptr;
	int args = 0;
	int n;
	unsigned short token;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	args = stack - sdata->stack +1;
	stack -= (args-1);	// move to start of args.

	n=0;
	for (ptr = tokenBuffer; (*((unsigned short *) ptr) != 0x008C) &&(n<args) ;ptr+=2)
	{
		token = *((unsigned short *) ptr);

		switch ( token )
		{
			case 0x0006:	

				ref = (struct reference *) (ptr+2);

				data.lastVar = ref->ref;
				_setVar( &data,0 );

				ptr += sizeof(struct reference ) + ref -> length;
				n++;
				stack ++;
				break;

			case 0x005C:
				break;
		}

		ptr += 2;	 // next token
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );

	return ptr;
}

char *cmdBracket(struct nativeCommand *cmd, char *tokenBuffer )
{
	char *(*scmd )( struct glueCommands *data, int nextToken ) = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (cmdStack) scmd = cmdTmp[cmdStack-1].cmd ;

	if ( scmd == _procedure )
	{
		return read_kitty_args(tokenBuffer, &cmdTmp[cmdStack-1]);
	}

	return tokenBuffer;
}

char *cmdBracketEnd(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned int flags;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	while (cmdStack)
	{
		flags = cmdTmp[cmdStack-1].flag;

		if  ( flags & (cmd_loop | cmd_never | cmd_onEol | cmd_proc | cmd_normal)) break;
		cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack], 0);
	}

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _procAndArgs )
	{
		cmdTmp[cmdStack-1].tokenBuffer2 = tokenBuffer;
	}

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _endProc ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);

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

char *cmdGlobal(struct nativeCommand *cmd, char *tokenBuffer )
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

char *cmdParamStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	setStackStrDup(var_param_str ? var_param_str : "" );
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

	if (cmdStack)
	{
		while (cmdTmp[cmdStack-1].cmd != _procedure ) 
		{
			cmdStack--;
			if (cmdStack==0) break;
		}
	}

	return cmdEndProc( cmd, tokenBuffer );
}


extern char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ );

char *_cmdRestore( struct glueCommands *data, int nextToken )
{
	char *ptr = NULL;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	char *name = getStackString( stack );
	if (name)	ptr = findLabel(name);
	popStack( stack - data->stack  );

	if (ptr)
	{
		ptr = FinderTokenInBuffer( ptr-2, 0x0404 , -1, -1, _file_end_ );
		data_read_pointers[proc_stack_frame] = ptr;
	}
	else
	{
		if (name) 	printf("find name: '%s'\n",name);
		setError( 40, data->tokenBuffer );
	}

	return NULL;
}

char *cmdRestore(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned short next_token = NEXT_TOKEN(tokenBuffer);
	proc_names_printf("%s:%d\n", __FUNCTION__,__LINE__);

	if ((next_token == 0x0006 ) || (next_token == 0x0018))
	{
		struct reference *ref = (struct reference *) (tokenBuffer + 2);

		if (ref -> ref)
		{
			char *name;
			int idx = ref->ref-1;
			switch (globalVars[idx].var.type & 7 )
			{
				case type_int:
				case type_proc:
						if (name = dupRef( ref ))
						{
							char *ptr = findLabel(name);
							free(name);

							if (ptr) 
							{
								ptr = FinderTokenInBuffer( ptr-2, 0x0404 , -1, -1, _file_end_ );
								data_read_pointers[proc_stack_frame] = ptr;
							}
							else 	setError( 40, tokenBuffer );
						}
						return tokenBuffer + 2 + sizeof(struct reference) + ref -> length;
			}

			printf("type: %d\n",globalVars[idx].var.type & 7);
		}
	}

	// if we are here, then we did not use name of var as label name.
	stackCmdNormal( _cmdRestore, tokenBuffer );

	return tokenBuffer;
}

char *cmdData(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned short jump = *((unsigned short *) tokenBuffer) * 2;
	return tokenBuffer + jump - 4;	// next token (size 2) + data (size 2) will be added on exit.
}

/*
char *cmdOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	int num = 0;
	unsigned short ref_num = 0;
	unsigned short token = 0;
	unsigned short next_token = 0;
	unsigned int is_token = 0;
	char *ret = NULL;

	tokenBuffer += 4;	// skip crap, no idea what use this for... :-P

	if (NEXT_TOKEN(tokenBuffer) == 0x0006 )	// next is variable
	{
		struct reference *ref = (struct reference *) (tokenBuffer + 2);
		int idx = ref->ref-1;

		switch ( globalVars[idx].var.type )
		{
			case type_int:
				num = globalVars[idx].var.value;
				break;
		}

		tokenBuffer += (2 + ref -> length + sizeof(struct reference ));
		token = NEXT_TOKEN(tokenBuffer);

		if ( (token==0x02A8) || (token==0x02B2) || (token==0x0386) ) 
		{
			tokenBuffer += 2;	// we know this tokens..

			for(;;)
			{	
				next_token = NEXT_TOKEN(tokenBuffer);

				switch (next_token)
				{
					case 0x0006:

						is_token = 0x0006;
						tokenBuffer +=2;
						ref = (struct reference *) (tokenBuffer);
						num--;

						if (num == 0)
						{
							if (ref -> flags & type_proc )
							{
								ref_num = ref -> ref;
							}
							else
							{
								ref -> flags |= type_proc;
								ref -> ref = var_find_proc_ref( ref );
								ref_num = ref -> ref;
							}

							if (ref_num == 0) setError(22,tokenBuffer);
						}

						tokenBuffer += sizeof(struct reference) + ref -> length;
						break;

					case 0x0012:

						is_token = 0x0012;
						tokenBuffer +=2;
						ref = (struct reference *) (tokenBuffer);
						num--;

						if (num == 0)
						{
							if (ref -> flags & type_proc )
							{
								ref_num = ref -> ref;
							}
							else
							{
								ref -> ref = var_find_proc_ref( ref );
								ref_num = ref -> ref;
							}

							if (ref_num == 0) setError(22,tokenBuffer);
						}

						tokenBuffer += sizeof(struct reference) + ref -> length;
						break;

					case 0x0018:
						is_token = 0x0018;
						tokenBuffer +=2;
						ref = (struct reference *) (tokenBuffer);
						num--;

						if (ref->ref == -1) // Because line start with 0x0018 is labels.
						{
							char *name = dupRef( ref ) ;
							if (name)
							{
								ref->ref = findLabelRef( name );	free(name); 
							}
						}

						if (num == 0)	ref_num = ref -> ref;
						if (ref_num == 0) setError(22,tokenBuffer);

						tokenBuffer += sizeof(struct reference) + ref -> length;
						break;

					case 0x001E:
					case 0x0036:
					case 0x003E:
						is_token = 0x003E;
						tokenBuffer +=2;

						num--;
						if (num == 0)
						{
							char num[50];
							sprintf(num,"%d", *((int *) tokenBuffer));
							ref_num = findLabelRef( num );
						}

						tokenBuffer += 4;
						break;

					case 0x005C:
						tokenBuffer +=2;
						break;

					default: 

						goto exit_on_for_loop;
				}
			}

exit_on_for_loop:

			if (ref_num>0)
			{
				switch (token)
				{
					case 0x02A8:	// goto
							ret = findLabel(globalVars[ref_num-1].varName);
							break;

					case 0x02B2:	// gosub

							// we don't point at data so we -2 and point at token.

							switch (is_token)
							{
								case 0x0006:
										stackCmdLoop( _gosub_return, tokenBuffer-2 );
										ret = findLabel(globalVars[ref_num-1].varName);
										break;
								case 0x0018:
										stackCmdLoop( _gosub_return, tokenBuffer-2 );
										ret = labels[ref_num-1].tokenLocation;
										break;
								case 0x003E:
										stackCmdLoop( _gosub_return, tokenBuffer-2 );
										ret = labels[ref_num-1].tokenLocation;
										break;
							}
							break;

					case 0x0386:	// proc

							stackCmdProc( _procedure, tokenBuffer);  
							ret = globalVars[ref_num-1].var.tokenBufferPos;
							break;
				}
			}
		}
	}

	if (ret) tokenBuffer = ret - 6;	// +2 token +data 4 on exit.

	return tokenBuffer;
}
*/

char *_cmdExit(struct glueCommands *data, int nextToken )
{
	int args = stack - data -> stack +1;
	int exit_loops = 1;
	unsigned short token;
	char *ptr;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1) exit_loops = getStackNum(stack);
	popStack( stack - data -> stack  );

	while (exit_loops>1)
	{
		if (dropProgStackToFlag( cmd_loop )) cmdStack--;
		exit_loops--;
	}

	if (dropProgStackToFlag( cmd_loop ))
	{
		ptr = cmdTmp[cmdStack-1].tokenBuffer;
		token = *((unsigned short *) (ptr - 2)) ;

		switch (token)
		{
			case 0x023C:	// For
			case 0x0250:	// Repeat
			case 0x0268:	// While
			case 0x027E:	// DO

				cmdTmp[cmdStack-1].cmd = _exit;
				cmdTmp[cmdStack-1].flag = cmd_never ;
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
	int args = stack - data -> stack +1;

	switch (args)
	{
		case 1:
			is_true = getStackNum(stack);
			break;
		case 2:
			is_true = getStackNum(stack -1);
			exit_loops = getStackNum(stack);
			break;
	}

	popStack( stack - data -> stack  );

	if (is_true == false) return NULL;

	while (exit_loops>1)
	{
		if (dropProgStackToFlag( cmd_loop )) cmdStack--;
		exit_loops--;
	}

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	dump_prog_stack();

	if (dropProgStackToFlag( cmd_loop ))
	{
		ptr = cmdTmp[cmdStack-1].tokenBuffer;
		token = *((unsigned short *) (ptr - 2)) ;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		switch (token)
		{
			case 0x023C:	// For
			case 0x0250:	// Repeat
			case 0x0268:	// While
			case 0x027E:	// DO

				cmdTmp[cmdStack-1].cmd = _exit;
				cmdTmp[cmdStack-1].flag = cmd_never ;
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
	int args = stack - cmdTmp[cmdStack-1].stack +1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
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
							on_every_gosub_location = findLabel(name);
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

char *_cmdWait( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;

	Delay( getStackNum(data->stack) / 2 );

	return  NULL ;
}

char *cmdWait(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdWait, tokenBuffer );

	return tokenBuffer;
}


char *_set_timer( struct glueCommands *data, int nextToken )
{
	timer_offset = getStackNum( stack );
	gettimeofday(&timer_before, NULL);	// reset diff.
	_do_set = _setVar;
	return NULL;
}


char *cmdTimer(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned int ms_before;
	unsigned int ms_after;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( ((last_tokens[parenthesis_count] == 0x0000) || (last_tokens[parenthesis_count] == 0x0054)) && (NEXT_TOKEN(tokenBuffer) == 0xFFA2 ))
	{
		tokenMode = mode_store;
		_do_set = _set_timer;
	}

	gettimeofday(&timer_after, NULL);	// reset diff.

	ms_before = (timer_before.tv_sec * 1000) + (timer_before.tv_usec/1000);
	ms_after = (timer_after.tv_sec * 1000) + (timer_after.tv_usec/1000);

	setStackNum( ((ms_after - ms_before) / 20) + timer_offset );		// 1/50 sec = every 20 ms

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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdAmosToFront(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *_cmdNot( struct glueCommands *data, int nextToken )
{
	int res;
	int args = stack - data->stack +1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	res = getStackNum( stack );
	popStack( stack - cmdTmp[cmdStack-1].stack  );
	setStackNum(~res);

	return  NULL ;
}

char *cmdNot(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( last_tokens[parenthesis_count] == 0x02BE )
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
	popStack( stack - data->stack  );
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
	popStack( stack - data -> stack );
	return NULL;
}

char *cmdExtension( struct nativeCommand *cmd, char *tokenBuffer )
{
	struct extension *ext = (struct extension *) tokenBuffer;
	char *(*ext_cmd)( struct nativeCommand *cmd, char *tokenBuffer ) = NULL;
	struct extension_lib *_this = NULL;

	_this = &kitty_extensions[ ext-> ext ];
	if (_this) 
	{
		if (_this -> lookup)
		{
			ext_cmd = (char* (*)(nativeCommand*, char*)) *((void **) (kitty_extensions[ext-> ext].lookup + ext -> token));
		}
	}

	if (ext_cmd)
	{
		return ext_cmd(cmd, tokenBuffer);
	}
	else
	{
		printf("*** warning extensions not yet supported, extention %d, token %04x at line %d ****\n", ext-> ext, ext-> token,getLineFromPointer( tokenBuffer ));
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

char *cmdStop( struct nativeCommand *cmd, char *tokenBuffer )
{
	return tokenBuffer;
}

char *cmdCommandLineStr( struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackStrDup("");
	return tokenBuffer;
}

char *_cmdExec( struct glueCommands *data, int nextToken )
{
	int args = stack - data -> stack + 1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	char *cmd;

	switch (args)
	{
		case 1:	cmd = getStackString(stack);

#ifdef __amigaos4__
				if (cmd) SystemTags( cmd, 
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

char *_cmdCallEditor( struct glueCommands *data, int nextToken )
{
	int args = stack - data -> stack + 1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		default:
				setError(22,data->tokenBuffer);
	}

	return NULL;
}

char *cmdCallEditor(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdCallEditor, tokenBuffer );
}

