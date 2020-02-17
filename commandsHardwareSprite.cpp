
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>

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

int XScreen_formula( struct retroScreen *screen, int x );
int YScreen_formula( struct retroScreen *screen, int y );

extern int sig_main_vbl;

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

extern int current_screen;

extern struct retroScreen *screens[8] ;
extern struct retroSprite *sprite;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];

extern std::vector<int> collided;
extern bool has_collided(int id);
extern void flush_collided();

int cmpMask( struct retroMask *leftMask, struct retroMask *rightMask, int offInt16, int lshift, int dy );

int inSprite( struct retroMask *thisMask, int minX,int minY, int maxX, int maxY, struct retroSpriteObject *otherBob );

#define getSprite(num) &(video -> sprites[num])


char *_hsGetSpritePalette( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		if ((sprite)&&(screen))
		{
			screen -> fade_speed = 0;

			switch (kittyStack[stack].type)
			{
				case type_none:

					for (n=0;n<256;n++)
					{
						retroScreenColor( screen, n, sprite -> palette[n].r, sprite -> palette[n].g, sprite -> palette[n].b );		
					}
					break;

				case type_int:
					{
						int mask = kittyStack[stack].integer.value;

						for (n=0;n<256;n++)
						{
							if (mask & n) retroScreenColor( screen, n, sprite -> palette[n].r, sprite -> palette[n].g, sprite -> palette[n].b );		
						}
					}
					break;

			}
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *hsGetSpritePalette(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _hsGetSpritePalette, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

extern int XSprite_formula(int x);
extern int YSprite_formula(int y);

char *_hsSprite( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num;
	struct retroSpriteObject *sprite;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	engine_lock();
	num = getStackNum( stack - 3 );
	sprite = &video -> sprites[num];

	sprite -> id = num;

	if (stack_is_number( stack - 2)) sprite->x = XSprite_formula(getStackNum( stack - 2 ));
	if (stack_is_number( stack - 1)) sprite->y = YSprite_formula(getStackNum( stack - 1 ));
	sprite->image = getStackNum( stack );
	engine_unlock();

//	printf("sprite %d,%d,%d,%d\n",num,sprite->x,sprite->y,sprite->image);

	popStack( stack - data->stack );
	return NULL;
}

char *hsSprite(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _hsSprite, tokenBuffer );
	return tokenBuffer;
}

/*

char *_hsGetSprite( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *hsGetSprite(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _hsSprite, tokenBuffer );
	return tokenBuffer;
}

*/

char *_hsSpriteOff( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		struct kittyData *s = &kittyStack[stack];
		engine_lock();
		if (s -> type == type_int)
		{
			video -> sprites[ s -> integer.value ].image = -1;	// not deleted, just gone.
		}
		else
		{
			int n;
			for (n=0;n<64;n++)
			{
				video -> sprites[n].image = -1;
			}
		}
		engine_unlock();
		return NULL;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *hsSpriteOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _hsSpriteOff, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_hsSpriteBase( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int pick = 0;
	void *ret = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		pick = getStackNum(stack);

		if (sprite)	if ((pick>0)&&(pick<=sprite->number_of_frames))
		{
			ret = &sprite -> frames[pick-1] ;
		}

		if (NULL == ret) setError(23,data->tokenBuffer);
	}
	else setError(22, data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum( (int) ret );
	return NULL;
}

char *hsSpriteBase(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _hsSpriteBase, tokenBuffer );
	return tokenBuffer;
}

char *_hsSetSpriteBuffer( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
	}
	else setError(22, data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *hsSetSpriteBuffer(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _hsSetSpriteBuffer, tokenBuffer );
	return tokenBuffer;
}

int spriteColAll( unsigned short Sprite );
int spriteColRange( unsigned short Sprite, unsigned short start, unsigned short end );

char *_hsSpriteCol( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num = 0;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	flush_collided();

	if (args==1)
	{
		num = getStackNum(stack);

//		Printf("Sprite Col(%ld)\n",num);

		if (num>=0) ret = spriteColAll( num );

	}
	else setError(22, data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum( (int) ret );
	return NULL;
}

char *hsSpriteCol(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _hsSpriteCol, tokenBuffer );
	return tokenBuffer;
}

void spriteBox( int x0,int y0,int x1,int y1, int c)
{
//	x0=XScreen_formula( screens[current_screen], x0 );
//	y0=YScreen_formula( screens[current_screen], y0 );
//	x1=XScreen_formula( screens[current_screen], x1 );
//	y1=YScreen_formula( screens[current_screen], y1 );

	retroBox( screens[current_screen], 0, x0,y0,x1,y1,c );
}

extern void dump_collided();

int spriteColAll( unsigned short Sprite )
{
	struct retroSpriteObject *thisSprite;
	struct retroSpriteObject *otherSprite;
	struct retroFrameHeader *frame;
	int minX, maxX, minY, maxY;
	unsigned int n;
	int r;
	int x,y;

	thisSprite = getSprite(Sprite);

	if ( ! thisSprite )
	{
		Printf("SpriteCol Sprite %ld not found\n",Sprite);
		Delay(30);
	 	return 0;
	}

	if (thisSprite -> image == 0) return 0;

	frame = &sprite -> frames[ thisSprite -> image-1 ];

	x = thisSprite -> x / 2;
	y = thisSprite -> y / 2;

	minX = x - frame -> XHotSpot;
	minY = y - frame -> XHotSpot;
	maxX = minX + frame -> width;
	maxY = minY + frame -> height;

	for (n=0;n<sprite -> number_of_frames;n++)
	{
		otherSprite = getSprite(n);

		// filter out bad data....
		if ( ! otherSprite) continue;
		if (otherSprite == thisSprite) continue;
		if (otherSprite -> image <1) continue;

		// check if Sprite is inside.
		r = inSprite( frame -> mask, minX,minY,maxX,maxY, otherSprite );

		if (r)
		{	
			if (has_collided(otherSprite -> id) == false)
			{
				collided.push_back( otherSprite -> id );
			}

			return r;
		}
	}

	return 0;
}

int spriteColRange( unsigned short Sprite, unsigned short start, unsigned short end )
{
	struct retroSpriteObject *thisSprite;
	struct retroSpriteObject *otherSprite;
	struct retroFrameHeader *frame;
	int x,y;
	int minX, maxX, minY, maxY;
	int n,r;

	thisSprite = getSprite(Sprite);

	if ( ! thisSprite )
	{
		Printf("SpriteCol Sprite %ld not found\n",Sprite);
		Delay(30);
	 	return 0;
	}

	if (thisSprite -> image < 1) return 0;	// does not have image.

	frame = &sprite -> frames[ thisSprite -> image-1 ];

	x=thisSprite -> x/2;
	y=thisSprite -> y/2;

	minX = x - frame -> XHotSpot;
	minY = y - frame -> XHotSpot;
	maxX = minX + frame -> width;
	maxY = minY + frame -> height;

	for ( n=start ; n<=end ; n++ )
	{
		otherSprite = getSprite(n);

		// filter out bad data....
		if ( ! otherSprite) continue;
		if (otherSprite == thisSprite) continue;
		if (otherSprite -> image <1) continue;

		// check if Sprite is inside.
		r = inSprite( frame -> mask, minX,minY,maxX,maxY, otherSprite );
	
		if (r)
		{
			if (has_collided(otherSprite -> id) == false)	collided.push_back( otherSprite -> id );
			return r;
		}
	}

	return 0;
}

int inSprite( struct retroMask *thisMask, int minX,int minY, int maxX, int maxY, struct retroSpriteObject *otherBob )
{
	int x,y;
	struct retroFrameHeader * otherFrame = &sprite -> frames[ otherBob -> image -1 ];

	x = otherBob -> x  / 2;
	y  = otherBob -> y / 2;

	int ominX = x - otherFrame -> XHotSpot;
	int ominY = y - otherFrame -> XHotSpot;
	int omaxX = ominX + otherFrame -> width;
	int omaxY = ominY + otherFrame -> height;	

	if ( maxX < ominX ) return 0;
	if ( minX > omaxX ) return 0;
	if ( maxY < ominY ) return 0;
	if ( minY > omaxY ) return 0;

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
