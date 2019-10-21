
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if defined(__amigaos4__) || defined(__amigaos__)
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#define Printf printf
#endif

#include "commandsScreens.h"
#include "engine.h"

extern struct retroScreen *screens[8] ;

#ifndef test_app

int XScreen_formula( struct retroScreen *screen, int x )
{
	x = x - hardware_upper_left - (screen -> scanline_x/2) - screen -> offset_x;
	if  (screen -> videomode & retroHires)  x *= 2;
	return x;
}

int YScreen_formula( struct retroScreen *screen, int y )
{
	y = y - hardware_upper_top - (screen -> scanline_y/2) - screen -> offset_y;
	if  (screen -> videomode & retroInterlaced) y *= 2;
	return y;
}

int XHard_formula( struct retroScreen *screen, int x )
{
	if (screen -> videomode & retroHires)  x /= 2;
	x = x + (screen -> scanline_x/2) + screen -> offset_x + hardware_upper_left;
	return x;
}

int YHard_formula( struct retroScreen *screen, int y )
{
	if  (screen -> videomode & retroInterlaced) y /= 2;
	y = y + (screen -> scanline_y/2) + screen -> offset_y + hardware_upper_top;
	return y;
}

#else

#ifdef _MSC_VER

struct retroScreen
{
	void *ptr;
};

#endif



int XScreen_formula(struct retroScreen *screen, int x)
{
	return x;
}

int YScreen_formula(struct retroScreen *screen, int y)
{
	return y;
}

int XHard_formula(struct retroScreen *screen, int x)
{
	return x;
}

int YHard_formula(struct retroScreen *screen, int y)
{
	return y;
}

#endif

