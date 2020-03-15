
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
#include <amoskittens.h>
#endif

#ifdef __linux__
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#endif

#include "engine.h"

extern std::vector<int> collided;
extern std::vector<struct retroSpriteObject *> bobs;

extern struct retroSpriteObject *getBob(unsigned int id);
extern struct retroSpriteObject *getBobOnScreen(unsigned int id,int screen);
extern int inBob( struct retroMask *thisMask, int minX,int minY, int maxX, int maxY, struct retroSpriteObject *otherBob );
extern bool has_collided(int id);
extern void dump_collided();
extern int cmpMask( struct retroMask *leftMask, struct retroMask *rightMask, int offInt16, int lshift, int dy );


int inSprite( struct retroMask *thisMask, int minX,int minY, int maxX, int maxY, struct retroSpriteObject *otherBob );


#define getSprite(num) &(instance.video -> sprites[num])

void spriteBox( int x0,int y0,int x1,int y1, int c)
{
//	x0=XScreen_formula( instance.screens[current_screen], x0 );
//	y0=YScreen_formula( instance.screens[current_screen], y0 );
//	x1=XScreen_formula( instance.screens[current_screen], x1 );
//	y1=YScreen_formula( instance.screens[current_screen], y1 );

	retroBox( instance.screens[instance.current_screen], 0, x0,y0,x1,y1,c );
}

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

	frame = &instance.sprites -> frames[ thisSprite -> image-1 ];

	x = thisSprite -> x / 2;
	y = thisSprite -> y / 2;

	minX = x - frame -> XHotSpot;
	minY = y - frame -> XHotSpot;
	maxX = minX + frame -> width;
	maxY = minY + frame -> height;

	for (n=0;n<instance.sprites -> number_of_frames;n++)
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

	frame = &instance.sprites -> frames[ thisSprite -> image-1 ];

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
	struct retroFrameHeader * otherFrame = &instance.sprites -> frames[ otherBob -> image -1 ];

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


