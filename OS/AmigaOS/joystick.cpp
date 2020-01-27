
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <proto/exec.h>
#include <proto/dos.h>

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
	int		get_count;
	uint32  	count;
	int		id;
} enumPacket;

enumPacket userdata;

static BOOL get_joy (AIN_Device *device, struct joystick *joy)
{
	BOOL ret = FALSE;
	int connected = 0;

	Printf("%s\n",__FUNCTION__);

	if (device->Type == AINDT_JOYSTICK) 
	{
		Printf("is a joystick\n");

		AIN_Query(joy ->controller, device -> DeviceID,AINQ_CONNECTED,0,&connected,4 );
		if (connected)
		{
			if (found_joysticks==joy->num)
			{
				Printf("Devce Type %ld \tID %lx \tdevce Name %s\n",
					device -> Type, 
					device -> DeviceID,
					device -> DeviceName);

				ret = TRUE;
				joy -> id = device -> DeviceID;
			}
			found_joysticks ++;
		}
	}
	else
	{
		Printf("Not a joystick, device type is %ld\n",device->Type);
	}

	return ret;
}

void init_joysticks()
{
	int nn;
	int n;
	struct TagItem AIN_Tags[2];

	joystick_msgport = (struct MsgPort *) AllocSysObjectTags(ASOT_PORT, TAG_END );	
	if (!joystick_msgport) return ;

	AIN_Tags[0].ti_Tag = AINCC_Port;
	AIN_Tags[0].ti_Data = (ULONG) joystick_msgport;
	AIN_Tags[1].ti_Tag = TAG_END;

	for (nn=0;nn<4;nn++)
	{
		// joy0 is mouse port on Amiga (2 player games)
		// joy1 is joystick port (1 player games)

		switch (nn)
		{
			case 0: n=1;	break;
			case 1: n=0;	break;
			default: n=nn;	break;
		}

		joysticks[n].id = -1;
		joysticks[n].num = nn;
		joysticks[n].controller = AIN_CreateContext (1, (TagItem*) &AIN_Tags);

		if (joysticks[n].controller)
		{
			Printf("looking for joystcik #%ld\n",n);

			found_joysticks = 0;
			AIN_EnumDevices(joysticks[n].controller, (void *) get_joy, (void *) &joysticks[n] );
		}
		else
		{
			Printf("Amiga input can't create context\n");
		}
	}

	// show found joysticks
	for (n=0;n<4;n++)
	{
		if (joysticks[n].id>0)
		{
			 Printf("joystick #%ld Using device ID %lx\n",n,joysticks[n].id);
		}
	}

	// obtain joysticks
	for (n=0;n<4;n++)
	{
		if (joysticks[n].id>0)
		{
			joysticks[n].handle = AIN_ObtainDevice(joysticks[n].controller, joysticks[n].id );

			if (joysticks[n].handle)
			{
				joysticks[n].res =  AIN_SetDeviceParameter(joysticks[n].controller,joysticks[n].handle,AINDP_EVENT,TRUE);
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
		1<<0,		//0
		1<<0,		//1
		1<<0,		//2
		1<<0,		//3
		1<<0,		//4
		1<<1,		//5	// fire2
		1<<0,		//6	// fire1
		1<<2,		//7
		1<<3,		//8
		1<<4,		//9
		1<<5,		//10
		1<<6,		//11
		1<<7,		//12
		1<<8,		//13
		1<<9,		//14
		1<<10,		//15
		1<<11,		//16
		1<<12,		//17
		1<<13,		//18
		1<<14,		//19
		1<<15,		//20
		1<<16,		//21
		1<<17,		//22
	};

void joy_stick(int joy,void *controller)
{
	int j,n;
	AIN_InputEvent *ain_mess;

	while ( ain_mess = AIN_GetEvent(controller))
	{
		j = 0;

		for (n=0;n<4;n++)	if ( (unsigned int) joysticks[n].id == ain_mess -> ID) j =n;


		switch (ain_mess -> Type)
		{
			case AINET_AXIS: // axes
					break;

			case AINET_BUTTON:
					{
						unsigned int bit = lot[ain_mess -> Index];


//						Printf("value %ld index %ld\n",ain_mess -> Value, ain_mess -> Index);
//						Delay(20);

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

			case AINET_HAT:
					amiga_joystick_dir[j] = dir[ ain_mess -> Value ];
					break;
		}
/*
		printf("amiga_joystick_button[%d] = %",j);
		print_bin(amiga_joystick_button[j]);
		printf("\n");

		printf("amiga_joystick_dir[%d] = %",j);
		print_bin(amiga_joystick_dir[j]);
		printf("\n");
*/
		AIN_FreeEvent(controller,ain_mess);
	}
}


