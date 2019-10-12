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
#include "kittyErrors.h"
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

extern int current_screen;
extern char *(*_do_set) ( struct glueCommands *data, int nextToken );
extern char *_setVar( struct glueCommands *data, int nextToken );

int os_text_width(struct stringData *txt)
{
	return TextLength(&font_render_rp, &txt->ptr, txt -> size );
}

int os_text_height(struct stringData *txt)
{
	struct TextExtent te;
	TextExtent( &font_render_rp, &txt->ptr, txt -> size, &te );
	return -te.te_Extent.MinY;
}

int os_text_base(struct stringData *txt)
{
	struct TextExtent te;
	TextExtent( &font_render_rp, &txt->ptr, txt -> size, &te );
	return -te.te_Extent.MinY - te.te_Extent.MaxY;
}


void os_text(struct retroScreen *screen,int x, int y, struct stringData *txt, int ink0, int ink1)
{
	struct TextExtent te;

	TextExtent( &font_render_rp, &txt->ptr, txt -> size, &te );

	SetAPen( &font_render_rp, ink0 );
	SetBPen( &font_render_rp, ink1 );
	Move( &font_render_rp, 0,-te.te_Extent.MinY );
	Text( &font_render_rp, &txt->ptr, txt -> size );

	switch (screen -> autoback)
	{
		case 0:	retroBitmapBlit( font_render_rp.BitMap, 0,0, te.te_Width,te.te_Height, screen, screen -> double_buffer_draw_frame, x , y + te.te_Extent.MinY);
				break;
		default:	retroBitmapBlit( font_render_rp.BitMap, 0,0, te.te_Width,te.te_Height, screen, 0, x , y + te.te_Extent.MinY);
				if (screen -> Memory[1]) retroBitmapBlit( font_render_rp.BitMap, 0,0, te.te_Width,te.te_Height, screen, 1, x , y + te.te_Extent.MinY);
				break;

	}
}

void os_text_no_outline(struct retroScreen *screen,int x, int y, struct stringData *txt, int pen)
{
	struct TextExtent te;
	ULONG mode;

	TextExtent( &font_render_rp, &txt->ptr, txt -> size, &te );

	retroScreenToBitmap( screen, x , y+te.te_Extent.MinY, te.te_Width,te.te_Height, font_render_rp.BitMap, 0 , 0);

      	mode = GetDrMd( &font_render_rp );
	SetDrMd( &font_render_rp, JAM1 );
	SetAPen( &font_render_rp, pen );
	Move( &font_render_rp, 0,-te.te_Extent.MinY );
	Text( &font_render_rp, &txt->ptr, txt -> size );

	SetDrMd( &font_render_rp, mode );	// restore mode

	switch (screen -> autoback)
	{
		case 0:	retroBitmapBlit( font_render_rp.BitMap, 0,0, te.te_Width,te.te_Height, screen, screen -> double_buffer_draw_frame, x , y + te.te_Extent.MinY);
				break;
		default:	retroBitmapBlit( font_render_rp.BitMap, 0,0, te.te_Width,te.te_Height, screen, 0, x , y + te.te_Extent.MinY);
				if (screen -> Memory[1]) retroBitmapBlit( font_render_rp.BitMap, 0,0, te.te_Width,te.te_Height, screen, 1, x , y + te.te_Extent.MinY);
				break;

	}
}


void os_text_outline(struct retroScreen *screen,int x, int y, struct stringData *txt, int pen,int outline)
{
	struct TextExtent te;
	ULONG mode;

       mode = GetDrMd( &font_render_rp );

	TextExtent( &font_render_rp, &txt->ptr, txt -> size, &te );
	RectFill ( &font_render_rp, 0,0, te.te_Width,te.te_Height );

	retroScreenToBitmap( screen, x , y+ te.te_Extent.MinY, te.te_Width,te.te_Height, font_render_rp.BitMap, 0 , 0);

	SetDrMd( &font_render_rp, JAM1 );

	SetAPen( &font_render_rp, outline );

		Move( &font_render_rp, 1,-te.te_Extent.MinY-1 );
		Text( &font_render_rp, &txt->ptr, txt -> size );

		Move( &font_render_rp, 1,-te.te_Extent.MinY+1 );
		Text( &font_render_rp, &txt->ptr, txt -> size );

		Move( &font_render_rp, 0,-te.te_Extent.MinY );
		Text( &font_render_rp, &txt->ptr, txt -> size );

		Move( &font_render_rp, 2,-te.te_Extent.MinY );
		Text( &font_render_rp, &txt->ptr, txt -> size );

	SetAPen( &font_render_rp, pen );

		Move( &font_render_rp, 1,-te.te_Extent.MinY );
		Text( &font_render_rp, &txt->ptr, txt -> size );


	SetDrMd( &font_render_rp, mode );	// restore mode

	switch (screen -> autoback)
	{
		case 0:	retroBitmapBlit( font_render_rp.BitMap, 0,0, te.te_Width,te.te_Height, screen, screen -> double_buffer_draw_frame, x , y + te.te_Extent.MinY);
				break;
		default:	retroBitmapBlit( font_render_rp.BitMap, 0,0, te.te_Width,te.te_Height, screen, 0, x , y + te.te_Extent.MinY);
				if (screen -> Memory[1]) retroBitmapBlit( font_render_rp.BitMap, 0,0, te.te_Width,te.te_Height, screen, 1, x , y + te.te_Extent.MinY);
				break;
	}
}


