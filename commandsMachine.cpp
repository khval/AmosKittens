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
	char *adr;
	int value;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);
	dump_stack();

	if (args==2)
	{
		adr = (char *) _stackInt(stack-1);
		value = _stackInt(stack);

		printf("adr: %d\n",adr);

		if (adr)	// we can only Poke positive addresses
		{
			*adr = (char) value;
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
	short *adr;
	int value;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
		adr = (short *) _stackInt(stack-1);
		value = _stackInt(stack);

		if (adr)	// we can only Doke positive addresses
		{
			*adr = (short) value;
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
	int *adr;
	int value;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
		adr = (int *) _stackInt(stack-1);
		value = _stackInt(stack);

		if (adr)
		{
			*adr = (int) value;
			success = true;
		}

		printf("%04x\n",adr);
	}

	if (success == false) setError(25);

	popStack( stack - data->stack );
	_num(ret);
	return NULL;
}

char *_machinePeek( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		char *adr = (char *) _stackInt(data->stack);

		if (adr)
		{
			ret = *adr;
			success = true;
		}
	}

	if (success == false) setError(25);

	popStack( stack - data->stack );
	_num(ret);
	return NULL;
}

char *_machineDeek( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		short *adr = (short *) _stackInt(data->stack);

		if (adr)
		{
			ret = *adr;
			success = true;
		}
	}

	if (success == false) setError(25);

	popStack( stack - data->stack );
	_num(ret);
	return NULL;
}


char *_machineLeek( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int *adr = (int *) _stackInt(data->stack);

		if (adr)
		{
			ret = *adr;
			success = true;
		}
	}

	if (success == false) setError(25);

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

char *machineVarPtr(struct nativeCommand *cmd, char *ptr)
{

	if (NEXT_TOKEN( ptr ) == 0x0074) ptr+=2;

	if (NEXT_TOKEN( ptr ) == 0x0006)
	{
		struct reference *ref = (struct reference *) (ptr + 2);
		int idx = ref->ref-1;

		printf("globalVars[idx].var.type: %d\n",globalVars[idx].var.type);

		switch ( globalVars[idx].var.type )
		{
			case type_float:
				setStackPtr( &globalVars[idx].var.decimal );
				break;

			case type_int:
				setStackPtr( &globalVars[idx].var.value );
				break;

			case type_string:
				setStackPtr( globalVars[idx].var.str );
				break;
		}
		ptr += (2 + sizeof(struct reference) + ref -> length) ;
	}

	if (NEXT_TOKEN( ptr ) == 0x007C) ptr+=2;

	dump_stack();

	printf("%04x\n",NEXT_TOKEN( ptr ));

	return ptr;
}

