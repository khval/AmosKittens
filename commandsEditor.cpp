

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
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
#include "commandsEditor.h"
#include "var_helper.h"
#include "kittyErrors.h"
#include "engine.h"
#include "amosString.h"

extern int last_var;

char *_cmdCallEditor( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{

		case 1:	
				break;
		defaut:
				popStack( stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *cmdCallEditor(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdCallEditor, tokenBuffer );
	return tokenBuffer;
}

char *_cmdAskEditor( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{

		case 1:	
				break;
		defaut:
				popStack( stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *cmdAskEditor(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdAskEditor, tokenBuffer );
	return tokenBuffer;
}

