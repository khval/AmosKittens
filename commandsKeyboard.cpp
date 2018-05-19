
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "debug.h"
#include <string>
#include <iostream>
#include <proto/dos.h>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "errors.h"
#include "engine.h"

char *cmdWaitKey(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (engine_started)
	{
		engine_wait_key = true;
		do
		{
			Delay(1);
		} while ((engine_wait_key == true) && (engine_started));
	}
	else
	{
		getchar();
	}

	return tokenBuffer;
}


char *cmdInkey(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	setStackStrDup("");
	return tokenBuffer;
}

char *cmdScancode(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	setStackNum(0);
	return tokenBuffer;
}

char *cmdClearKey(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	setStackNum(0);
	return tokenBuffer;
}
