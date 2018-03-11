
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "debug.h"
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "errors.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

extern void setStackStr( char *str );
extern void setStackStrDup( const char *str );

using namespace std;

extern char *findLabel( char *name );


// dummy not used, see code in cmdNext
char *_for( struct glueCommands *data )
{
	return NULL;
}

char *_procedure( struct glueCommands *data )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	return  data -> tokenBuffer ;
}

char *_endProc( struct glueCommands *data )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

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

char *_procAndArgs( struct glueCommands *data )
{
	int oldStack;
	struct reference *ref = (struct reference *) (data->tokenBuffer);

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((ref -> ref) && (data -> tokenBuffer2))
	{
		int idx = ref->ref-1;

		switch (globalVars[idx].var.type & 7)
		{
			case type_proc:

				oldStack = data -> stack;

				printf("****\n");

				// I think we over write data here ;-)
				stackCmdLoop( _procedure, data -> tokenBuffer2);	//  data->tokenBuffer+sizeof(struct reference)+ref->length ) ;

				cmdTmp[cmdStack-1].stack = oldStack;	// carry stack.

				printf("Goto %08x\n", globalVars[idx].var.tokenBufferPos);

				tokenMode = mode_store;
				return globalVars[idx].var.tokenBufferPos  ;
		}
	}

	printf("ref->ref %d\n",ref->ref);

	getchar();

	return  data -> tokenBuffer ;
}

char *_gosub( struct glueCommands *data )
{
	char *ptr = data -> tokenBuffer ;
	ptr-=2;
	if (NEXT_TOKEN( ptr ) == 0x0006)
	{
		struct reference *ref = (struct reference *) (ptr + 2);
		ptr += (2 + sizeof(struct reference) + ref -> length) ;
	}

	return ptr ;
}

char *_step( struct glueCommands *data )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag == cmd_loop ))
	{
		cmdTmp[cmdStack-1].step = kittyStack[stack].value;
	}

	return NULL;
}

char *_input( struct glueCommands *data )
{
	int n;

	for (n=data->stack;n<=stack;n++)
	{
		switch (kittyStack[n].type)
		{
			case 0:
				printf("%d", kittyStack[n].value);
				break;
			case 1:
				printf("%f", kittyStack[n].decimal);
				break;
			case 2:
				if (kittyStack[n].str) printf("%s", kittyStack[n].str);
				break;

		}
	}
	return NULL;
}

char *_if( struct glueCommands *data )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (kittyStack[data->stack].value == 0)	// 0 is FALSE always -1 or 1 can be TRUE
	{
		int offset = *((unsigned short *) data -> tokenBuffer);

		if (offset) 
		{
			printf("IF is FALSE --  read from %08x jump to %08x - %04x\n" ,data->tokenBuffer ,data->tokenBuffer+(offset*2), offset);
			return data->tokenBuffer+(offset*2);
		}
	}
	return NULL;
}

char *_while( struct glueCommands *data )	// jumps back to the token.
{
	return data -> tokenBuffer-2;
}

char *_whileCheck( struct glueCommands *data )		
{
	int offset = 0;

	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

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
				printf("Jump over\n");

				return data->tokenBuffer+(offset*2);
			}
		}
	}
	return NULL;
}




char *_do( struct glueCommands *data )
{
	return data -> tokenBuffer-2;
}

char *_repeat( struct glueCommands *data )
{
	if (kittyStack[stack].value == 0) return data -> tokenBuffer-2;
	return 0;
}


char *cmdInput(nativeCommand *cmd, char *ptr)
{
	tokenMode = mode_input;
	stackCmdNormal( _input, ptr );
	return ptr;
}


char *_less( struct glueCommands *data )
{
	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			_num( item0->decimal < (double) item1->value );
		}
		else if (type1 == type_float)
		{
			_num( item0->decimal < item1->decimal );
		}
		return NULL;
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			_num( item0->value < item1->value );
		}
		else if (type1 == type_float)
		{
			_num( (double) item0->value < item1->decimal );
		}
		return NULL;
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		success = stackLessStr( item0, item1 ); 
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_more( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	dump_global();
	dump_stack();

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			_num( item0->decimal > (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			_num( item0->decimal > item1->decimal );
			success = TRUE;
		}

		correct_for_hidden_sub_data();
		return NULL;
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			_num( item0->value > item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			_num( (double) item0->value > item1->decimal );
			success = TRUE;
		}

		correct_for_hidden_sub_data();
		return NULL;
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		success = stackMoreStr( item0, item1 ); 
		correct_for_hidden_sub_data();
	}

	if (success == FALSE)
	{
		printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}


char *_equal( struct glueCommands *data )
{
	printf("%s\n",__FUNCTION__);

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	if (kittyStack[stack].type != kittyStack[stack+1].type)
	{
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	printf("(%d == %d) == %d \n", kittyStack[stack].value , kittyStack[stack+1].value, (kittyStack[stack].value == kittyStack[stack+1].value) );

	switch (kittyStack[stack].type & 3)
	{
		case 0:	_num( kittyStack[stack].value == kittyStack[stack+1].value) ;
				break;
		case 1:	_num (kittyStack[stack].decimal == kittyStack[stack+1].decimal);
				break;
//		case 2:	_equalStr( data );
//				break;
	}

	return NULL;
}

char *_not_equal( struct glueCommands *data )
{
	printf("%s\n",__FUNCTION__);

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	if (kittyStack[stack].type != kittyStack[stack+1].type)
	{
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	printf("(%d == %d) == %d \n", kittyStack[stack].value , kittyStack[stack+1].value, (kittyStack[stack].value != kittyStack[stack+1].value) );

	switch (kittyStack[stack].type & 3)
	{
		case 0:	_num( kittyStack[stack].value != kittyStack[stack+1].value) ;
				break;
		case 1:	_num (kittyStack[stack].decimal != kittyStack[stack+1].decimal);
				break;
//		case 2:	_not_equalStr( data );
//				break;
	}

	return NULL;
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
			var->str = strdup(kittyStack[stack].str);
			var->len = kittyStack[stack].len;
			return TRUE;
	}

	return FALSE;
}

BOOL setVarIntArray( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_int:
			var->int_array[var -> index] = kittyStack[stack].value;
			return TRUE;

		case type_float:
			var->int_array[var -> index] = (int) kittyStack[stack].decimal;
			return TRUE;
	}

	return FALSE;
}

BOOL setVarDecimalArray( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_int:
			var->float_array[var -> index] = (double) kittyStack[stack].value;
			return TRUE;

		case type_float:
			var->float_array[var -> index] = kittyStack[stack].decimal;
			return TRUE;
	}

	return FALSE;
}

BOOL setVarStringArray( struct kittyData *var )
{
	switch (kittyStack[stack].type)
	{
		case type_string:
			if (var->str_array[var -> index] ) free(var->str_array[var->index]);
			var->str_array[var -> index] = strdup(kittyStack[stack].str);	
			return TRUE;
	}

	return FALSE;
}

char *_setVar( struct glueCommands *data )
{
	BOOL success;
	struct kittyData *var;

	printf("%s:%d -- set var %d\n",__FUNCTION__,__LINE__, data -> lastVar-1);

	dump_prog_stack();

	dump_stack();

	printf("SET var %s \n",globalVars[ data->lastVar-1].varName);

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
		printf("kittyStack[%d].type= %d, (globalVars[%d].var.type & 7)=%d\n",
				stack, 
				kittyStack[stack].type, 
				data -> lastVar, 
				var -> type & 7);

		setError(ERROR_Type_mismatch);
	}

	return NULL;
}

char *_setVarReverse( struct glueCommands *data )
{
	printf("%20s:%08d data: lastVar %d\n",__FUNCTION__,__LINE__, data -> lastVar);

	// if variable is string it will have stored its data on the stack.

	if (kittyStack[stack].str) free(kittyStack[stack].str);
	kittyStack[stack].str = NULL;
	stack --;

	data->lastVar = last_var;	// we did know then, but now we know,
	return _setVar( data );
}

//--------------------------------------------------------

char *nextArg(struct nativeCommand *cmd, char *tokenBuffer)
{
	flushCmdParaStack();

	stack++;
	return tokenBuffer;
}

char *subCalc(struct nativeCommand *cmd, char *tokenBuffer)
{
	kittyStack[stack].str = NULL;
	kittyStack[stack].state = state_subData;

	stack++;

	if (kittyStack[stack].str) printf("%s::Unexpcted data %08x on new stack pos %d\n",__FUNCTION__, kittyStack[stack].str,stack);

	return tokenBuffer;
}

char *subCalcEnd(struct nativeCommand *cmd, char *tokenBuffer)
{
	flushCmdParaStack();
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	if (stack > 0)
	{
		if (kittyStack[stack-1].state == state_subData)
		{
			kittyStack[stack-1] = kittyStack[stack];
			kittyStack[stack].str = NULL;
			stack --;
			if (cmdStack) if (stack) if (kittyStack[stack-1].state == state_none) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
		}
	}

	return tokenBuffer;
}

void	input_mode( char *tokenBuffer )
{
	std::string input;
	int num;
	double des;

	if (NEXT_TOKEN(tokenBuffer) == 0x0006 )	// next is variable
	{
		struct reference *ref = (struct reference *) (tokenBuffer + 2);
		int idx = ref->ref-1;

		if (cmdStack) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
		getline(cin, input);

		switch ( globalVars[idx].var.type & 7 )
		{
			case type_int:
				sscanf( input.c_str(), "%d", &num );
				_num( num );
				break;

			case type_float:

				sscanf( input.c_str(), "%llf", &des );
				setStackDecimal( des );
				break;

			case type_string:
				setStackStrDup( input.c_str() );
				break;
		}

		stack++;
				 
		stackCmdParm( _setVarReverse, tokenBuffer );
	}
}

char *breakData(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (tokenMode)
	{
 		case mode_standard:
			stackCmdParm( _addData, tokenBuffer );
			stack++;
			break;

		case mode_input:
			input_mode( tokenBuffer );
			break;
	}

	return tokenBuffer;
}

char *cmdLess(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (tokenMode == mode_logical)
	{
		stackCmdParm(_less, tokenBuffer);
		stack++;
	}
	else
	{
		printf("Syntax error\n");
	}

	return tokenBuffer;
}

char *cmdMore(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (tokenMode == mode_logical)
	{
		stackCmdParm(_more, tokenBuffer);
		stack++;
	}
	else
	{
		printf("Syntax error\n");
	}

	return tokenBuffer;
}

char *cmdNotEqual(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	if (tokenMode == mode_logical)
	{
		stackCmdParm(_not_equal, tokenBuffer);
		stack++;
	}
	else
	{
		printf("Syntax error\n");
	}

	return tokenBuffer;
}

char *setVar(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	if (tokenMode == mode_logical)
	{
		stackCmdParm(_equal, tokenBuffer);
		stack++;
	}
	else
	{
		stackCmdNormal(_setVar, tokenBuffer);
		tokenMode = mode_logical;		// first equal is set, next equal is logical
	}

	return tokenBuffer;
}

char *cmdIf(struct nativeCommand *cmd, char *tokenBuffer)
{
	_num(0);	// stack reset.
	stackCmdNormal(_if, tokenBuffer);
	tokenMode = mode_logical;
	return tokenBuffer;
}

char *cmdThen(struct nativeCommand *cmd, char *tokenBuffer)
{
	void *fn;
	char *ret = NULL;

	// empty the stack for what ever is inside the IF.

	while ((cmdStack)&&(stack))
	{
		if (cmdTmp[cmdStack-1].cmd == _if ) break;
		cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	}

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _if ) ret=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	if (ret) tokenBuffer = ret;
	tokenMode = mode_standard;

	return tokenBuffer;
}

char *cmdElse(struct nativeCommand *cmd, char *tokenBuffer)
{
	int offset = *((unsigned short *)  tokenBuffer);
	if (offset) return tokenBuffer+(offset*2);
	return tokenBuffer;
}

char *cmdEndIf(struct nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}


char *cmdGoto(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	jump_mode = jump_mode_goto;
	return tokenBuffer;
}

char *cmdGosub(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	jump_mode = jump_mode_gosub;
	return tokenBuffer;
}

char *cmdDo(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	stackCmdLoop( _do, tokenBuffer );
	return tokenBuffer;
}

char *cmdRepeat(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	stackCmdLoop( _repeat, tokenBuffer );
	return tokenBuffer;
}

char *cmdLoop(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _do ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	return tokenBuffer;
}

char *cmdWhile(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	tokenMode = mode_logical;

	stackCmdLoop( _while, tokenBuffer );
	stackCmdNormal( _whileCheck, tokenBuffer );
	
	return tokenBuffer;
}

char *cmdWend(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _while ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	return tokenBuffer;
}

char *cmdUntil(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	// we are changin the stack from loop to normal, so when get to end of line or next command, it be executed after the logical tests.

	tokenMode = mode_logical;

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _repeat ) cmdTmp[cmdStack-1].flag = cmd_first;
	return tokenBuffer;
}

char *cmdTrue(struct nativeCommand *cmd, char *tokenBuffer)
{
	_num(-1);
	return tokenBuffer;
}

char *cmdFalse(struct nativeCommand *cmd, char *tokenBuffer)
{
	_num(0);
	return tokenBuffer;
}

char *cmdFor(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _for, tokenBuffer );
	cmdTmp[cmdStack-1].step = 1;		// set default counter step

	return tokenBuffer;
}

char *cmdTo(struct nativeCommand *cmd, char *tokenBuffer )
{
	int flag;
	bool is_for_to = false;

	if (cmdStack) 
	{
		flushCmdParaStack();

		// We loop back to "TO" not "FOR", we are not reseting COUNTER var.
		if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag == cmd_first ))
		{
			cmdTmp[cmdStack-1].tokenBuffer = tokenBuffer ;
			cmdTmp[cmdStack-1].flag = cmd_loop;
			is_for_to = true;
		}
	}

	if (is_for_to == false) stack ++;

	return tokenBuffer;
}

char *cmdStep(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _step, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdNext(struct nativeCommand *cmd, char *tokenBuffer )
{
	char *ptr = tokenBuffer ;

	if (NEXT_TOKEN(ptr) == 0x0006 )	// next is variable
	{
		struct reference *ref = (struct reference *) (ptr + 2);
		int idx_var = ref -> ref -1;
		
		if (( cmdTmp[cmdStack-1].cmd == _for ) && (cmdTmp[cmdStack-1].flag == cmd_loop ))
		{
			ptr = cmdTmp[cmdStack-1].tokenBuffer;

			if (NEXT_TOKEN(ptr) == 0x003E ) 
			{
				if (globalVars[idx_var].var.value < NEXT_INT(ptr)  )
				{
					globalVars[idx_var].var.value +=cmdTmp[cmdStack-1].step; 
					tokenBuffer = cmdTmp[cmdStack-1].tokenBuffer + 6;
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
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	return NULL;
}

char *cmdReturn(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _gosub ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	return tokenBuffer;
}

char *cmdProcedure(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	struct procedure *proc = (struct procedure *) tokenBuffer;

	printf("Goto %08x\n",proc -> EndOfProc);

	return proc -> EndOfProc - sizeof(struct procedure);
}

char *cmdProcAndArgs(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

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
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack)
	{
		printf("%04x\n",NEXT_TOKEN(tokenBuffer));

		if (NEXT_TOKEN(tokenBuffer) == 0x0084 )	//  End Proc[ return value ]
		{
			printf("yes we are here\n");

			// changes function pointer only so that ']' don't think its end of proc by accident.
			// we aslo push result of stack into Param.
			if (cmdTmp[cmdStack-1].cmd == _procedure ) cmdTmp[cmdStack-1].cmd = _endProc;
		}
		else 	// End Proc
		{
			if (cmdTmp[cmdStack-1].cmd == _procedure ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
		}
	}

	return tokenBuffer;
}


char *cmdBracket(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _procedure ) 
	{
		struct reference *ref;
		struct glueCommands data;
		char *ptr;
		int args = 0;
		int n;
		unsigned short token;

		printf("go down this path ....\n");

		args = stack - cmdTmp[cmdStack-1].stack +1;

		stack -= (args-1);
	
		n=0;
		for (ptr = tokenBuffer; (*((unsigned short *) ptr) != 0x008C) &&(n<args) ;ptr+=2)
		{
			token = *((unsigned short *) ptr);

			switch ( token )
			{
				case 0x0006:	

					ref = (struct reference *) (ptr+2);

					data.lastVar = ref->ref;
					_setVar( &data );

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

	return tokenBuffer;
}

char *cmdBracketEnd(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _procAndArgs )
	{
		cmdTmp[cmdStack-1].tokenBuffer2 = tokenBuffer;
	}

	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _endProc ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	return tokenBuffer;
}

char *cmdShared(struct nativeCommand *cmd, char *tokenBuffer )
{
	// we should not need to do anything here, but maybe good idea to jump over few tokens.
	return tokenBuffer;
}

char *cmdGlobal(struct nativeCommand *cmd, char *tokenBuffer )
{
	// we should not need to do anything here, but maybe good idea to jump over few tokens.
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
	_num( var_param_num );
	return tokenBuffer;
}

char *cmdPopProc(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

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
