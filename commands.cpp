
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "debug.h"
#include <string>
#include <proto/dos.h>
#include <vector>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "var_helper.h"
#include "errors.h"
#include "engine.h"

bool every_on = true;
int every_timer = 0;
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
extern unsigned short last_token;
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
char *_for( struct glueCommands *data, int nextToken )
{
	return NULL;
}

// dummy not used, we need to know what to do on "else if" and "else", "If" and "else if" does not know where "end if" is.

char *_ifSuccess( struct glueCommands *data, int nextToken ) 
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	setError(22);	// shoud not be executed
	return NULL;
}

char *_ifNotSuccess( struct glueCommands *data, int nextToken ) 
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	setError(22);	// shoud not be executed
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
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return  data -> tokenBuffer ;
}

char *_endProc( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

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

	return  data -> tokenBuffer ;
}

char *_procAndArgs( struct glueCommands *data, int nextToken )
{
	int oldStack;
	struct reference *ref = (struct reference *) (data->tokenBuffer);

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (ref -> ref)
	{
		int idx = ref->ref-1;

		printf("idx %d\n",idx);

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
					return globalVars[idx].var.tokenBufferPos   ;
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
					return globalVars[idx].var.tokenBufferPos   ;
			}
		}
	}

	setError(22);

	return  NULL ;
}


char *_if( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	unsigned short token;
	char *ptr;
	int args = stack - data -> stack + 1;

	if (args > 1) 
	{
		dump_stack();
		dump_prog_stack();
		setError(22);
	}

	if (kittyStack[data->stack].value == 0)	// 0 is FALSE always -1 or 1 can be TRUE
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

char *_else_if( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	unsigned short token;
	char *ptr;

	if (kittyStack[data->stack].value == 0)	// 0 is FALSE always -1 or 1 can be TRUE
	{
		int offset = *((unsigned short *) data -> tokenBuffer);

		if (offset) 
		{
			ptr = data->tokenBuffer+(offset*2) ;
			return ptr ;
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
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	return data -> tokenBuffer-2;
}

char *_repeat( struct glueCommands *data, int nextToken )
{
	if (kittyStack[stack].value == 0) return data -> tokenBuffer-2;
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

BOOL setVarIntArray( struct kittyData *var )
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
	else setError(25);

	return FALSE;
}

BOOL setVarDecimalArray( struct kittyData *var )
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
	else setError(25);

	return FALSE;
}

BOOL setVarStringArray( struct kittyData *var )
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
	else setError(25);

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
			success = setVarIntArray( var );
			break;
		case type_float | type_array:
			success = setVarDecimalArray( var );
			break;
		case type_string | type_array:
			success = setVarStringArray( var );
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
			setError(ERROR_Type_mismatch);
		}

		if (var -> type & type_array)
		{
			if (var -> count == 0) setError(27);
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

char *nextArg(struct nativeCommand *cmd, char *tokenBuffer)
{
	flushCmdParaStack(0);
	
	if (do_input) do_input( cmd, tokenBuffer );	// read from keyboad or disk.

	stack++;
	kittyStack[stack].type = type_none; 

	return tokenBuffer;
}

int parenthesis[100];
extern int parenthesis_count ;

char *parenthesisStart(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	parenthesis[parenthesis_count] =stack;
	parenthesis_count++;

	setParenthesis();
	stack++;

	return tokenBuffer;
}

extern int last_var;

char *parenthesisEnd(struct nativeCommand *cmd, char *tokenBuffer)
{
	char *ret;
	int nextToken = *((unsigned short *) tokenBuffer);

	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	_file_pos_ = tokenBuffer;		// needed by "Fn", need to return End Bracket after Fn call.

	ret = flushCmdParaStack(0);
	if (ret) return ret;

	if (parenthesis_count)
	{
		remove_parenthesis( parenthesis[parenthesis_count -1] );
		parenthesis[parenthesis_count -1] = 255;
		parenthesis_count--;

		if (cmdStack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack], nextToken);
	}

	ret = flushCmdParaStack(nextToken);
	if (ret) return ret;

	return tokenBuffer;
}

char *breakData(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
	if (do_breakdata) do_breakdata( cmd, tokenBuffer );
	return tokenBuffer;
}

char *setVar(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack], 0);

	if (tokenMode == mode_logical)
	{
		stackCmdParm(_equalData, tokenBuffer);
		stack++;
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
	void *fn;
	char *ret = NULL;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	
	// empty the stack for what ever is inside the IF.

	while ((cmdStack)&&(stack))
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
			cmdTmp[cmdStack-1].flag = cmd_eol;			// should run at end of line
		}
		else	if (cmdTmp[cmdStack-1].cmd == _ifNotSuccess) 
		{
			cmdTmp[cmdStack-1].cmd = _ifThenNotSuccess;
			cmdTmp[cmdStack-1].flag = cmd_eol;			// should run at end of line
		}
	}

	if (ret) tokenBuffer = ret;
	tokenMode = mode_standard;

	return tokenBuffer;
}

char *cmdElse(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d - line %d\n",__FUNCTION__,__LINE__, getLineFromPointer( tokenBuffer ));

	if (cmdStack)
	{
		if ((cmdTmp[cmdStack-1].cmd == _ifSuccess) || (cmdTmp[cmdStack-1].cmd == _ifThenSuccess)) 		// if success jump over else
		{
			char *ptr;
			int offset = *((unsigned short *) tokenBuffer);

			if (offset) 
			{
				ptr = tokenBuffer+(offset*2) -2;
				return ptr;
			}
		}
	}

	return tokenBuffer;
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
				ptr = tokenBuffer+(offset*2) -2;
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
	}

	return tokenBuffer;
}

char *_goto( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return NULL ;
}


char *cmdGoto(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);
	char *ptr;
	char *return_tokenBuffer;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

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
								return_tokenBuffer = tokenBuffer + 2 + ReferenceByteLength(tokenBuffer + 2) + sizeof(struct reference ) ;
								tokenBuffer = var_JumpToName( (struct reference *) (tokenBuffer+2) );
								break;

						case type_string:	// jump to string.
								printf("%s:%d\n",__FUNCTION__,__LINE__);
								stackCmdNormal( _goto, tokenBuffer );
								break;

						case type_float:

								printf("%s:%d\n",__FUNCTION__,__LINE__);
								setError(22);
								break;
					}
				
					stackCmdNormal( _gosub, tokenBuffer );
					break;

		default:

				printf("bad token: %04x\n", next_token);
				setError(22);
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

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args != 1) setError(22);

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
				setError(22);
	}

	if (ref_num)
	{
		char *return_tokenBuffer = data -> tokenBuffer;

		printf("jump to %08x\n",labels[ref_num-1].tokenLocation);

		while ( *((unsigned short *) return_tokenBuffer) != nextToken  ) return_tokenBuffer += 2;
		stackCmdLoop( _gosub_return, return_tokenBuffer );
		return labels[ref_num-1].tokenLocation;
	}
	else
	{
		dump_stack();
		setError(22);
	}

	return NULL ;
}




char *cmdGosub(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);
	char *ptr;
	char *return_tokenBuffer;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (next_token)
	{
		case 0x0026:	// text

					stackCmdNormal( _gosub, tokenBuffer );
					break;

		case 0x0074:  // ( data )
					stackCmdNormal( _gosub,tokenBuffer );
					break;

		case 0x0018:	// label
		case 0x0006: 	// variable

					switch ( var_type_is( (struct reference *) (tokenBuffer+2), 0x7 ))
					{
						case type_int:		// jump to label with same name as var.

								return_tokenBuffer = tokenBuffer + 2 + ReferenceByteLength(tokenBuffer + 2) + sizeof(struct reference ) ;
								tokenBuffer = var_JumpToName( (struct reference *) (tokenBuffer+2) );
								if (tokenBuffer) stackCmdLoop( _gosub_return, return_tokenBuffer );

								break;

						case type_string:	// jump to string.

								printf("%s:%d\n",__FUNCTION__,__LINE__);
								stackCmdNormal( _gosub, tokenBuffer );
								break;

						case type_float:

								setError(22);
								break;
					}
				
					break;

		default:

				printf("bad token: %04x\n", next_token);
				setError(22);
	}

	return tokenBuffer;
}

char *cmdDo(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	stackCmdLoop( _do, tokenBuffer );
	return tokenBuffer;
}

char *cmdRepeat(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	stackCmdLoop( _repeat, tokenBuffer );
	return tokenBuffer;
}

char *cmdLoop(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _do ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);

	return tokenBuffer;
}

char *cmdWhile(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	tokenMode = mode_logical;
	stackCmdLoop( _while, tokenBuffer );
	stackCmdNormal( _whileCheck, tokenBuffer );
	return tokenBuffer;
}

char *cmdWend(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _while ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
	return tokenBuffer;
}

char *cmdUntil(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	// we are changin the stack from loop to normal, so when get to end of line or next command, it be executed after the logical tests.

	tokenMode = mode_logical;
	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _repeat ) cmdTmp[cmdStack-1].flag = cmd_first;
	return tokenBuffer;
}

char *cmdTrue(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	setStackNum(-1);
	flushCmdParaStack(NEXT_TOKEN(tokenBuffer));
	return tokenBuffer;
}

char *cmdFalse(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	setStackNum(0);
	flushCmdParaStack(NEXT_TOKEN(tokenBuffer));
	return tokenBuffer;
}


void do_for_to( struct nativeCommand *cmd, char *tokenBuffer);

char *cmdFor(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	stackCmdNormal( _for, tokenBuffer );
	cmdTmp[cmdStack-1].step = 1;		// set default counter step
	do_to = do_for_to;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	return tokenBuffer;
}


void do_for_to( struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);

	if (cmdStack) if ( cmdTmp[cmdStack-1].cmd == _setVar ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);

	if (cmdStack) 
	{
		// We loop back to "TO" not "FOR", we are not reseting COUNTER var.

		if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag == cmd_first ))
		{
			cmdTmp[cmdStack-1].tokenBuffer2 = tokenBuffer ;
			cmdTmp[cmdStack-1].cmd_type = cmd_loop;
			popStack( stack - cmdTmp[cmdStack-1].stack );
		}
	}
}

void do_to_default( struct nativeCommand *, char * )
{
	stack ++;
}

char *cmdTo(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	if (do_to) do_to( cmd, tokenBuffer );	
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
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

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

		last_token = token;
		token = *( (short *) ptr);
		ptr += 2;
	};

	*new_ptr = ptr - 2;

	return getStackNum(stack);
}

char *cmdNext(struct nativeCommand *cmd, char *tokenBuffer )
{
	char *ptr = tokenBuffer ;
	char *new_ptr = NULL;
	int idx_var = -1;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

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
		if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag == cmd_loop ))
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
		}
	}

	return tokenBuffer;
}

char *cmdEnd(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	return NULL;
}

char *cmdReturn(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _gosub_return ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
	return tokenBuffer;
}

char *cmdProcedure(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	struct procedure *proc = (struct procedure *) tokenBuffer;

	proc_names_printf("Goto %08x -- line %d\n",proc -> EndOfProc, getLineFromPointer(proc -> EndOfProc ));

	return proc -> EndOfProc - sizeof(struct procedure);
}

char *cmdProcAndArgs(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	struct reference *ref = (struct reference *) (tokenBuffer);

	stackCmdNormal( _procAndArgs, tokenBuffer );
	tokenBuffer += ref -> length ;

	return tokenBuffer;
}

char *cmdProc(struct nativeCommand *cmd, char *tokenBuffer )
{
// this is dummy does not do anything, silly thing 
	return tokenBuffer;
}

char *cmdEndProc(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack)
	{
		proc_names_printf("%04x\n",NEXT_TOKEN(tokenBuffer));

		if (NEXT_TOKEN(tokenBuffer) == 0x0084 )	//  End Proc[ return value ]
		{
			proc_names_printf("yes we are here\n");

			// changes function pointer only so that ']' don't think its end of proc by accident.
			// we aslo push result of stack into Param.
			if (cmdTmp[cmdStack-1].cmd == _procedure ) cmdTmp[cmdStack-1].cmd = _endProc;
		}
		else 	// End Proc
		{
			if (cmdTmp[cmdStack-1].cmd == _procedure ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);
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
	int read_args = 0;
	unsigned short token;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	args = stack - sdata->stack +1;
	stack -= (args-1);	// move to start of args.
	token = *((unsigned short *) ptr);

	for (ptr = tokenBuffer; (token != 0x0000) && (token != 0x0054) && (read_args<args) ;)
	{
		ptr+=2;	// skip token
		ptr = executeToken( ptr, token );

		if ((token == 0x005C) || (token == 0x0054) || (token == 0x0000 ))
		{

			printf("--- last_var %d ---\n", last_var);
			dump_stack();
			getchar();
			read_args ++;
		}


		token = *((unsigned short *) ptr);
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

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

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

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) scmd = cmdTmp[cmdStack-1].cmd ;

	if ( scmd == _procedure )
	{
		return read_kitty_args(tokenBuffer, &cmdTmp[cmdStack-1]);
	}

	return tokenBuffer;
}

char *cmdBracketEnd(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _procAndArgs )
	{
		cmdTmp[cmdStack-1].tokenBuffer2 = tokenBuffer;
	}

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _endProc ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);

	return tokenBuffer;
}

char *cmdShared(struct nativeCommand *cmd, char *tokenBuffer )
{
	// we should not need to do anything here, but maybe good idea to jump over few tokens.
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
					setError(22);
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
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

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

char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ );

char *_cmdRead( struct glueCommands *data, int nextToken );

void read_from_data()
{
	unsigned short token;
	int _len;

		if (data_read_pointer)
		{
			bool try_next_token;
	
			do
			{
				try_next_token = false;
				token = *((short *) data_read_pointer);

				dprintf("data pointer %08x (line %d) - token %04x\n",data_read_pointer, getLineFromPointer(data_read_pointer), token);

				switch (token)
				{
					case 0x0404:	// data
							data_read_pointer+=4;	// token + data size 2
							try_next_token = true;
							break;

					case 0x003E: 	// num
							setStackNum ( *((int *) (data_read_pointer + 2)) );
							data_read_pointer +=6;
							break;
	
					case 0x0026:	// string
							_len = *((unsigned short *) (data_read_pointer + 2));
							_len = _len + (_len & 1);
							if (kittyStack[stack].str) free(kittyStack[stack].str);
							kittyStack[stack].str = strndup( data_read_pointer + 4, _len );
							kittyStack[stack].len = strlen( kittyStack[stack].str );
							data_read_pointer +=4 + _len;
							break;

					case 0x000c:	// label
							{
								struct reference *ref = (struct reference *) (data_read_pointer + 2);
								data_read_pointer += 2 + sizeof(struct reference) + ref -> length;
							}
							try_next_token = true;
							break;

					case 0x005C:	// comma
							data_read_pointer +=2;
							try_next_token = true;
							break;

					case 0x0000:	// new line
							data_read_pointer +=4;
							try_next_token = true;	
							break;
					default:
							data_read_pointer = FinderTokenInBuffer( data_read_pointer, 0x0404 , -1, -1, _file_end_ );
							try_next_token = true;	
				}

				if (data_read_pointer == 0x0000) break;
			} while ( try_next_token );
		}
}

void _read_arg( struct nativeCommand *cmd, char *tokenBuffer )
{
	int args = 0;
	int num = 0;
	double des = -1.0f;
	struct glueCommands data;

	if (cmd == NULL)
	{
		args = stack - cmdTmp[cmdStack].stack + 1;
	}
	else
	{
		if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _cmdRead)
		{
			args = stack - cmdTmp[cmdStack-1].stack + 1;
		}
	}
	
	if (last_var)
	{
		_set_var_index = globalVars[last_var -1].var.index;
		read_from_data();
	}

	data.lastVar = last_var;
	_setVar( &data,0 );
}

char *_cmdRead( struct glueCommands *data, int nextToken )
{
	int args = stack - data -> stack +1;
	_read_arg( NULL, NULL );
	popStack( stack - data -> stack  );
	do_input = NULL;
	do_breakdata = NULL;
	return NULL;
}

char *cmdRead(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	do_input = _read_arg;
	do_breakdata = NULL;
	stackCmdNormal( _cmdRead, tokenBuffer );

	return tokenBuffer;
}

char *_cmdRestore( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	char *name = getStackString( stack );
	if (name)
	{
		char *ptr = findLabel(name);
		if (ptr) data_read_pointer = ptr;
	}

	popStack( stack - data->stack  );

	return NULL;
}

char *cmdRestore(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n", __FUNCTION__,__LINE__);

	if (NEXT_TOKEN(tokenBuffer) == 0x0006 )	// next is variable
	{
		struct reference *ref = (struct reference *) (tokenBuffer + 2);
		char *name = dupRef( ref );

		if (name)
		{
			char *ptr = findLabel(name);

			if (ptr) data_read_pointer = ptr;
			free(name);
		}
	}
	else
	{
		stackCmdNormal( _cmdRestore, tokenBuffer );
	}

	return tokenBuffer;
}

char *cmdData(struct nativeCommand *cmd, char *tokenBuffer )
{
	return tokenBuffer;
}


char *cmdOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	int num = 0;
	unsigned short ref_num = 0;
	unsigned short token = 0;
	unsigned short next_token = 0;
	unsigned int is_token = 0;

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

							if (ref_num == 0) setError(22);
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

							if (ref_num == 0) setError(22);
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
						if (ref_num == 0) setError(22);

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
							tokenBuffer = findLabel(globalVars[ref_num-1].varName);
							break;

					case 0x02B2:	// gosub

							switch (is_token)
							{
								case 0x0006:
										stackCmdLoop( _gosub_return, tokenBuffer );
										tokenBuffer = findLabel(globalVars[ref_num-1].varName);
										break;
								case 0x0018:
										stackCmdLoop( _gosub_return, tokenBuffer );
										tokenBuffer = labels[ref_num-1].tokenLocation;
										break;
								case 0x003E:
										stackCmdLoop( _gosub_return, tokenBuffer );
										tokenBuffer = labels[ref_num-1].tokenLocation;
										break;
							}
							break;

					case 0x0386:	// proc

							stackCmdProc( _procedure, tokenBuffer);  
							tokenBuffer = globalVars[ref_num-1].var.tokenBufferPos;
							break;
				}
			}
		}
	}

	tokenBuffer -= 4;	// yes right, 4 will be added by main program.

	return tokenBuffer;
}

char *_cmdExit(struct glueCommands *data, int nextToken )
{
	int args = stack - data -> stack +1;
	int exit_loops = 1;
	unsigned short token;
	char *ptr;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1) exit_loops = getStackNum(stack);
	popStack( stack - data -> stack  );

	while (exit_loops>1)
	{
		if (dropProgStackToType( cmd_loop )) cmdStack--;
		exit_loops--;
	}

	if (dropProgStackToType( cmd_loop ))
	{
		ptr = cmdTmp[cmdStack-1].tokenBuffer;
		token = *((unsigned short *) (ptr - 2)) ;

		switch (token)
		{
			case 0x023C:	// For
			case 0x0250:	// Repeat
			case 0x0268:	// While
			case 0x027E:	// DO

				cmdStack --;
				return ptr + ( *((unsigned short *) ptr) * 2 )-2;
				break;

			default:
				dump_prog_stack();
				proc_names_printf("token was %08x\n", token);
				getchar();
		}
	}

	return NULL;
}

char *cmdExit(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
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

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
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
		if (dropProgStackToType( cmd_loop )) cmdStack--;
		exit_loops--;
	}

	if (dropProgStackToType( cmd_loop ))
	{
		ptr = cmdTmp[cmdStack-1].tokenBuffer;
		token = *((unsigned short *) (ptr - 2)) ;

		switch (token)
		{
			case 0x023C:	// For
			case 0x0250:	// Repeat
			case 0x0268:	// While
			case 0x027E:	// DO

				cmdStack --;
				proc_names_printf("exit from loop %04x\n", token);
				ptr =  ptr + ( *((unsigned short *) ptr) * 2 )   ;
				return (ptr+2);
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
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdExitIf, tokenBuffer );
	tokenMode = mode_logical;

	return tokenBuffer;
}


char *_cmdEvery( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	return  data -> tokenBuffer ;
}

char *_cmdWait( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;

	Delay( getStackNum(data->stack) );

	return  data -> tokenBuffer ;
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
	printf("%s:%d\n",__FUNCTION__,__LINE__);

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

		printf("every timer: %d\n",every_timer);

	}

	gettimeofday(&every_before, NULL);	// reset diff.

	stackCmdNormal( _cmdEvery, tokenBuffer );

	return tokenBuffer;
}

char *cmdWait(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
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

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((last_token == 0x0000) && (NEXT_TOKEN(tokenBuffer) == 0xFFA2 ))
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
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdBreakOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdCloseWorkbench(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdCloseEditor(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdAmosToBack(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdAmosToFront(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *_cmdNot( struct glueCommands *data, int nextToken )
{
	int res;
	int args = stack - data->stack +1;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	res = getStackNum( stack );
	popStack( stack - cmdTmp[cmdStack-1].stack  );
	setStackNum(~res);

	return  NULL ;
}

char *cmdNot(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _cmdNot, tokenBuffer );
	return tokenBuffer;
}

char *_cmdSetBuffers( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	popStack( stack - data->stack  );
	return NULL;
}

char *cmdSetBuffers(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdSetBuffers, tokenBuffer );
	return tokenBuffer;
}

char *cmdMultiWait(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *cmdEdit(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return NULL;	// quit
}

char *cmdDirect(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return NULL;	// quit
}

char *cmdPop(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

