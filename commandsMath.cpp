#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <string>
#include <iostream>
#include <math.h>

#include "debug.h"
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

char *_mathInc( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *var = NULL;
	int args = stack - data->stack ;
	char *ptr = data -> tokenBuffer ;

	if (NEXT_TOKEN( ptr ) == 0x0006)
	{
		struct reference *ref = (struct reference *) (ptr + 2);
		var = &globalVars[ref->ref-1].var;
	}

	if (var)
	{
		switch (var->type)
		{
			case type_int:
				var->value++;
				break;
	
			case type_int | type_array:
				var->int_array[var -> index]++;
				break;
	
			default:
				setError(ERROR_Type_mismatch);
		}
	}

	popStack(args);
	return NULL;
}

char *_mathDec( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *var = NULL;
	int args = stack - data->stack ;
	char *ptr = data -> tokenBuffer ;

	if (NEXT_TOKEN( ptr ) == 0x0006)
	{
		struct reference *ref = (struct reference *) (ptr + 2);
		var = &globalVars[ref->ref-1].var;
	}

	if (var)
	{
		switch (var->type)
		{
			case type_int:
				var->value--;
				break;
	
			case type_int | type_array:
				var->int_array[var -> index]--;
				break;
	
			default:
				setError(ERROR_Type_mismatch);
		}
	}

	popStack(args);
	return NULL;
}

char *_mathAdd( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *var = NULL;
	int args = stack - data->stack ;
	char *ptr = data -> tokenBuffer ;

	if (NEXT_TOKEN( ptr ) == 0x0006)
	{
		struct reference *ref = (struct reference *) (ptr + 2);
		var = &globalVars[ref->ref-1].var;
	}
	
	if (var)
	{
		switch (var->type)
		{
			case type_int:
				var->value+= _stackInt( data -> stack + 2 );
				break;
	
			case type_int | type_array:
				var->int_array[var -> index]+= _stackInt( data -> stack + 2 );
				break;
	
			default:
				setError(ERROR_Type_mismatch);
		}
	}

	popStack(args);
	return NULL;
}

char *mathInc(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	stackCmdNormal( _mathInc, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *mathDec(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	stackCmdNormal( _mathDec, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *mathAdd(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	stackCmdNormal( _mathAdd, tokenBuffer );
	stack++;
	return tokenBuffer;
}

//------------------------------------------------------------



char *_mathSin( struct glueCommands *data )
{
	double r;
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack;

	dump_stack();

	if (args == 1)
	{
		r = _stackDecimal( stack );
	}

	popStack(stack - data->stack);

	setStackDecimal( sin( r ) );

	return NULL;
}

char *_mathCos( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathTan( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathAcos( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathAsin( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathAtan( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathHsin( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathHcos( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathHtan( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathLog( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathExp( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathLn( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathSqr( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathAbs( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathInt( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathSgn( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathRnd( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathRandomize( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathMax( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathMin( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathSwap( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathFix( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathDefFn( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_mathFn( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}


//------------------------------------------------------------

char *mathDegree(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	return tokenBuffer;
}

char *mathRadian(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	return tokenBuffer;
}

// is const number so,

char *mathPi(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((short *) (tokenBuffer) );

	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if ( correct_order( last_token,  next_token ) == false )
	{
		kittyStack[stack].str = NULL;
		kittyStack[stack].value = 0;
		kittyStack[stack].state = state_hidden_subData;
		stack++;
	}

	setStackDecimal(M_PI);
	dump_stack();

	return tokenBuffer;
}

char *mathSin(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathSin, tokenBuffer );
	return tokenBuffer;
}

char *mathCos(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathCos, tokenBuffer );
	return tokenBuffer;
}

char *mathTan(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathTan, tokenBuffer );
	return tokenBuffer;
}

char *mathAcos(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathAcos, tokenBuffer );
	return tokenBuffer;
}

char *mathAsin(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathAsin, tokenBuffer );
	return tokenBuffer;
}

char *mathAtan(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathAtan, tokenBuffer );
	return tokenBuffer;
}

char *mathHsin(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathHsin, tokenBuffer );
	return tokenBuffer;
}

char *mathHcos(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathHcos, tokenBuffer );
	return tokenBuffer;
}

char *mathHtan(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathHtan, tokenBuffer );
	return tokenBuffer;
}

char *mathLog(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathLog, tokenBuffer );
	return tokenBuffer;
}

char *mathExp(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _mathExp, tokenBuffer );
	return tokenBuffer;
}

char *mathLn(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _mathLn, tokenBuffer );
	return tokenBuffer;
}

char *mathSqr(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathSqr, tokenBuffer );
	return tokenBuffer;
}

char *mathAbs(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathAbs, tokenBuffer );
	return tokenBuffer;
}

char *mathInt(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathInt, tokenBuffer );
	return tokenBuffer;
}

char *mathSgn(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathSgn, tokenBuffer );
	return tokenBuffer;
}

char *mathRnd(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathRnd, tokenBuffer );
	return tokenBuffer;
}

char *mathRandomize(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _mathRandomize, tokenBuffer );
	return tokenBuffer;
}

char *mathMax(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathMax, tokenBuffer );
	return tokenBuffer;
}

char *mathMin(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathMin, tokenBuffer );
	return tokenBuffer;
}

char *mathSwap(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathSwap, tokenBuffer );
	return tokenBuffer;
}

char *mathFix(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathFix, tokenBuffer );
	return tokenBuffer;
}

char *mathDefFn(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	return tokenBuffer;
}

char *mathFn(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	return tokenBuffer;
}

