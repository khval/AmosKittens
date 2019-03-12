#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "config.h"

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <retromode.h>
#include <retromode_lib.h>
#include <unistd.h>
#endif

#include "debug.h"
#include <string>
#include <vector>
#include <iostream>
#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsBanks.h"
#include "errors.h"
#include "engine.h"

extern int last_var;
extern struct globalVar globalVars[];
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern int current_screen;
extern std::vector<struct kittyBank> kittyBankList;

void _my_print_text(struct retroScreen *screen, char *text, int maxchars);


char *_guiDialogRun( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	int guiChannel = 0;
	int label = 0;
	int x = 0,y = 0;
	const char *ret = NULL;

	switch (args)
	{
		case 1:	guiChannel = getStackNum(stack);
				break;

		case 2:	guiChannel = getStackNum(stack-1);
				label = getStackNum(stack);
				break;

		case 4:	guiChannel = getStackNum(stack-3);
				label = getStackNum(stack-2);
				x = getStackNum(stack-1);
				y = getStackNum(stack);
				break;

		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( 0 );

	return NULL;
}

char *guiDialogRun(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _guiDialogRun, tokenBuffer );
	return tokenBuffer;
}

char *_guiDialog( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	int guiChannel = 0;
	int label = 0;
	int x = 0,y = 0;
	const char *ret = NULL;

	switch (args)
	{
		case 1:	guiChannel = getStackNum(stack);
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( 0 );

	return NULL;
}

char *guiDialog(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _guiDialog, tokenBuffer );
	return tokenBuffer;
}

char *_guiDialogStr( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	int guiChannel = 0;
	int label = 0;
	int x = 0,y = 0;
	const char *ret = NULL;

	switch (args)
	{
		case 2:	guiChannel = getStackNum(stack);
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( 0 );

	return NULL;
}

char *guiDialogStr(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _guiDialogStr, tokenBuffer );
	return tokenBuffer;
}

//---

char *_guiDialogBox( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	const char *script = NULL;

	switch (args)
	{
		case 1:	script = getStackString(stack);
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( 0 );

	return NULL;
}

char *guiDialogBox(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _guiDialogBox, tokenBuffer );
	return tokenBuffer;
}

char *guiDialogFreeze(nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

char *guiDialogUnfreeze(nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

char *guiDialogClose(nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

char *_guiDialogOpen( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	int guiChannel = 0;
	int label = 0;
	int x = 0,y = 0;
	const char *ret = NULL;

	switch (args)
	{
		case 2:	guiChannel = getStackNum(stack);
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( 0 );

	return NULL;
}

char *guiDialogOpen(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _guiDialogOpen, tokenBuffer );
	return tokenBuffer;
}

