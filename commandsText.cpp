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
#include <math.h>

#include "stack.h"
#include "amosKittens.h"
#include "commandsData.h"
#include "commandsText.h"
#include "commandsScreens.h"
#include "errors.h"
#include "engine.h"
#include "bitmap_font.h"

extern int last_var;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern int current_screen;
extern int cursor_color;

bool curs_on = true;
bool underLine = false;
bool shade = false;

int _tab_size = 3;

extern struct TextFont *topaz8_font;

bool next_print_line_feed = false;
void _print_break( struct nativeCommand *cmd, char *tokenBuffer );
struct retroTextWindow *findTextWindow(struct retroScreen *screen,int id);
struct retroTextWindow *redrawWindowsExceptID(struct retroScreen *screen,int exceptID);
void delTextWindow( struct retroScreen *screen, struct retroTextWindow *window );

void retroPutBlock(struct retroScreen *screen, struct retroBlock *block,  int x, int y, unsigned char bitmask);

char *_textLocate( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen; 
	bool success = false;
	struct retroTextWindow *textWindow = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==2)
	{
		if (screen=screens[current_screen])
		{
			textWindow = screen -> currentTextWindow;

			if (textWindow)
			{
				clear_cursor(screen);

				if (kittyStack[stack-1].type == type_int ) 
				{
					textWindow -> locateX = kittyStack[stack-1].value;
				}
		
				if (kittyStack[stack].type == type_int )
				{
					textWindow -> locateY = kittyStack[stack].value;
				}

				draw_cursor(screen);
			}
		}
		next_print_line_feed = false;
		success = true;
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *textLocate(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textLocate, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_textHome( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen; 
	bool success = false;
	struct retroTextWindow *textWindow = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		if (screen=screens[current_screen])
		{
			textWindow = screen -> currentTextWindow;

			if (textWindow)
			{
				clear_cursor(screen);
				textWindow -> locateX = 0;
				textWindow -> locateY = 0;
				draw_cursor(screen);
			}
		}
		next_print_line_feed = false;
		success = true;
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );


	return NULL;
}

char *_textPen( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		struct retroScreen *screen = screens[current_screen];

		if (screen)
		{
			screen -> pen  = getStackNum( stack );
			success = true;
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *textPen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textPen, tokenBuffer );
	return tokenBuffer;
}

char *_textPaper( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		struct retroScreen *screen = screens[current_screen];

		if (screen)
		{
			screen -> paper  = getStackNum( stack );
			success = true;
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

void __print_text(struct retroScreen *screen, const char *txt, int maxchars)
{
	if (engine_ready())
	{
		_my_print_text(  screen, underLine, shade, (char *) txt, maxchars);
	}
	else
	{
		printf("%s", txt);
	}
}

void __print_num( struct retroScreen *screen, int num )
{
	char tmp[50];

	if (num>-1)
	{
		sprintf(tmp," %d",num);
	}
	else
	{
		sprintf(tmp,"%d",num);
	}
	__print_text(screen, tmp,0);
}

void __print_double( struct retroScreen *screen, double d )
{
	char tmp[40];

	if (d>=0.0)
	{
		sprintf(tmp," %0.3lf",d);
	}
	else
	{
		sprintf(tmp,"%0.3lf",d);
	}
	__print_text(screen, tmp,0);
}

char *_print( struct glueCommands *data, int nextToken )
{
	struct retroScreen *screen = screens[current_screen];
	int n;

	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;

		for (n=data->stack;n<=stack;n++)
		{
			switch (kittyStack[n].type)
			{
				case type_int:
					__print_num( screen, kittyStack[n].value);
					break;
				case type_float:
					__print_double( screen, kittyStack[n].decimal);
					break;
				case type_string:
					if (kittyStack[n].str) __print_text( screen, kittyStack[n].str,0);
					break;
				case type_none:
					if (n>data->stack) next_print_line_feed = false;
					break;
			}

			if ((n<stack)&&( kittyStack[n+1].type != type_none ))  __print_text( screen, "    ",0);
		}

		draw_cursor(screen);

	}

	popStack( stack - data->stack );
	do_breakdata = NULL;	// done doing that.

	return NULL;
}

int strlen_no_esc(const char *txt);


char *_textCentre( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen; 
	const char *txt = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args!=1) setError(22,data->tokenBuffer);

	if (screen = screens[current_screen])
	{
		if (engine_started)
		{
			struct retroTextWindow *textWindow = screen -> currentTextWindow;
			txt = getStackString(stack);

			clear_cursor(screen);

			if (next_print_line_feed == true) __print_text( screen, "\n",0);

			if ((txt)&&(textWindow))
			{
				textWindow -> locateX = (textWindow -> charsPerRow/2) - (strlen_no_esc( txt ) / 2);

				if (textWindow -> locateX<0)
				{
					txt -= textWindow -> locateX;	// its read only.
					textWindow -> locateX = 0;
				}
			}
		}
	}

	if (txt)
	{
		__print_text( screen, txt,0);
	}

	if (screens[current_screen]) draw_cursor(screens[current_screen]);

	popStack( stack - data->stack );
	do_breakdata = NULL;	// done doing that.

	next_print_line_feed = false;

	return NULL;
}

char *textPaper(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textPaper, tokenBuffer );
	return tokenBuffer;
}

char *textCentre(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textCentre, tokenBuffer );
	return tokenBuffer;
}

char *_addDataToText( struct glueCommands *data, int nextToken );

void _print_break( struct nativeCommand *cmd, char *tokenBuffer )
{
	int nextToken = *((short *) tokenBuffer );

	flushCmdParaStack( nextToken );

	stackCmdOnBreakOrNewCmd( _addDataToText, tokenBuffer );
	stack++;
 	kittyStack[stack].type = type_none;
}

char *textPrint(nativeCommand *cmd, char *ptr)
{
	struct retroScreen *screen = screens[current_screen];
	stackCmdNormal( _print, ptr );
	do_breakdata = _print_break;

	if (screen)
	{
		 clear_cursor(screen);
		if (next_print_line_feed == true) __print_text(screen, "\n",0);
	}
	next_print_line_feed = true;

	setStackNone();

	return ptr;
}

char *textCursOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	clear_cursor(screens[current_screen]);
	curs_on = false;

	return tokenBuffer;
}

char *textCursOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	clear_cursor(screens[current_screen]);
	curs_on = true;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	return tokenBuffer;
}

char *textMemorizeX(struct nativeCommand *cmd, char *tokenBuffer)
{
	clear_cursor(screens[current_screen]);
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *textMemorizeY(struct nativeCommand *cmd, char *tokenBuffer)
{
	clear_cursor(screens[current_screen]);
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *textRememberX(struct nativeCommand *cmd, char *tokenBuffer)
{
	clear_cursor(screens[current_screen]);
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *textRememberY(struct nativeCommand *cmd, char *tokenBuffer)
{
	clear_cursor(screens[current_screen]);
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}


char *textHome(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _textHome, tokenBuffer );
	return tokenBuffer;
}

char *textInverseOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	struct retroScreen *screen = screens[current_screen];

	if (screen)
	{
		unsigned int t;
		t = screen -> ink0 ;
		screen -> ink0 = screen -> ink1;
		screen -> ink1 = t;
	}

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *textInverseOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	struct retroScreen *screen = screens[current_screen];

	if (screen)
	{
		unsigned int t;
		t = screen -> ink0 ;
		screen -> ink0 = screen -> ink1;
		screen -> ink1 = t;
	}

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *_textBorderStr( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	char *newstr = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 2)
	{
		char *txt = getStackString( stack-1 );
		int border = getStackNum( stack );

		if ((txt)&&(border>=0)&&(border<16))
		{
			newstr = (char *) malloc( strlen(txt) + 6 + 1 ); 
			if (newstr)
			{
				sprintf(newstr,"%cE0%s%cR%c",27,txt,27,48+ border );
			}
		}

		if (newstr == NULL) setError(60,data->tokenBuffer);
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	if (newstr) setStackStr( newstr );

	return NULL;
}

char *textBorderStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _textBorderStr, tokenBuffer );
	return tokenBuffer;
}

char *_textAt( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x =-1,y= -1;
	int index = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 2)
	{
		if (kittyStack[stack-1].type == type_int ) 
		{
			x = kittyStack[stack-1].value;
			index = 1;
		}

		if (kittyStack[stack].type == type_int )
		{
			y = kittyStack[stack].value;
			index |= 2;
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );

	switch (index)
	{
		case 1:
				{
					char str[] = {27,'X','0',0};
					if (x>-1) str[2]='0'+x;
					setStackStrDup( str );
				}
				break;
		case 2:
				{
					char str[] = {27,'Y','0',0};
					if (y>-1) str[2]='0'+y;
					setStackStrDup( str );
				}
				break;
		case 3:
				{
					char str[] = {27,'X','0',27,'Y','0',0};
					if (x>-1) str[2]='0'+x;
					if (y>-1) str[5]='0'+y;
					setStackStrDup( str );
				}
				break;
		default:
				setStackStrDup("");
				break;
	}

	return NULL;
}

extern char *textAt(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _textAt, ptr );
	kittyStack[stack].type = type_none; 
	return ptr;
}

char *_textPenStr( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	char str[] = {27,'P','0',0};

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		int n = getStackNum( stack );
		if (n>-1) str[2]='0'+n;
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackStrDup( str );

	return NULL;
}

extern char *textPenStr(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _textPenStr, ptr );
	return ptr;
}

char *_textPaperStr( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	char str[] = {27,'B','0',0};

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		int n = getStackNum( stack );
		if (n>-1) str[2]='0'+n;
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackStrDup( str );

	return NULL;
}

extern char *textPaperStr(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _textPaperStr, ptr );
	return ptr;
}

char *_textWriting( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *textWriting(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _textWriting, ptr );
	return ptr;
}

char *textShadeOff(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	shade = false;
	return ptr;
}

char *textShadeOn(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	shade = true;
	return ptr;
}

char *textUnderOff(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	underLine = false;
	return ptr;
}

char *textUnderOn(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	underLine = true;
	return ptr;
}

char *_textXText( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int gx = 0, x=0, b=0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		struct retroTextWindow *textWindow = screens[current_screen]->currentTextWindow;

		if (textWindow)
		{
			b = textWindow -> border ? 1 : 0;
			gx = getStackNum( stack );
			x = (gx/8) - textWindow -> x - b ;
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum( x );

	return NULL;
}

char *textXText(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _textXText, ptr );
	return ptr;
}

char *_textYText( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int gy = 0, y=0, b=0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		struct retroTextWindow *textWindow = screens[current_screen]->currentTextWindow;

		if (textWindow)
		{
			b = textWindow -> border ? 1 : 0;
			gy = getStackNum( stack );
			y = (gy/8) - textWindow -> y - b ;
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum( y );

	return NULL;
}

char *textYText(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _textYText, ptr );
	return ptr;
}

char *_textCMove( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x = 0, y = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 2)
	{
		struct retroScreen *screen = screens[current_screen]; 

		if (screen)
		{
			struct retroTextWindow *textWindow = screen -> currentTextWindow;
	
			if (textWindow)
			{
				clear_cursor(screen);

				x = textWindow -> locateX ;
				y = textWindow -> locateY ;

				x += getStackNum( stack - 1 ) ;
				y += getStackNum( stack ) ;

				textWindow -> locateX = x;
				textWindow -> locateY = y;

				draw_cursor(screen);
			}
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );

	return NULL;
}

char *textCMove(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _textCMove, ptr );
	setStackDecimal(0);
	return ptr;
}

char *textCLeft(nativeCommand *cmd, char *ptr)
{
	struct retroScreen *screen = screens[current_screen];

	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;
	
		if (textWindow)
		{
			clear_cursor(screens[current_screen]);
			textWindow-> locateX-- ;
			if (textWindow -> locateX<0) textWindow -> locateX = screens[current_screen] -> realWidth / 8;
			draw_cursor(screens[current_screen]);
		}
	}
	return ptr;
}

char *textCRight(nativeCommand *cmd, char *ptr)
{
	struct retroScreen *screen = screens[current_screen];

	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;
	
		if (textWindow)
		{
			clear_cursor(screens[current_screen]);
			textWindow -> locateX++ ;
			draw_cursor(screens[current_screen]);
		}
	}
	return ptr;
}

char *textCUp(nativeCommand *cmd, char *ptr)
{
	struct retroScreen *screen = screens[current_screen];

	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;
	
		if (textWindow)
		{
			clear_cursor(screens[current_screen]);
			textWindow -> locateY--;
			if (textWindow -> locateY<0) textWindow -> locateY = 0;
			draw_cursor(screens[current_screen]);
		}
	}
	return ptr;
}


char *textCDown(nativeCommand *cmd, char *ptr)
{
	struct retroScreen *screen = screens[current_screen];

	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;
	
		if (textWindow)
		{
			clear_cursor(screens[current_screen]);
			textWindow -> locateY++ ;
			draw_cursor(screens[current_screen]);
		}
	}
	return ptr;
}

char *_textSetTab( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x = 0, y = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		_tab_size = getStackNum(stack);
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );

	return NULL;
}

char *textSetTab(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _textSetTab, ptr );
	setStackDecimal(0);
	return ptr;
}

char *_textSetCurs( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	popStack( stack - data->stack );
	return NULL;
}

char *textSetCurs(nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _textSetCurs, ptr );

	return ptr;
}

char *_textCursPen( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		cursor_color = getStackNum( stack ) ;
		if (screens[current_screen]) draw_cursor(screens[current_screen]);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *textCursPen(struct nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _textCursPen, ptr );

	return ptr;
}

char *_textVscroll( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
	}

	popStack( stack - data->stack );
	return NULL;
}

char *textVscroll(struct nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _textVscroll, ptr );
	return ptr;
}

char *_textHscroll( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int x0=0;
	int y0=0;
	int x1=0;
	int y1=0;
	int dx=0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		struct retroScreen *screen = screens[current_screen];

		if (screen)
		{
			struct retroTextWindow *textWindow = screen -> currentTextWindow;
			unsigned char *mem = screen -> Memory[ screen -> double_buffer_draw_frame ];
			unsigned char *src,*des;
			int bytesPerRow = screen -> bytesPerRow;
			int x,y;

			if (textWindow)
			{
				switch (getStackNum( stack ))
				{
					case 1:	y0 = textWindow -> locateY * 8;
							y1 = y0+8;
							dx = -8;
							break;
					case 2:	y0 = 0;
							y1 = screen -> realHeight;
							dx = -8;
							break;
					case 3:	y0 = textWindow -> locateY * 8;
							y1 = y0+8;
							dx = 8;
							break;
					case 4:	y0 = 0;
							y1 = screen -> realHeight;
							dx = 8;
							break;
				}
			}

			if (dx>0)
			{
				x0 = 0;
				x1 = screen -> realWidth -dx-1;

				for (y=y0;y<y1;y++)
				{
					src = mem + (bytesPerRow * y) + x1;	des = mem + (bytesPerRow * y) + x1 +dx;
					for (x=x1;x>=x0;x--) *des--=*src--;		
				}
			}
			else
			{
				x0 = -dx +1;
				x1 = screen -> realWidth ;

				for (y=y0;y<y1;y++)
				{
					src = mem + (bytesPerRow * y) + x0; 	des = mem + (bytesPerRow * y) + x0 +dx;
					for (x=x0;x<=x1;x++) *des++=*src++;		
				}
			}
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *textHscroll(struct nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _textHscroll, ptr );
	return ptr;
}

char *_textClw( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	struct retroScreen *screen ;
	struct retroTextWindow *textWindow = NULL;
	int x=0,y=0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	screen = screens[current_screen];

	if (args==1)
	{
		if (screen)
		{
			if (textWindow = screen -> currentTextWindow)
			{
				int _has_border =(textWindow -> border ? 1 : 0);
				int _x = textWindow -> x + _has_border;
				int _y = textWindow -> y + _has_border;
				int _w = textWindow -> charsPerRow - (2*_has_border);
				int _h = textWindow -> rows - (2*_has_border);
				int gx = _x * 8; 
				int gy = _y * 8;
				int gw = _w * 8 - 1;
				int gh = _h * 8  -1;

				textWindow -> locateX = 0;
				textWindow -> locateY = 0;

				retroBAR( screen, 
					gx, gy ,
					gx + gw, gy + gh,
					screen -> paper);
			}
		}		
	}
	else 
	{
		printf("args: %d\n",args);

		setError(22,data->tokenBuffer );
	}
	

	popStack( stack - data->stack );
	return NULL;
}

char *textClw(struct nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _textClw, ptr );
	return ptr;
}

char *_textCline( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int chars = 100000;
	int x0=0,y0,x1=0,y1;

	struct retroScreen *screen = screens[current_screen];

	if ((args==1) && (screen))
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;

		if (textWindow)
		{
			int _has_border =(textWindow -> border ? 1 : 0);
			int _x = textWindow -> x + _has_border;
			int _y = textWindow -> y + _has_border;
			int _w = textWindow -> charsPerRow - (_has_border * 2);

			y0 = (textWindow -> locateY + _y )*8;
			y1 = y0+7;

			switch(kittyStack[stack].type)
			{
				case type_none:
					x0 = _x * 8;
					x1 = x0 + (_w * 8);
					break;
				case type_int:
					chars = getStackNum( stack );
					x0 = (textWindow -> locateY + _x ) *8;;
					x1 = x0 + 7;
					if (chars<0) x0 += chars * 8;
					if (chars>0) x1 += chars * 8;
					break;
			}

			retroBAR(screen,x0,y0,x1,y1,screen -> paper);
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *textCline(struct nativeCommand *cmd, char *ptr)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _textCline, ptr );
	setStackNone();
	return ptr;
}

char *textCDownStr(nativeCommand *cmd, char *ptr)
{
	char str[] = {31,0};
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackStrDup(str);
	return ptr;
}

char *textCUpStr(nativeCommand *cmd, char *ptr)
{
	char str[] = {30,0};
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackStrDup(str);
	return ptr;
}

char *textCLeftStr(nativeCommand *cmd, char *ptr)
{
	char str[] = {29,0};
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	setStackStrDup(str);
	dump_stack();
	return ptr;
}

char *textCRightStr(nativeCommand *cmd, char *ptr)
{
	char str[] = {28,0};
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackStrDup(str);
	return ptr;
}

void _print_using_break( struct nativeCommand *cmd, char *tokenBuffer )
{
	stack++;
	setStackNone();
}

int stringSymbCount(char *str, char c)
{
	int cnt = 0;
	char *s;
	for (s=str;*s;s++) if (*s==c) cnt++;
	return cnt;
}

int numberCount(int n)
{
	int cnt =0;
	while (n!=0) { n/=10; cnt++; } 
	return cnt;
}

void write_format( bool sign, char *buf, char *dest )
{
	char *buf_end = NULL;
	char *comma_dest = NULL, *comma_buf = NULL;
	char *psign = NULL;
	char *d,*s;

	for (s=dest;*s;s++)
	{
		if ((*s=='+')||(*s=='-')) psign = s;

		if ((*s=='.')||(*s==';')) 
		{
			comma_dest = s; 
			if (*s==';') *s=' ';
			break;
		}
	}

	for (s=buf;*s;s++)
	{
		if ((*s=='.')||(*s==';')) { comma_buf = s; break; }
	}

	if ((comma_buf) && (comma_dest))
	{
		s=comma_buf-1;
		for (d=comma_dest;d>=dest;d--)
		{
			if (*d=='#')
			{
				if (s>=buf)
				{ *d=*s; s--; }
				else *d=' ';
			}
		}

		s=comma_buf+1;
		buf_end = buf + strlen(buf);

		for (d=comma_dest;*d;d++)
		{
			if (*d=='#')
			{
				if (s<buf_end)
				{ *d=*s; s++; }
				else *d='0'; 
			}
		}
	}

	if (psign)
	{
		if (*psign=='-') *psign = ' ';
		if (sign) *psign = '-';
	}
}

char *_textPrintUsing( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	char *dest = NULL;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==2)
	{
		char *fmt = getStackString(stack-1);
		dest = strdup(fmt);
		char *d;
		int numPos = 1;
		int _div = 0;

		switch(kittyStack[stack].type)
		{
			case type_string:
					{
						int fmtCount = stringSymbCount(fmt,'~');
						char *str = getStackString(stack);
						char *s = str;

						for (d=dest;*d;d++)
						{
							switch (*d)
							{
								case '~':	if (*s) { *d = *s; s++; }	break;
							}
						}
					}
					break;

			case type_float:
					{
						char buf[60];
						double  decimal = getStackDecimal(stack);
						sprintf(buf,"%lf",decimal);
						write_format( decimal < 0.0f, buf, dest );
					}
					break;

			case type_int:
					{
						char buf[60];
						int num = getStackNum(stack);
						sprintf(buf,"%d.0",num);
						write_format( num<0, buf, dest );
					}
					break;
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	do_breakdata = NULL;	// done doing that.
	if (dest) setStackStr( dest );

	return NULL;
}

char *textPrintUsing(nativeCommand *cmd, char *ptr)
{
	stackCmdNormal( _textPrintUsing, ptr );
	do_breakdata = _print_using_break;
	setStackNone();

	return ptr;
}

struct retroTextWindow *findTextWindow(struct retroScreen *screen,int id)
{
	if (screen)
	{
		struct retroTextWindow **tab = screen -> textWindows;
		struct retroTextWindow **eot = screen -> textWindows + screen -> allocatedTextWindows;

		for (tab = screen -> textWindows; tab < eot ; tab++)
		{
			if (*tab) 
			{
				if ( (*tab)->id == id) return *tab;
			}
		}
	}
	return NULL;
}

struct retroTextWindow *redrawWindowsExceptID(struct retroScreen *screen,int exceptID)
{
	if (screen)
	{
		struct retroTextWindow **tab = screen -> textWindows;
		struct retroTextWindow **eot = screen -> textWindows + screen -> allocatedTextWindows;

		for (tab = screen -> textWindows; tab < eot ; tab++)
		{
			if (*tab) 
			{
				if ( (*tab)->id != exceptID) 
				{
					if ((*tab) -> saved) 
					{
						retroPutBlock( screen, (*tab) -> saved, (*tab) -> x * 8, (*tab) -> y * 8, 0xFF );
					}
				}
			}
		}
	}
	return NULL;
}

char *_textWindow( struct glueCommands *data, int nextToken )
{
	struct retroScreen *screen = screens[current_screen];
	int args = stack - data->stack +1 ;
	int id;
	struct retroTextWindow *textWindow = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (screen)
	{
		switch (args)
		{
			case 1:	id = getStackNum( stack );

					textWindow = findTextWindow(screen,id);
					if (textWindow)
					{
						clear_cursor(screens[current_screen]);
						screen -> currentTextWindow = textWindow;
						draw_cursor(screens[current_screen]);

						if (textWindow -> saved) 
						{
							retroPutBlock( screen, textWindow -> saved, textWindow -> x * 8, textWindow -> y * 8, 0xFF );
						}
					}
					else printf("not found id: %d\n",id);
					break;
			default:
				setError(22, data->tokenBuffer);
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *textWindow(nativeCommand *cmd, char *ptr)
{
	stackCmdNormal( _textWindow, ptr );
	setStackNone();

	return ptr;
}

char *textXCurs(nativeCommand *cmd, char *tokenBuffer)
{
	struct retroScreen *screen = screens[current_screen];

	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;
		if (textWindow)	 setStackNum( textWindow -> locateX);
	}
	return tokenBuffer;
}

char *textYCurs(nativeCommand *cmd, char *tokenBuffer)
{
	struct retroScreen *screen = screens[current_screen];
	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;
		if (textWindow)	setStackNum( textWindow -> locateY);
	}
	return tokenBuffer;
}

extern struct retroTextWindow *newTextWindow( struct retroScreen *screen, int id );

void delTextWindow( struct retroScreen *screen, struct retroTextWindow *window )
{
	if (screen)
	{
		struct retroTextWindow **tab = screen -> textWindows;
		struct retroTextWindow **src = screen -> textWindows;
		struct retroTextWindow **eot = screen -> textWindows + screen -> allocatedTextWindows;

		for (tab = screen -> textWindows; tab < eot ; tab++)
		{
			if (tab) if ( *tab == window ) 
			{
				if ( (*tab) -> saved )
				{
					retroFreeBlock( (*tab) -> saved );
					(*tab) -> saved = NULL;
				}

				if ( (*tab) -> title_top )
				{
					free( (*tab) -> title_top );
					(*tab) -> title_top = NULL;
				}

				if ( (*tab) -> title_bottom )
				{
					free( (*tab) -> title_bottom );
					(*tab) -> title_bottom = NULL;
				}

				FreeVec( *tab );

				// count down number of windows.
				screen -> allocatedTextWindows --;

				// fill empty location

				for (src = tab+1; src < eot ; tab++)
				{
					*tab = *src;
					tab++;
				}

				break;
			}
		}

		if (screen -> allocatedTextWindows == 0)
		{
			if (screen -> textWindows) FreeVec( screen -> textWindows );
			screen -> textWindows = NULL;
		}
	}
}


void renderWindow( struct retroScreen *screen, struct retroTextWindow *textWindow )
{
	int x0,y0,x1,y1;
	x0 = textWindow -> x*8;
	y0 = textWindow -> y*8;
	x1 = x0 + (textWindow -> charsPerRow*8)-1;
	y1 = y0 + (textWindow -> rows*8)-1;

	retroBAR( screen, x0,y0,x1,y1,screen -> paper);
}

void renderWindowBorder( struct retroScreen *screen, struct retroTextWindow *textWindow )
{
	int x0,y0,x1,y1;
	x0 = textWindow -> x*8;
	y0 = textWindow -> y*8;
	x1 = x0 + (textWindow -> charsPerRow*8)-1;
	y1 = y0 + (textWindow -> rows*8)-1;
	int _x,_y;
	char *c;

	retroBAR( screen, x0,y0,x1,y0+7,screen -> paper);
	retroBAR( screen, x0,y0,x0+7,y1,screen -> paper);
	retroBAR( screen, x1-7,y0,x1,y1,screen -> paper);
	retroBAR( screen, x0,y1-7,x1,y1,screen -> paper);

	x0+=2;
	y0+=2;
	x1-=2;
	y1-=2;

	retroBox( screen, x0,y0,x1,y1, 2 );
	retroBox( screen, x0+1,y0+1,x1-1,y1-1, 2 );


	x0-=2;
	y0-=2;
	x1+=2;
	y1+=2;

	_x = textWindow -> x + 2;
	_y = textWindow -> y;

	if (textWindow -> title_top)
	{
		retroBAR( screen, _x*8,y0,(_x + strlen(textWindow -> title_top) )*8,y0+7,screen -> paper);

		for (c = textWindow -> title_top; *c; c++)
		{
			draw_glyph( screen, topaz8_font,_x*8,_y*8,*c, screen -> pen );
			_x ++;
		}
	}

	_x = textWindow -> x + 2;
	_y = textWindow -> y + textWindow -> rows -1;

	if (textWindow -> title_bottom)
	{
		retroBAR( screen, _x*8,y1-7,(_x + strlen(textWindow -> title_bottom) )*8,y1,screen -> paper);

		for (c = textWindow -> title_bottom; *c; c++)
		{
			draw_glyph( screen, topaz8_font,_x*8,_y*8,*c, screen -> pen );
			_x ++;
		}
	}
}

char *_textWindOpen( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen ;
	struct retroTextWindow *textWindow = NULL;
	int id=0,x=0,y=0,w=0,h=0,b=0,s=0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	screen = screens[current_screen];
	if (screen)
	{
		printf("args: %d\n",args);

		switch (args)
		{
			case 5:
					id = getStackNum( stack -4);
					x = getStackNum( stack -3 );
					y = getStackNum( stack -2 );
					w = getStackNum( stack -1 );
					h = getStackNum( stack );

					textWindow = newTextWindow( screen, id );
					break;

			case 6:
					id = getStackNum( stack -5 );
					x = getStackNum( stack -4 );
					y = getStackNum( stack -3 );
					w = getStackNum( stack -2 );
					h = getStackNum( stack -1 );
					b = getStackNum( stack );

					textWindow = newTextWindow( screen, id );
					break;	

			case 7:
					id = getStackNum( stack -6 );
					x = getStackNum( stack -5 );
					y = getStackNum( stack -4 );
					w = getStackNum( stack -3 );
					h = getStackNum( stack -2 );
					b = getStackNum( stack -1 );
					s = getStackNum( stack );

					textWindow = newTextWindow( screen, id );
					break;

			default:
					setError(22,data-> tokenBuffer);
		}
	}

	if (textWindow)
	{
		textWindow -> x = x / 8;
		textWindow -> y = y / 8; 
		textWindow -> charsPerRow = w;
		textWindow -> rows = h; 
		textWindow -> border = b;
		textWindow -> set = s;
		screen -> currentTextWindow = textWindow;

		if (b)
		{
			renderWindowBorder( screen, textWindow );
		}
		else
		{
			renderWindow( screen, textWindow );
		}
	}
	else
	{
		setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *textWindOpen(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textWindOpen, tokenBuffer );
	return tokenBuffer;
}

char *_textWindMove( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen ;
	struct retroTextWindow *textWindow = NULL;
	int x=0,y=0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	screen = screens[current_screen];
	if (screen)
	{
		switch (args)
		{
			case 2:
					x = getStackNum( stack -1 );
					y = getStackNum( stack  );
					textWindow = screen -> currentTextWindow;
					break;
		}
	}

	if (textWindow)
	{
		struct retroBlock *block = retroAllocBlock( textWindow -> charsPerRow*8, textWindow -> rows*8 );

		if (block)
		{
			retroGetBlock(screen,block, textWindow -> x * 8, textWindow -> y *8 );

			redrawWindowsExceptID( screen, textWindow -> id );

			textWindow -> x = x /8;
			textWindow -> y = y /8; 

			if (textWindow -> saved) retroGetBlock(screen,textWindow -> saved, textWindow -> x * 8, textWindow -> y *8 );

			retroPutBlock( screen, block, textWindow -> x * 8, textWindow -> y * 8, 0xFF );
			retroFreeBlock(block);
		}
	}
	else
	{
		setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *textWindMove(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textWindMove, tokenBuffer );
	return tokenBuffer;
}


char *_textWindSize( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen ;
	struct retroTextWindow *textWindow = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *textWindSize(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textWindSize, tokenBuffer );
	return tokenBuffer;
}


char *_textXGraphic( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int gx=0;
	int x=0,b=0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		gx = getStackNum( stack  )*8;

		struct retroTextWindow *textWindow = screens[current_screen]->currentTextWindow;
		if (textWindow)
		{
			b = textWindow -> border ? 1 : 0;
			gx = gx - (textWindow -> x + (b * 8));
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum(gx);
	return NULL;
}

char *textXGraphic(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _textXGraphic, tokenBuffer );
	return tokenBuffer;
}

char *_textYGraphic( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int gy=0;
	int y=0,b=0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("Amos Kittens don't not support %s yet, but kittens are brave, and try\n",__FUNCTION__);

	if (args == 1)
	{
		gy = getStackNum( stack  )*8;

		struct retroTextWindow *textWindow = screens[current_screen]->currentTextWindow;
		if (textWindow)
		{
			b = textWindow -> border ? 1 : 0;
			gy = gy - (textWindow -> y + (b*8));
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum(gy);
	return NULL;
}

char *textYGraphic(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _textYGraphic, tokenBuffer );
	return tokenBuffer;
}

char *_textTitleTop( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		struct retroScreen *screen = screens[current_screen];
		if (screen)
		{
			struct retroTextWindow *textWindow = screen -> currentTextWindow;

			if (textWindow)
			{
				char *title = getStackString(stack);

				if (textWindow -> title_top) free( textWindow -> title_top );
				textWindow -> title_top = strdup( title );
				
				renderWindowBorder( screen, textWindow );
			}
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum(0);
	return NULL;
}

char *textTitleTop(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textTitleTop, tokenBuffer );
	return tokenBuffer;
}

char *_textTitleBottom( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		struct retroScreen *screen = screens[current_screen];
		if (screen)
		{
			struct retroTextWindow *textWindow = screen -> currentTextWindow;

			if (textWindow)
			{
				char *title = getStackString(stack);

				if (textWindow -> title_bottom) free( textWindow -> title_bottom );
				textWindow -> title_bottom = strdup( title );
				
				renderWindowBorder( screen, textWindow );
			}
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum(0);
	return NULL;
}

char *textTitleBottom(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textTitleBottom, tokenBuffer );
	return tokenBuffer;
}

char *_textWindClose( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		struct retroScreen *screen = screens[current_screen];
		if (screen)
		{
			struct retroTextWindow *textWindow = screen -> currentTextWindow;

			if (textWindow -> id !=  0)	// ID 0 can't not be deleted.
			{
				int gx = textWindow -> x * 8; 
				int gy = textWindow -> y * 8;
				int gw = textWindow -> charsPerRow * 8 - 1;
				int gh = textWindow -> rows * 8  -1;

				retroBAR( screen, 
					gx, gy ,
					gx + gw, gy + gh,
					screen -> paper);

				delTextWindow( screen, textWindow );
				screen -> currentTextWindow = screen -> textWindows[0];
			}
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}


char *textWindClose(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textWindClose, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_textWindon( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("Amos Kittens don't not support %s yet, but kittens are brave, and try\n",__FUNCTION__);

	if (args == 1)
	{
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );

	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;
		setStackNum(textWindow -> id);
	}

	return NULL;
}

char *textWindon(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _textWindon, tokenBuffer );
	return tokenBuffer;
}

char *textWindSave(nativeCommand *cmd, char *tokenBuffer)
{
	struct retroScreen *screen = screens[current_screen];

	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;

		if (textWindow)
		{
			if (textWindow -> saved) retroFreeBlock( textWindow -> saved );
			textWindow -> saved = retroAllocBlock( textWindow -> charsPerRow * 8, textWindow -> rows *8 );
			if (textWindow -> saved)	retroGetBlock( screen, textWindow -> saved, textWindow -> x * 8, textWindow -> y *8 );
		}
	}

	return tokenBuffer;
}


char *_textBorder( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 3)
	{
		if (screen=screens[current_screen])
		{
			struct retroTextWindow *textWindow = screen -> currentTextWindow;

			if (textWindow)
			{
				textWindow -> border =	getStackNum( stack-2 );
				screen -> paper = getStackNum( stack-1 );
				screen -> pen = getStackNum( stack );

				renderWindowBorder( screen, textWindow );
			}
		}
	}
	else setError(22, data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *textBorder(struct nativeCommand *disc, char *tokenBuffer)
{
	stackCmdNormal( _textBorder, tokenBuffer );
	return tokenBuffer;
}


