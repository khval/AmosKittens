
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <proto/dos.h>
#include <libraries/retroMode.h>
#include <proto/retroMode.h>
#include "AmalCompiler.h"
#include "channel.h"

extern struct retroVideo *video;

static int getMax ( void )
{
	return 64;
}

static int getImage (int object)
{
	return video -> sprites[object].image;
}

static int getX (int object)
{
	return video -> sprites[object].x*2;
}

static int getY (int object)
{
	return video -> sprites[object].y*2;
}

static void setImage (int object,int image)
{
	video -> sprites[object].image = image;
}

static void setX (int object,int x)
{
	video -> sprites[object].x = x/2;
}

static void setY (int object,int y)
{
	video -> sprites[object].y = y/2;
}

struct channelAPI sprite_api = 
{
	getMax,
	getImage,
	getX,
	getY,
	setImage,
	setX,
	setY
};

