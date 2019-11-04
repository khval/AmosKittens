#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifdef __amigaos4__
#include <proto/dos.h>
#include <libraries/retroMode.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <stdint.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#define Pritnf printf
#endif

#include "AmalCompiler.h"
#include "channel.h"
#include "engine.h"

extern struct retroVideo *video;

int XSprite_formula(int x);
int YSprite_formula(int y);
int from_XSprite_formula(int x);
int from_YSprite_formula(int y);

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
	return from_XSprite_formula(video -> sprites[object].x);
}

static int getY (int object)
{
	return from_YSprite_formula(video -> sprites[object].y);
}

static void setImage (int object,int image)
{
	video -> sprites[object].image = image;
}

static void setX (int object,int x)
{
	video -> sprites[object].x = XSprite_formula(x);
}

static void setY (int object,int y)
{
	video -> sprites[object].y = YSprite_formula(y);
}

//-----

int XSprite_formula(int x)
{
	return (x - hardware_upper_left)*2 ;
}

int YSprite_formula(int y)
{
	return (y - hardware_upper_top)*2 ;
}

int from_XSprite_formula(int x)
{
	return (x/2) + hardware_upper_left ;
}

int from_YSprite_formula(int y)
{
	return (y/2) + hardware_upper_top ;
}

//----

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

