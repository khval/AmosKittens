
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"
#include "debug.h"
#include <string>
#include <iostream>
#include "errors.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;

using namespace std;

extern char *findLabel( char *name );

char *_print( struct glueCommands *data )
{
	int n;
	printf("PRINT: ");

	for (n=data->stack;n<=stack;n++)
	{
//		printf("stack %d, type: %d value %d\n",n, kittyStack[n].type, kittyStack[n].value);

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

		if (n<=stack) printf("    ");
	}
	printf("\n");
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
	if (kittyStack[data->stack].value != -1)
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

	dump_stack();
	dump_prog_stack();

	// two command should be stacked, while loop, and while check.
	// while loop is removed from stack, if check is false
	// and we jump over the wend

	if (kittyStack[data->stack].value != -1)
	{
		if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _while ) 
		{
			cmdStack --;
			offset = *((unsigned short *) cmdTmp[cmdStack].tokenBuffer);
			if (offset) 
			{
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

char *_goto( struct glueCommands *data )
{
	char *labelName = kittyStack[data->stack].str;

	printf("%s\n",labelName);

	return findLabel( labelName );
}

char *cmdInput(nativeCommand *cmd, char *ptr)
{
	tokenMode = mode_input;
	stackCmdNormal( _input, ptr );
	return ptr;
}

char *_addStr( struct glueCommands *data )
{
	int len = 0;
	char *tmp;
	char *_new;

	len = kittyStack[stack].len + kittyStack[stack+1].len;

	_new = (char *) malloc(len+1);
	if (_new)
	{
		_new[0] = 0;
		strcpy(_new, kittyStack[stack].str);
		strcpy(_new+kittyStack[stack].len,kittyStack[stack+1].str);
		if (kittyStack[stack].str) free( kittyStack[stack].str );
		kittyStack[stack].str = _new;
		kittyStack[stack].len = len;

	}

	// delete string from above.
	if (kittyStack[stack+1].str) free( kittyStack[stack+1].str );
	kittyStack[stack+1].str = NULL;
	return NULL;
}

char *_addData( struct glueCommands *data )
{
//	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	if (kittyStack[stack].type != kittyStack[stack+1].type)
	{
		printf("mismatch error\n");
		return NULL;
	}

	switch (kittyStack[stack].type & 3)
	{
		case 0:
				printf("stack[%d].value=%d+%d\n", stack, kittyStack[stack].value, kittyStack[stack+1].value );
				kittyStack[stack].value += kittyStack[stack+1].value;
				break;
		case 1:	kittyStack[stack].decimal += kittyStack[stack+1].decimal;
				break;
		case 2:	_addStr( data );
				break;
	}
	return NULL;
}

char *_subStr( struct glueCommands *data )
{
	char *find;
 	int find_len;
	char *d,*s;

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack--;

	find = kittyStack[stack+1].str;
 	find_len = kittyStack[stack+1].len;

	s=d= kittyStack[stack].str;

//	printf("%s - %s\n",s, find);

	for(;*s;s++)
	{
		if (strncmp(s,find,find_len)==0) s+=find_len;
		if (*s) *d++=*s;
	}
	*d = 0;

	kittyStack[stack].len = d - kittyStack[stack].str;

	// delete string from above.
	if (kittyStack[stack+1].str) free( kittyStack[stack+1].str );
	kittyStack[stack+1].str = NULL;

	return NULL;
}

char *_subData( struct glueCommands *data )
{
//	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	if (kittyStack[stack].type != kittyStack[stack+1].type)
	{
		printf("mismatch error\n");
		return NULL;
	}

	switch (kittyStack[stack].type & 3)
	{
		case 0:
				printf("stack[%d].value=%d-%d\n", stack, kittyStack[stack].value, kittyStack[stack+1].value );
				kittyStack[stack].value -= kittyStack[stack+1].value;
				break;
		case 1:	kittyStack[stack].decimal -= kittyStack[stack+1].decimal;
				break;
		case 2:	_subStr( data );
				break;
	}

	return NULL;
}

char *_mulData( struct glueCommands *data )
{
//	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	if (kittyStack[stack].type != kittyStack[stack+1].type)
	{
		printf("mismatch error\n");
		return NULL;
	}

	switch (kittyStack[stack].type & 3)
	{
		case 0:	
				printf("stack[%d].value=%d*%d\n", stack, kittyStack[stack].value, kittyStack[stack+1].value );
				kittyStack[stack].value *= kittyStack[stack+1].value;
				break;
		case 1:	kittyStack[stack].decimal *= kittyStack[stack+1].decimal;
				break;
		case 2:	printf("mismatch error\n");
				break;
	}
	return NULL;
}

char *_divData( struct glueCommands *data )
{
//	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (stack==0) 
	{
		printf("can't do this :-(\n");
		return NULL;
	}

	stack --;

	if (kittyStack[stack].type != kittyStack[stack+1].type)
	{
		printf("mismatch error\n");
		return NULL;
	}

	switch (kittyStack[stack].type & 3)
	{
		case 0:	
				printf("stack[%d].value=%d/%d\n", stack, kittyStack[stack].value, kittyStack[stack+1].value );
				kittyStack[stack].value /= kittyStack[stack+1].value;
				break;
		case 1:	kittyStack[stack].decimal /= kittyStack[stack+1].decimal;
				break;
		case 2:	printf("mismatch error\n");
				break;
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
		printf("mismatch error\n");
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
		printf("mismatch error\n");
		return NULL;
	}

	printf("(%d == %d) == %d \n", kittyStack[stack].value , kittyStack[stack+1].value, (kittyStack[stack].value == kittyStack[stack+1].value) );

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



char *_setVar( struct glueCommands *data )
{
	struct kittyData *var;
	var = &globalVars[data -> lastVar-1].var;
	
	if (kittyStack[stack].type == ( var -> type & 7) )
	{
		switch (globalVars[data -> lastVar].var.type)
		{
			case type_int:
				var->value = kittyStack[stack].value;

printf("set %s = %d\n",  globalVars[data -> lastVar-1].varName , kittyStack[stack].value);

				break;
			case type_float:
				var->decimal = kittyStack[stack].decimal;
				break;
			case type_string:
				if (var->str) free(var->str);
				var->str = strdup(kittyStack[stack].str);
				var->len = kittyStack[stack].len;
				break;
			case type_int | type_array:
				var->int_array[var -> index] = kittyStack[stack].value;
				break;
			case type_float | type_array:
				var->float_array[var -> index] = kittyStack[stack].decimal;
				break;
			case type_string | type_array:
				if (var->str_array[var -> index] ) free(var->str_array[var->index]);
				var->str_array[var -> index] = strdup(kittyStack[stack].str);	
			break;
		}
	}
	else
	{

		printf("kittyStack[%d].type= %d, (globalVars[%d].var.type & 7)=%d\n",
				stack, kittyStack[stack].type, data -> lastVar, (globalVars[data -> lastVar-1].var.type & 7));

		printf("Mismatch error\n");
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
//	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stack++;
	return tokenBuffer;
}

char *addStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	cmdParm( _addStr, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *subCalc(struct nativeCommand *cmd, char *tokenBuffer)
{
	kittyStack[stack].str = NULL;
	kittyStack[stack].state = state_subData;

	stack++;
	return tokenBuffer;
}

char *subCalcEnd(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	if (stack > 0)
	{
		if (kittyStack[stack-1].state == state_subData)
		{
			kittyStack[stack-1] = kittyStack[stack];
			stack --;
			if (cmdStack) if (stack) if (kittyStack[stack-1].state == state_none) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
		}
	}
	return tokenBuffer;
}

extern void _str(const char *str);

char *breakData(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	std::string input;

	switch (tokenMode)
	{
 		case mode_standard:
			cmdParm( _addData, tokenBuffer );
			stack++;
			break;
		case mode_input:
			if (cmdStack) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
			getline(cin, input);
			_str( (const char *) input.c_str() );
			stack++;
					 
			cmdParm( _setVarReverse, tokenBuffer );
			break;
	}

	return tokenBuffer;
}


char *addData(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	cmdParm( _addData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *subData(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	cmdParm(_subData,tokenBuffer);
	stack++;
	return tokenBuffer;
}

char *cmdNotEqual(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	if (tokenMode == mode_logical)
	{
		cmdParm(_not_equal, tokenBuffer);
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
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	if (tokenMode == mode_logical)
	{
		cmdParm(_equal, tokenBuffer);
		stack++;
	}
	else
	{
		stackCmdNormal(_setVar, tokenBuffer);
	}

	return tokenBuffer;
}

char *cmdIf(struct nativeCommand *cmd, char *tokenBuffer)
{
	_num(0);	// stack reset.
	stackCmdNormal(_if, tokenBuffer);
	return tokenBuffer;
}

char *cmdThen(struct nativeCommand *cmd, char *tokenBuffer)
{
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

char *mulData(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	cmdParm( _mulData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *divData(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	cmdParm( _divData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *cmdGoto(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	tokenMode = mode_goto;
	return tokenBuffer;
}

char *cmdDo(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdLoop( _do, tokenBuffer );
	return tokenBuffer;
}


char *cmdRepeat(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdLoop( _repeat, tokenBuffer );
	return tokenBuffer;
}

char *cmdLoop(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _do ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	return tokenBuffer;
}

char *cmdWhile(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("--------------------------------------\n%s:%d\n",__FUNCTION__,__LINE__);

	stackCmdLoop( _while, tokenBuffer );
	stackCmdNormal( _whileCheck, tokenBuffer );
	
	return tokenBuffer;
}

char *cmdWend(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _while ) tokenBuffer=cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	return tokenBuffer;
}

char *cmdUntil(struct nativeCommand *cmd, char *tokenBuffer)
{
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

