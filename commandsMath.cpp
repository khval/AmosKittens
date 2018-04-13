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

char *_incMath( struct glueCommands *data )
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

char *_decMath( struct glueCommands *data )
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

char *_addMath( struct glueCommands *data )
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

char *incMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	stackCmdNormal( _incMath, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *decMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	stackCmdNormal( _decMath, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *addMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	stackCmdNormal( _addMath, tokenBuffer );
	stack++;
	return tokenBuffer;
}

//------------------------------------------------------------



char *_sinMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_cosMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_tanMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_acosMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_asinMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_atanMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_hsinMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_hcosMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_htanMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_logMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_expMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_lnMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_sqrMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_absMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_intMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_sgnMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_rndMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_randomizeMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_maxMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_minMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_swapMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_fixMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_defFnMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}

char *_fnMath( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	return NULL;
}


//------------------------------------------------------------

char *degreeMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	return tokenBuffer;
}

char *radianMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	return tokenBuffer;
}

char *piMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	return tokenBuffer;
}

char *sinMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _sinMath, tokenBuffer );
	return tokenBuffer;
}

char *cosMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _cosMath, tokenBuffer );
	return tokenBuffer;
}

char *tanMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _tanMath, tokenBuffer );
	return tokenBuffer;
}

char *acosMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _acosMath, tokenBuffer );
	return tokenBuffer;
}

char *asinMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _asinMath, tokenBuffer );
	return tokenBuffer;
}

char *atanMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _atanMath, tokenBuffer );
	return tokenBuffer;
}

char *hsinMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _hsinMath, tokenBuffer );
	return tokenBuffer;
}

char *hcosMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _hcosMath, tokenBuffer );
	return tokenBuffer;
}

char *htanMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _htanMath, tokenBuffer );
	return tokenBuffer;
}

char *logMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _logMath, tokenBuffer );
	return tokenBuffer;
}

char *expMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _expMath, tokenBuffer );
	return tokenBuffer;
}

char *lnMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _lnMath, tokenBuffer );
	return tokenBuffer;
}

char *sqrMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _sqrMath, tokenBuffer );
	return tokenBuffer;
}

char *absMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _absMath, tokenBuffer );
	return tokenBuffer;
}

char *intMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _intMath, tokenBuffer );
	return tokenBuffer;
}

char *sgnMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _sgnMath, tokenBuffer );
	return tokenBuffer;
}

char *rndMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _rndMath, tokenBuffer );
	return tokenBuffer;
}

char *randomizeMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _randomizeMath, tokenBuffer );
	return tokenBuffer;
}

char *maxMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _maxMath, tokenBuffer );
	return tokenBuffer;
}

char *minMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _minMath, tokenBuffer );
	return tokenBuffer;
}

char *swapMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _swapMath, tokenBuffer );
	return tokenBuffer;
}

char *fixMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _fixMath, tokenBuffer );
	return tokenBuffer;
}

char *defFnMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	return tokenBuffer;
}

char *fnMath(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	return tokenBuffer;
}

