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

void copyToClear( char *src,char *dest , int x0, int y0, int x1, int y1, int srcbpr, int destbpr);
void copyClearTo( char *dest,char *src , int x0, int y0, int x1, int y1, int destbpr, int srcbpr);

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

	for (n=63;n>-1;n--)
	{
		bob = &bobs[n];
		screen = screens[bob->screen_id];

		if (screen)
		{
			clear = &bob -> clear[ 0 ];

			if (clear -> mem)
			{
				copyClearTo(
					(char *) screen -> Memory[ screen -> double_buffer_draw_frame ], 
					clear -> mem , 
					clear -> x, clear -> y, 
					clear -> x + clear -> w, clear -> y + clear -> h, 
					screen -> bytesPerRow, clear -> w);
			}
		}
	}
}

void copyToClear( char *src,char *dest , int x0, int y0, int x1, int y1, int srcbpr, int destbpr)
{
	int x,y;
	int destX=0;
	int destY=0;
	
	if (y0<0) { destY=-y0; y0=0; }
	if (x0<0) { destX=-x0; x0=0; }

	dest += (destbpr * destY);
	src += (srcbpr * y0) ;

	for (y=y0; y<y1;y++)
	{
		for (x=x0; x<x1;x++)
		{
			dest[x-x0]=src[x];
		}
		dest += destbpr;
		src += srcbpr;
	}
}

void copyClearTo( char *src,char *dest , int x0, int y0, int x1, int y1, int srcbpr, int destbpr)
{
	int x,y;
	int srcX=0;
	int srcY=0;
	
	if (y0<0) { srcY=-y0; y0=0; }
	if (x0<0) { srcX=-x0; x0=0; }

	src += (srcbpr * y0);
	dest += (destbpr * srcY) ;

	for (y=y0; y<y1;y++)
	{
		for (x=x0; x<x1;x++)
		{
			src[x]=dest[x-x0];
		}
		src += srcbpr;
		dest += destbpr;
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

	if (!sprite) return;

	for (n=0;n<64;n++)
	{
		bob = &bobs[n];
		screen = screens[bob->screen_id];

		if (screen)
		{
			image = bob->image & 0x3FFFF;
			flags = bob -> image & 0xC000;

			if ((image<0) || (image >= sprite -> number_of_frames)) image = -1;	

			if ( (image > -1) && (sprite -> frames) )
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

				if (size > clear -> size) 
				{
					if (clear -> mem) FreeVec(clear -> mem);

					clear -> mem = (char *) AllocVecTags( size, 
							AVT_Type, MEMF_PRIVATE, 
							AVT_ClearWithValue, 0,
							TAG_END );

					clear -> size = size;
				}

				if (clear -> mem)
				{
					copyToClear(
						(char *) screen -> Memory[screen -> double_buffer_draw_frame], 
						clear -> mem , 
						clear -> x, clear -> y, 
						clear -> x + clear -> w, clear -> y + clear -> h, 
						screen -> bytesPerRow, clear -> w);
				}

				retroPasteSprite(screen, sprite, bob->x, bob->y, image-1, flags);
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


char *_boSetBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *boSetBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _boNoMask, tokenBuffer );
	return tokenBuffer;
}

char *_boXBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x = 0;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int n;
		n = getStackNum(stack);
		x= bobs[ n & 63 ].x;
	} else setError(22);

	popStack( stack - data->stack );
	setStackNum(x);
	return NULL;
}

char *boXBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _boXBob, tokenBuffer );
	return tokenBuffer;
}

char *_boYBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int y=0;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int n;
		n = getStackNum(stack);
		y= bobs[ n & 63 ].y;

	} else setError(22);

	popStack( stack - data->stack );
	setStackNum(y);
	return NULL;
}

char *boYBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _boYBob, tokenBuffer );
	return tokenBuffer;
}

char *_boPasteBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num;
	struct retroSpriteObject *bob;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	switch (args)
	{
		case 3:	// past bob x,y,i
				{
					int x = getStackNum( stack-2 );
					int y = getStackNum( stack-1 );
					int image = getStackNum( stack );
					int flags = image & 0xC000;
					image &= 0x3FFF;

					printf("PasteSprite %d,%d,%d\n",x,y,image);
					retroPasteSprite(screens[current_screen],sprite,x,y,image-1,flags);
				}
				break;
		default:
				setError(22);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *boPasteBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _boPasteBob, tokenBuffer );
	return tokenBuffer;
}

char *_boGetBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num;
	struct retroSpriteObject *bob;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 5:	// get bob i,x,y to x2,y2
				{
					int image = getStackNum( stack-4 );
					int x0 = getStackNum( stack-3 );
					int y0 = getStackNum( stack-2 );
					int x1 = getStackNum( stack-1 );
					int y1 = getStackNum( stack );

					retroGetSprite(screens[current_screen],sprite,image-1,x0,y0,x1,y1);
				}
				break;

		case 6:	// get bob s,i,x,y to x2,y2
				break;
	 }

	popStack( stack - data->stack );
	return NULL;
}

char *boGetBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _boGetBob, tokenBuffer );
	return tokenBuffer;
}

char *_boPutBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int n;
	int image,flags;
	struct retroSpriteObject *bob;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

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
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _boPutBob, tokenBuffer );
	return tokenBuffer;
}

char *_boHotSpot( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int image;
	int p;
	int x,y;
	struct retroSpriteObject *bob;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	dump_stack();

	switch (args)
	{
		case 2:
				image = getStackNum( stack-1 );
				p = getStackNum( stack );

				if (sprite)
				{
					struct retroFrameHeader *frame = &sprite -> frames[image];
					x = (p >> 4) & 0xF;
					y = p & 0xF;
					frame -> XHotSpot = (x * frame -> Width) >> 1;
					frame -> YHotSpot = (y * frame -> Height) >> 1;
				}

				break;

		case 3:
				image = getStackNum( stack-2 );
				x = getStackNum( stack-1 );
				y = getStackNum( stack );

				if (sprite)
				{
					struct retroFrameHeader *frame = &sprite -> frames[image];
					frame -> XHotSpot = x;
					frame -> YHotSpot = y;
				}
				break;

	}


	popStack( stack - data->stack );
	return NULL;
}

char *boHotSpot(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _boHotSpot, tokenBuffer );
	return tokenBuffer;
}
char *_boLimitBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num;
	struct retroSpriteObject *bob;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	dump_stack();

	popStack( stack - data->stack );
	return NULL;
}

char *boLimitBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _boLimitBob, tokenBuffer );
	return tokenBuffer;
}

char *_boHrev( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		ret = getStackNum(stack) | 0x8000;
	} else setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *boHrev(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _boHrev, tokenBuffer );
	return tokenBuffer;
}

char *_boVrev( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		ret = getStackNum(stack) | 0x4000;
	} else setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *boVrev(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _boVrev, tokenBuffer );
	return tokenBuffer;
}

char *_boRev( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		ret = getStackNum(stack) | 0xC000;
	} else setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *boRev(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _boRev, tokenBuffer );
	return tokenBuffer;
}

