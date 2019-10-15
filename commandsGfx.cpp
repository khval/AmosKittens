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
#include <unistd.h>
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
#include "kittyErrors.h"
#include "engine.h"

extern int sig_main_vbl;

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern struct retroSprite *sprite;

extern struct RastPort font_render_rp;

extern bool next_print_line_feed;
extern retroSprite *patterns;

extern int bobDoUpdate ;
extern int bobUpdateNextWait ;

int xgr = 0,  ygr = 0;

int GrWritingMode = 0;
int paintMode = 0;
int currentPattern = 0;

int sliderBPen,	sliderBPaper, sliderBOutline, sliderBStyle, sliderSPen, sliderSPaper, sliderSOutline, sliderSStyle;

extern int current_screen;
extern char *(*_do_set) ( struct glueCommands *data, int nextToken );
extern char *_setVar( struct glueCommands *data, int nextToken );

struct defScroll
{
	int x0;
	int y0;
	int x1;
	int y1;
	int dx;
	int dy;
};

struct defScroll defScrolls[16];

char *_gfxFlash( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==2)
	{
		int color = getStackNum( stack-1 );
		struct stringData *str = getStackString( stack );

		if (screens[current_screen])
		{
			if (str) retroFlash( screens[current_screen], color, &(str->ptr) );
		}
		success = true;
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}


char *_gfxColour( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int num = 0;
	unsigned int color;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	// get color
			num = getStackNum( stack );
			popStack( stack - data->stack );

			if (screens[current_screen])
			{
				struct retroRGB rgb = screens[current_screen]->orgPalette[num];
				setStackNum( ( (rgb.r / 17) << 8) + ( (rgb.g / 17) << 4) + (rgb.b / 17) );
			}
			break;

		case 2:	// set color
			num = getStackNum( stack-1 );
			color = getStackNum( stack );
			popStack( stack - data->stack );

			if ((num>-1)&&(num<256))
			{
				if (screens[current_screen])
				{
					retroScreenColor( screens[current_screen], 	num, ((color &0xF00) >>8) * 17, ((color & 0xF0) >> 4) * 17, (color & 0xF)  * 17);
					dprintf("Screen %d,Color %d,R %02x G %02x,B %02x\n",current_screen, num, (color &0xF00 >>8) * 17, (color & 0xF0 >> 4) * 17, (color & 0xF)  * 17);

				}
				success = true;
			}
			break;

		defaut:
			setError(22,data->tokenBuffer);
			popStack( stack - data->stack );
	}

	return NULL;
}

extern struct amos_selected _selected_;
extern int getMenuEvent();		// is atomic
extern bool onMenuEnabled;

void __wait_vbl()
{

	engine_lock();
	if (bobUpdateNextWait)
	{
		bobDoUpdate = 1;
		bobUpdateNextWait = 0;
	}
	engine_unlock();

#if defined(__amigaos4__) || defined(__morphos__) || defined(__aros__)
	if (( sig_main_vbl )&&( EngineTask ))
	{
		Wait(1<<sig_main_vbl);
	}
	else
	{
		Delay(1);
	}
#endif

#ifdef __linux__
	sleep(1);
#endif
}

extern char *onMenuTokenBuffer ;
extern uint16_t onMenuToken ;
extern char *execute_on( int num, char *tokenBuffer, char *returnTokenBuffer, unsigned short token );

char *gfxWaitVbl(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (onMenuEnabled)
	{
		if (getMenuEvent())
		{
			char *ret;
			ret  = execute_on( _selected_.menu +1, onMenuTokenBuffer , tokenBuffer, onMenuToken );
			if (ret) tokenBuffer = ret - 2;		// +2 will be added on exit.
		}
	}

	__wait_vbl();

	return tokenBuffer;
}

char *gfxColour(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxColour, tokenBuffer );
	return tokenBuffer;
}

void __bar( struct retroScreen *screen, int buffer, int x0,int y0, int x1, int y1 )
{
	if ((currentPattern)&&(patterns))
	{
		retroBarPattern( screen, buffer, x0,y0,x1,y1,patterns, currentPattern>0 ? currentPattern+3 : -currentPattern, screen -> ink0, screen -> ink1 );
	}
	else retroBAR( screen, buffer, x0,y0,x1,y1,screen -> ink0 );

	if (paintMode) retroBox( screen, buffer, x0,y0,x1,y1,screen -> ink2);
}

char *_gfxBar( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr ,y0 = ygr,x1,y1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==4)
	{
		struct retroScreen *screen = screens[current_screen];

		stack_get_if_int( stack-3, &x0 );
		stack_get_if_int( stack-2, &y0 );
		xgr = x1 = getStackNum( stack-1 );
		ygr = y1 = getStackNum( stack );

		if (screen)
		{
			switch (screen -> autoback)
			{
				case 0:	__bar( screen, screen -> double_buffer_draw_frame, x0, y0, x1, y1 );
						break;
				default:	__bar( screen, 0,  x0, y0,  x1,  y1 );
						if (screen -> Memory[1]) __bar( screen, 1,  x0, y0,  x1,  y1 );
						break;
			}
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxCls( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int color = 0;
	struct retroScreen *screen = screens[current_screen];
	struct retroTextWindow *textWindow = NULL;
	int x0=0,y0=0,x1=0,y1=0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (screen)
	{
		textWindow = screen -> currentTextWindow;

		if (textWindow)
		{
			textWindow -> locateX = 0;
			textWindow -> locateY = 0;
			next_print_line_feed = false;
		}
	}

	switch (args)
	{
		case 1:
				if (kittyStack[stack].type == type_none)
				{
					color = screen -> paper;
				}
				else
				{
					 color = getStackNum( stack );
				}

				x1 = screen->realWidth;
				y1 = screen->realHeight;
				break;

		case 5:
				color = getStackNum( stack -4 );
				x0 = getStackNum( stack -3 );
				y0 = getStackNum( stack -2 );
				x1 = getStackNum( stack -1 )-1;
				y1 = getStackNum( stack )-1;

			break;
		default:
			setError(22,data->tokenBuffer);
	}
	popStack( stack - data->stack );

	switch (screen -> autoback)
	{
		case 0:	retroBAR( screen,screen -> double_buffer_draw_frame, x0,y0,x1,y1,color );
				break;
		default:
				retroBAR( screen,0, x0,y0,x1,y1,color );
				break;
	}

	return NULL;
}


char *_gfxDraw( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr ,y0 = ygr,x1=0,y1=0;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			xgr = x1 = getStackNum( stack-1 );
			ygr = y1 = getStackNum( stack );

			break;

		case 4:
			stack_get_if_int( stack-3, &x0 );
			stack_get_if_int( stack-2, &y0 );
			xgr = x1 = getStackNum( stack-1 );
			ygr = y1 = getStackNum( stack );

			if (screen) retroLine( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1,screen -> ink0 );
			break;

		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );

	switch (screen ->autoback)
	{
		case 0:
				if (screen) retroLine( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1,screen -> ink0 );
				break;

		default:
				if (screen)
				{
					retroLine( screen, 0,x0,y0,x1,y1,screen -> ink0 );
					if (screen -> Memory[1])	 retroLine( screen, 1,x0,y0,x1,y1,screen -> ink0 );
				}
				break;
	}


	return NULL;
}

char *_gfxPolygon( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int array[100*2];
	int lx,ly;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( (args>=4) && ((args&1) == 0) && (screen) )
	{
		int n;
		int _stack = data -> stack;
		int coordinates = args >> 1;

		for (n=0;n<args;n++)
		{
			array[n] = getStackNum( _stack++ );
		}
		array[n] = array[0]; n++;
		array[n] = array[1]; n++;


		retroPolyGonArray( screen, screen -> double_buffer_draw_frame,screen -> ink0, (args+2) * sizeof(int), array );

		if (paintMode)
		{
			lx = array[ 0 ];
			ly = array[ 1 ];
			for (n=1;n<coordinates;n++)
			{
				xgr = array[ n*2 ];
				ygr = array[ n*2+1];
				retroLine( screen, screen -> double_buffer_draw_frame, lx,ly,xgr,ygr,screen -> ink2 );
				lx = xgr;
				ly=ygr;
			}

			retroLine( screen, screen -> double_buffer_draw_frame,lx,ly,array[ 0 ],array[ 1 ],screen -> ink2 );
		}
	}
	else	setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxPolyline( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (screen)
	{
		if ( (args>2) && ((args&1) == 0))
		{
			int coordinates = args >> 1;
			int lx,ly,_stack,n;

			_stack = data -> stack;

			lx = getStackNum( _stack++ );
			ly = getStackNum( _stack++ );

			switch (screen -> autoback)
			{
				case 0:
						for (n=1;n<coordinates;n++)
						{
							xgr = getStackNum( _stack++ );
							ygr = getStackNum( _stack++ );
							retroLine( screen, screen -> double_buffer_draw_frame,lx,ly,xgr,ygr,screen -> ink0 );
							lx = xgr;
							ly=ygr;
						}
						break;
				default:
						for (n=1;n<coordinates;n++)
						{
							xgr = getStackNum( _stack++ );
							ygr = getStackNum( _stack++ );
							retroLine( screen, 0 ,lx,ly,xgr,ygr,screen -> ink0 );
							if (screen -> Memory[1])	retroLine( screen, 1 ,lx,ly,xgr,ygr,screen -> ink0 );
							lx = xgr;
							ly=ygr;
						}
						break;
			}


			success = true;
		}
		else if (args == 3)
		{
			int x,y;
			x = getStackNum( stack-1 );
			y = getStackNum( stack );

			switch (screen -> autoback)
			{
				case 0:	retroLine( screen, screen -> double_buffer_draw_frame,xgr,ygr,x,y,screen -> ink0 );
						break;
				default:
						retroLine( screen, 0,xgr,ygr,x,y,screen -> ink0 );
						if (screen -> Memory[1])	retroLine( screen, 1,xgr,ygr,x,y,screen -> ink0 );
			}
			xgr=x;ygr=y;
			success = true;
		}
	}
	
	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxCircle( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x0=xgr,y0=ygr,r;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==3)
	{
		stack_get_if_int( stack-2, &x0 );
		stack_get_if_int( stack-1, &y0 );
		r = getStackNum( stack );

		if (screen) 
		{
			switch(screen -> autoback)
			{
				case 0:	retroCircle( screen, screen -> double_buffer_draw_frame,x0,y0,r,screen -> ink0 ); 
						break;
				default:
						retroCircle( screen, 0,x0,y0,r,screen -> ink0 ); 
						if (screen -> Memory[1]) retroCircle( screen, 1,x0,y0,r,screen -> ink0 ); 
						break;
			}
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxEllipse( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x0=xgr,y0=ygr,r0,r1;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==4)
	{
		stack_get_if_int( stack-3, &x0 );
		stack_get_if_int( stack-2, &y0 );
		r0 = getStackNum( stack-1 );
		r1 = getStackNum( stack );

		if (screen) 
		{
			switch (screen->autoback)
			{
				case 0:	retroEllipse( screen, screen -> double_buffer_draw_frame,x0,y0,r0,r1,0,screen -> ink0 ); 
						break;
				default:
						retroEllipse( screen, 0,x0,y0,r0,r1,0,screen -> ink0 ); 
						if (screen -> Memory[1]) retroEllipse( screen, 1,x0,y0,r0,r1,0,screen -> ink0 ); 
						break;
			}
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxBox( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr ,y0 = ygr,x1,y1;
	int t;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==4)
	{
		stack_get_if_int( stack-3, &x0 );
		stack_get_if_int( stack-2, &y0 );
		xgr = x1 = getStackNum( stack-1 );
		ygr = y1 = getStackNum( stack );

		if (x1<x0) { t = x0; x0 = x1; x1 = t; }
		if (y1<y0) { t = y0; y0 = y1; y1 = t; }

		if (screen) 
		{
			switch (screen -> autoback)
			{
				case 0 :
						retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1,screen -> ink0 );
						break;
				default:
						retroBox( screen, 0, x0,y0,x1,y1,screen -> ink0 );
						if (screen -> Memory[1]) retroBox( screen, 1, x0,y0,x1,y1,screen -> ink0 );
						break;
			}
		}

	}
	else setError(22,data->tokenBuffer);

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

char *_gfxInk( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (screen)
	{
		switch (args)
		{
			case 1:
				screen -> ink0 = getStackNum( stack );
				break;
			case 2:
				stack_get_if_int( stack-1, (int *) &screen -> ink0 );
				stack_get_if_int( stack, (int *) &screen -> ink1 );
				break;
			case 3:
				stack_get_if_int( stack-2, (int *) &screen -> ink0 );
				stack_get_if_int( stack-1, (int *) &screen -> ink1 );
				stack_get_if_int( stack, (int *) &screen -> ink2 );
				break;
			default: 
				setError(22,data->tokenBuffer);
				break;
		}
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

char *_gfxPlot( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			stack_get_if_int( stack-1, &xgr );
			stack_get_if_int( stack, &ygr );
			if (screen) retroPixel( screen, screen -> Memory[ screen -> double_buffer_draw_frame ], xgr,ygr,screen -> ink0 );
			break;
		case 3:
			stack_get_if_int( stack-2, &xgr );
			stack_get_if_int( stack-1, &ygr );
			if (screen) retroPixel( screen, screen -> Memory[ screen -> double_buffer_draw_frame ], xgr,ygr,getStackNum( stack ) );
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxPaint( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr, y0 = ygr,c;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			stack_get_if_int( stack-1, &x0 );
			stack_get_if_int( stack, &y0 );
			if (screen) retroFill( screen, screen -> double_buffer_draw_frame, x0,y0,screen -> ink0 );
			break;
		case 3:
			stack_get_if_int( stack-2, &x0 );
			stack_get_if_int( stack-1, &y0 );
			c = getStackNum( stack );
			if (screen) retroFill( screen, screen -> double_buffer_draw_frame, x0,y0,c );
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}


char *_gfxPoint( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = -1;
	int x0 = xgr, y0 = ygr;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			stack_get_if_int( stack-1, &x0 );
			stack_get_if_int( stack, &y0 );
			popStack( stack - data->stack );
			if (screens[current_screen]) ret = retroPoint(screens[current_screen], x0, y0) ;
			break;
		default:
			popStack( stack - data->stack );
			setError(22,data->tokenBuffer);
	}

	dprintf("%d=Point(%d,%d)\n",ret,x0,y0);

	setStackNum(ret);

	return NULL;
}

char *_gfxGrLocate( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			xgr = getStackNum( stack -1 );
			ygr = getStackNum( stack );
			break;
		default:
			setError(22,data->tokenBuffer);
	}
	popStack( stack - data->stack );

	return NULL;
}

char *_gfxGetColour( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int c;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	switch (args)
	{
		case 1:
			c = getStackNum( stack );

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
			setError(22,data->tokenBuffer);
	}
	popStack( stack - data->stack );

	return NULL;
}

char *_gfxPalette( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int color,n,num;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	engine_lock();

	if ((screens[current_screen])&&(args > 0))
	{
		struct retroRGB *Palette = screens[current_screen]->orgPalette;
		struct retroRGB rgb;

		for (n=data->stack;n<=stack;n++)
		{
			num = n-data->stack;
			rgb = Palette[num];
			color =  ( (rgb.r / 17) << 8) + ( (rgb.g / 17) << 4) + (rgb.b / 17);
			stack_get_if_int( n, &color );
			retroScreenColor( screens[current_screen], 	num, ((color &0xF00) >>8) * 17, ((color & 0xF0) >> 4) * 17, (color & 0xF)  * 17);
		}
	}

	engine_unlock();


	popStack( stack - data->stack );

	return NULL;
}

char *_gfxGetPalette( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int n;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	engine_lock();

	if ((current_screen>-1)&&(current_screen<8))
	{
		if ((screens[current_screen])&&(args == 1))
		{
			int screen_num = getStackNum( stack );

			if ((screen_num>-1)&&(screen_num<8))
			{
				if (screens[screen_num])
				{
					struct retroRGB *Palette = screens[screen_num]->orgPalette;

					for (n=0;n<256;n++)
					{
						retroScreenColor( screens[current_screen], n,Palette[n].r,Palette[n].g,Palette[n].b);
					}
				}
			}
		}
	}

	engine_unlock();

	popStack( stack - data->stack );

	return NULL;
}

char *_gfxDefaultPalette( struct glueCommands *data, int nextToken )
{
	int color,n,num;
	struct retroRGB rgb;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (n=data->stack;n<=stack;n++)
	{
		num = n-data->stack;

		if ((num>-1)&&(num<256))
		{
			rgb = DefaultPalette[num];
			color =  ( (rgb.r / 17) << 8) + ( (rgb.g / 17) << 4) + (rgb.b / 17);
			stack_get_if_int( n, &color );

			DefaultPalette[num].r = ((color &0xF00) >>8) * 17;
			DefaultPalette[num].g = ((color & 0xF0) >> 4) * 17; 
			DefaultPalette[num].b = (color & 0xF)  * 17;
		}
	}

	popStack( stack - data->stack );

	return NULL;
}

char *_gfxDefScroll( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	switch (args)
	{
		case 7:
			{
				int n = getStackNum( stack -6 ) -1;

				if ((n>-1)&&(n<16))
				{
					defScrolls[n].x0 = getStackNum( stack -5 );
					defScrolls[n].y0 = getStackNum( stack -4 );
					defScrolls[n].x1 = getStackNum( stack -3 );
					defScrolls[n].y1 = getStackNum( stack -2 );
					defScrolls[n].dx = getStackNum( stack -1 );
					defScrolls[n].dy = getStackNum( stack );
					success = true;
				}
			}
			break;
	}
	popStack( stack - data->stack );

	if (success == false) setError(22,data->tokenBuffer);

	return NULL;
}


void _scroll( struct retroScreen *screen, int buf, int x0,int y0, int x1, int y1, int dx,int dy )
{
	int bytesPerRow = screen -> bytesPerRow;
	unsigned char *mem = screen -> Memory[ buf ];
	unsigned char *src;
	unsigned char *des;
	int x,y;

	if (dy>0)
	{
		if (dx>0)
		{
			for (y=y1;y>=y0;y--)
			{
				src = mem + (bytesPerRow * y) + x1;	des = mem + (bytesPerRow * (y+dy)) + x1 +dx;
				for (x=x1;x>=x0;x--) *des--=*src--;		
			}
		}
		else
		{
			for (y=y1;y>=y0;y--)
			{
				src = mem + (bytesPerRow * y) + x0; 	des = mem + (bytesPerRow * (y+dy)) + x0 +dx;
				for (x=x0;x<=x1;x++) *des++=*src++;		
			}
		}
	}
	else
	{
		if (dx>0)
		{
			for (y=y0;y<=y1;y++)
			{
				src = mem + (bytesPerRow * (y-dy)) + x1;	des = mem + (bytesPerRow * y) + x1 +dx;
				for (x=x1;x>=x0;x--) *des--=*src--;		
			}
		}
		else
		{
			for (y=y0;y<=y1;y++)
			{
				src = mem + (bytesPerRow * (y-dy)) + x0; 	des = mem + (bytesPerRow * y) + x0 +dx;
				for (x=x0;x<=x1;x++) *des++=*src++;		
			}
		}
	}
}

char *_gfxScroll( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	switch (args)
	{
		case 1:
			{
				int n = getStackNum( stack  ) -1;

				if ((n>-1)&&(n<16))
				{
					struct retroScreen *screen = screens[current_screen];
					int x0 = defScrolls[n].x0 ;
					int y0 = defScrolls[n].y0 ;
					int x1 = defScrolls[n].x1 ;
					int y1 = defScrolls[n].y1 ;
					int dx = defScrolls[n].dx ;
					int dy = defScrolls[n].dy ;

					if (screen)
					{

						// limit to screen first
						if (x0<0) x0=0;
						if (x1>screens[current_screen] -> realWidth-1) x1 = screens[current_screen] -> realWidth -1;

						if (y0<0) y0=0;
						if (y1>screens[current_screen] -> realHeight-1) y1 = screens[current_screen] -> realHeight -1;

						// limit to x0,y0,x1,y1

						if (dx>0) { x1 = x1 - dx; } else { if (x0+dx<0) x0=-(x0+dx); }

						if (dy>0) 
						{ 
							y1 = y1 -dy; 
						} 
						else 
						{
							if (y0+dy<0) y0=-(y0+dy);
							y1 = y1 + dy;
						}


						switch (screen -> autoback)
						{
							case 0:	_scroll( screen, screen -> double_buffer_draw_frame, x0,y0,x1,y1,dx,dy );
									break;
							default:	_scroll( screen, 0, x0,y0,x1,y1,dx,dy );
									if (screen -> Memory[1]) _scroll( screen, 1, x0,y0,x1,y1,dx,dy );
						}
					}
				}
			}
			break;
		default:
			setError(22,data->tokenBuffer);
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

char *gfxPaint(struct nativeCommand *cmd, char *tokenBuffer)
{
	kittyStack[stack].type = type_none;
	stackCmdNormal( _gfxPaint, tokenBuffer );
	return tokenBuffer;
}

char *gfxPoint(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxPoint, tokenBuffer );
	setStackNone();
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

char *gfxCls(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxCls, tokenBuffer );
	kittyStack[stack].type = type_none;
	return tokenBuffer;
}

char *gfxGetPalette(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxGetPalette, tokenBuffer );
	return tokenBuffer;
}

char *gfxDefaultPalette(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxDefaultPalette, tokenBuffer );
	return tokenBuffer;
}

char *gfxDefScroll(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxDefScroll, tokenBuffer );
	return tokenBuffer;
}

char *gfxScroll(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScroll, tokenBuffer );
	return tokenBuffer;
}

//---

struct _never_used_shift_
{
				int delay ;
				int firstColour ;
				int lastColour ;
				int flag ;
} shift;


char *_gfxShiftUp( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 4:
			{
				if (screens[current_screen])
				{
					engine_lock();
					retroCycleOff(screens[current_screen] );
					retroCycleColorsUp( 
						screens[current_screen],
						 getStackNum( stack -3 ),
						 getStackNum( stack -2 ),
						 getStackNum( stack -1 ),
						 getStackNum( stack ));

					engine_unlock();
				}
			};

			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxShiftDown( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 4:
				if (screens[current_screen])
				{
					engine_lock();

					retroCycleOff(screens[current_screen] );
					retroCycleColorsDown( 
						screens[current_screen],
						 getStackNum( stack -3 ),
						 getStackNum( stack -2 ),
						 getStackNum( stack -1 ),
						 getStackNum( stack ));

					engine_unlock();
				}
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}


char *gfxShiftUp(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxShiftUp, tokenBuffer );
	return tokenBuffer;
}

char *gfxShiftDown(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxShiftDown, tokenBuffer );
	return tokenBuffer;
}

char *gfxShiftOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (screens[current_screen])
	{
		engine_lock();
		retroCycleOff(screens[current_screen] );
		engine_unlock();
	}

	return tokenBuffer;
}

void channelRainbowOCS( unsigned char *rgb, int channel, int lines, int step, int count, int length )
{
	int n;
	int C = 0;
	int y= 0;
	unsigned char rgb_value;

	if (step == 0) return;

	y = 0;
	rgb +=  channel +1;		// (Channels are in short format).
	do
	{
		rgb_value = ((C  & 15) << 4) | (C & 15);

		for (n=0;n<lines;n++)
		{
			*rgb = rgb_value;	
			rgb+=sizeof(struct retroRGB);
			y++;
			if (y>=length)	break;
		}

		C+=step;

	} while (y<length) ;
}

char *_gfxSetRainbow( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

#if defined(__amigaos4__)

	if (args==6)
	{
		int n = getStackNum( stack-5 );
		int colour = getStackNum( stack-4 );
		int length = getStackNum( stack-3 );
		struct stringData *r = getStackString( stack-2 );
		struct stringData *g = getStackString( stack-1 );
		struct stringData *b = getStackString( stack );
		unsigned char *rgb;

		engine_lock();

		if (video -> rainbow[n].table) sys_free(video -> rainbow[n].table);
		video -> rainbow[n].color = colour;
		video -> rainbow[n].tableSize = length;
		video -> rainbow[n].table = (struct retroRGB *) sys_public_alloc_clear(sizeof(struct retroRGB)  * video -> rainbow[n].tableSize );

		if (rgb = (unsigned char *) video -> rainbow[n].table)
		{
			 int lines, step, count;
			if (r)	if (sscanf( &r -> ptr ,"(%d,%d,%d)", &lines, &step, &count ) == 3)	channelRainbowOCS( rgb, offsetof(struct retroRGB,r),  lines, step, count, length );
			if (g)	if (sscanf( &g -> ptr,"(%d,%d,%d)", &lines, &step, &count ) == 3)	channelRainbowOCS( rgb, offsetof(struct retroRGB,g), lines, step, count, length );
			if (b)	if (sscanf( &b -> ptr,"(%d,%d,%d)", &lines, &step, &count ) == 3)	channelRainbowOCS( rgb, offsetof(struct retroRGB,b), lines, step, count, length );
		}

		engine_unlock();
	}
	else setError(22,data->tokenBuffer);

#endif

	popStack( stack - data->stack );

	return NULL;
}

char *_gfxRainbow( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==4)
	{
		int rainbowNumber = getStackNum( stack-3 );
		int base = getStackNum( stack-2 );
		int verticalOffset = getStackNum( stack-1 ) - 38;
		int height = getStackNum( stack );

#if defined(__amigaos4__) 
// is this correct, should it not be in engine lock?
		WaitTOF();
#endif

#if defined(__linux__)
		sleep(1);
#endif

		retroRainbow( video, rainbowNumber, base, verticalOffset, height);
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );

	return NULL;
}

int _set_rainbow = -1;
int _set_rainbow_index = -1;

char *_set_rain( struct glueCommands *data, int nextToken )
{
	int _rgb_;
	struct retroRGB *rgb;
	struct retroRainbow *rainbow;

	if (_set_rainbow<0) return NULL;

	if (rainbow = &video -> rainbow[_set_rainbow])
	{
		if ((rgb = rainbow -> table) && ( _set_rainbow_index>-1 ) && (_set_rainbow_index < rainbow -> tableSize))
		{
			_rgb_= getStackNum( stack );
			rgb[_set_rainbow_index].r =	((_rgb_ & 0xF00) >> 8) * 0x11 ;
			rgb[_set_rainbow_index].g =	((_rgb_ & 0x0F0) >> 4) * 0x11 ;
			rgb[_set_rainbow_index].b =	(_rgb_ & 0x00F) * 0x11 ;
		}
	}

	_do_set = _setVar;
	return NULL;
}



char *_gfxRain( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==2)
	{
		_set_rainbow = getStackNum( stack-1 );
		_set_rainbow_index = getStackNum( stack );
		_do_set = _set_rain;
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );

	return NULL;
}

char *_gfxZoom( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 10:
			{
				int from_screen = getStackNum( stack-9 );
				int x0 = getStackNum( stack-8 );
				int y0 = getStackNum( stack-7 );
				int x1 = getStackNum( stack-6 );
				int y1 = getStackNum( stack-5 );

				int to_screen = getStackNum( stack-4 );
				int x2 = getStackNum( stack-3 );
				int y2 = getStackNum( stack-2 );
				int x3 = getStackNum( stack-1 );
				int y3 = getStackNum( stack );

				if (((from_screen>=0)&&(from_screen<8))&&((to_screen>=0)&&(to_screen<8)))	// is in range.
				{
					if ((screens[from_screen])&&(screens[to_screen]))	// is open
					{
						engine_lock();
						retroZoom(screens[from_screen],  x0,  y0, x1,  y1, screens[to_screen],  x2,  y2,  x3,  y3);
						engine_unlock();
					}
				}
			}
			break;

		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}


char *_gfxAppear( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int from_screen;
	int to_screen;
	int step = 0;
	int pixels;
	bool args_ok = true;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:
			{
				from_screen = getStackNum( stack-2 );
				to_screen = getStackNum( stack-1 );
				step = getStackNum( stack );
				pixels = 0;
			}
			break;
		case 4:
			{
				from_screen = getStackNum( stack-3 );
				to_screen = getStackNum( stack-2 );
				step = getStackNum( stack-1 );
				pixels = getStackNum( stack );
			}
			break;

		default:
			args_ok = false;
	}

	if ((args_ok)&&(step>0))
	{
		retroScreen *fscreen,*tscreen;
		fscreen = screens[from_screen];
		tscreen = screens[to_screen];

		if ((fscreen)&&(tscreen))
		{
			int n = 0;
			int x,y;
			int mw = fscreen->realWidth < tscreen->realWidth ? fscreen->realWidth : tscreen->realWidth;
			int mh = fscreen->realHeight < tscreen->realHeight ? fscreen->realHeight : tscreen->realHeight;
			int updateEveryNPixels = (mh * 10 / 100) * mw;
			unsigned char *mem = tscreen -> Memory[ tscreen -> double_buffer_draw_frame ];

			if (pixels == 0) pixels = mw *mh;

			while (n<(pixels*step))
			{
				n += step;
				x = n % mw;
				y = ((n-x)/mw) % mh;
				retroPixel(tscreen, mem ,x,y,retroPoint(fscreen,x,y));

				if ( (n % updateEveryNPixels) == 0 )
				{
#ifdef __amigaos4__
					Delay(1);
#endif					

#ifdef __linux__					
					sleep(1);					
#endif					
				}
			}
		}
	}
	else
	{
		setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxSetRainbow(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxSetRainbow, tokenBuffer );
	return tokenBuffer;
}

char *gfxRainbow(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxRainbow, tokenBuffer );
	return tokenBuffer;
}

char *gfxZoom(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxZoom, tokenBuffer );
	return tokenBuffer;
}

char *_gfxFade( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("check if last arg is none, way\n");

	if (kittyStack[data->stack].type == type_none)
	{
		setError(22, data -> tokenBuffer);
		popStack( stack - data->stack );
		return NULL;
	}

	if ( screen = screens[current_screen] )
	{
		int fade_speed;
		struct retroRGB *dest_pal;
		int c;
		int rgb;

		dest_pal = screen -> fadePalette;

		if (args>1)	// fade to colors listed after fade speed.
		{
			fade_speed = getStackNum( data->stack );
			screen -> fade_speed = 0;	// disable fade.. until its setup again.

			if ((fade_speed == 0)&&(args>255))
			{
				setError(22, data -> tokenBuffer);
				popStack( stack - data->stack );
				return NULL;
			}

			c = 0;	// color
			for (int s = data->stack+1 ; s <= stack ; s++ )
			{
				rgb = getStackNum( s );
				dest_pal[c].r = ((rgb & 0xF00) >> 8) * 0x11;
				dest_pal[c].g = ((rgb & 0x0F0) >> 4) * 0x11;
				dest_pal[c].b = ((rgb & 0x00F)) * 0x11;
				c ++;
			}
		}
		else		// fade to black, if no colors after fade.
		{
			if (kittyStack[ stack ].type == type_int)
			{
				fade_speed = kittyStack[stack].integer.value;
			}
			else fade_speed = 1;

			for (c = 0; c < 256 ; c++ )
			{
				dest_pal[c].r = 0;
				dest_pal[c].g = 0;
				dest_pal[c].b = 0;
				c ++;
			}
		}

		screen -> fade_speed = fade_speed;
		screen -> fade_count = fade_speed;		// next vbl is auto fade 1. (no delay)
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxFadeTo( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int fade_speed = 0;
	int mask = 0xFFFFFFFF;	// 8 bit set, all colors in use.
	int source_screen = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
				fade_speed = getStackNum( stack -1);
				source_screen = getStackNum( stack );
				break;
		case 3:
				fade_speed = getStackNum( stack -1);
				source_screen = getStackNum( stack );
				mask = getStackNum( stack );
				break;
		default:
				setError(22,data->tokenBuffer);
				popStack( stack - data->stack );
				return NULL;
	}

	if ((source_screen>=0)&&(source_screen<8))
	{
		if ((screens[current_screen])&&(screens[source_screen]))
		{
			int n;
			struct retroRGB *org_pal;
			struct retroRGB *source_pal;
			struct retroRGB *dest_pal;

			screens[current_screen] -> fade_count = 0;
			screens[current_screen] -> fade_speed = 0;

			org_pal = screens[current_screen] -> orgPalette;
			source_pal = screens[source_screen] -> orgPalette;
			dest_pal = screens[current_screen] -> fadePalette;

			n = 0;
			while ( n<256)
			{
				*dest_pal=*source_pal;

				source_pal++;
				dest_pal++;
				n++;
			}

			screens[current_screen] -> fade_speed = fade_speed;
			screens[current_screen] -> fade_count = fade_speed;		// next vbl is auto fade 1. (no delay)
		}

		popStack( stack - data->stack );
		return NULL;
	}

	if (source_screen<0)
	{
		if ((screens[current_screen])&&(sprite))
		{
			int n;
			struct retroRGB *source_pal;
			struct retroRGB *dest_pal;

			dest_pal = screens[current_screen] -> fadePalette;
			source_pal = sprite -> palette;

			n = 0;
			while ( n<256)
			{
				*dest_pal=*source_pal;

				source_pal++;
				dest_pal++;
				n++;
			}	

			screens[current_screen] -> fade_speed = fade_speed;
			screens[current_screen] -> fade_count = fade_speed;		// next vbl is auto fade 1. (no delay)
		}
	}

	popStack( stack - data->stack );

	return NULL;
}

char *do_to_fade( struct nativeCommand *cmd, char *tokenBuffer )
{
	if (cmdStack)
	{
		if (cmdTmp[cmdStack-1].cmd == _gfxFade)
		{
			cmdTmp[cmdStack-1].cmd = _gfxFadeTo;
		}
	}
	stack++;
	setStackNum(0);
	return NULL;
}

char *gfxFade(struct nativeCommand *cmd, char *tokenBuffer)
{
	do_to[parenthesis_count] = do_to_fade;
	stackCmdNormal( _gfxFade, tokenBuffer );
	setStackNone();

	return tokenBuffer;
}

char *gfxRain(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxRain, tokenBuffer );
	return tokenBuffer;
}

char *gfxAppear(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxAppear, tokenBuffer );
	return tokenBuffer;
}

char *gfxNtsc(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum( 0 );
	return tokenBuffer;
}

char *_gfxAutoback( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		if (screens[current_screen])
		{
			screens[current_screen]->autoback = getStackNum( stack );
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}


char *gfxAutoback(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _gfxAutoback, tokenBuffer );
	return tokenBuffer;
}

char *_gfxColourBack( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	uint32_t color;
	uint16_t r,g,b;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
				color = getStackNum(stack);
				r = (color & 0xF00) >> 8;
				g = (color & 0x0F0) >> 4;
				b = color & 0xF;
				engine_back_color = (r * 0x110000) | (g * 0x001100) | ( b * 0x000011 );
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxColourBack(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _gfxColourBack, tokenBuffer );
	return tokenBuffer;
}

char *_gfxSetPaint( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			paintMode = getStackNum( stack );
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxSetPaint(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _gfxSetPaint, tokenBuffer );
	return tokenBuffer;
}


char *_gfxSetTempras( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxSetTempras(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _gfxSetTempras, tokenBuffer );
	return tokenBuffer;
}

char *_gfxClip( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 4:
			if (screen)
			{
				screen -> clip_x0 = getStackNum( stack -3);
				screen -> clip_y0 = getStackNum( stack -2 );
				screen -> clip_x1 = getStackNum( stack -1);
				screen -> clip_y1 = getStackNum( stack );

				if (screen -> clip_x0<0) screen -> clip_x0 = 0;
				if (screen -> clip_y0<0) screen -> clip_y0 = 0;
				if (screen -> clip_x1>=screen -> realWidth) screen -> clip_x1 = screen -> realWidth -1;
				if (screen -> clip_y1>=screen -> realHeight) screen -> clip_y1 = screen -> realHeight -1;
			}
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxClip(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _gfxClip, tokenBuffer );
	return tokenBuffer;
}

char *_gfxSetPattern( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			currentPattern = getStackNum(stack);
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxSetPattern(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _gfxSetPattern, tokenBuffer );
	return tokenBuffer;
}

char *_gfxSetLine( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxSetLine(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _gfxSetLine, tokenBuffer );
	return tokenBuffer;
}

char *_gfxRainbowDel( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxRainbowDel(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _gfxRainbowDel, tokenBuffer );
	return tokenBuffer;
}

char *_gfxScrollOff( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxScrollOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _gfxScrollOff, tokenBuffer );
	return tokenBuffer;
}

char *_gfxPhysic( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1: 
				if (kittyStack[stack].type == type_none)
				{
					ret = current_screen | 0xC0000000;
				}
				else 
				{
					ret = getStackNum( stack ) | 0xC0000000;
				}
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( ret );

	return NULL;
}

char *gfxPhysic(struct nativeCommand *cmd, char *tokenBuffer)
{
	int nextToken = *((unsigned short *) tokenBuffer);

	if (nextToken == token_parenthesis_start)	
	{
		stackCmdParm( _gfxPhysic, tokenBuffer );
		setStackNone();
		return tokenBuffer;
	}

	setStackNum( current_screen | 0xC0000000 );
	return tokenBuffer;
}

char *_gfxLogic( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
				if (kittyStack[stack].type == type_none)
				{
					ret = current_screen | 0x80000000;
				}
				else 
				{
					ret = getStackNum( stack ) | 0x80000000;
				}
			break;

		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( ret );

	return NULL;
}


char *gfxLogic(struct nativeCommand *cmd, char *tokenBuffer)
{
	int nextToken = *((unsigned short *) tokenBuffer);

	if (nextToken == token_parenthesis_start)	
	{
		stackCmdParm( _gfxLogic, tokenBuffer );
		setStackNone();
		return tokenBuffer;
	}

	setStackNum( current_screen | 0x80000000 );
	return tokenBuffer;
}

void dotBAR( struct retroScreen *screen, int buffer, int x0,int y0, int x1, int y1, int bc, int oc )
{
	int x,y,n;
	unsigned char *mem = screen -> Memory[ buffer ];

	for (y=y0;y<=y1;y++)
	{
		n=y&1;
		for (x=x0;x<=x1;x++)
		{
			retroPixel(screen, mem,x,y, (x^y) & 1 ? bc : oc );
		}
	}
}

char *_gfxHslider( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x1 ,y1,x2,y2,total,pos,size;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==7)
	{
		x1 = getStackNum( stack-6 );
		y1 = getStackNum( stack-5 );
		x2 = getStackNum( stack-4 );
		y2 = getStackNum( stack-3 );
		total = getStackNum( stack-2 );
		pos = getStackNum( stack-1 );
		size = getStackNum( stack );

		if (screen = screens[current_screen])
		{
			int xpos1,xpos2;

			xpos1 =  (x2-x1) * pos / total;
			xpos2 =  (x2-x1) * (pos+size) / total;

			retroBox( screen, screen -> double_buffer_draw_frame, x1,y1,x2,y2,sliderBOutline );

			if (sliderBStyle)
			{
				retroBarPattern( screen, screen -> double_buffer_draw_frame, x1+1, y1+1,x2-1,y2-1, patterns, sliderBStyle+3, sliderBOutline, sliderBPaper );
			}
			else
			{
				retroBAR( screen, screen -> double_buffer_draw_frame, x1+1, y1+1,x2-1,y2-1, sliderBOutline );
			}

			retroBox( screen, screen -> double_buffer_draw_frame, x1+xpos1,y1,x1+xpos2,y2,sliderSOutline );

			if (sliderSStyle)
			{
				retroBarPattern( screen, screen -> double_buffer_draw_frame, x1+xpos1+1,y1+1,x1+xpos2-1,y2-1,patterns, sliderSStyle+3, sliderSOutline, sliderSPaper );
			}
			else
			{
				retroBAR( screen, screen -> double_buffer_draw_frame, x1+xpos1+1,y1+1,x1+xpos2-1,y2-1,sliderSOutline );
			}
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *gfxHslider(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxHslider, tokenBuffer );
	return tokenBuffer;
}

char *_gfsVslider( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x1 ,y1,x2,y2,total,pos,size;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==7)
	{
		x1 = getStackNum( stack-6 );
		y1 = getStackNum( stack-5 );
		x2 = getStackNum( stack-4 );
		y2 = getStackNum( stack-3 );
		total = getStackNum( stack-2 );
		pos = getStackNum( stack-1 );
		size = getStackNum( stack );

		if ( screen = screens[current_screen])
		{
			int ypos1,ypos2;

			ypos1 =  (y2-y1) * pos / total;
			ypos2 =  (y2-y1) * (pos+size) / total;

			retroBox( screen, screen -> double_buffer_draw_frame, x1,y1,x2,y2,sliderBOutline );
			dotBAR( screen, screen -> double_buffer_draw_frame, x1+1,	y1+1,x2-1,y2-1,sliderBPaper, sliderBOutline );

			retroBox( screen, screen -> double_buffer_draw_frame, x1,y1+ypos1,x2,y1+ypos2-2,sliderSOutline );
			retroBAR( screen, screen -> double_buffer_draw_frame, x1+1,y1+ypos1+1,x2-1,y1+ypos2-1,sliderSPaper );
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *gfsVslider(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfsVslider, tokenBuffer );
	return tokenBuffer;
}

char *_gfxSetSlider( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==8)
	{
		sliderBPen = getStackNum( stack-7 );
		sliderBPaper = getStackNum( stack-6 );
		sliderBOutline = getStackNum( stack-5 );
		sliderBStyle = getStackNum( stack-4 );
		sliderSPen = getStackNum( stack-3 );
		sliderSPaper = getStackNum( stack-2 );
		sliderSOutline = getStackNum( stack-1 );
		sliderSStyle = getStackNum( stack );
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *gfxSetSlider(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _gfxSetSlider, tokenBuffer );
	return tokenBuffer;
}


