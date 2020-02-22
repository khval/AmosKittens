
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


extern struct retroSprite *sprite;
extern std::vector<int> collided;
extern std::vector<struct retroSpriteObject *> bobs;

extern struct retroSpriteObject *getBob(unsigned int id);
extern struct retroSpriteObject *getBobOnScreen(unsigned int id,int screen);
extern int inBob( struct retroMask *thisMask, int minX,int minY, int maxX, int maxY, struct retroSpriteObject *otherBob );
extern bool has_collided(int id);


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
	
		if (r)
		{
			if (has_collided(otherBob -> id) == false)	collided.push_back( otherBob -> id );
			return r;
		}
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
