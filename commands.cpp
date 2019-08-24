
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
#include "label.h"
#include "amosString.h"

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

extern int bobDoUpdate ;
extern int bobUpdateNextWait ;

extern int findVarPublic( char *name, int type );
extern int ReferenceByteLength(char *ptr);

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
	popStack(stack - data->stack);
	return  data -> tokenBuffer ;
}


void stack_frame_up(int varIndex)
{
	struct kittyData *var = &globalVars[ varIndex ].var;

	proc_stack_frame++;		// move stack frame up.
	procStcakFrame[proc_stack_frame].id = globalVars[ varIndex ].proc;

	if ( var -> procDataPointer )
	{
		procStcakFrame[proc_stack_frame].dataPointer  = var -> procDataPointer;
	}
	else procStcakFrame[proc_stack_frame].dataPointer = procStcakFrame[proc_stack_frame-1].dataPointer;
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

					cmdTmp[cmdStack-1].stack = oldStack;	// carry stack.
					dprintf("Goto %08x -- line %d\n", globalVars[idx].var.tokenBufferPos, getLineFromPointer(globalVars[idx].var.tokenBufferPos ) );

					tokenMode = mode_store;
					stack_frame_up( idx);
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
		setError(22,data -> tokenBuffer);
	}

	if (kittyStack[stack].integer.value == 0)	// 0 is FALSE always -1 or 1 can be TRUE
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

	if (kittyStack[data->stack].integer.value == 0)
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
	if (kittyStack[stack].integer.value == 0) return data -> tokenBuffer;
	return 0;
}

BOOL setVarInt( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_int:
			var->integer.value = kittyStack[stack].integer.value;
			return TRUE;

		case type_float:
			var->integer.value = (int) kittyStack[stack].decimal.value;
			return TRUE;
	}

	return FALSE;
}

BOOL setVarDecimal( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_int:
			var->decimal.value =  (double) kittyStack[stack].integer.value;
			return TRUE;

		case type_float:
			var->decimal.value =  kittyStack[stack].decimal.value;
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
				if (var->str) free(var->str);
				var->str = amos_strdup(s -> str);
			}
			else
			{
				if (var->str)
				{
					var->str->ptr = 0;
					var->str->size = 0;
				}

				getchar();
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
				(&(var->int_array -> ptr) + var -> index) -> value = kittyStack[stack].integer.value;
				return TRUE;

			case type_float:
				(&(var->int_array -> ptr) + var -> index) -> value = (int) kittyStack[stack].decimal.value;
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
				(&(var->float_array -> ptr) + var -> index) -> value = (double) kittyStack[stack].integer.value;
				return TRUE;

			case type_float:
				(&(var->float_array -> ptr) + var -> index) -> value = kittyStack[stack].decimal.value;
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
				{
					struct stringData **str_item = &(var->str_array -> ptr) + var -> index;

					if (*str_item) free(*str_item);
					*str_item = amos_strdup(kittyStack[stack].str);	

				}
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
			success = setVarString( var , &kittyStack[stack] );
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

int parenthesis[MAX_PARENTHESIS_COUNT];
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

	ret = flushCmdParaStack(0);
	if (ret) return ret;

	if (parenthesis_count)
	{
		remove_parenthesis( parenthesis[parenthesis_count -1] );
		parenthesis[parenthesis_count -1] = 255;
		do_input[parenthesis_count] = do_std_next_arg;
		parenthesis_count--;

		if (cmdStack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack], nextToken);
	}

	lastToken = getLastProgStackToken();

	if ( correct_order( lastToken ,  nextToken ) == false )
	{
		// hidden ( condition.
		setStackHiddenCondition();
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

	token_is_fresh = false;

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

	token_is_fresh = true;

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
			cmdTmp[cmdStack-1].flag = cmd_onEol | cmd_true;			// should run at end of line
		}
		else	if (cmdTmp[cmdStack-1].cmd == _ifNotSuccess) 
		{
			cmdTmp[cmdStack-1].cmd = _ifThenNotSuccess;
			cmdTmp[cmdStack-1].flag = cmd_onEol | cmd_false;			// should run at end of line
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

	token_is_fresh = true;

	char *retTokenBuffer = nextCmd(NULL, tokenBuffer);
	if (retTokenBuffer != tokenBuffer) tokenBuffer = retTokenBuffer + 2;	// nextCmd() should expect +2 token

	if (cmdStack)
	{
		if (cmdTmp[cmdStack-1].flag & cmd_true ) 		// if success jump over else
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
		if ( cmdTmp[cmdStack-1].flag & (cmd_true | cmd_false) )
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
					sprintf(num,"%d", kittyStack[stack].integer.value );
					ref_num = findLabelRef( num , procStcakFrame[proc_stack_frame].id  );
				}
				break;

		case type_string:
				{
					struct stringData *str = getStackString( stack );
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
	struct label *label;

	if (args != 1) setError(22,data -> tokenBuffer);

	switch (kittyStack[stack].type)
	{
		case type_int:	
				{
					char num[50];
					sprintf(num,"%d", kittyStack[stack].integer.value );
					ref_num = findLabelRef( num, procStcakFrame[proc_stack_frame].id );
				}
				break;

		case type_string:
				{
					struct stringData *str = getStackString( stack );
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
			cmdTmp[cmdStack-1].flag = cmd_normal | cmd_onNextCmd | cmd_onEol;
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
	kittyData *var = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (NEXT_TOKEN( (tokenBuffer+2) ) == 0x0006 )	// Next var
	{
		struct reference *ref = (struct reference *) (tokenBuffer + 4);
		var = &globalVars[ref -> ref -1].var;

		stackCmdNormal( _for, tokenBuffer );

		switch (var -> type)
		{
			case type_int:
					{
						cmdTmp[cmdStack-1].optionsType = glue_option_for_int;
						cmdTmp[cmdStack-1].optionsInt.step = 1.0;
					}
					break;
			case type_float:
					{
						cmdTmp[cmdStack-1].optionsType = glue_option_for_float;
						cmdTmp[cmdStack-1].optionsFloat.step = 1.0f;
					}
					break;
		}
	}

	do_to[parenthesis_count] = do_for_to;

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
			cmdTmp[cmdStack-1].FOR_NUM_TOKENBUFFER = tokenBuffer ;
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	struct glueCommands *gcmd = &cmdTmp[cmdStack-1];
	
	if (( gcmd -> cmd == _for ) && ( gcmd -> flag & cmd_loop ))
	{
		struct kittyData *var = &kittyStack[stack];
		switch (gcmd -> optionsType)
		{
			case glue_option_for_int:
					gcmd -> optionsInt.step = (var -> type == type_int) ? var -> integer.value :  (int) var -> decimal.value ;
					dprintf("step (int %d)\n", gcmd -> optionsInt.step);
					break;
			case glue_option_for_float:
					gcmd -> optionsFloat.step = (var -> type == type_int) ? (double) var -> integer.value : var -> decimal.value ;
					dprintf("step (float %lf)\n", gcmd -> optionsFloat.step);
					break;
		}
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

extern char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ );


void FOR_NEXT_VALUE_ON_STACK( char *tokenBuffer , char **new_ptr )
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

		token = *( (short *) ptr);
		ptr += 2;
	};

	if (token == 0x0356 )	// skip Step...
	{
		ptr = FinderTokenInBuffer( ptr-2, token , 0x0000, 0x0054, _file_end_ )+2;
	}

	*new_ptr = ptr - 2;

	// forcefully flush all cmds

	while ( cmdStack > cmdStack_start ) 
	{
		cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack], 0);
	}
}


char *cmdNext(struct nativeCommand *cmd, char *tokenBuffer )
{
	char *ptr = tokenBuffer ;
	char *new_ptr = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( cmdTmp[cmdStack-1].cmd == _for )
	{
		kittyData *var = NULL;

		if (NEXT_TOKEN(ptr) == 0x0006 )	// Next var
		{
			struct reference *ref = (struct reference *) (ptr + 2);
			var = &globalVars[ref -> ref -1].var;
		}
		else 	// For var=
		{
			if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag == cmd_loop ))
			{
				char *ptr = cmdTmp[cmdStack-1].tokenBuffer + 2  ;	// first short is JMP address, next after is token.
	
				if (NEXT_TOKEN(ptr) == 0x0006 )	// next is variable
				{
					struct reference *ref = (struct reference *) (ptr + 2);
					var = &globalVars[ref -> ref -1].var;
				}
			}
			else setError( 22,  tokenBuffer );
		}
		
		if (var)
		{
			int next_num=0;
			double next_float=0.0;
			struct glueCommands *gcmd;

			gcmd = &cmdTmp[cmdStack-1];

			ptr = cmdTmp[cmdStack-1].FOR_NUM_TOKENBUFFER;

			FOR_NEXT_VALUE_ON_STACK(ptr, &new_ptr);

			switch ( var -> type )
			{
				case type_int:	

						switch ( kittyStack[stack].type )
						{
							case type_int:		next_num = kittyStack[stack].integer.value;	break;
							case type_float:	next_num = (int) kittyStack[stack].decimal.value;	break;
						}
						break;

				case type_float:

						switch ( kittyStack[stack].type )
						{
							case type_int:		next_float = (double) kittyStack[stack].integer.value;	break;
							case type_float:	next_float = kittyStack[stack].decimal.value;	break;
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
						} else cmdStack--;
					} else if (gcmd->optionsInt.step < 0) {
						if (var -> integer.value >= next_num  )	{
							tokenBuffer = new_ptr;
						} else cmdStack--;
					} else setError(23,tokenBuffer);
					break;

				case glue_option_for_float:

					var -> decimal.value += gcmd->optionsFloat.step; 

					if (gcmd->optionsFloat.step > 0.0)  {

						printf("%0.2lf + %0.2lf <= %0.2lf \n",var -> decimal.value, gcmd->optionsFloat.step, next_float );

						if (var -> decimal.value <= next_float )	{
							tokenBuffer = new_ptr;
						} else cmdStack--;
					} else if (gcmd->optionsFloat.step < 0.0) {
						if (var -> decimal.value >= next_float  )	{
							tokenBuffer = new_ptr;
						} else cmdStack--;
					} else setError(23,tokenBuffer);
					break;
			}

		}
		else
		{
			setError(22,tokenBuffer);	
		}
	}
	else	if (cmdTmp[cmdStack-1].cmd == _exit )
	{
		cmdStack--;
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
	tokenMode = mode_logical;					// parmiters should be handled logicaly, not as store.

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
			var_param_num = kittyStack[stack].integer.value;
			break;
		case type_float:
			var_param_decimal = kittyStack[stack].decimal.value;
			break;
		case type_string:
			if (var_param_str) free(var_param_str);
			var_param_str = amos_strdup(kittyStack[stack].str);
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
		if (cmdTmp[cmdStack-1].cmd == _procedure )
		{
			if (proc_stack_frame)
			{
				if (NEXT_TOKEN(tokenBuffer) == 0x0084 )	//  End Proc[ return value ]
				{
					if (cmdTmp[cmdStack-1].cmd == _procedure ) cmdTmp[cmdStack-1].cmd = _endProc;
					do_input[parenthesis_count] = _set_return_param;
				}
				else 	// End Proc
				{
					tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
					proc_stack_frame--;		// move stack frame down.
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

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	args = stack - read_stack +1;

	// the idea, stack to be read is stored first,

	stack ++;					// prevent read stack form being trached.

	token = *((unsigned short *) ptr);

	printf("token %04x\n", token);

	for (ptr = tokenBuffer; (token != 0x0000) && (token != 0x0054) && (read_args<=args) ;)
	{
		if (token == end_token)
		{
			// save stack
//			int tmp_stack = stack;
			stack = read_stack;

			// set var
			data.lastVar = last_var;
			_setVar( &data,0 );
			break;
		}

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

	popStack( stack - read_stack );
	return ptr;
}

char *cmdBracket(struct nativeCommand *cmd, char *tokenBuffer )
{
	char *(*scmd )( struct glueCommands *data, int nextToken ) = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (cmdStack) scmd = cmdTmp[cmdStack-1].cmd ;

	if ( scmd == _procedure )
	{
		return read_kitty_args(tokenBuffer, cmdTmp[cmdStack-1].stack, 0x0000 );
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
						mainVar = &globalVars[mainVar_ref-1].var;
						localVar = &globalVars[localVar_ref-1].var;

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

	struct stringData *name = getStackString( stack );
	if (name)	
	{
		struct label *label = findLabel( &name -> ptr, procStcakFrame[proc_stack_frame].id );
		ptr = label ? label -> tokenLocation : NULL;
	}
	popStack( stack - data->stack  );

	if (ptr)
	{
		ptr = FinderTokenInBuffer( ptr-2, 0x0404 , -1, -1, _file_end_ );
		procStcakFrame[proc_stack_frame].dataPointer = ptr;
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
							struct label *label = findLabel(name, procStcakFrame[proc_stack_frame].id);
							char *ptr = label -> tokenLocation;
							free(name);

							if (ptr) 
							{
								ptr = FinderTokenInBuffer( ptr-2, 0x0404 , -1, -1, _file_end_ );
								procStcakFrame[proc_stack_frame].dataPointer = ptr;
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

char *_cmdWait( struct glueCommands *data, int nextToken )
{
	engine_lock();
	if (bobUpdateNextWait)
	{
		bobDoUpdate = 1;
		bobUpdateNextWait = 0;
	}
	engine_unlock();

	Delay( getStackNum(stack) / 2 );
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

	if ( (token_is_fresh) && (NEXT_TOKEN(tokenBuffer) == 0xFFA2 ))
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	res = getStackNum( stack );
	popStack( stack - cmdTmp[cmdStack-1].stack  );
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

char *cmdWaitKey(struct nativeCommand *cmd, char *tokenBuffer );

char *cmdStop( struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("** command stop **\n");
	dump_prog_stack();
	dump_stack();
	dump_global();
	dump_680x0_regs();
	dumpScreenInfo();
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
	int args = stack - data -> stack + 1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	struct stringData *cmd;

	switch (args)
	{
		case 1:	cmd = getStackString(stack);

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

