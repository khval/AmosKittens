#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>

#include "AmosKittens.h"
#include "joysticks.h"

struct Library		*AIN_Base = NULL;
struct AIN_IFace	*IAIN = NULL;

struct MsgPort *joystick_msgport = NULL;

int found_joysticks = 0;
int used_joysticks = 0;
struct joystick joysticks[4];

typedef struct
{
	APTR	context;
	uint32_t	get_count;
	uint32_t  	count;
	uint32_t	id;
} enumPacket;

enumPacket userdata;

static BOOL get_joy (void *device, struct joystick *joy)
{
	BOOL ret = FALSE;
	return ret;
}

void init_joysticks()
{
}


void close_joysticks()
{
}

unsigned int dir[]={ 0x00, 
				joy_up, 				// 1
				joy_up | joy_left, 
				joy_left, 				// 3
				joy_down | joy_left, 
				joy_down, 			// 5
				joy_down | joy_right, 
				joy_right, 				// 7
				joy_right | joy_up
				};

void print_bin(unsigned int v)
{
	int n;
	for (n=31;n>=0;n--) printf("%c", (v & (1L<<n)) ? '1' : '0' );
}

void joy_stick(int joy,void *controller)
{
}


