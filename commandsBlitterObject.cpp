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
#include "errors.h"
#include "engine.h"
#include "commandsBanks.h"

extern int sig_main_vbl;

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;
extern int priorityReverse;
extern int bobUpdate;

extern int current_screen;

extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern struct retroSprite *sprite;
extern struct retroSpriteObject bobs[64];

void copyScreenToClear( struct retroScreen *screen, struct retroSpriteClear *clear );
void copyClearToScreen( struct retroSpriteClear *clear, struct retroScreen *screen );

void clearBobs()
{
	int n;
	struct retroScreen *screen;
	struct retroSpriteObject *bob;
	struct retroSpriteClear *clear;

	if (!sprite) return;
	if (!sprite -> frames) return;

	int start=0,end=0,dir=0;

	if (!sprite) return;

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
		bob = &bobs[n];
		screen = screens[bob->screen_id];

		if (screen)
		{
			clear = &bob -> clear[ 0 ];

			if (clear -> mem)
			{
				copyClearToScreen( clear,screen );
			}
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

void drawBobs()
{
	int n;
	struct retroScreen *screen;
	struct retroSpriteObject *bob;
	struct retroFrameHeader *frame;
	struct retroSpriteClear *clear;
	int image, flags;
	int size;
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

		screen = screens[bob->screen_id];

		if (screen)
		{
			image = (bob->image & 0x3FFFF) - 1;
			flags = bob -> image & 0xC000;

			if ( (image >= 0 ) && (image < sprite -> number_of_frames) )
			{
				frame = &sprite -> frames[ image ];

				clear = &bob -> clear[ 0 ];

				clear -> x = bob -> x - frame -> XHotSpot;
				clear -> y = bob -> y - frame -> YHotSpot;
				clear -> w = frame -> Width;
				clear -> h = frame -> Height;
				clear -> image = bob -> image;
				clear -> drawn = 1;

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
				retroPasteSprite(screen, sprite, bob->x, bob->y, image, flags);
			}
		}
	}
}

char *_boBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num;
	struct retroSpriteObject *bob;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	num = getStackNum( stack - 3 );
	bob = &bobs[num];

	stack_get_if_int( stack - 2 , &bob->x );
	stack_get_if_int( stack - 1 , &bob->y );

	bob->image = getStackNum( stack );
	bob->screen_id = current_screen;


	if (screens[current_screen])
	{
		screens[current_screen] -> force_swap = TRUE;
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

char *boBobOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *_boNoMask( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *boSetBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _boNoMask, tokenBuffer );
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

char *_boPasteBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:	// past bob x,y,i
				if ( screens[current_screen] )
				{
					int x = getStackNum( stack-2 );
					int y = getStackNum( stack-1 );
					int image = getStackNum( stack );
					int flags = image & 0xC000;
					image &= 0x3FFF;

					retroPasteSprite(screens[current_screen],sprite,x,y,image-1,flags);
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
		if (sprite==NULL)
		{
			struct kittyBank *bank1;
			sprite = (struct retroSprite *) sys_public_alloc_clear( sizeof(struct retroSprite) );

			bank1 = findBank(1);
			if (!bank1) 
			{
				if (bank1 = __ReserveAs( bank_type_sprite, 1, sizeof(void *),NULL, NULL))							
				{
					bank1 -> object_ptr = (char *) sprite;
				} 
			}
		}

		if (sprite)
		{
			engine_lock();
			retroGetSprite(screen,sprite,image-1,x0,y0,x1,y1);
			engine_unlock();
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

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	n = getStackNum( stack );
				image  = bobs[ n & 63 ].image;
				flags = image & 0xC000;
				image &= image & 0x3FFF;

				retroPasteSprite(screens[current_screen],sprite,
						bobs[ n & 63 ].x,
						bobs[ n & 63 ].y,
						image -1, flags);
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
							frame -> XHotSpot = (x * frame -> Width) >> 1;
							frame -> YHotSpot = (y * frame -> Height) >> 1;
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

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	dump_stack();

	popStack( stack - data->stack );
	return NULL;
}

char *boLimitBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _boLimitBob, tokenBuffer );
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
	bobUpdate = 0;
	return tokenBuffer;
}

char *boBobUpdate(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}


char *_boBobCol( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	setStackNum(ret);
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

char *_boDelBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int del = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		del = getStackNum(stack);

		if ((sprite->number_of_frames)&&(del<sprite->number_of_frames))
		{
			int f;

			if (sprite -> frames[del].data) sys_free(sprite -> frames[del].data);

			for (f=sprite->number_of_frames-1;f>del;f--)
			{
				sprite -> frames[f-1] = sprite -> frames[f];
			}
			sprite->number_of_frames--;
		}
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

