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

