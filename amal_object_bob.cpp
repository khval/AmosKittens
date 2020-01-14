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

extern std::vector<struct retroSpriteObject *> bobs;
extern struct retroScreen *screens[8] ;

static int getMax ( void )
{
	return 64;
}

extern struct retroSpriteObject *getBob(int id);

extern struct retroSpriteObject *getBob(unsigned int id);
extern int getBobImage(unsigned int id);
extern int getBobX(unsigned int id);
extern int getBobY(unsigned int id);

static void setImage (unsigned int object,int image)
{
	struct retroScreen *s;
	struct retroSpriteObject *bob;

	if (bob = getBob( object ))
	{
		bob -> image = image;
		s = screens[ bob -> screen_id ];
		if (s) s ->event_flags |= rs_bob_moved;
	}
}

static void setX (unsigned int object,int x)
{
	struct retroScreen *s;
	struct retroSpriteObject *bob;

	if (bob = getBob( object ))
	{
		bob -> x = x;
		s = screens[ bob -> screen_id ];
		if (s) s ->event_flags |= rs_bob_moved;
	}
}

static void setY (unsigned int object,int y)
{
	struct retroScreen *s;
	struct retroSpriteObject *bob;

	if (bob = getBob( object ))
	{
		bob -> y = y;
		s = screens[ bob -> screen_id ];
		if (s) s ->event_flags |= rs_bob_moved;
	}
}

struct channelAPI bob_api = 
{
	getMax,
	getBobImage,
	getBobX,
	getBobY,
	setImage,
	setX,
	setY
};

