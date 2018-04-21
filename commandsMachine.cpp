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
#include "commandsBanks.h"
#include "errors.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

char *_machineCopy( struct glueCommands *data )
{
	int adrFromStart, adrFromEnd, adrTo;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==3)
	{
		adrFromStart = _stackInt(stack-2);
		adrFromEnd = _stackInt(stack-1);
		adrTo = _stackInt(stack);

		if ((adrFromStart>0)&&(adrFromEnd>0)&&(adrTo>0))
		{
			memcpy( (void *) adrTo, (void *) adrFromStart, adrFromEnd - adrFromStart );
			success = true;
		}
	}

	if (success == false) setError(25);

	popStack( stack - data->stack );
	_num(ret);
	return NULL;
}

char *_machinePoke( struct glueCommands *data )
{
	int adr, value;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);
	dump_stack();

	if (args==2)
	{
		adr = _stackInt(stack-1);
		value = _stackInt(stack);

		printf("adr: %d\n",adr);

		if (adr>0)	// we can only Poke positive addresses
		{
			*((char *) adr) = value;
			success = true;
		}
	}

	if (success == false) setError(25);

	popStack( stack - data->stack );
	_num(ret);
	return NULL;
}

char *_machineDoke( struct glueCommands *data )
{
	int adr, value;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
		adr = _stackInt(stack-1);
		value = _stackInt(stack);

		if (adr>0)	// we can only Doke positive addresses
		{
			*((short *) adr) = value;
			success = true;
		}
	}

	if (success == false) setError(25);

	popStack( stack - data->stack );
	_num(ret);
	return NULL;
}

char *_machineLoke( struct glueCommands *data )
{
	int adr, value;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
		adr = _stackInt(data->stack-1);
		value = _stackInt(data->stack);

		if (adr>0)	// we can only Loke positive addresses
		{
			*((int *) adr) = value;
			success = true;
		}
	}

	if (success == false) setError(25);

	popStack( stack - data->stack );
	_num(ret);
	return NULL;
}

char *_machinePeek( struct glueCommands *data )
{
	int n;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		n = _stackInt(data->stack);
		if (n>0)	// we can only peek positive addresses
		{
			ret = *((char *) n);
		}
	}

	popStack( stack - data->stack );
	_num(ret);
	return NULL;
}

char *_machineDeek( struct glueCommands *data )
{
	int n;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		n = _stackInt(data->stack);
		if (n>0)	// we can only peek positive addresses
		{
			ret = *((short *) n);
		}
	}

	popStack( stack - data->stack );
	_num(ret);
	return NULL;
}


char *_machineLeek( struct glueCommands *data )
{
	int n;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		n = _stackInt(data->stack);
		if (n>0)	// we can only peek positive addresses
		{
			ret = *((int *) n);
		}
	}

	popStack( stack - data->stack );
	_num(ret);
	return NULL;
}


char *machinePoke(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _machinePoke, tokenBuffer );
	return tokenBuffer;
}

char *machinePeek(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _machinePeek, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *machineDoke(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _machineDoke, tokenBuffer );
	return tokenBuffer;
}

char *machineDeek(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _machineDeek, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *machineLoke(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _machineLoke, tokenBuffer );
	return tokenBuffer;
}

char *machineLeek(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _machineLeek, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *machineCopy(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _machineCopy, tokenBuffer );
	return tokenBuffer;
}

