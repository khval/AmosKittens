#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "debug.h"
#include <string>
#include <proto/dos.h>
#include <vector>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsMenu.h"
#include "var_helper.h"
#include "errors.h"
#include "engine.h"

extern int last_var;

char *_menuChoice( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
	}

	popStack( stack - data->stack );
}

char *menuChoice(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _menuChoice, tokenBuffer );
	return tokenBuffer;
}

char *_menuMenuStr( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
	}

	popStack( stack - data->stack );
}

char *menuMenuStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _menuMenuStr, tokenBuffer );
	return tokenBuffer;
}

char *_menuSetMenu( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
	}

	popStack( stack - data->stack );
}

char *menuSetMenu(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _menuSetMenu, tokenBuffer );
	return tokenBuffer;
}

char *menuMenuOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *menuMenuOff(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *_menuMenuInactive( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
	}

	popStack( stack - data->stack );
}

char *menuMenuInactive(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _menuMenuInactive, tokenBuffer );
	return tokenBuffer;
}

