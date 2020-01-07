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
#include "commandsBlitterObject.h"
#include "kittyErrors.h"
#include "engine.h"
#include "commandsBanks.h"

extern int sig_main_vbl;

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;
extern int priorityReverse;

 // extern int bobUpdateNextWait;

extern int current_screen;

extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern struct retroSprite *sprite;
extern struct retroSpriteObject bobs[64];

void copyScreenToClear( struct retroScreen *screen, struct retroSpriteClear *clear );
void copyClearToScreen( struct retroSpriteClear *clear, struct retroScreen *screen );


void clearBob(struct retroSpriteObject *bob)
{
	if (bob->screen_id<0) return;

	{
		struct retroScreen *screen = screens[bob->screen_id];
		struct retroSpriteClear *clear;

		if (screen == NULL) return;

		clear = &bob -> clear[ screen -> double_buffer_draw_frame ];

		if ((clear -> mem)&&(bob->background==0))
		{
			copyClearToScreen( clear,screen );
		}
		else
		{
			if (bob->background>0)
			{
				retroBAR( screen, screen -> double_buffer_draw_frame, clear -> x, clear -> y, clear->x +clear->w, clear->y+clear->h,bob->background-1);
			}
		}
	}
}

void clearBobs()
{
	int n;
	int start=0,end=0,dir=0;

	switch ( priorityReverse )
	{
		case 0:
			start = 63; end = -1; dir = -1;
			break;
		case 1:
			start = 0; end = 64; dir = 1;
			break;
	}

	for (n=start;n!=end;n+=dir)
	{
		clearBob(&bobs[n]);
	}
}

void clearBobsOnScreen(struct retroScreen *screen)
{
	int n;
	int start=0,end=0,dir=0;

	switch ( priorityReverse )
	{
		case 0:
			start = 63; end = -1; dir = -1;
			break;
		case 1:
			start = 0; end = 64; dir = 1;
			break;
	}

	for (n=start;n!=end;n+=dir)
	{
		if (screens[bobs[n].screen_id] == screen)
		{
			clearBob(&bobs[n]);
		}
	}
}



void freeScreenBobs( int screen_id )
{
	int n;
	struct retroSpriteObject *bob;

	for (n=0;n<64;n++)
	{
		bob = &bobs[n];

		bob -> x = 0;
		bob -> y = 0;
		bob -> image = 0;

		if (bob->screen_id == screen_id)
		{
			freeBobClear( bob );
		}
	}
}

void freeBobClear( struct retroSpriteObject *bob )
{
	struct retroSpriteClear *clear = bob -> clear;

	if (clear -> mem)
	{
		sys_free(clear -> mem);
		clear -> mem = NULL;
	}	
}

void copyScreenToClear( struct retroScreen *screen, struct retroSpriteClear *clear )
{
	bool newX = false;
	bool newY = false;
	int x,y;
	int destX=0;
	int destY=0;
	int _w = clear -> w;
	int _h = clear -> h;
	int x0 = clear -> x;
	int y0 = clear -> y;
	int x1 = x0 + clear -> w;
	int y1 = y0 + clear -> h;
	unsigned char *dest,*src;

	if (y0<0) { destY=-y0; y0=0; newY = true; }
	if (x0<0) { destX=-x0; x0=0; newX = true; }
	if (y1>screen->realHeight) { y1=screen->realHeight; newY=true; }
	if (x1>screen->realWidth) { x1=screen->realWidth; newX=true; }
	if (newX) _w = x1-x0;
	if (newY) _h = y1-y0;	

	src = screen -> Memory[ screen -> double_buffer_draw_frame ] + ( screen -> bytesPerRow * y0 ) + x0;
	dest = (unsigned char *) ( clear -> mem + (clear -> w * destY) + destX);

	for (y=y0; y<y1;y++)
	{
		for (x=0; x<_w;x++)
		{
			dest[x]=src[x];
		}
		dest += clear -> w;
		src += screen -> bytesPerRow;
	}
}

void copyClearToScreen( struct retroSpriteClear *clear, struct retroScreen *screen )
{
	bool newX = false;
	bool newY = false;
	int x,y;
	int destX=0;
	int destY=0;
	int _w = clear -> w;
	int _h = clear -> h;
	int x0 = clear -> x;
	int y0 = clear -> y;
	int x1 = x0 + clear -> w;
	int y1 = y0 + clear -> h;
	unsigned char *dest,*src;

	if (y0<0) { destY=-y0; y0=0; newY = true; }
	if (x0<0) { destX=-x0; x0=0; newX = true; }
	if (y1>screen->realHeight) { y1=screen->realHeight; newY=true; }
	if (x1>screen->realWidth) { x1=screen->realWidth; newX=true; }
	if (newX) _w = x1-x0;
	if (newY) _h = y1-y0;	

	dest = screen -> Memory[ screen -> double_buffer_draw_frame ] + ( screen -> bytesPerRow * y0 ) + x0;
	src = (unsigned char *) ( clear -> mem + (clear -> w * destY) + destX);

	for (y=y0; y<y1;y++)
	{
		for (x=0; x<_w;x++)
		{
			dest[x]= src[x];
		}
		src += clear -> w;
		dest += screen -> bytesPerRow;
	}
}

void drawBob(struct retroSpriteObject *bob)
{
	struct retroScreen *screen = screens[bob->screen_id];
	struct retroFrameHeader *frame;
	struct retroSpriteClear *clear;
	int image, flags;
	int size;

	if (screen)
	{
		image = (bob->image & 0x3FFFF) - 1;
		flags = bob -> image & 0xC000;

		if ( (image >= 0 ) && (image < sprite -> number_of_frames) )
		{
			frame = &sprite -> frames[ image ];

			clear = &bob -> clear[ screen -> double_buffer_draw_frame ];

			clear -> x = bob -> x - frame -> XHotSpot;
			clear -> y = bob -> y - frame -> YHotSpot;
			clear -> w = frame -> width;
			clear -> h = frame -> height;
			clear -> image = bob -> image;
			clear -> drawn = 1;

			if (bob-> background == 0)
			{
				size = clear -> w * clear -> h;

				if (clear -> mem)
				{
					sys_free(clear -> mem);
					clear -> mem = NULL;
				}

				if (size) 
				{
					clear -> mem = (char *) sys_public_alloc_clear(size);
					clear -> size = size;
				}

				if (clear -> mem) copyScreenToClear( screen,clear );
			}

			retroPasteSpriteObject(
				screen, 
				screen -> double_buffer_draw_frame, 
				bob, 
				sprite,
				image, 
				flags);
		}
	}
}

void drawBobs()
{
	int n;
	struct retroSpriteObject *bob;

	int start=0,end=0,dir=0;

	if (!sprite) return;
	if (!sprite -> frames) return;

	switch ( priorityReverse )
	{
		case 0:
			start = 0; end = 64; dir = 1;
			break;
		case 1:
			start = 63; end = -1; dir = -1;
			break;
	}

	for (n=start;n!=end;n+=dir)
	{
		bob = &bobs[n];
		drawBob(bob);
	}
}

void drawBobsOnScreen(struct retroScreen *screen)
{
	int n;
	struct retroSpriteObject *bob;

	int start=0,end=0,dir=0;

	if (!sprite) return;
	if (!sprite -> frames) return;

	switch ( priorityReverse )
	{
		case 0:
			start = 0; end = 64; dir = 1;
			break;
		case 1:
			start = 63; end = -1; dir = -1;
			break;
	}

	for (n=start;n!=end;n+=dir)
	{
		bob = &bobs[n];

		if (screens[bob->screen_id] == screen)
		{
			drawBob(bob);
		}
	}
}


char *_boBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num;
	int lx,ly,li;
	struct retroSpriteObject *bob;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 4:
			num = getStackNum( stack - 3 );
			bob = &bobs[num];

			lx =bob->x;
			ly =bob->y;
			li = bob -> image;

			stack_get_if_int( stack - 2 , &bob->x );
			stack_get_if_int( stack - 1 , &bob->y );

			bob->image = getStackNum( stack );
			bob->screen_id = current_screen;

			if (struct retroScreen *screen = screens[bob->screen_id])
			{
				if ((lx ^ bob -> x) | (ly ^ bob -> y) | ( li ^ bob -> image)) 		// xor should remove bits not changed, so if this has value its changed.
				{	
					screen -> event_flags |= rs_bob_moved;		// normaly, screen is swaped if bob is moved.
				}
			}

			break;

		default:
			setError(22, data->tokenBuffer);
	 }

	popStack( stack - data->stack );
	return NULL;
}

char *boBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _boBob, tokenBuffer );
	return tokenBuffer;
}

char *_boNoMask( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int image;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			image = getStackNum( stack )-1;

			if (sprite)
			{
				if (image < sprite -> number_of_frames)
				{
					struct retroFrameHeader *frame = &sprite -> frames[image];

					if (frame)
					{
						frame -> alpha = 0;
						retroFreeMask( frame );
						return NULL;
					}
				}
			}

			setError( 23, data -> tokenBuffer );
			break;
		default:
			popStack( stack - data->stack );
			setError(22, data -> tokenBuffer);
	}
	return NULL;
}

char *boNoMask(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _boNoMask, tokenBuffer );
	return tokenBuffer;
}


char *_boSetBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroSpriteObject *bob;
	int n;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 4:	n = getStackNum(stack-3);
				bob = &bobs[ n & 63 ];
				bob -> background = getStackNum(stack-2);
				bob -> plains = getStackNum(stack-1);
				bob -> mask = getStackNum(stack);
				break;
		default:
				setError(22, data-> tokenBuffer);
				break;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *boSetBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _boSetBob, tokenBuffer );
	return tokenBuffer;
}

char *_boXBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int n;
		n = getStackNum(stack);
		x= bobs[ n & 63 ].x;
	} else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum(x);
	return NULL;
}

char *boXBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _boXBob, tokenBuffer );
	return tokenBuffer;
}

char *_boYBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int y=0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int n;
		n = getStackNum(stack);
		y= bobs[ n & 63 ].y;

	} else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum(y);
	return NULL;
}

char *boYBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _boYBob, tokenBuffer );
	return tokenBuffer;
}

char *_boIBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int i=0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int n;
		n = getStackNum(stack);
		i= bobs[ n & 63 ].image;

		if (i<0) setError(23,data->tokenBuffer);

	} else setError(23,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum(i);
	return NULL;
}

char *boIBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _boIBob, tokenBuffer );
	return tokenBuffer;
}

char *_boPasteBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = NULL;
	int hx=0,hy=0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:	// past bob x,y,i

				screen = screens[current_screen];
				if (( screen ) && (sprite))
				{
					int x = getStackNum( stack-2 );
					int y = getStackNum( stack-1 );
					int image = getStackNum( stack );
					int flags = image & 0xC000;
					image &= 0x3FFF;

					if ((image) && (sprite))	// PasteBob should not use hotspot, need subtract it.
					{
						hx=-sprite -> frames[image-1].XHotSpot;
						hy=-sprite -> frames[image-1].YHotSpot;
					}

					switch (screen -> autoback)
					{
						case 0:	retroPasteSprite(screen,screen -> double_buffer_draw_frame,sprite,x-hx,y-hy,image-1,flags, 0 );
								break;

						default:	retroPasteSprite(screen,0,sprite,x-hx,y-hy,image-1,flags, 0 );
								if (screen -> Memory[1]) retroPasteSprite(screen,1,sprite,x,y,image-1,flags, 0 );
								break;
					}

				}
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *boPasteBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _boPasteBob, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_boGetBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = NULL;
	int screen_nr = 0;
	int image = 0;
	int x0 = 0;
	int y0 = 0;
	int x1 = 0;
	int y1 = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 5:	// get bob i,x,y to x2,y2

				image = getStackNum( stack-4 );
				x0 = getStackNum( stack-3 );
				y0 = getStackNum( stack-2 );
				x1 = getStackNum( stack-1 );
				y1 = getStackNum( stack );
				screen = screens[current_screen];
				break;

		case 6:	// get bob s,i,x,y to x2,y2

				screen_nr = getStackNum( stack-5 );
				image = getStackNum( stack-4 );
				x0 = getStackNum( stack-3 );
				y0 = getStackNum( stack-2 );
				x1 = getStackNum( stack-1 );
				y1 = getStackNum( stack );

				if ((screen_nr > -1) && (screen_nr < 8)) screen = screens[ screen_nr ];
				break;
	 }

	if (screen)
	{
		struct kittyBank *bank1;

		if (sprite==NULL)
		{
			sprite = (struct retroSprite *) sys_public_alloc_clear( sizeof(struct retroSprite) );

			bank1 = findBank(1);
			if (!bank1) 
			{
				if (bank1 = __ReserveAs( bank_type_sprite, 1,0,NULL, NULL))							
				{
					int n;

					// we only copy palette if the bob/sprite is new.
					struct retroRGB *color = screen->orgPalette;
					for (n=0;n<256;n++) sprite -> palette[n] = color[n];

					bank1 -> object_ptr = (char *) sprite;
				} 
			}
		}

		if (sprite)
		{
			engine_lock();
			retroGetSprite(screen,sprite,image-1,x0,y0,x1,y1);
			sprite -> frames[image-1].alpha  = 1;
			retroMakeMask( &sprite -> frames[ image-1 ] );
			engine_unlock();

			bank1 = findBank(1);
			if (bank1) 
			{
				if (bank1 -> object_ptr == (char *) sprite)							
				{
					bank1 -> length = sprite -> number_of_frames;
				} 
			}
		}

	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *boGetBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _boGetBob, tokenBuffer );
	return tokenBuffer;
}

char *_boPutBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int n,image,flags;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	n = getStackNum( stack );
				image  = bobs[ n & 63 ].image;
				flags = image & 0xC000;
				image &= image & 0x3FFF;

				screen = screens[current_screen];

				if (screen)	
				{
					switch (screen -> autoback)
					{
						case 0:	retroPasteSprite(screen,screen -> double_buffer_draw_frame,sprite,
									bobs[ n & 63 ].x,bobs[ n & 63 ].y,image -1, flags, bobs[ n & 63].plains);
								break;

						default:	retroPasteSprite(screen,0,sprite,bobs[ n & 63 ].x,bobs[ n & 63 ].y,image -1, flags, bobs[ n & 63].plains);
								if (screen -> Memory[1]) retroPasteSprite(screen,1,sprite,bobs[ n & 63 ].x,bobs[ n & 63 ].y,image -1, flags, bobs[ n & 63].plains);
								break;
					}
				}
				break;
		default:
				setError(22, data -> tokenBuffer);
				break;
	 }


	popStack( stack - data->stack );
	return NULL;
}

char *boPutBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _boPutBob, tokenBuffer );
	return tokenBuffer;
}

char *_boHotSpot( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int image;
	int p;
	int x,y;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (!sprite)
	{
		popStack( stack - data->stack );
		setError(36,data->tokenBuffer);
		return NULL;
	}


	printf("sprite -> number_of_frames: %d\n",sprite -> number_of_frames);

	switch (args)
	{
		case 2:
				image = getStackNum( stack-1 )-1;
				p = getStackNum( stack );

				if (sprite)
				{
					if (image < sprite -> number_of_frames)
					{
						struct retroFrameHeader *frame = &sprite -> frames[image];

						if (frame)
						{
							x = (p >> 4) & 0xF;
							y = p & 0xF;
							frame -> XHotSpot = (x * frame -> width) >> 1;
							frame -> YHotSpot = (y * frame -> height) >> 1;
							success = true;
						}
					}
				}

				 if (success == false) setError( 23, data -> tokenBuffer );
				break;

		case 3:
				image = getStackNum( stack-2 )-1;
				x = getStackNum( stack-1 );
				y = getStackNum( stack );

				if (sprite)
				{
					if (image < sprite -> number_of_frames)
					{
						struct retroFrameHeader *frame = &sprite -> frames[image];

						if (frame)
						{
							frame -> XHotSpot = x;
							frame -> YHotSpot = y;
							success = true;
						}
					}
				}

				 if (success == false) setError( 23, data -> tokenBuffer );
				break;

		default:
				printf("args: %d\n",args);
				setError(22,data->tokenBuffer);

	}


	popStack( stack - data->stack );
	return NULL;
}

char *boHotSpot(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _boHotSpot, tokenBuffer );
	return tokenBuffer;
}

char *_boLimitBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int n;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

//	NYI(__FUNCTION__);

	switch (args)
	{
		case 1:

			if (kittyStack[stack].type == type_none)		// delete all limits from bobs
			{
				for (n=0;n<63;n++)
				{
					bobs[n].limitXmin = 0;
					bobs[n].limitYmin = 0;
					bobs[n].limitXmax = 0;
					bobs[n].limitYmax = 0;
				}
			}
			else	// delete limit from one bob (not supported by Amos Pro)
			{
				n = getStackNum(stack);
				bobs[n].limitXmin = 0;
				bobs[n].limitYmin = 0;
				bobs[n].limitXmax = 0;
				bobs[n].limitYmax = 0;
			}
			break;

		case 4:	// limit bob x0,y0 to x1,y1
			{
				int y0 = getStackNum(stack-3);
				int x0 = getStackNum(stack-2);
				int x1 = getStackNum(stack-1);
				int y1 = getStackNum(stack);

				for (n=0;n<63;n++)	// 0-63 is a amos the creator limit, not a amos pro limit.
				{
					bobs[n].limitXmin = x0;
					bobs[n].limitYmin = y0;
					bobs[n].limitXmax = x1;
					bobs[n].limitYmax = y1;
				}
			}
			break;

		case 5:	// limit bob <bob>,x0,y0 to x1,y1

			{
				n = getStackNum(stack-4);
				int y0 = getStackNum(stack-3);
				int x0 = getStackNum(stack-2);
				int x1 = getStackNum(stack-1);
				int y1 = getStackNum(stack);

				bobs[n].limitXmin = x0;
				bobs[n].limitYmin = y0;
				bobs[n].limitXmax = x1;
				bobs[n].limitYmax = y1;
			}
			break;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *boLimitBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _boLimitBob, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_boHrev( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		ret = getStackNum(stack) | 0x8000;
	} else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *boHrev(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _boHrev, tokenBuffer );
	return tokenBuffer;
}

char *_boVrev( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		ret = getStackNum(stack) | 0x4000;
	} else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *boVrev(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _boVrev, tokenBuffer );
	return tokenBuffer;
}

char *_boRev( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		ret = getStackNum(stack) | 0xC000;
	} else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *boRev(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _boRev, tokenBuffer );
	return tokenBuffer;
}

char *boBobUpdateOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	engine_update_flags &= ~rs_bob_moved;	// disable update on bob move.
	return tokenBuffer;
}

char *boBobUpdateOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	engine_update_flags |= rs_bob_moved;	// enable update on bob move.
	return tokenBuffer;
}

extern void __wait_vbl();

char *boBobUpdate(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	engine_lock();	
	if (screens[current_screen])
	{
		screens[current_screen] -> event_flags |= rs_force_swap;
	}
	engine_unlock();

	return tokenBuffer;
}

int cmpMask( struct retroMask *leftMask, struct retroMask *rightMask, int offInt16, int lshift, int dy )
{
	int xx,yy;
	uint16_t *ptrLeft;
	uint16_t *ptrRight;
	uint16_t shiftbits;
	uint16_t rshift;
	uint16_t bitMask;
	int leftYStart, rightYStart;
	uint16_t leftHeight, rightHeight;
	uint16_t *rowLeft;
	uint16_t *rowRight;
	int maxHeight;
	bitMask = (1<<lshift)-1;
	rshift = 15 - lshift;
	leftYStart = 0;
	rightYStart = 0;
	leftHeight = leftMask -> height - rightYStart;
	rightHeight = rightMask -> height - leftYStart;

	if (dy>0)
	{
		leftYStart = dy;
		rightYStart = -leftYStart;		// subtract offset from right

		rightHeight = rightMask -> height - leftYStart;
		leftHeight = leftMask -> height - leftYStart;
	}
	else
	{
		rightYStart = -dy;
		rightHeight = rightMask -> height - rightYStart;
	}

	maxHeight = leftHeight < rightHeight ? leftHeight : rightHeight;		// pick the smallest hight to be the max height.

	for (yy=leftYStart;yy<leftYStart+maxHeight;yy++)
	{
		shiftbits =0;
		rowLeft = leftMask -> data + (yy*leftMask->int16PerRow);
		rowRight =rightMask -> data + ((yy+rightYStart)*rightMask->int16PerRow);

		for (xx=offInt16;xx < leftMask -> int16PerRow;xx++)
		{
			ptrLeft = rowLeft + xx;
			ptrRight = rowRight +xx - offInt16;
			if (*ptrLeft & ((*ptrRight >> lshift) | shiftbits))	return true;

//			if (*ptrLeft & ((*ptrRight >> lshift) | shiftbits)) ret=true;
//			retroDrawShortPlanar( screens[1], *ptrLeft | (*ptrRight >> lshift) | shiftbits ,xx*16,yy);

			shiftbits = (*ptrRight & bitMask) << rshift;
		}
	}

//	return ret;

	return false;
}


int inBob( struct retroMask *thisMask, int minX,int minY, int maxX, int maxY, struct retroSpriteObject *otherBob )
{
	if (otherBob -> image)
	{
		struct retroFrameHeader * frame = &sprite -> frames[ otherBob -> image -1 ];
		int ominX = otherBob -> x - frame -> XHotSpot;
		int ominY = otherBob -> y - frame -> XHotSpot;
		int omaxX = ominX + frame -> width;
		int omaxY = ominY + frame -> height;	

		if ( maxX < ominX ) return 0;
		if ( minX > omaxX ) return 0;
		if ( maxY < ominY ) return 0;
		if ( maxY > omaxY ) return 0;

		if (minX< ominX)
		{
			int dx = ominX - minX;
			int dy = ominY - minY;
			int bitx = dx & 15;
			dx = dx >> 4;

			if (cmpMask( thisMask, frame -> mask, dx, bitx, dy ))
			{
				return ~0;
			}
			else return 0;
		}
		else
		{
			int dx = minX - ominX;
			int dy = minY - ominY;

			int bitx = dx & 15;
			dx = dx >> 4;

			if (cmpMask(  frame-> mask, thisMask, dx, bitx, dy ))
			{
				return ~0;
			}
			else return 0;
		}

		return ~0;
	}
	return 0;
}


int bobCol( unsigned short bob, unsigned short start, unsigned short end )
{
	struct retroSpriteObject *thisBob;
	struct retroSpriteObject *otherBob;
	struct retroFrameHeader *frame;
	int minX, maxX, minY, maxY;
	int n,r;

	if (bob & 0xFFC0 ) return 0;		// 0 to 63  (0x3F)
	if (start & 0xFFC0 ) return 0;		// 0 to 63, 
	if (end & 0xFFC0 ) return 0;		// 0 to 63, 

	thisBob = &bobs[bob];
	if (thisBob -> image == 0) return 0;

	frame = &sprite -> frames[ thisBob -> image-1 ];
	minX = thisBob -> x - frame -> XHotSpot;
	minY = thisBob -> y - frame -> XHotSpot;
	maxX = minX + frame -> width;
	maxY = minY + frame -> height;

//	retroBox( screens[current_screen], 0, minX,minY,maxX,maxY,1 );

	for (n=start;n<=end;n++)
	{
		otherBob = &bobs[n];

		if (otherBob != thisBob)
		{
			if (otherBob -> image) // is valid bob
			{
				r = inBob( frame -> mask, minX,minY,maxX,maxY, otherBob );
				if (r) return r;
			}
		}
	}

	return 0;
}

char *_boBobCol( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int bob = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	bob = getStackNum(stack);
				setStackNum(bobCol( bob, 0, 63 ));	
				return NULL;
	}

	printf("Error Error robinson... %d\n",args);

	popStack( stack - data->stack );
	setStackNum(0);			
	return NULL;
}

char *boBobCol(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _boBobCol, tokenBuffer );
	return tokenBuffer;
}

char *_boCol( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *boCol(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _boCol, tokenBuffer );
	return tokenBuffer;
}

bool del_sprite_object( struct retroSprite *objList, int del)
{
	printf("Del: %d Items %d\n",del, objList->number_of_frames);

	if (objList == NULL)
	{
		printf("no object, so can't delete frame\n");
		return false;
	}

	if ((del>-1)&&(del<objList->number_of_frames))
	{
		int f;

		if (objList -> frames[del].data) 
		{
			sys_free(objList -> frames[del].data);
			objList -> frames[del].data = NULL;
		}

		for (f=del+1;f<objList->number_of_frames;f++)
		{
			printf("move %d to %d\n",f,f-1);
			objList -> frames[f-1] = objList -> frames[f];
		}
		objList->number_of_frames--;

		return true;
	}

	return false;
}

char *_boDelBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int del = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		del = getStackNum(stack);
		del_sprite_object(sprite, del-1);
	}
	else setError(22, data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *boDelBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _boDelBob, tokenBuffer );
	return tokenBuffer;
}

char *boBobClear(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	
	engine_lock();
	clearBobs();
	engine_unlock();
	return tokenBuffer;
}

char *boBobDraw(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	engine_lock();	
	drawBobs();
	engine_unlock();

	return tokenBuffer;
}


char *_boBobOff( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int del = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		del = getStackNum(stack);

		if ((del>-1)&&(del<64))
		{
			engine_lock();

			clearBob(&bobs[del]);

			if (bobs[del].clear[0].mem) sys_free(bobs[del].clear[0].mem);
			bobs[del].clear[0].mem = NULL;

			bobs[del].image = -1;
			engine_unlock();
		}
	}
	else setError(22, data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *boBobOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _boBobOff, tokenBuffer );
	return tokenBuffer;
}


void makeMaskForAll()
{
	int n;

	if (sprite == NULL) return;

	for (n=0;n<sprite -> number_of_frames;n++)
	{
		retroMakeMask( &sprite -> frames[n] );
	}
}

char *_boMakeMask( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:

			switch (kittyStack[stack].type)
			{
				case type_none:
					makeMaskForAll();
					break;

				case type_int:
					retroMakeMask( &sprite -> frames[ kittyStack[stack].integer.value ] );
					break;
			}

			break;
		default:
			setError(22,data->tokenBuffer);;
			break;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *boMakeMask(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _boMakeMask, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

