#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifdef __amigaos4__
#include <proto/dos.h>
#include <libraries/retroMode.h>
#include <proto/retroMode.h>
#include <amoskittens.h>
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

extern struct retroVideo *video;
extern struct retroScreen *screens[8] ;

static int getMax ( void )
{
	return 4;
}

static int getImage (unsigned int object)
{
	return 0;
}

static int getX (unsigned int object)
{
	return video -> rainbow[object].offset;
}

static int getY (unsigned int object)
{
	return video -> rainbow[object].verticalOffset+50;
}

static void setImage (unsigned int object,int image)
{
}

static void setX (unsigned int object,int x)
{
	video -> rainbow[object].offset = x;
}

static void setY (unsigned int object,int y)
{
	video -> rainbow[object].verticalOffset = y-50;
}

static struct retroScreen *getScreen(unsigned int object)
{
	return NULL;
}

struct channelAPI rainbow_api = 
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

