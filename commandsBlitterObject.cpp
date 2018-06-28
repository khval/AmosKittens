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
#include "commandsGfx.h"
#include "errors.h"
#include "engine.h"

extern int sig_main_vbl;

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

extern int current_screen;


extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern struct retroSprite *sprite;
extern struct retroSpriteObject bobs[64];

void clearBobs()
{
	int n;
	struct retroScreen *screen;
	struct retroSpriteObject *bob;
	struct retroFrameHeader *frame;
	struct retroSpriteClear *clear;
	int image;
	int x1,y1,x2,y2;

	if (!sprite) return;
	if (!sprite -> frames) return;

	for (n=0;n<64;n++)
	{
		bob = &bobs[n];
		screen = screens[bob->screen_id];

		if (screen)
		{
			clear = &bob -> clear[ screen -> double_buffer_draw_frame ];

			if (clear -> drawn == 1)
			{
				x1 = clear -> x;
				y1 = clear -> y;
				image = clear -> image;
				clear -> drawn = 0;

				if ((image<0) || (image >= sprite -> number_of_frames)) image = -1;	

				if ( image > -1 )
				{
					frame = &sprite -> frames[ image ];

						x1 -= frame -> XHotSpot;
						y1 -= frame -> YHotSpot;
						x2 = x1 + frame -> Width;
						y2 = y1 + frame -> Height;

//					printf("%d, retroBAR(%08x,%d,%d,%d,%d)\n",n ,screen, x1,y1,x2,y2,screen -> double_buffer_draw_frame);	

						retroBAR( screen, x1,y1,x2,y2,screen -> double_buffer_draw_frame );
				}
			}
		}
	}
}


void drawBobs()
{
	int n;
	struct retroScreen *screen;
	struct retroSpriteObject *bob;
	struct retroSpriteClear *clear;
	int image;

	if (!sprite) return;

	for (n=0;n<64;n++)
	{
		bob = &bobs[n];
		screen = screens[bob->screen_id];

		if (screen)
		{
			image = bob->image;

			if ((image<0) || (image >= sprite -> number_of_frames)) image = -1;	

			if (( image > -1 ) && (sprite -> frames))
			{
				printf("%d retroPasteSprite(%08x,%08x,%d,%d,%d)\n",n, screen, sprite, bob->x, bob->y, image);	

				clear = &bob -> clear[ screen -> double_buffer_draw_frame ];

				clear -> x = bob -> x;
				clear -> y = bob -> y;
				clear -> image = bob -> image;
				clear -> drawn = 1;

				retroPasteSprite(screen, sprite, bob->x, bob->y, image);
			}
		}
	}
}

char *_boBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num;
	struct retroSpriteObject *bob;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	num = getStackNum( stack - 3 );
	bob = &bobs[num];

	stack_get_if_int( stack - 2 , &bob->x );
	stack_get_if_int( stack - 1 , &bob->y );

	bob->image = getStackNum( stack );

	bob->screen_id = current_screen;

	printf( "bob %d,%d,%d,%d\n", num,bob->x,bob->y,bob->image);

	popStack( stack - data->stack );
	return NULL;
}

char *boBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _boBob, tokenBuffer );
	return tokenBuffer;
}

char *boBobOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *_boNoMask( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *boNoMask(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _boNoMask, tokenBuffer );
	return tokenBuffer;
}

