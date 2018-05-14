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
#include "commandsScreens.h"
#include "errors.h"
#include "engine.h"

extern int last_var;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern int current_screen;

int pen = 2;
int paper = 1;

char *_textLocate( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	bool success = false;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
		if (screens[current_screen])
		{
			screens[current_screen] -> locateX = _stackInt( stack-1 );
			screens[current_screen] -> locateY = _stackInt( stack );
		}
		success = true;
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *textLocate(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textLocate, tokenBuffer );
	return tokenBuffer;
}

char *_textPen( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	bool success = false;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		pen  = _stackInt( stack );
		success = true;
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *textPen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textPen, tokenBuffer );
	return tokenBuffer;
}

char *_textPaper( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	bool success = false;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		paper = _stackInt( stack );
		success = true;
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *textPaper(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textPaper, tokenBuffer );
	return tokenBuffer;
}



