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

static int getImage (int object)
{
	return 0;
}

static int getX (int object)
{
	return screens[object]->offset_x;
}

static int getY (int object)
{
	return screens[object]->offset_y;
}

static void setImage (int object,int image)
{
}

static void setX (int object,int x)
{
	screens[object]->offset_x = x;

 	screens[object] -> refreshScanlines = TRUE;
	video -> refreshSomeScanlines = TRUE;
}

static void setY (int object,int y)
{
	screens[object]->offset_y = y;

 	screens[object] -> refreshScanlines = TRUE;
	video -> refreshSomeScanlines = TRUE;
}

struct channelAPI screen_offset_api = 
{
	getMax,
	getImage,
	getX,
	getY,
	setImage,
	setX,
	setY
};

