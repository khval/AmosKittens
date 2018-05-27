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

int _reserve_zones_ = 20;

extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];

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

char *_ocMouseLimit( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *ocMouseLimit(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _ocMouseLimit, tokenBuffer );
	return tokenBuffer;
}

char *_ocReserveZone( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *ocReserveZone(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _ocReserveZone, tokenBuffer );
	return tokenBuffer;
}

char *_ocZoneStr( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	char *newstr = NULL;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 2)
	{
		char *txt = _stackString( stack-1 );
		int zone = _stackInt( stack );

		dump_stack();
		printf("zone: %d\n",zone);

		if ((txt)&&(zone>-1)&&(zone<_reserve_zones_))
		{
			newstr = (char *) malloc( strlen(txt) + 6 + 1 ); 
			if (newstr)
			{
				sprintf(newstr,"%cZ0%s%cR%c",27,txt,27,48+ zone );
			}
		}

		if (newstr == NULL) setError(60);
	}
	else setError(22);

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

char *_ocMouseZone( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *ocMouseZone(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _ocMouseZone, tokenBuffer );
	return tokenBuffer;
}

char *_ocChangeMouse( struct glueCommands *data )
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
