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
#define Printf printf
#endif

#include "AmalCompiler.h"
#include "channel.h"

extern struct retroSpriteObject bobs[64];
extern struct retroScreen *screens[8] ;

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
	struct retroScreen *s;
	bobs[object].image = image;
	s = screens[ bobs[object].screen_id ];
	if (s) s -> force_swap = TRUE;
	Printf("%s(%ld) = %ld\n",__FUNCTION__,object,image);
}

static void setX (int object,int x)
{
	struct retroScreen *s;
	bobs[object].x = x;
	s = screens[ bobs[object].screen_id ];
	if (s) s -> force_swap = TRUE;
	Printf("%s(%ld) = %ld\n",__FUNCTION__,object,x);
}

static void setY (int object,int y)
{
	struct retroScreen *s;
	bobs[object].y = y;
	s = screens[ bobs[object].screen_id ];
	if (s) s -> force_swap = TRUE;
	Printf("%s(%ld) = %ld\n",__FUNCTION__,object,y);
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

