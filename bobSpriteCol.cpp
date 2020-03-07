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

#include "commandsScreens.h"

extern std::vector<int> collided;
extern struct retroVideo *video;
extern std::vector<struct retroSpriteObject *> bobs;

extern struct retroSpriteObject *getBob(unsigned int id);
extern struct retroSpriteObject *getBobOnScreen(unsigned int id,int screen);
extern int inBob( struct retroMask *thisMask, int minX,int minY, int maxX, int maxY, struct retroSpriteObject *otherBob );
extern bool has_collided(int id);

extern int XSprite_formula(int x);
extern int YSprite_formula(int y);
extern int from_XSprite_formula(int x);
extern int from_YSprite_formula(int y);

extern int inSprite( struct retroMask *thisMask, int minX,int minY, int maxX, int maxY, struct retroSpriteObject *otherBob );
#define getSprite(num) &(video -> sprites[num])

int bobSpriteColAll( unsigned short bob )
{
	struct retroScreen *screen;
	struct retroSpriteObject *thisBob;
	struct retroSpriteObject *otherSprite;
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

	frame = &instance.sprites -> frames[ thisBob -> image-1 ];

	screen = instance.screens[thisBob -> screen_id ];
	if (screen == NULL) return 0;

	minX =  thisBob -> x - frame -> XHotSpot ;
	minY =  thisBob -> y - frame -> XHotSpot ;

	maxX = XSprite_formula(XHard_formula( screen, minX + frame -> width ));
	maxY = YSprite_formula(YHard_formula( screen, minY + frame -> height ));
	minX = XSprite_formula(XHard_formula( screen, minX ));
	minY = YSprite_formula(YHard_formula( screen, minY ));

	for (n=0;n<instance.sprites -> number_of_frames;n++)
	{
		otherSprite = getSprite(n);

		// filter out bad data....
		if ( ! otherSprite) continue;

		if (otherSprite == thisBob) continue;
		if (otherSprite -> image <1) continue;

		// check if bob is inside.
		r = inSprite( frame -> mask, minX,minY,maxX,maxY, otherSprite );

		if (r)
		{
			if (has_collided(otherSprite -> id) == false)	collided.push_back( otherSprite -> id );
			return r;
		}
	}

	return 0;
}

int bobSpriteColRange( unsigned short bob, unsigned short start, unsigned short end )
{
	struct retroScreen *screen;
	struct retroSpriteObject *thisBob;
	struct retroSpriteObject *otherSprite;
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

	frame = &instance.sprites -> frames[ thisBob -> image-1 ];

	screen = instance.screens[thisBob -> screen_id ];
	if (screen == NULL) return 0;

	minX =  thisBob -> x - frame -> XHotSpot ;
	minY =  thisBob -> y - frame -> XHotSpot ;

	maxX = XSprite_formula(XHard_formula( screen, minX + frame -> width ));
	maxY = YSprite_formula(YHard_formula( screen, minY + frame -> height ));
	minX = XSprite_formula(XHard_formula( screen, minX ));
	minY = YSprite_formula(YHard_formula( screen, minY ));

	for ( n=start ; n<=end ; n++ )
	{
		otherSprite = getSprite(n);

		// filter out bad data....
		if ( ! otherSprite) continue;
		if (otherSprite == thisBob) continue;
		if (otherSprite -> image <1) continue;

		// check if bob is inside.
		r = inSprite( frame -> mask, minX,minY,maxX,maxY, otherSprite );
	
		if (r)
		{
			if (has_collided(otherSprite -> id) == false)	collided.push_back( otherSprite -> id );
			return r;
		}
	}

	return 0;
}

