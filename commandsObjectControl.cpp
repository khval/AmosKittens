#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <stdint.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#endif

#include "debug.h"
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commandsGfx.h"
#include "commandsScreens.h"
#include "errors.h"
#include "engine.h"

extern int sig_main_vbl;

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

extern int current_screen;
extern int autoView;

extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];

int priorityReverse = 0;

int find_zone_in_any_screen( int hx, int hy)
{
	int z,x,y;
	struct zone *zz;
	struct retroScreen *s;

	for (z=0;z<zones_allocated;z++)
	{
		if ((zones[z].screen>-1) && (zones[z].screen<8))
		{
			if (s = screens[zones[z].screen])
			{
				x = XScreen_formula( s, hx );
				y = YScreen_formula( s, hy );
				zz = &zones[z];
				if ((x>zz->x0)&&(y>zz->y0)&&(x<zz->x1)&&(y<zz->y1))	return z;
			}
		}
	}
	return -1;
}

int find_zone_in_only_screen( int screen, int hx, int hy)
{
	int z,x,y;
	struct zone *zz;
	struct retroScreen *s;

	for (z=0;z<zones_allocated;z++)
	{
		if (zones[z].screen == screen)
		{
			if (s = screens[zones[z].screen])
			{
				x = XScreen_formula( s, hx );
				y = YScreen_formula( s, hy );
				zz = &zones[z];
				if ((x>zz->x0)&&(y>zz->y0)&&(x<zz->x1)&&(y<zz->y1))	return z;
			}
		}
	}
	return -1;
}

char *ocXMouse(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(engine_mouse_x);
	return tokenBuffer;
}

char *ocYMouse(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(engine_mouse_y);
	return tokenBuffer;
}

char *ocMouseKey(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(engine_mouse_key);
	return tokenBuffer;
}

char *ocMouseClick(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(engine_mouse_key);
	return tokenBuffer;
}

char *ocHide(struct nativeCommand *cmd, char *tokenBuffer)
{
	// hide mouse pointer.
	return tokenBuffer;
}

char *_ocMouseLimit( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *ocMouseLimit(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _ocMouseLimit, tokenBuffer );
	return tokenBuffer;
}

char *_ocReserveZone( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (zones) free(zones);
	zones_allocated = 0;

	if (args == 1)
	{
		int newzones = getStackNum( stack );

		if (newzones)
		{
			zones = (struct zone *) malloc( sizeof(struct zone) * (newzones+1) );
			zones_allocated = (newzones+1);
		}
	}
	else setError(22,data->tokenBuffer);;

	popStack( stack - data->stack );
	return NULL;
}

char *ocReserveZone(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _ocReserveZone, tokenBuffer );
	return tokenBuffer;
}

char *_ocZoneStr( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	char *newstr = NULL;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 2)
	{
		char *txt = getStackString( stack-1 );
		int zone = getStackNum( stack );

		dump_stack();

		if ((txt)&&(zone>-1)&&(zone<zones_allocated))
		{
			newstr = (char *) malloc( strlen(txt) + 6 + 1 ); 
			if (newstr)
			{
				sprintf(newstr,"%cZ0%s%cR%c",27,txt,27,48+ zone );
			}
		}

		if (newstr == NULL) setError(60,data->tokenBuffer);
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	if (newstr) setStackStr( newstr );

	return NULL;
}

char *ocZoneStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);


	stackCmdParm( _ocZoneStr, tokenBuffer );
	return tokenBuffer;
}

char *_ocMouseZone( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int rz;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	rz = find_zone_in_any_screen( engine_mouse_x, engine_mouse_y );

	popStack( stack - data->stack );
	setStackNum( rz );

	return NULL;
}

char *ocMouseZone(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _ocMouseZone, tokenBuffer );
	return tokenBuffer;
}

char *_ocChangeMouse( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *ocChangeMouse(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _ocChangeMouse, tokenBuffer );
	return tokenBuffer;
}

char *_ocSetZone( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 5)
	{
		int z = getStackNum( stack -4 );
		int x0 = getStackNum( stack -3 );
		int y0 = getStackNum( stack -2 );
		int x1 = getStackNum( stack -1 );
		int y1 = getStackNum( stack );

		if ((zones)&&(z>-1)&&(z<zones_allocated))
		{
			zones[z].screen = current_screen;
			zones[z].x0 = x0;
			zones[z].y0 = y0;
			zones[z].x1 = x1;
			zones[z].y1 = y1;
		}
	}
	else setError(22,data->tokenBuffer);;

	popStack( stack - data->stack );
	return NULL;
}

char *ocSetZone(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _ocSetZone, tokenBuffer );
	return tokenBuffer;
}

char *_ocResetZone( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 1)
	{
		int z = getStackNum( stack );

		if ((zones)&&(z>-1)&&(z<zones_allocated))
		{
			zones[z].screen = NULL;
			zones[z].x0 = 0;
			zones[z].y0 = 0;
			zones[z].x1 = 0;
			zones[z].y1 = 0;
		}
	}
	else setError(22,data->tokenBuffer);;

	popStack( stack - data->stack );
	return NULL;
}

char *ocResetZone(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _ocResetZone, tokenBuffer );
	return tokenBuffer;
}

char *ocShowOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *ocHideOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *ocPriorityOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *ocPriorityOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *ocPriorityReverseOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	priorityReverse = 0;
	return tokenBuffer;
}

char *ocPriorityReverseOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	priorityReverse = 1;
	return tokenBuffer;
}

char *ocAutoViewOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	autoView = 0;
	return tokenBuffer;
}

char *ocAutoViewOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	autoView = 1;
	return tokenBuffer;
}

char *ocShow(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	autoView = 1;
	return tokenBuffer;
}

char *ocView(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	autoView =1;
	return tokenBuffer;
}

// lots of dummy functions don't do anything, I'm just trying to see, if can get something running!!.

char *ocUpdateOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

char *ocUpdate(struct nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

char *ocSynchroOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

char *ocSynchroOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

char *_ocJUp( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = FALSE;

	if (args == 1)
	{
		int j = getStackNum( stack );
		if ((j>-1)&&(j<4)) ret = (amiga_joystick_dir[j] & joy_up) ? TRUE : FALSE;
	}
	else setError(22,data->tokenBuffer);;
	popStack( stack - data->stack );
	setStackNum( ret );

	if (ret) 	printf("%s:%d\n",__FUNCTION__,__LINE__);

	return NULL;
}

char *ocJUp(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _ocJUp, tokenBuffer );
	return tokenBuffer;
}

char *_ocJDown( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = FALSE;

	if (args == 1)
	{
		int j = getStackNum( stack );
		if ((j>-1)&&(j<4)) ret = (amiga_joystick_dir[j] & joy_down) ? TRUE : FALSE;
	}
	else setError(22,data->tokenBuffer);;
	popStack( stack - data->stack );
	setStackNum( ret );

	if (ret) 	printf("%s:%d\n",__FUNCTION__,__LINE__);
	return NULL;
}

char *ocJDown(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _ocJDown, tokenBuffer );
	return tokenBuffer;
}

char *_ocJLeft( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = FALSE;

	if (args == 1)
	{
		int j = getStackNum( stack );
		if ((j>-1)&&(j<4)) ret = (amiga_joystick_dir[j] & joy_left) ? TRUE : FALSE;
	}
	else setError(22,data->tokenBuffer);;
	popStack( stack - data->stack );
	setStackNum( ret );

	if (ret) 	printf("%s:%d\n",__FUNCTION__,__LINE__);

	return NULL;
}

char *ocJLeft(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _ocJLeft, tokenBuffer );
	return tokenBuffer;
}

char *_ocJRight( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = FALSE;

	if (args == 1)
	{
		int j = getStackNum( stack );
		if ((j>-1)&&(j<4)) ret = (amiga_joystick_dir[j] & joy_right) ? TRUE : FALSE;
	}
	else setError(22,data->tokenBuffer);;
	popStack( stack - data->stack );
	setStackNum( ret );
	if (ret) 	printf("%s:%d\n",__FUNCTION__,__LINE__);

	return NULL;
}

char *ocJRight(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _ocJRight, tokenBuffer );
	return tokenBuffer;
}

char *_ocSynchro( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 1)
	{
	}
	else setError(22,data->tokenBuffer);;

	popStack( stack - data->stack );
	return NULL;
}

char *ocSynchro(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _ocSynchro, tokenBuffer );
	return tokenBuffer;
}

char *ocUpdateOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *_ocUpdateEvery( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 1)
	{
	}
	else setError(22,data->tokenBuffer);;

	popStack( stack - data->stack );
	return NULL;
}

char *ocUpdateEvery(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _ocUpdateEvery, tokenBuffer );
	return tokenBuffer;
}

char *_ocFire( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 1)
	{
		int j = getStackNum( stack );
		if ((j>-1)&&(j<4)) ret = amiga_joystick_button[j];
	}
	else setError(22,data->tokenBuffer);;

	popStack( stack - data->stack );
	setStackNum( ret );
	return NULL;
}

char *ocFire(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _ocFire, tokenBuffer );
	return tokenBuffer;
}

char *_ocMakeMask( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 0:
		case 1:
			break;
		default:
			setError(22,data->tokenBuffer);;
			break;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *ocMakeMask(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _ocMakeMask, tokenBuffer );
	return tokenBuffer;
}



char *_ocHZone( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = -1;
	int s=-1,x=-1,y=-1;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
				x = getStackNum( stack-1 );
				y = getStackNum( stack );
				ret = find_zone_in_any_screen( x,y );
				break;
		case 3:
				s = getStackNum( stack-2 );
				x = getStackNum( stack-1 );	
				y = getStackNum( stack );
				ret = find_zone_in_only_screen( s, x,y );
				break;
		default:
				setError(22, data-> tokenBuffer);
	}

	printf("HZone(%d,%d,%d) is %d\n",s,x,y,ret);

	popStack( stack - data->stack );
	setStackNum( ret );
	return NULL;
}

char *ocHZone(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _ocHZone, tokenBuffer );
	return tokenBuffer;
}

char *_ocJoy( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 1)
	{
		int j = getStackNum( stack );
		if ((j>-1)&&(j<4)) ret = amiga_joystick_dir[j] | (amiga_joystick_button[j] << 4);
	}
	else setError(22,data->tokenBuffer);;

	popStack( stack - data->stack );
	setStackNum( ret );
	return NULL;
}

char *ocJoy(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _ocJoy, tokenBuffer );
	return tokenBuffer;
}

char *_ocIconMakeMask( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 1)
	{
		int n = getStackNum( stack );
	}
	else setError(22,data->tokenBuffer);;

	popStack( stack - data->stack );
	setStackNum( ret );
	return NULL;
}

char *ocIconMakeMask(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _ocIconMakeMask, tokenBuffer );
	return tokenBuffer;
}

