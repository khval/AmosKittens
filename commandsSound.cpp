
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#endif

#ifdef __linux__
#include "os/linux/stuff.h"
#endif

#include "debug.h"
#include <string>
#include <vector>
#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "var_helper.h"
#include "kittyErrors.h"
#include "engine.h"

extern int last_var;


char *soundBoom(struct nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

char *_soundTempo( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	switch (args)
	{
		case 1:
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *soundTempo(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _soundTempo, tokenBuffer );
	return tokenBuffer;
}


