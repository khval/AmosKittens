#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
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
#include "kittyErrors.h"
#include "engine.h"
#include "amosstring.h"

#include "commandsData.h"

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

extern void __wait_vbl();

extern int bobDoUpdate;
extern int bobAutoUpdate;
extern int bobUpdateEvery;
extern int bobDoUpdateEnable;

int priorityReverse = 0;

extern struct retroEngine *engine ;

#ifdef __amigaos3__
extern UWORD *ImagePointer ;
#endif

#ifdef __amigaos4__
extern uint32 *ImagePointer ;
extern Object *objectPointer ;
#endif

int find_zone_in_any_screen_hard( int hx, int hy)
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

int find_zone_in_any_screen_pixel( int hx, int hy)
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

int find_zone_in_only_screen_hard( int screen, int hx, int hy)
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

int find_zone_in_only_screen_pixel( int screen, int x, int y)
{
	int z;
	struct zone *zz;

	for (z=0;z<zones_allocated;z++)
	{
		if (zones[z].screen == screen)
		{
			zz = &zones[z];
			if ((x>zz->x0)&&(y>zz->y0)&&(x<zz->x1)&&(y<zz->y1))	return z;
		}
	}
	return -1;
}


char *ocXMouse(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(engine_mouse_x/2+hardware_upper_left);
	return tokenBuffer;
}

char *ocYMouse(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(engine_mouse_y/2+hardware_upper_top);
	return tokenBuffer;
}

char *ocMouseKey(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((short *) (tokenBuffer));

	if ( correct_order( getLastProgStackToken(),  next_token ) == false )
	{
		dprintf("---hidden ( symbol \n");
		setStackHiddenCondition();
	}

	setStackNum(engine_mouse_key);

	kittyStack[stack].state = state_none;
	flushCmdParaStack( next_token );

	return tokenBuffer;
}

char *ocMouseClick(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(engine_mouse_key);
	return tokenBuffer;
}

char *ocHide(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	engine_lock();
	engine_ShowMouse( false );
	engine_unlock();

	return tokenBuffer;
}

char *ocShow(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	engine_lock();
	engine_ShowMouse( true );
	engine_unlock();

	return tokenBuffer;
}

char *_ocMouseLimit( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	NYI(__FUNCTION__);

	popStack( stack - data->stack );
	return NULL;
}

char *ocMouseLimit(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ocMouseLimit, tokenBuffer );
	return tokenBuffer;
}

char *_ocReserveZone( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ocReserveZone, tokenBuffer );
	return tokenBuffer;
}

char *_ocZoneStr( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct stringData *newstr = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 2)
	{
		struct stringData *txt = getStackString( stack-1 );
		int zone = getStackNum( stack );

		if ((txt)&&(zone>-1)&&(zone<zones_allocated))
		{
			newstr = alloc_amos_string( txt -> size + 6 );

			if (newstr)
			{
				sprintf( &newstr -> ptr,"%cZ0%s%cR%c",27,txt,27,48+ zone );
				newstr -> size = strlen(&newstr -> ptr);
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);


	stackCmdParm( _ocZoneStr, tokenBuffer );
	return tokenBuffer;
}


char *ocMouseZone(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	int rz = find_zone_in_any_screen_hard( engine_mouse_x, engine_mouse_y );
	setStackNum( rz );

	return tokenBuffer;
}

extern struct retroSprite *patterns ;

char getPixelFromFrame(struct retroFrameHeader *frame , int x, int y )
{
	int _x = x %  frame -> width;
	int _y = y % frame -> height;
	return frame -> data[ frame -> bytesPerRow * _y + _x  ];
}

UWORD getUWord( struct retroFrameHeader *frame, int x, int y, int plane )
{
	UWORD ret = 0;
	int readBit = 1 << plane;
	int r;
	int xx;
	int writeBit[] = {
		0x8000 | 0x4000,	// 0
		0x2000 | 0x1000,	// 1
		0x0800 | 0x0400,	// 2
		0x0200 | 0x0100,	// 3
		0x0080 | 0x0040,	// 4
		0x0020 | 0x0010,	// 5
		0x0008 | 0x0004, 	// 6
		0x0002 | 0x0001};	// 7

	for ( xx = 0; xx < 8; xx++ )
	{
		r = getPixelFromFrame(frame , xx + (x*8),  y ) & readBit;
		ret |= r ? writeBit[ xx ] : 0;
	}
	return ret;
}

#ifdef __amigaos3__

void frameToPointer(struct retroFrameHeader *frame)
{
	if (ImagePointer)
	{
		int x,y,p,r;

		ImagePointer[0]=255;
		ImagePointer[1]=255;

		for (y=0;y<frame -> height;y++)
		{
			for (x=0;x<2;x++)
			{
				for (p=0;p<2;p++)
				{
					r = getUWord( frame, 0, y, p );
					if (y<16) ImagePointer[(4*y)+2 + p ] = r;
					if (y<16) ImagePointer[(4*y)+4 + p ] = r;
				}
			}
		}

		SetPointer( engine -> window, ImagePointer, 32, 32, 0, 0);
	}
}

#endif

#ifdef __amigaos4__

void frameToPointer(struct retroFrameHeader *frame)
{
	int c;
	int x,y;
	int xx,yy;
	int ww,hh;
	uint32 argb;

	memset( ImagePointer, 0, 64*64 * sizeof(uint32) );
	
	if (ww>16) ww = 16;
	if ( hh>16) hh = 16; 

	for (y=0;y<hh;y++)
	{
		for (x=0;x<ww;x++)
		{
			c = getPixelFromFrame( frame,  x,  y );

			argb = 0;

			if (c)
			{
				c+=15;
				argb = 0xFF000000;
				argb |= DefaultPalette[c].r	* 0x0010000;
				argb |= DefaultPalette[c].g	* 0x0000100;
				argb |= DefaultPalette[c].b	* 0x0000001;
			}

			xx = x * 2;
			yy = y * 2;

			ImagePointer[ yy * 64 + xx ] = argb ;
			ImagePointer[ (yy+1) * 64 + xx ] = argb ;
			ImagePointer[ yy * 64 + (xx+1) ] = argb ;
			ImagePointer[ (yy+1) * 64 + (xx+1) ] = argb ;

		}
	}

	if (objectPointer)
	{
		SetWindowPointer( engine -> window, 
				WA_Pointer, objectPointer, TAG_END );
	}
}


#endif


char *_ocChangeMouse( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroFrameHeader *frame = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					int image = getStackNum( stack ) -1;
					if (image>-1) frame = patterns ? patterns -> frames + image : NULL;
				}
				break;

		default:	popStack( stack - data->stack );
				setError(22, data -> tokenBuffer );
				break;
	}

	engine_lock();
	if (engine_ready())
	{
		frameToPointer( frame );
	}
 	engine_unlock();


	return NULL;
}

char *ocChangeMouse(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ocChangeMouse, tokenBuffer );
	return tokenBuffer;
}

char *_ocSetZone( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ocSetZone, tokenBuffer );
	return tokenBuffer;
}

char *_ocResetZone( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		switch (kittyStack[stack].type)
		{
			case type_none:
					setError(23,data->tokenBuffer);
					break;

			case type_int:
					{
						int z = kittyStack[stack].integer.value;

						if ((zones)&&(z>-1)&&(z<zones_allocated))
						{
							zones[z].screen = NULL;
							zones[z].x0 = 0;
							zones[z].y0 = 0;
							zones[z].x1 = 0;
							zones[z].y1 = 0;
						}
					}
					popStack( stack - data->stack );
					return NULL;
		}
	}

	setError(22,data->tokenBuffer);
	popStack( stack - data->stack );
	return NULL;
}

char *ocResetZone(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ocResetZone, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}


char *ocShowOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	NYI(__FUNCTION__);
	return tokenBuffer;
}

char *ocHideOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	NYI(__FUNCTION__);
	return tokenBuffer;
}


char *ocPriorityOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	NYI(__FUNCTION__);
	return tokenBuffer;
}

char *ocPriorityOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	NYI(__FUNCTION__);
	return tokenBuffer;
}

char *ocPriorityReverseOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	priorityReverse = 0;
	return tokenBuffer;
}

char *ocPriorityReverseOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
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

char *ocView(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	autoView =1;
	return tokenBuffer;
}

// lots of dummy functions don't do anything, I'm just trying to see, if can get something running!!.

char *ocUpdateOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	int prev = bobAutoUpdate;
	bobAutoUpdate = 0;
	bobDoUpdateEnable = 0;
	return tokenBuffer;
}

char *ocUpdateOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int prev = bobAutoUpdate;
	bobAutoUpdate = 1;
	bobDoUpdateEnable = 1;
	return tokenBuffer;
}

char *ocUpdate(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("bobAutoUpdate %d\n",bobAutoUpdate);
	bobDoUpdate = 1;
	__wait_vbl();
	return tokenBuffer;
}

char *ocSynchroOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	NYI(__FUNCTION__);
	return tokenBuffer;
}

char *ocSynchroOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	NYI(__FUNCTION__);
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _ocJRight, tokenBuffer );
	return tokenBuffer;
}

char *_ocSynchro( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	NYI(__FUNCTION__);

	if (args == 1)
	{
	}
	else setError(22,data->tokenBuffer);;

	popStack( stack - data->stack );
	return NULL;
}

char *ocSynchro(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _ocSynchro, tokenBuffer );
	return tokenBuffer;
}


char *_ocUpdateEvery( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	NYI(__FUNCTION__);


	if (args == 1)
	{
	}
	else setError(22,data->tokenBuffer);;

	popStack( stack - data->stack );
	return NULL;
}

char *ocUpdateEvery(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _ocUpdateEvery, tokenBuffer );
	return tokenBuffer;
}

char *_ocFire( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _ocFire, tokenBuffer );
	return tokenBuffer;
}

char *_ocMakeMask( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ocMakeMask, tokenBuffer );
	return tokenBuffer;
}

char *_ocHZone( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = -1;
	int s=-1,x=-1,y=-1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
				x = getStackNum( stack-1 );
				y = getStackNum( stack );
				ret = find_zone_in_any_screen_hard( x,y );
				break;
		case 3:
				s = getStackNum( stack-2 );
				x = getStackNum( stack-1 );	
				y = getStackNum( stack );
				ret = find_zone_in_only_screen_hard( s, x,y );
				break;
		default:
				setError(22, data-> tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( ret );
	return NULL;
}

char *ocHZone(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _ocHZone, tokenBuffer );
	return tokenBuffer;
}

char *_ocZone( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = -1;
	int s=-1,x=-1,y=-1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
				x = getStackNum( stack-1 );
				y = getStackNum( stack );
				ret = find_zone_in_only_screen_pixel( current_screen, x,y );
				break;
		case 3:
				s = getStackNum( stack-2 );
				x = getStackNum( stack-1 );
				y = getStackNum( stack );
				ret = find_zone_in_only_screen_pixel( s,x,y );
				break;

		default:
				setError(22, data-> tokenBuffer);
	}

	printf("Zone(%d,%d,%d) is %d\n",s,x,y,ret);

	popStack( stack - data->stack );
	setStackNum( ret );
	return NULL;
}

char *ocZone(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _ocZone, tokenBuffer );
	return tokenBuffer;
}

char *_ocJoy( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _ocJoy, tokenBuffer );
	return tokenBuffer;
}

/*
char *_ocIconMakeMask( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ocIconMakeMask, tokenBuffer );
	return tokenBuffer;
}
*/

char *ocMouseScreen(struct nativeCommand *cmd, char *tokenBuffer)
{
	struct retroScreen *screen;
	int x,y;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (int s=0;s<8;s++)
	{
		if (screen = screens[s])
		{
			x = XScreen_formula( screen, engine_mouse_x );
			y = YScreen_formula( screen, engine_mouse_y );

			if	((y>=0)&&(y<screen -> displayHeight)
			&&	(x>=0)&&(x<screen -> displayWidth))
			{
				setStackNum(s);
				return tokenBuffer;
			}
		}
	}
	setStackNum(-123456789);

	return tokenBuffer;
}

