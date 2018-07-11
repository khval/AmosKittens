#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "debug.h"
#include <string>
#include <iostream>
#include <proto/retroMode.h>

#include "stack.h"
#include "amosKittens.h"
#include "commandsGfx.h"
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

int XScreen_formula( struct retroScreen *screen );
int YScreen_formula( struct retroScreen *screen );

char *_ocMouseZone( struct glueCommands *data, int nextToken )
{
	struct retroScreen *s;
	struct zone *zz;
	int args = stack - data->stack +1 ;
	int z,rz = -1;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	for (z=0;z<zones_allocated;z++)
	{
		if ((zones[z].screen>-1) && (zones[z].screen<8))
		{
			if (s = screens[zones[z].screen])
			{
				int x = XScreen_formula( s );
				int y = YScreen_formula( s );
				zz = &zones[z];
				if ((x>zz->x0)&&(y>zz->y0)&&(x<zz->x1)&&(y<zz->y1))	rz = z;
			}
		}
	}

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
	autoView = 1;
	return tokenBuffer;
}

char *ocView(struct nativeCommand *cmd, char *tokenBuffer)
{
	autoView =1;
	return tokenBuffer;
}



