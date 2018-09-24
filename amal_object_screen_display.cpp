
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
	return screens[object]->scanline_x;
}

static int getY (int object)
{
	return screens[object]->scanline_y;
}

static void setImage (int object,int image)
{
}

static void setX (int object,int x)
{
	screens[object]->scanline_x = x;

	retroApplyScreen( screens[object], video, 
			screens[object] -> scanline_x,
			screens[object] -> scanline_y,
			screens[object] -> displayWidth,
			screens[object] -> displayHeight );

	video -> refreshAllScanlines = TRUE;
}

static void setY (int object,int y)
{
	screens[object]->scanline_y = y;

	retroApplyScreen( screens[object], video, 
			screens[object] -> scanline_x,
			screens[object] -> scanline_y,
			screens[object] -> displayWidth,
			screens[object] -> displayHeight );

	video -> refreshAllScanlines = TRUE;
}

struct channelAPI screen_display_api = 
{
	getMax,
	getImage,
	getX,
	getY,
	setImage,
	setX,
	setY
};

