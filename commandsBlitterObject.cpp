#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <limits.h>

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

#include "AmalCompiler.h"
#include "channel.h"

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
extern std::vector<struct retroSpriteObject *> bobs;
extern ChannelTableClass *channels;

std::vector<int> collided;

void copyScreenToClear( struct retroScreen *screen, struct retroSpriteClear *clear );
void copyClearToScreen( struct retroSpriteClear *clear, struct retroScreen *screen );

struct retroSpriteObject *getBob(unsigned int id)
{
	for (std::vector<struct retroSpriteObject * >::iterator bob=bobs.begin();bob != bobs.end(); ++bob)
	{
		if ((*bob) -> id == id) return *bob;
	}
	return NULL;
}

struct retroSpriteObject *getBobOnScreen(unsigned int id,int screen)
{
	for (std::vector<struct retroSpriteObject * >::iterator bob=bobs.begin();bob != bobs.end(); ++bob)
	{
		if ((*bob) -> screen_id != screen ) continue;
		if ((*bob) -> id == id) return *bob;
	}
	return NULL;
}

int getBobX(unsigned int id)
{
	for (std::vector<struct retroSpriteObject * >::iterator bob=bobs.begin();bob != bobs.end(); bob++)
	{
		if ((*bob) -> id == id) return (*bob) -> x;
	}
	return 0;
}


int getBobY(unsigned int id)
{
	for (std::vector<struct retroSpriteObject * >::iterator bob=bobs.begin();bob != bobs.end(); bob++)
	{
		if ((*bob) -> id == id) return (*bob) -> y;
	}
	return 0;
}

int getBobImage(unsigned int id)
{
	for (std::vector<struct retroSpriteObject * >::iterator bob=bobs.begin();bob != bobs.end(); bob++)
	{
		if ((*bob) -> id == id) return (*bob) -> image;
	}
	return 0;
}

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
			start = bobs.size()-1; end = -1; dir = -1;
			break;
		case 1:
			start = 0; end = bobs.size(); dir = 1;
			break;
	}

	for (n=start;n!=end;n+=dir)
	{
		clearBob(bobs[n]);
	}
}

void clearBobsOnScreen(struct retroScreen *screen)
{
	int n;
	int start=0,end=0,dir=0;

	switch ( priorityReverse )
	{
		case 0:
			start = bobs.size()-1; end = -1; dir = -1;
			break;
		case 1:
			start = 0; end = bobs.size(); dir = 1;
			break;
	}

	for (n=start;n!=end;n+=dir)
	{
		if (screens[bobs[n] -> screen_id] == screen)
		{
			clearBob(bobs[n]);
		}
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
	int start=0,end=0,dir=0;

	if (!sprite) return;
	if (!sprite -> frames) return;

	switch ( priorityReverse )
	{
		case 0:
			start = 0; end = bobs.size(); dir = 1;
			break;
		case 1:
			start = bobs.size()-1; end = -1; dir = -1;
			break;
	}

	for (n=start;n!=end;n+=dir) drawBob(bobs[n]);
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
			start = 0; end = bobs.size(); dir = 1;
			break;
		case 1:
			start = bobs.size()-1; end = -1; dir = -1;
			break;
	}

	for (n=start;n!=end;n+=dir)
	{
		bob = bobs[n];

		if (screens[bob->screen_id] == screen)
		{
			drawBob(bob);
		}
	}
}


struct retroSpriteObject *__new_bob__(int id)
{
	struct retroSpriteObject *bob = new retroSpriteObject;

	if (bob)
	{
		bob -> id = id;
		bob -> x = 0;
		bob -> y = 0;
		bob -> image = 0;
		bob -> screen_id = current_screen;
		bob -> sprite = NULL;
		bob -> frame = NULL;
		bob -> clear[0].mem = NULL;
		bob -> clear[1].mem = NULL;
		bob -> mask = 0;
		bob -> limitXmin = 0;
		bob -> limitYmin = 0;
		bob -> limitXmax = 0;
		bob -> limitYmax = 0;
		bob -> background = 0;	// if background color is set, background is not copied.

		engine_lock();				
		bobs.push_back( bob );
		engine_unlock();
	}

	return bob;
}


void freeBobClear( struct retroSpriteObject *bob )
{
	if (bob->clear[0].mem) sys_free(bob->clear[0].mem);
	bob->clear[0].mem = NULL;

	if (bob->clear[1].mem) sys_free(bob->clear[1].mem);
	bob->clear[1].mem = NULL;

}



void __erase_bob__(struct retroSpriteObject *bob)
{
	unsigned int n;

	printf( "%s:%d\n",__FUNCTION__,__LINE__);

	freeBobClear( bob );

	for (n=0;n<bobs.size();n++)
	{
		if (bobs[n] == bob)
		{
			delete bob;
			bobs[n] = NULL;
			bobs.erase(bobs.begin()+n);
			return;
		}
	}
}


void freeScreenBobs(int screen_id)
{
	unsigned int n;
	for (n=bobs.size();n>0;)
	{
		n--;
		if (bobs[n] -> screen_id == screen_id)
		{
			printf("erase obj index %d - id: %d\n", n, bobs[n] -> id);

			__erase_bob__( bobs[n] );
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
			bob = getBob(num);

			if (!bob) bob = __new_bob__(num);

			if (bob)
			{
				lx =bob -> x;
				ly =bob -> y;
				li = bob -> image;

				stack_get_if_int( stack - 2 , &(bob->x) );
				stack_get_if_int( stack - 1 , &(bob->y) );

				bob->image = getStackNum( stack );

				if (struct retroScreen *screen = screens[bob->screen_id])
				{
					if ((lx ^ bob -> x) | (ly ^ bob -> y) | ( li ^ bob -> image)) 		// xor should remove bits not changed, so if this has value its changed.
					{	
						screen -> event_flags |= rs_bob_moved;		// normaly, screen is swaped if bob is moved.
					}
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


char *_boSetBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroSpriteObject *bob;
	int n;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 4:	n = getStackNum(stack-3);

				bob = getBob( n );
				if (bob)
				{
					bob -> background = getStackNum(stack-2);
					bob -> plains = getStackNum(stack-1);
					bob -> mask = getStackNum(stack);
				}
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
	struct retroSpriteObject *bob;
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		bob = getBob( getStackNum(stack) );

		if (bob == NULL)
		{
			setError(23,data->tokenBuffer);
			return NULL;
		}

		setStackNum(bob -> x);
		return NULL;	// don't need to pop stack.
	} 

	setError(23,data->tokenBuffer);
	popStack( stack - data->stack );
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
	struct retroSpriteObject *bob;
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		bob = getBob( getStackNum(stack) );

		if (bob == NULL)
		{
			setError(23,data->tokenBuffer);
			return NULL;
		}

		setStackNum(bob -> y);
		return NULL;	// don't need to pop stack.
	} 

	setError(23,data->tokenBuffer);
	popStack( stack - data->stack );
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
	struct retroSpriteObject *bob;
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		bob = getBob( getStackNum(stack) );

		if (bob == NULL)
		{
			setError(23,data->tokenBuffer);
			return NULL;
		}

		setStackNum(bob -> image);
		return NULL;	// don't need to pop stack.
	} 

	setError(23,data->tokenBuffer);
	popStack( stack - data->stack );
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
	int n,flags;
	int image;
	struct retroScreen *screen;
	struct retroSpriteObject *bob;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	n = getStackNum( stack );
				bob  = getBob( n );
				flags = bob -> image & 0xC000;
				image = bob -> image & 0x3FFF;

				screen = screens[current_screen];

				if (screen)	
				{
					switch (screen -> autoback)
					{
						case 0:	retroPasteSprite(screen,screen -> double_buffer_draw_frame,sprite,
									bob -> x,bob -> y,image -1, flags, bob -> plains);
								break;

						default:	retroPasteSprite(screen,0,sprite,bob->x,bob->y,image -1, flags, bob->plains);
								if (screen -> Memory[1]) retroPasteSprite(screen,1,sprite,bob->x,bob->y,image -1, flags, bob->plains);
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

void removeBobLimit( struct retroSpriteObject *bob )
{
	if (!bob) return;
	bob -> limitXmin = 0;
	bob -> limitYmin = 0;
	bob -> limitXmax = 0;
	bob -> limitYmax = 0;
}

char *_boLimitBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	unsigned int n;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

//	NYI(__FUNCTION__);

	switch (args)
	{
		case 1:

			if (kittyStack[stack].type == type_none)		// delete all limits from bobs
			{
				for (n=0;n<bobs.size();n++)	removeBobLimit( bobs[n] );
			}
			else	// delete limit from one bob (not supported by Amos Pro)
			{
				n = getStackNum(stack);
				removeBobLimit( getBob(n) );
			}
			break;

		case 4:	// limit bob x0,y0 to x1,y1
			{
				struct retroSpriteObject *bob;
				int y0 = getStackNum(stack-3);
				int x0 = getStackNum(stack-2);
				int x1 = getStackNum(stack-1);
				int y1 = getStackNum(stack);

				for (n=0;n<bobs.size();n++)	// 0-63 is a amos the creator limit, not a amos pro limit.
				{
					if (bob = bobs[n])
					{
						bob->limitXmin = x0;
						bob->limitYmin = y0;
						bob->limitXmax = x1;
						bob->limitYmax = y1;
					}
				}
			}
			break;

		case 5:	// limit bob <bob>,x0,y0 to x1,y1

			{
				struct retroSpriteObject *bob;
				n = getStackNum(stack-4);
				int y0 = getStackNum(stack-3);
				int x0 = getStackNum(stack-2);
				int x1 = getStackNum(stack-1);
				int y1 = getStackNum(stack);

				if (bob = getBob(n))
				{
					bob->limitXmin = x0;
					bob->limitYmin = y0;
					bob->limitXmax = x1;
					bob->limitYmax = y1;
				}
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
	int n;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	engine_lock();
	for (n=0;n<8;n++)
	{
		if (screens[n])
		{
			screens[n] -> event_flags |= rs_force_swap;
		}
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
	struct retroFrameHeader * otherFrame = &sprite -> frames[ otherBob -> image -1 ];
	int ominX = otherBob -> x - otherFrame -> XHotSpot;
	int ominY = otherBob -> y - otherFrame -> XHotSpot;
	int omaxX = ominX + otherFrame -> width;
	int omaxY = ominY + otherFrame -> height;	

	if ( maxX < ominX ) return 0;
	if ( minX > omaxX ) return 0;
	if ( maxY < ominY ) return 0;
	if ( maxY > omaxY ) return 0;

	if (minX< ominX)
	{
		int dx = (ominX - minX), dy = (ominY - minY);
		int bitx = dx & 15;
		dx = dx >> 4;
		if (cmpMask( thisMask, otherFrame -> mask, dx, bitx, dy ))	return ~0;
		return 0;
	}
	else
	{
		int dx = (minX - ominX), dy = (minY - ominY);
		int bitx = dx & 15;
		dx = dx >> 4;
		if (cmpMask(  otherFrame-> mask, thisMask, dx, bitx, dy ))	return ~0;
		return 0;
	}

	return 0;
}

void bobBox( struct retroSpriteObject *thisBob )
{
	struct retroFrameHeader *frame;
	int minX, maxX, minY, maxY;

	frame = &sprite -> frames[ thisBob -> image-1 ];

	minX = thisBob -> x - frame -> XHotSpot;
	minY = thisBob -> y - frame -> XHotSpot;
	maxX = minX + frame -> width;
	maxY = minY + frame -> height;

	retroBox( screens[thisBob -> screen_id], 0, minX,minY,maxX,maxY,1 );
}

void flush_collided()
{
	while (collided.size()) collided.erase(collided.begin());
}

bool has_collided(int id)
{
	for (unsigned int n=0;n<collided.size();n++)
	{
		if (collided[n] == id) return true;
	}
	return false;
}

int bobColRange( unsigned short bob, unsigned short start, unsigned short end )
{
	struct retroSpriteObject *thisBob;
	struct retroSpriteObject *otherBob;
	struct retroFrameHeader *frame;
	int minX, maxX, minY, maxY;
	int n,r;

	thisBob = getBob(bob);

	if ( ! thisBob )
	{
		Printf("bobCol bob %ld not found\n",bob);
		Delay(30);
	 	return 0;
	}

	if (thisBob -> image < 1) return 0;	// does not have image.

	frame = &sprite -> frames[ thisBob -> image-1 ];
	minX = thisBob -> x - frame -> XHotSpot;
	minY = thisBob -> y - frame -> XHotSpot;
	maxX = minX + frame -> width;
	maxY = minY + frame -> height;

	for ( n=start ; n<=end ; n++ )
	{
		otherBob = getBobOnScreen(n , thisBob -> screen_id );

		// filter out bad data....
		if ( ! otherBob) continue;
		if (otherBob == thisBob) continue;
		if (otherBob -> image <1) continue;

		// check if bob is inside.
		r = inBob( frame -> mask, minX,minY,maxX,maxY, otherBob );
		if (has_collided(otherBob -> id) == false)	collided.push_back( otherBob -> id );
		if (r) return r;
	}

	return 0;
}

int bobColAll( unsigned short bob )
{
	struct retroSpriteObject *thisBob;
	struct retroSpriteObject *otherBob;
	struct retroFrameHeader *frame;
	int minX, maxX, minY, maxY;
	unsigned int n;
	int r;

	thisBob = getBob(bob);

	if ( ! thisBob )
	{
		Printf("bobCol bob %ld not found\n",bob);
		Delay(30);
	 	return 0;
	}

	if (thisBob -> image == 0) return 0;

	frame = &sprite -> frames[ thisBob -> image-1 ];
	minX = thisBob -> x - frame -> XHotSpot;
	minY = thisBob -> y - frame -> XHotSpot;
	maxX = minX + frame -> width;
	maxY = minY + frame -> height;

//	retroBox( screens[current_screen], 0, minX,minY,maxX,maxY,1 );

	for (n=0;n<bobs.size();n++)
	{
		otherBob = bobs[n];

		// filter out bad data....
		if ( ! otherBob) continue;

		if (otherBob -> screen_id != thisBob -> screen_id ) continue;
		if (otherBob == thisBob) continue;
		if (otherBob -> image <1) continue;

		// check if bob is inside.
		r = inBob( frame -> mask, minX,minY,maxX,maxY, otherBob );

		if (r)
		{
			if (has_collided(otherBob -> id) == false)	collided.push_back( otherBob -> id );
			return r;
		}
	}

	return 0;
}

char *_boBobCol( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	flush_collided();

	switch (args)
	{
		case 1:	setStackNum(bobColAll( getStackNum(stack) ));
				return NULL;

		case 3:	setStackNum(bobColRange( getStackNum(stack-2), getStackNum(stack-1), getStackNum(stack) ));
				popStack( stack - data->stack );
				return NULL;
		default:
				setError(22,data->tokenBuffer);
	}

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

	switch (args)
	{
		case 1:

			ret = has_collided(getStackNum(stack)) ? ~0 : 0;

			break;

		default:

			popStack( stack - data->stack );
			break;
	}


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


void __remove_bob__(struct retroSpriteObject *bob)
{
	struct kittyChannel *item;

	engine_lock();

	// remove all refs to object.
	while (item = channels -> findChannelByItem( 0x1B9E, bob -> id ))
	{
		item -> token = 0;
		item -> number = 0;
	}

	clearBob(bob);
	__erase_bob__( bob );
	engine_unlock();
}


char *_boBobOff( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroSpriteObject *bob;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		switch (kittyStack[stack].type)
		{
			case type_none:

					while (bobs.size()>0) __remove_bob__(bobs[0]);
					return NULL;

			case type_int:

					if (bob = getBob( getStackNum(stack) ))
					{
						__remove_bob__( bob );
						return NULL;
					}
					break;
		}
	}
	else setError(22, data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *boBobOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _boBobOff, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}



void __no_mask__(struct retroFrameHeader *frame)
{
	if (frame)
	{
		frame -> alpha = 0;
		retroFreeMask( frame );
	}
}

void NoMaskForAll()
{
	int n;

	if (sprite == NULL) return;

	for (n=0;n<sprite -> number_of_frames;n++)
	{
		__no_mask__( &sprite -> frames[n] );
	}
}

char *_boNoMask( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int image;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:

			switch (kittyStack[stack].type)
			{
				case type_none:
					NoMaskForAll();
					break;

				case type_int:
					if (sprite)
					{
						image = getStackNum( stack )-1;
						if (image < sprite -> number_of_frames)
						{
							__no_mask__(&sprite -> frames[image]);
						}
					}
					break;
			}

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

