#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "debug.h"
#include <string>
#include <iostream>
#include <proto/retroMode.h>

#include "stack.h"
#include "amosKittens.h"
#include "commandsFonts.h"
#include "errors.h"
#include "engine.h"

extern int last_var;

char *fontsGetRomFonts(struct nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

char *_fontsSetFont( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1: ret = getStackNum( stack ) ;
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *fontsSetFont(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _fontsSetFont, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}
