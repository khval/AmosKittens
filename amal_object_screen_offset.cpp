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

extern struct retroVideo *video;
extern struct retroScreen *screens[8] ;

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
	return screens[object]->offset_x;
}

static int getY (unsigned int object)
{
	return screens[object]->offset_y;
}

static void setImage (unsigned int object,int image)
{
}

static void setX (unsigned int object,int x)
{
	screens[object]->offset_x = x;

 	screens[object] -> refreshScanlines = TRUE;
	video -> refreshSomeScanlines = TRUE;
}

static void setY (unsigned int object,int y)
{
	screens[object]->offset_y = y;

 	screens[object] -> refreshScanlines = TRUE;
	video -> refreshSomeScanlines = TRUE;
}

static struct retroScreen *getScreen(unsigned int object)
{
	return NULL;
}

struct channelAPI screen_offset_api = 
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

