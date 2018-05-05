#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <exec/emulation.h>
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

extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;

int current_screen = 0;
int pen0=2, pen1,pen2;
int xgr = 0,  ygr = 0;

char *_gfxScreenOpen( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==5)
	{
		int screen_num = _stackInt( stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			current_screen = screen_num;

			// Kitty ignores colors we don't care, allways 256 colors.

			engine_lock();
			if (screens[screen_num]) retroCloseScreen(&screens[screen_num]);
			screens[screen_num] = retroOpenScreen(_stackInt( stack-3 ),_stackInt( stack-2 ),_stackInt( stack ));

			if (screens[screen_num])
			{
				retroApplyScreen( screens[screen_num], video, 0, 0,
					screens[screen_num] -> realWidth,screens[screen_num]->realHeight );

				retroBAR( screens[screen_num], 0,0, screens[screen_num] -> realWidth,screens[screen_num]->realHeight, 1 );

				set_default_colors( screen_num );
			}

			engine_unlock();

			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxScreenClose( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = _stackInt( stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			current_screen = screen_num;

			engine_lock();
			if (screens[screen_num]) retroCloseScreen(&screens[screen_num]);
			engine_unlock();

			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}


char *_gfxScreenDisplay( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==5)
	{
		int screen_num = _stackInt( stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (kittyStack[stack-3].type ==  type_int) screens[screen_num] -> scanline_x = _stackInt( stack-3 );
			if (kittyStack[stack-2].type ==  type_int) screens[screen_num] -> scanline_y = _stackInt( stack-2 );
			if (kittyStack[stack-1].type ==  type_int) screens[screen_num] -> displayWidth = _stackInt( stack-1 );
			if (kittyStack[stack].type ==  type_int) screens[screen_num] -> displayHeight = _stackInt( stack );

			engine_lock();

			if (screens[screen_num])
				retroApplyScreen( screens[screen_num], video, 
					screens[screen_num] -> scanline_x,
					screens[screen_num] -> scanline_y,
					screens[screen_num] -> displayWidth,
					screens[screen_num] -> displayHeight );

			retroClearVideo( video );
			retroDrawVideo( video );

			engine_unlock();

			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_gfxScreenOffset( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==3)
	{
		int screen_num = _stackInt( stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			engine_lock();

			retroClearVideo( video );
			retroDrawVideo( video );
			engine_unlock();

			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_gfxScreen( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = _stackInt( stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			current_screen = screen_num;
			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_gfxFlash( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}


char *_gfxColour( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int num = 0;
	unsigned int color;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
		num = _stackInt( stack-1 );

		if ((num>-1)&&(num<256))
		{
			color = _stackInt( stack );

			if (screens[current_screen])
			{
				retroScreenColor( screens[current_screen], 	num, ((color &0xF00) >>8) * 17, ((color & 0xF0) >> 4) * 17, (color & 0xF)  * 17);
				printf("Screen %d,Color %d,R %d,G %d,B %d\n",current_screen, num, (color &0xF00 >>8) * 17, (color & 0xF0 >> 4) * 17, (color & 0xF)  * 17);

			}
			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *gfxScreenOpen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenOpen, tokenBuffer );
	return tokenBuffer;
}

char *gfxLowres(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(retroLowres);
	return tokenBuffer;
}

char *gfxHires(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(retroHires);
	return tokenBuffer;
}

char *gfxMouseKey(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(engine_mouse_key);
	return tokenBuffer;
}

char *gfxWaitVbl(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (( sig_main_vbl )&&( engine_started ))
	{
		Wait(1<<sig_main_vbl);
	}
	else
	{
		Delay(1);
	}
	return tokenBuffer;
}

char *gfxScreenDisplay(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenDisplay, tokenBuffer );
	return tokenBuffer;
}

char *gfxColour(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxColour, tokenBuffer );
	return tokenBuffer;
}

char *_gfxBox( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr ,y0 = ygr,x1,y1;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==4)
	{
		stack_get_if_int( stack-3, &x0 );
		stack_get_if_int( stack-2, &y0 );
		xgr = x1 = _stackInt( stack-1 );
		ygr = y1 = _stackInt( stack );

		if (screens[current_screen]) retroBox( screens[current_screen], x0,y0,x1,y1,pen0 );
	}
	else setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxBar( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr ,y0 = ygr,x1,y1;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==4)
	{
		stack_get_if_int( stack-3, &x0 );
		stack_get_if_int( stack-2, &y0 );
		xgr = x1 = _stackInt( stack-1 );
		ygr = y1 = _stackInt( stack );

		if (screens[current_screen]) retroBAR( screens[current_screen], x0,y0,x1,y1,pen0 );
	}
	else setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxDraw( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr ,y0 = ygr,x1,y1;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			xgr = x1 = _stackInt( stack-1 );
			ygr = y1 = _stackInt( stack );
			if (screens[current_screen]) retroLine( screens[current_screen], x0,y0,x1,y1,pen0 );
			break;

		case 4:
			stack_get_if_int( stack-3, &x0 );
			stack_get_if_int( stack-2, &y0 );
			xgr = x1 = _stackInt( stack-1 );
			ygr = y1 = _stackInt( stack );

			if (screens[current_screen]) retroLine( screens[current_screen], x0,y0,x1,y1,pen0 );
			break;

		default:
			setError(22);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxPolygon( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int array[100*2];

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ( (args>=4) && ((args&1) == 0) && (screens[current_screen]) )
	{
		int n;
		int _stack = data -> stack;

		for (n=0;n<args;n++)
		{
			array[n] = _stackInt( _stack++ );
		}

		retroPolyGonArray( screens[current_screen], pen0, args, array );
	}
	else	setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxPolyline( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ( (args>2) && ((args&1) == 0) && (screens[current_screen]) )
	{
		int coordinates = args >> 1;
		int lx,ly,x,y,_stack,n;

		_stack = data -> stack;

		lx = _stackInt( _stack++ );
		ly = _stackInt( _stack++ );

		for (n=1;n<coordinates;n++)
		{
			x = _stackInt( _stack++ );
			y = _stackInt( _stack++ );
			retroLine( screens[current_screen], lx,ly,x,y,pen0 );
			lx = x;
			ly=y;
		}
	}
	else	setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxCircle( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int x0=xgr,y0=ygr,r;

	printf("%s:%d\n",__FUNCTION__,__LINE__);
	dump_stack();

	if (args==3)
	{
		stack_get_if_int( stack-2, &x0 );
		stack_get_if_int( stack-1, &y0 );
		r = _stackInt( stack );

		if (screens[current_screen]) retroEllipse( screens[current_screen], x0,y0,r,r,0,pen0 );
	}
	else setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxEllipse( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int x0=xgr,y0=ygr,r0,r1;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==4)
	{
		stack_get_if_int( stack-3, &x0 );
		stack_get_if_int( stack-2, &y0 );
		r0 = _stackInt( stack-1 );
		r1 = _stackInt( stack );

		if (screens[current_screen]) retroEllipse( screens[current_screen], x0,y0,r0,r1,0,pen0 );
	}
	else setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *gfxBox(struct nativeCommand *cmd, char *tokenBuffer)
{
	kittyStack[stack].type = type_none;
	stackCmdNormal( _gfxBox, tokenBuffer );
	return tokenBuffer;
}

char *gfxBar(struct nativeCommand *cmd, char *tokenBuffer)
{
	kittyStack[stack].type = type_none;
	stackCmdNormal( _gfxBar, tokenBuffer );
	return tokenBuffer;
}

char *gfxDraw(struct nativeCommand *cmd, char *tokenBuffer)
{
	kittyStack[stack].type = type_none;
	stackCmdNormal( _gfxDraw, tokenBuffer );
	return tokenBuffer;
}

char *gfxCircle(struct nativeCommand *cmd, char *tokenBuffer)
{
	kittyStack[stack].type = type_none;
	stackCmdNormal( _gfxCircle, tokenBuffer );
	return tokenBuffer;
}

char *gfxEllipse(struct nativeCommand *cmd, char *tokenBuffer)
{
	kittyStack[stack].type = type_none;
	stackCmdNormal( _gfxEllipse, tokenBuffer );
	return tokenBuffer;
}

char *_gfxInk( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int x0,y0,r0,r1;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			pen0 = _stackInt( stack );
			break;
		case 2:
			stack_get_if_int( stack-1, &pen0 );
			stack_get_if_int( stack, &pen1 );
			break;
		case 3:
			stack_get_if_int( stack-2, &pen0 );
			stack_get_if_int( stack-1, &pen1 );
			stack_get_if_int( stack, &pen2 );
			break;
		default: 
			setError(22);
			break;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxInk(struct nativeCommand *cmd, char *tokenBuffer)
{
	// make sure there is nothing on the stack.
	// this command takes nothing as a arg.

	kittyStack[stack].type = type_none;
	stackCmdNormal( _gfxInk, tokenBuffer );

	return tokenBuffer;
}

char *gfxXMouse(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(engine_mouse_x);
	return tokenBuffer;
}

char *gfxYMouse(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(engine_mouse_y);
	return tokenBuffer;
}

char *gfxCursOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

char *gfxFlash(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxFlash, tokenBuffer );
	return tokenBuffer;
}

char *gfxFlashOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (screens[current_screen])
	{
		struct retroFlashTable **ptr = screens[current_screen]->allocatedFlashs;
		struct retroFlashTable **ptr_end = screens[current_screen]->allocatedFlashs_end;

		engine_lock();

		for ( ; ptr<ptr_end;ptr++)
		{
			if (*ptr) retroDeleteFlash( screens[current_screen], (*ptr) -> color );
		}

		engine_unlock();
	}

	return tokenBuffer;
}

char *_gfxPlot( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr, y0 = ygr,c;

	printf("%s:%d\n",__FUNCTION__,__LINE__);
	dump_stack();

	switch (args)
	{
		case 2:
			stack_get_if_int( stack-1, &x0 );
			stack_get_if_int( stack, &y0 );
			if (screens[current_screen]) retroPixel( screens[current_screen], x0,y0,pen0 );
			break;
		case 3:
			stack_get_if_int( stack-2, &x0 );
			stack_get_if_int( stack-1, &y0 );
			c = _stackInt( stack );
			if (screens[current_screen]) retroPixel( screens[current_screen], x0,y0,c );
			break;
		default:
			setError(22);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxPoint( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr, y0 = ygr,c;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			stack_get_if_int( stack-1, &x0 );
			stack_get_if_int( stack, &y0 );
			popStack( stack - data->stack );
			if (screens[current_screen]) _stackInt( retroPoint(screens[current_screen], x0, y0) );
			break;
		default:
			popStack( stack - data->stack );
			setError(22);
	}

	return NULL;
}

char *_gfxGrLocate( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	switch (args)
	{
		case 2:
			xgr = _stackInt( stack -1 );
			ygr = _stackInt( stack );
			break;
		default:
			setError(22);
	}
	popStack( stack - data->stack );

	return NULL;
}

char *_gfxGetColour( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int c;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	switch (args)
	{
		case 1:
			c = _stackInt( stack );

			if ((c>-1)&&(c<256))
			{
				if (screens[current_screen])	// check if screen is open.
				{
					struct retroRGB rgb = screens[current_screen]->orgPalette[c];

					setStackNum( ( (rgb.r / 17) << 8) + ( (rgb.g / 17) << 4) + (rgb.b / 17) );
				}
			}


			break;
		default:
			setError(22);
	}
	popStack( stack - data->stack );

	return NULL;
}

char *_gfxPalette( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int color,n,num;
	struct retroRGB rgb;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	for (n=data->stack;n<=stack;n++)
	{
		num = n-data->stack;
		rgb = screens[current_screen]->orgPalette[num];
		color =  ( (rgb.r / 17) << 8) + ( (rgb.g / 17) << 4) + (rgb.b / 17);
		stack_get_if_int( n, &color );
		retroScreenColor( screens[current_screen], 	num, ((color &0xF00) >>8) * 17, ((color & 0xF0) >> 4) * 17, (color & 0xF)  * 17);
	}

	popStack( stack - data->stack );

	return NULL;
}

char *gfxPlot(struct nativeCommand *cmd, char *tokenBuffer)
{
	kittyStack[stack].type = type_none;
	stackCmdNormal( _gfxPlot, tokenBuffer );
	return tokenBuffer;
}

char *gfxPoint(struct nativeCommand *cmd, char *tokenBuffer)
{
	kittyStack[stack].type = type_none;
	stackCmdParm( _gfxPoint, tokenBuffer );
	return tokenBuffer;
}

char *gfxGrLocate(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxGrLocate, tokenBuffer );
	return tokenBuffer;
}

char *gfxXGR(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(xgr);
	return tokenBuffer;
}

char *gfxYGR(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(ygr);
	return tokenBuffer;
}

char *gfxGetColour(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxGetColour, tokenBuffer );
	return tokenBuffer;
}

char *gfxPolyline(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxPolyline, tokenBuffer );
	return tokenBuffer;
}

char *gfxPalette(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxPalette, tokenBuffer );
	return tokenBuffer;
}

char *gfxPolygon(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxPolygon, tokenBuffer );
	return tokenBuffer;
}

char *gfxScreenClose(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenClose, tokenBuffer );
	return tokenBuffer;
}

char *gfxScreenOffset(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenOffset, tokenBuffer );
	return tokenBuffer;
}

char *gfxScreen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreen, tokenBuffer );
	return tokenBuffer;
}


