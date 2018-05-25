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

extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];

int pen0=2, pen1,pen2;
int xgr = 0,  ygr = 0;

extern int current_screen;
extern char *(*_do_set) ( struct glueCommands *data );
char *_setVar( struct glueCommands *data );

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

char *_gfxFlash( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
		int color = _stackInt( stack-1 );
		char *str = _stackString( stack );

		if (screens[current_screen])
		{
			if (str) retroFlash( screens[current_screen], color, str );
		}
		success = true;
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

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
		num = _stackInt( stack-1 );

		if ((num>-1)&&(num<256))
		{
			color = _stackInt( stack );

			if (screens[current_screen])
			{
				retroScreenColor( screens[current_screen], 	num, ((color &0xF00) >>8) * 17, ((color & 0xF0) >> 4) * 17, (color & 0xF)  * 17);
				dprintf("Screen %d,Color %d,R %d,G %d,B %d\n",current_screen, num, (color &0xF00 >>8) * 17, (color & 0xF0 >> 4) * 17, (color & 0xF)  * 17);

			}
			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
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

char *_gfxCls( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			{
				int color = _stackInt( stack );

				if ((color >-1)&&(color<256))
				{
					if (screens[current_screen]) 
						retroBAR( screens[current_screen],0,0,screens[current_screen]->realWidth,screens[current_screen]->realHeight,color );
				}
			}
			break;

		case 5:
			{
				int color = _stackInt( stack -4 );
				int x0 = _stackInt( stack -3 );
				int y0 = _stackInt( stack -2 );
				int x1 = _stackInt( stack -1 )-1;
				int y1 = _stackInt( stack )-1;

				if ((color >-1)&&(color<256))
				{
					if (screens[current_screen]) retroBAR( screens[current_screen], x0,y0,x1,y1,color );
				}
			}
			break;
		default:
			setError(22);
	}
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

	if (args==3)
	{
		stack_get_if_int( stack-2, &x0 );
		stack_get_if_int( stack-1, &y0 );
		r = _stackInt( stack );

//		if (screens[current_screen])  retroEllipse( screens[current_screen], x0,y0,r,r,0,pen0 );
		if (screens[current_screen])  retroCircle( screens[current_screen], x0,y0,r,pen0 );
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

char *_gfxPaint( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr, y0 = ygr,c;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			stack_get_if_int( stack-1, &x0 );
			stack_get_if_int( stack, &y0 );
			if (screens[current_screen]) retroPixel( screens[current_screen], x0,y0,pen0 );
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

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

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

char *_gfxGetPalette( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int color,n,num;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	engine_lock();

	if ((current_screen>-1)&&(current_screen<8))
	{
		if ((screens[current_screen])&&(args == 1))
		{
			int screen_num = _stackInt( stack );

			if ((screen_num>-1)&&(screen_num<8))
			{
				if (screens[screen_num])
				{
					struct retroRGB rgb;
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

char *_gfxDefaultPalette( struct glueCommands *data )
{
	int color,n,num;
	struct retroRGB rgb;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

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

char *_gfxDefScroll( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	switch (args)
	{
		case 7:
			{
				int n = _stackInt( stack -6 ) -1;

				if ((n>-1)&&(n<16))
				{
					defScrolls[n].x0 = _stackInt( stack -5 );
					defScrolls[n].y0 = _stackInt( stack -4 );
					defScrolls[n].x1 = _stackInt( stack -3 );
					defScrolls[n].y1 = _stackInt( stack -2 );
					defScrolls[n].dx = _stackInt( stack -1 );
					defScrolls[n].dy = _stackInt( stack );
					success = true;
				}
			}
			break;
	}
	popStack( stack - data->stack );

	if (success == false) setError(22);

	return NULL;
}

char *_gfxScroll( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	switch (args)
	{
		case 1:
			{
				int n = _stackInt( stack  ) -1;

				if ((n>-1)&&(n<16))
				{
					int x0 = defScrolls[n].x0 ;
					int y0 = defScrolls[n].y0 ;
					int x1 = defScrolls[n].x1 ;
					int y1 = defScrolls[n].y1 ;
					int dx = defScrolls[n].dx ;
					int dy = defScrolls[n].dy ;

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

					if (screens[current_screen])
					{
						unsigned char *mem = screens[current_screen] -> Memory;
						unsigned int bytesPerRow = screens[current_screen] -> bytesPerRow;
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
				}
			}
			break;
		default:
			setError(22);
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

char *gfxCls(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxCls, tokenBuffer );
	setStackNum( 0 );
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


char *_gfxShiftUp( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

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
						 _stackInt( stack -3 ),
						 _stackInt( stack -2 ),
						 _stackInt( stack -1 ),
						 _stackInt( stack ));

					engine_unlock();
				}
			};

			break;
		default:
			setError(22);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxShiftDown( struct glueCommands *data )
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
						 _stackInt( stack -3 ),
						 _stackInt( stack -2 ),
						 _stackInt( stack -1 ),
						 _stackInt( stack ));

					engine_unlock();
				}
			break;
		default:
			setError(22);
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
	int pos;
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

char *_gfxSetRainbow( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr ,y0 = ygr,x1,y1;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==6)
	{
		int n = _stackInt( stack-5 );
		int colour = _stackInt( stack-4 );
		int length = _stackInt( stack-3 );
		char *r = _stackString( stack-2 );
		char *g = _stackString( stack-1 );
		char *b = _stackString( stack );
		int y;
		unsigned char *rgb;

		engine_lock();

		if (video -> rainbow[n].table) FreeVec(video -> rainbow[n].table);
		video -> rainbow[n].color = colour;
		video -> rainbow[n].tableSize = length;
		video -> rainbow[n].table = (struct retroRGB *) AllocVecTags(sizeof(struct retroRGB)  * video -> rainbow[n].tableSize,  AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0,	TAG_END );

		if (rgb = (unsigned char *) video -> rainbow[n].table)
		{
			 int lines, step, count;
			if (r)	if (sscanf( r,"(%d,%d,%d)", &lines, &step, &count ) == 3)	channelRainbowOCS( rgb, offsetof(struct retroRGB,r),  lines, step, count, length );
			if (g)	if (sscanf( g,"(%d,%d,%d)", &lines, &step, &count ) == 3)	channelRainbowOCS( rgb, offsetof(struct retroRGB,g), lines, step, count, length );
			if (b)	if (sscanf( b,"(%d,%d,%d)", &lines, &step, &count ) == 3)	channelRainbowOCS( rgb, offsetof(struct retroRGB,b), lines, step, count, length );
		}

		engine_unlock();
	}
	else setError(22);

	popStack( stack - data->stack );

	return NULL;
}

char *_gfxRainbow( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr ,y0 = ygr,x1,y1;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==4)
	{
		int rainbowNumber = _stackInt( stack-3 );
		int base = _stackInt( stack-2 );
		int verticalOffset = _stackInt( stack-1 ) - 38;
		int height = _stackInt( stack );

		WaitTOF();
		retroRainbow( video, rainbowNumber, base, verticalOffset, height);
	}
	else setError(22);

	popStack( stack - data->stack );

	return NULL;
}

int _set_rainbow = -1;
int _set_rainbow_index = -1;

char *_set_rain( struct glueCommands *data )
{
	int _rgb_;
	struct retroRGB *rgb;
	struct retroRainbow *rainbow;

	if (_set_rainbow<0) return NULL;

	if (rainbow = &video -> rainbow[_set_rainbow])
	{
		if ((rgb = rainbow -> table) && ( _set_rainbow_index>-1 ) && (_set_rainbow_index < rainbow -> tableSize))
		{
			_rgb_= _stackInt( stack );
			rgb[_set_rainbow_index].r =	((_rgb_ & 0xF00) >> 8) * 0x11 ;
			rgb[_set_rainbow_index].g =	((_rgb_ & 0x0F0) >> 4) * 0x11 ;
			rgb[_set_rainbow_index].b =	(_rgb_ & 0x00F) * 0x11 ;
		}
	}

	_do_set = _setVar;
	return NULL;
}

char *_gfxAutoback( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
	}
	else setError(22);

	popStack( stack - data->stack );
	return NULL;
}


char *_gfxRain( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int x0 = xgr ,y0 = ygr,x1,y1;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
		_set_rainbow = _stackInt( stack-1 );
		_set_rainbow_index = _stackInt( stack );
		_do_set = _set_rain;
	}
	else setError(22);

	popStack( stack - data->stack );

	return NULL;
}

char *_gfxZoom( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 10:
			{
				int from_screen = _stackInt( stack-9 );
				int x0 = _stackInt( stack-8 );
				int y0 = _stackInt( stack-7 );
				int x1 = _stackInt( stack-6 );
				int y1 = _stackInt( stack-5 );
				int to_screen = _stackInt( stack-4 );
				int x2 = _stackInt( stack-3 );
				int y2 = _stackInt( stack-2 );
				int x3 = _stackInt( stack-1 );
				int y3 = _stackInt( stack );

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
			setError(22);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxFade( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool done = false;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 0:
				setError(22);
				return NULL;
		case 1:
				if (screens[current_screen])
				{
					int n;
					struct retroRGB *pal;

					screens[current_screen] -> fade_count = 0;
					screens[current_screen] -> fade_speed = _stackInt( stack );

					n = 0;
					pal = screens[current_screen] -> fadePalette;
					while ( n++<256)
					{
						if (n!=2)
						{
							pal -> r = 0;
							pal -> g = 0;
							pal -> b = 0;
						}
						pal++;
					}
				}
				done = true;

				break;
		case 2:
				if (data->have_to == 1)
				{
					int source_screen = _stackInt( stack );

					if ((screens[current_screen])&&(screens[source_screen]))
					{
						int n;
						struct retroRGB *source_pal;
						struct retroRGB *dest_pal;

						screens[current_screen] -> fade_count = 0;
						screens[current_screen] -> fade_speed = _stackInt( stack-1 );

						source_pal = screens[source_screen] -> orgPalette;
						dest_pal = screens[current_screen] -> fadePalette;
						n = 0;
						while ( n++<256)
						{
							*dest_pal++=*source_pal++;
						}
					}
					done = true;
				}
				break;
	}

	if ((done == false) && (screens[current_screen] ))
	{
		int ecs_rgb = 0;
		int s;
		screens[current_screen] -> fade_speed = _stackInt( data -> stack );

		struct retroRGB *opal= screens[current_screen] -> orgPalette;
		struct retroRGB *fpal= screens[current_screen] -> fadePalette;

		for ( s = data -> stack+1; s<=stack;s++ );
		{
			ecs_rgb = ((opal -> r & 0xF0) << 4) | (opal -> g & 0xF0 ) | ((opal ->b & 0xF0) >>4);
			stack_get_if_int( s, &ecs_rgb );
			fpal -> r =	((ecs_rgb & 0xF00) >> 8) * 0x11;
			fpal -> g =	((ecs_rgb & 0x0F0) >> 4) * 0x11;
			fpal -> b =	(ecs_rgb & 0x00F) * 0x11;
			opal++;
			fpal++;
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxAppear( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int from_screen;
	int to_screen;
	int step = 0;
	int pixels;
	bool args_ok = true;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:
			{
				from_screen = _stackInt( stack-2 );
				to_screen = _stackInt( stack-1 );
				step = _stackInt( stack );
				pixels = 0;
			}
			break;
		case 4:
			{
				from_screen = _stackInt( stack-3 );
				to_screen = _stackInt( stack-2 );
				step = _stackInt( stack-1 );
				pixels = _stackInt( stack );
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

			if (pixels == 0) pixels = mw *mh;

			while (n<(pixels*step))
			{
				n += step;
				x = n % mw;
				y = ((n-x)/mw) % mh;
				retroPixel(tscreen,x,y,retroPoint(fscreen,x,y));

				if ( (n % updateEveryNPixels) == 0 )
				{
					WaitTOF();
				}
			}
		}
	}
	else
	{
		setError(22);
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

void do_to_set_have_to( struct nativeCommand *cmd, char *tokenBuffer )
{
	if (cmdStack)
	{
		if (cmdTmp[cmdStack-1].cmd == _gfxFade)
		{
			cmdTmp[cmdStack-1].have_to = 1;
		}
	}
	stack++;
}

char *gfxFade(struct nativeCommand *cmd, char *tokenBuffer)
{
	do_to = do_to_set_have_to;
	stackCmdNormal( _gfxFade, tokenBuffer );
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

char *gfxHide(struct nativeCommand *cmd, char *tokenBuffer)
{
	// hide mouse pointer.
	return tokenBuffer;
}

char *gfxAutoback(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _gfxAutoback, tokenBuffer );
	return tokenBuffer;
}


