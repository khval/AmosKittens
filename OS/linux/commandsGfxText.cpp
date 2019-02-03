#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <stdint.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#endif

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

extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern struct RastPort font_render_rp;
extern bool next_print_line_feed;
extern int pen0, pen1,pen2;
extern int xgr,  ygr;

extern int GrWritingMode;

extern int current_screen;
extern char *(*_do_set) ( struct glueCommands *data, int nextToken );
extern char *_setVar( struct glueCommands *data, int nextToken );


char *_gfxGrWriting( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			GrWritingMode = getStackNum(stack);
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxGrWriting(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _gfxGrWriting, tokenBuffer );
	return tokenBuffer;
}

char *_gfxText( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:
			{
				int x = getStackNum( stack-2 );
				int y = getStackNum( stack-1 );
				char *txt = getStackString( stack );

				if (txt)
				{
				}
			}
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxText(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _gfxText, tokenBuffer );
	return tokenBuffer;
}

char *_gfxTextLength( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	unsigned short ret = 0;
	proc_names_printf("%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			{
				char *txt = getStackString( stack );

				if (txt)
				{
				}
			}
			break;

		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *gfxTextLength(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdParm( _gfxTextLength, tokenBuffer );
	return tokenBuffer;
}


char *gfxTextBase(struct nativeCommand *cmd, char *tokenBuffer)
{
	int ret = 0;
	proc_names_printf("%s:s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	setStackNum(ret);

	return tokenBuffer;
}

char *_gfxSetText( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("%s:%s:%d -> dummy command ignored\n",__FILE__,__FUNCTION__,__LINE__);	

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *gfxSetText(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdParm( _gfxSetText, tokenBuffer );
	return tokenBuffer;
}






