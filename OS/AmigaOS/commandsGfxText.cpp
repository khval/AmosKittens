#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
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

int os_text_length(char *txt)
{
	int l = strlen(txt);
	return TextLength(&font_render_rp, txt, l );
}

void os_text(struct retroScreen *screen,int x, int y, char *txt)
{
	struct TextExtent te;
	int l = strlen(txt);
	int tl;

	TextExtent( &font_render_rp, txt, strlen( txt), &te );
	SetAPen( &font_render_rp, screen -> ink0 );
	SetBPen( &font_render_rp, screen -> ink1 );
	Move( &font_render_rp, 0,-te.te_Extent.MinY );
	Text( &font_render_rp, txt, l );
	tl = TextLength(&font_render_rp, txt, l );
	retroBitmapBlit( font_render_rp.BitMap, 0,0, tl,te.te_Height, screen, x , y-10);
}

void os_text_no_outline(struct retroScreen *screen,int x, int y, char *txt, uint16_t pen)
{
	struct TextExtent te;
	int l = strlen(txt);
	ULONG mode;

	TextExtent( &font_render_rp, txt, strlen( txt), &te );

	retroScreenToBitmap( screen, x , y-te.te_Extent.MinY-te.te_Height, te.te_Width,te.te_Height, font_render_rp.BitMap, 0 , 0);

      	mode = GetDrMd( &font_render_rp );
	SetDrMd( &font_render_rp, JAM1 );
	SetAPen( &font_render_rp, pen );
	Move( &font_render_rp, 0,-te.te_Extent.MinY );
	Text( &font_render_rp, txt, l );

	SetDrMd( &font_render_rp, mode );	// restore mode

	retroBitmapBlit( font_render_rp.BitMap, 0,0, te.te_Width,te.te_Height, screen, x , y-te.te_Extent.MinY-te.te_Height);
}

void os_text_outline(struct retroScreen *screen,int x, int y, char *txt, uint16_t pen,uint16_t outline)
{
	struct TextExtent te;
	int l = strlen(txt);
	ULONG mode;

       mode = GetDrMd( &font_render_rp );

	TextExtent( &font_render_rp, txt, strlen( txt), &te );
	RectFill ( &font_render_rp, 0,0, te.te_Width,te.te_Height );

	retroScreenToBitmap( screen, x , y-te.te_Extent.MinY-te.te_Height, te.te_Width,te.te_Height, font_render_rp.BitMap, 0 , 0);

	SetDrMd( &font_render_rp, JAM1 );

	SetAPen( &font_render_rp, outline );

		Move( &font_render_rp, 1,-te.te_Extent.MinY-1 );
		Text( &font_render_rp, txt, l );

		Move( &font_render_rp, 1,-te.te_Extent.MinY+1 );
		Text( &font_render_rp, txt, l );

		Move( &font_render_rp, 0,-te.te_Extent.MinY );
		Text( &font_render_rp, txt, l );

		Move( &font_render_rp, 2,-te.te_Extent.MinY );
		Text( &font_render_rp, txt, l );

	SetAPen( &font_render_rp, pen );

		Move( &font_render_rp, 1,-te.te_Extent.MinY );
		Text( &font_render_rp, txt, l );


	SetDrMd( &font_render_rp, mode );	// restore mode

	retroBitmapBlit( font_render_rp.BitMap, 0,0, te.te_Width,te.te_Height, screen, x , y-te.te_Extent.MinY-te.te_Height);
}


char *_gfxText( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:
			{
				int x = getStackNum( stack-2 );
				int y = getStackNum( stack-1 );
				char *txt = getStackString( stack );

				if ((txt)&&(screen))	os_text(screen, x,y,txt);
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
					ret = TextLength( &font_render_rp, txt, strlen( txt) );
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


/*

// I don't think this command takes arguments, must check manuall.

char *_gfxTextBase( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			{
				char *txt = getStackString( stack );

				if (txt)
				{
					struct TextExtent te;
					TextExtent( &font_render_rp, txt, strlen( txt), &te );
					ret = -te.te_Extent.MinY;
				}
			}
			break;

		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum(ret);

	dump_stack();

	return NULL;
}
*/


char *gfxTextBase(struct nativeCommand *cmd, char *tokenBuffer)
{
	int ret = 0;
	proc_names_printf("%s:s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	const char *txt="abcdefghijklmnopqrstuvwxyz";

	if (txt)
	{
		struct TextExtent te;
		TextExtent( &font_render_rp, txt, strlen( txt), &te );
		ret = -te.te_Extent.MinY;
	}

	setStackNum(ret);

//	stackCmdParm( _gfxTextBase, tokenBuffer );

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






