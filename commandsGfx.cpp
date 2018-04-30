#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <exec/emulation.h>
#include <proto/dos.h>
#include "debug.h"
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commandsGfx.h"
#include "errors.h"
#include "engine.h"

extern int sig_main_vbl;

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

char *_gfxScreenOpen( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	dump_stack();

	if (args==5)
	{
		int screen_num = _stackInt( stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			engine_lock();
			engine_unlock();

			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_gfxScreenDisplay( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);
	dump_stack();

	if (args==5)
	{
		int screen_num = _stackInt( stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			engine_lock();
			engine_unlock();

			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_gfxColour( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
		success = true;
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *gfxScreenOpen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenOpen, tokenBuffer );
	return tokenBuffer;
}

char *gfxLowres(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(0);
	return tokenBuffer;
}

char *gfxHires(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(0);
	return tokenBuffer;
}

char *gfxMouseKey(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(engine_mouse_key);
	return tokenBuffer;
}

char *gfxWaitVbl(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (sig_main_vbl)
	{
		Wait(1<<sig_main_vbl);
	}
	else
	{
		Delay(1);
	}
	return tokenBuffer;
}

char *gfxScreenDisplay(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenDisplay, tokenBuffer );
	return tokenBuffer;
}

char *gfxColour(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxColour, tokenBuffer );
	return tokenBuffer;
}

