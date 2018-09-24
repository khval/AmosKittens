
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <proto/dos.h>
#include <libraries/retroMode.h>
#include <proto/retroMode.h>
#include "AmalCompiler.h"
#include "channel.h"

extern struct retroSpriteObject bobs[64];

static int getMax ( void )
{
	return 64;
}

static int getImage (int object)
{
	return bobs[object].image;
}

static int getX (int object)
{
	return bobs[object].x;
}

static int getY (int object)
{
	return bobs[object].y;
}

static void setImage (int object,int image)
{
	bobs[object].image = image;
}

static void setX (int object,int x)
{
	bobs[object].x = x;
}

static void setY (int object,int y)
{
	bobs[object].y = y;
}

struct channelAPI bob_api = 
{
	getMax,
	getImage,
	getX,
	getY,
	setImage,
	setX,
	setY
};

