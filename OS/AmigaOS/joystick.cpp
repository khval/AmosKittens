
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>

#include "AmosKittens.h"
#include "joysticks.h"

struct Library		*AIN_Base = NULL;
struct AIN_IFace	*IAIN = NULL;

struct MsgPort *joystick_msgport = NULL;

int axis_max_value = 0;

int found_joysticks = 0;
int used_joysticks = 0;
struct joystick joysticks[4];

typedef struct
{
	APTR	context;
	int		get_count;
	uint32  	count;
	int		id;
} enumPacket;

enumPacket userdata;

static BOOL get_joy (AIN_Device *device, struct joystick *joy)
{
	ULONG connected = false;
	BOOL ret = FALSE;

	if (device->Type == AINDT_JOYSTICK) 
	{
		AIN_Query(joy ->controller, device -> DeviceID,AINQ_CONNECTED,0,&(connected), sizeof(ULONG) );

		if (connected)
		{
//			printf("** connected **\n");

			AIN_Query(joy ->controller, device -> DeviceID,AINQ_AXIS_OFFSET,0,&(joy -> axis_offset), sizeof(ULONG) );
			AIN_Query(joy ->controller, device -> DeviceID,AINQ_BUTTON_OFFSET,0,&(joy ->button_offset), sizeof(ULONG) );
			AIN_Query(joy ->controller, device -> DeviceID,AINQ_HAT_OFFSET,0,&(joy -> hat_offset), sizeof(ULONG) );

			if (found_joysticks==joy->type_count)
			{
				ret = TRUE;
				joy -> device_id = device -> DeviceID;
				joy -> connected = true;
			}
			found_joysticks ++;
		}
	}

	return ret;
}


void init_usb_joystick(int usb_count, int port, struct joystick *joy, struct TagItem *AIN_Tags)
{
	joy -> device_id = -1;
	joy -> type_count = usb_count;
	joy -> port = port;
	joy -> connected = false;
	joy -> controller = AIN_CreateContext (1, AIN_Tags);

	if (joy->controller)
	{
//		printf("looking for usb joystcik #%d on port #%d\n",usb_count, port);

		found_joysticks = 0;
		AIN_EnumDevices(joy->controller, (void *) get_joy, (void *) joy );
	}
	else
	{
//		printf("Amiga input can't create context\n");
	}
}

// this on is normaly run from engine... so needs to use Printf not newlib printf

void dump_joysticks()
{
	printf("-- joysticks --\n");

	// show found joysticks
	for (struct joystick *joy=joysticks;joy<joysticks+4;joy++)
	{
		printf("joy[%08x] port #%d, type %d, conncted %d, Using device ID %x\n",
			joy,
			joy -> port,
			joy -> type,
			joy -> connected,
			joy -> device_id);
	}
}

void init_usb_joysticks()
{
	int index;
	int usb_count = 0;
	int port_number;
	struct TagItem AIN_Tags[2];

//	printf("init_usb_joysticks!!\n");


	joystick_msgport = (struct MsgPort *) AllocSysObjectTags(ASOT_PORT, TAG_END );	
	if (!joystick_msgport) return ;

	AIN_Tags[0].ti_Tag = AINCC_Port;
	AIN_Tags[0].ti_Data = (ULONG) joystick_msgport;
	AIN_Tags[1].ti_Tag = TAG_END;

	for (index=0;index<4;index++)
	{
		// joy0 is mouse port on Amiga (2 player games)
		// joy1 is joystick port (1 player games)

		switch (index)
		{
			case 0: port_number=1;	break;
			case 1: port_number=0;	break;
			default: port_number=index;	break;
		}

//		printf("joysticks[ %d ].type: %d\n", port_number, joysticks[ port_number ].type);

		if ( joysticks[ port_number ].type == joy_usb )
		{
			init_usb_joystick( usb_count, port_number, joysticks + port_number, (TagItem*) &AIN_Tags );
			usb_count ++;
		}
	}

	dump_joysticks();

	// obtain joysticks
	for (index=0;index<4;index++)
	{
		if (joysticks[index].device_id>0)
		{
			joysticks[index].handle = AIN_ObtainDevice(joysticks[index].controller, joysticks[index].device_id );

			if (joysticks[index].handle)
			{
				joysticks[index].res =  AIN_SetDeviceParameter(joysticks[index].controller,joysticks[index].handle,AINDP_EVENT,TRUE);
			}
		}
	}
}


void close_joysticks()
{
	int n;
	int res;

	for (n=0;n<4;n++)
	{
		if (joysticks[n].handle)
		{
			res =  AIN_SetDeviceParameter(joysticks[n].controller,joysticks[n].handle,AINDP_EVENT,FALSE);
	 		AIN_ReleaseDevice( joysticks[n].controller,joysticks[n].handle );
		}

		if (joysticks[n].controller)
		{
			AIN_DeleteContext( joysticks[n].controller );
			joysticks[n].controller = NULL;
		}

		if (joystick_msgport)
		{
			FreeSysObject( ASOT_PORT, joystick_msgport );
			joystick_msgport = NULL;
		}
	}
}

unsigned int dir[]={ 0x00, 
				joy_up, 				// 1
				joy_up | joy_right, 
				joy_right, 				// 3
				joy_down | joy_right, 
				joy_down, 			// 5
				joy_down | joy_left, 
				joy_left, 				// 7
				joy_left | joy_up
				};

void print_bin(unsigned int v)
{
	int n;
	for (n=31;n>=0;n--) printf("%c", (v & (1L<<n)) ? '1' : '0' );
}

static int lot[]=
	{
		1<<1,		//1	// fire2
		1<<0,		//2	// fire1
		1<<2,		//3
		1<<3,		//4
		1<<4,		//5
		1<<5,		//6
		1<<6,		//7
		1<<7,		//8
		1<<8,		//9
		1<<9,		//10
		1<<10,		//11
		1<<11,		//12
		1<<12,		//13
		1<<13,		//14
		1<<14,		//15
		1<<15,		//16
		1<<16,		//17
		1<<17,		//18
		1<<18,		//19
		1<<19,		//20
		1<<20,		//21
	};

int axis_to_joy(struct joystick *joy, int n, int value)
{
	if (value>axis_max_value) axis_max_value = value;
	if (-value>axis_max_value) axis_max_value = -value;

	if ((value>(-axis_max_value/2)) && (value<(axis_max_value/2)) ) value = 0;

	switch (n&1)
	{
		case 0:	joy -> xpad = 0x00;
				if (value<0) { joy -> xpad = joy_left; } else if (value>0) { joy -> xpad = joy_right; }
				break;

		case 1:	joy -> ypad = 0x00;
				if (value<0) { joy -> ypad = joy_up; } else if (value>0) { joy -> ypad = joy_down; }
				break;
	}
	return joy -> xpad | joy -> ypad;
}

void joy_stick(int j, struct joystick *joy)
{
	AIN_InputEvent *ain_mess;

	printf("joystick: %d\n",j);

	if (joy -> connected == false) return;

	printf("is connected!! try to get event\n");

	while ( ain_mess = AIN_GetEvent(joy -> controller))
	{
		printf("type: %d\n",ain_mess -> Type );

		switch (ain_mess -> Type)
		{
			case AINET_BUTTON:
					{
						int b = ain_mess -> Index -  joy -> button_offset;
						unsigned int bit = lot[ b  ];

						if (ain_mess -> Value)
						{
							amiga_joystick_button[j] |= bit ;
						}
						else
						{
							amiga_joystick_button[j] &= ~bit;
						}
					}
					break;

			case AINET_AXIS: // axes
					{
						int axis = ain_mess -> Index -  joy -> axis_offset;
						amiga_joystick_dir[j] = axis_to_joy( joy, axis, ain_mess ->  Value);
					}
					break;

			case AINET_HAT:
					amiga_joystick_dir[j] = dir[ ain_mess -> Value ];
					break;
		}

		AIN_FreeEvent(joy -> controller,ain_mess);
	}
}


