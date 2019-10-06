
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "commandsLibs.h"
#include "errors.h"
#include "debug.h"

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

extern int last_var;

char *_libLibOpen( struct glueCommands *data, int nextToken )
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

char *libLibOpen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _libLibOpen, tokenBuffer );
}

char *_libLibClose( struct glueCommands *data, int nextToken )
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

char *libLibClose(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _libLibClose, tokenBuffer );
}

char *_libLibCall( struct glueCommands *data, int nextToken )
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

char *libLibCall(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _libLibCall, tokenBuffer );
}


char *_libLibBase( struct glueCommands *data, int nextToken )
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

char *libLibBase(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _libLibBase, tokenBuffer );
}




