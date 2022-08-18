#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifdef __amigaos4__
#include <proto/dos.h>
#include <libraries/retroMode.h>
#include <proto/retroMode.h>
#include <AmosKittens.h>
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


static int getMax ( void )
{
	return 64;
}

static int getImage (unsigned  int object)
{
	return instance.video -> sprites[object].image;
}

static int getX (unsigned int object)
{
	return from_XSprite_formula(instance.video -> sprites[object].x);
}

static int getY (unsigned int object)
{
	return from_YSprite_formula(instance.video -> sprites[object].y);
}

static void setImage (unsigned int object,int image)
{
	instance.video -> sprites[object].image = image;
}

static void setX (unsigned int object,int x)
{
	instance.video -> sprites[object].x = XSprite_formula(x);
}

static void setY (unsigned int object,int y)
{
	instance.video -> sprites[object].y = YSprite_formula(y);
}

//-----


static struct retroScreen *getScreen(unsigned int object)
{
	return NULL;
}

struct channelAPI sprite_api = 
{
	getMax,
	getImage,
	getX,
	getY,
	setImage,
	setX,
	setY,
	getScreen
};

