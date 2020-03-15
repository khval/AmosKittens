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

static int getMax ( void )
{
	return 8;
}

static int getImage (unsigned int object)
{
	return 0;
}

static int getX (unsigned int object)
{
	return instance.screens[object]->scanline_x;
}

static int getY (unsigned int object)
{
	return instance.screens[object]->scanline_y;
}

static void setImage (unsigned int object,int image)
{
}

static void setX (unsigned int object,int x)
{
	instance.screens[object]->scanline_x = x;

	retroApplyScreen( instance.screens[object], instance.video, 
			instance.screens[object] -> scanline_x,
			instance.screens[object] -> scanline_y,
			instance.screens[object] -> displayWidth,
			instance.screens[object] -> displayHeight );

	instance.video -> refreshAllScanlines = TRUE;
}

static void setY (unsigned int object,int y)
{
	instance.screens[object]->scanline_y = y;

	retroApplyScreen( instance.screens[object], instance.video, 
			instance.screens[object] -> scanline_x,
			instance.screens[object] -> scanline_y,
			instance.screens[object] -> displayWidth,
			instance.screens[object] -> displayHeight );

	instance.video -> refreshAllScanlines = TRUE;
}

static struct retroScreen *getScreen(unsigned int object)
{
	return NULL;
}

struct channelAPI screen_display_api = 
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

