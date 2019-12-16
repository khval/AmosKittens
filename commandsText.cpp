#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>

extern struct RastPort font_render_rp;
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
#include "kittyErrors.h"
#include "engine.h"
#include "bitmap_font.h"
#include "amosString.h"

extern int last_var;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern int current_screen;
extern int cursor_color;

bool curs_on = true;
bool underLine = false;
bool shade = false;
bool inverse = false;

int writing_w1,writing_w2;

extern int GrWritingMode;

int _tab_size = 3;

extern struct TextFont *topaz8_font;

bool next_print_line_feed = false;
void _print_break( struct nativeCommand *cmd, char *tokenBuffer );
struct retroTextWindow *findTextWindow(struct retroScreen *screen,int id);
struct retroTextWindow *redrawWindowsExceptID(struct retroScreen *screen,int exceptID);
void delTextWindow( struct retroScreen *screen, struct retroTextWindow *window );

extern int os_text_base(struct stringData *txt);
extern void os_text_no_outline(struct retroScreen *screen,int x, int y, struct stringData *txt, int pen);
extern void os_text(struct retroScreen *screen,int x, int y, struct stringData *txt, int ink0, int ink1);
extern int os_text_width(struct stringData *txt);

struct retroBlock *cursor_block = NULL; 

int curs_lines[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

void __clear_cursor(struct retroScreen *screen, struct retroTextWindow *textWindow, int buffer)
{
	int gx,gy;
	int x = (textWindow -> x + textWindow -> locateX) + (textWindow -> border ? 1 : 0);
	int y = (textWindow -> y + textWindow -> locateY) + (textWindow -> border ? 1 : 0);
	gx=8*x;	gy=8*y;

	if (cursor_block)
	{
		retroPutBlock( screen, screen -> double_buffer_draw_frame, cursor_block, gx, gy, 0xFF );
	}
}

void clear_cursor( struct retroScreen *screen )
{
	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;

		if ((curs_on)&&(textWindow))
		{
			if (screen -> Memory[1])
			{
				switch ( screen -> autoback )
				{
					case 0:
						__clear_cursor( screen, textWindow, screen -> double_buffer_draw_frame);
						break;
					default:
						engine_lock();
						__clear_cursor( screen, textWindow, 0 );
						__clear_cursor( screen, textWindow, 1 );
						engine_unlock();
						break;
				}
			}
			else
			{
				__clear_cursor( screen, textWindow, 0);
			}
		}
	}
}


void __draw_cursor(struct retroScreen *screen, struct retroTextWindow *textWindow, int buffer)
{
	int gx,gy;
	int d,m;
	int x = (textWindow -> x + textWindow -> locateX) + (textWindow -> border ? 1 : 0);
	int y = (textWindow -> y + textWindow -> locateY) + (textWindow -> border ? 1 : 0);
	unsigned char *memory = screen -> Memory[ buffer ]; 

	gx=8*x;	gy=8*y;

	if (cursor_block == NULL) cursor_block = retroAllocBlock( 8, 8 );

	if (cursor_block)
	{
		retroGetBlock( screen, 0, cursor_block, gx, gy );
	}

	for (y=0;y<8;y++)
	{
		if (d = curs_lines[y])
		{
			x=0;
			m = 0x80;
			while (m>0)
			{
				if (d&m) retroPixel( screen, memory, gx+x,gy+y, cursor_color );
				m>>=1;
				x++;
			}
		}
	}
}

void draw_cursor(struct retroScreen *screen)
{
	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;

		if ((curs_on)&&(textWindow))
		{
			if (screen -> Memory[1])
			{
				switch ( screen -> autoback )
				{
					case 0:
						__draw_cursor( screen, textWindow, screen -> double_buffer_draw_frame);
						break;
					default:
						engine_lock();
						__draw_cursor( screen, textWindow, 0 );
						__draw_cursor( screen, textWindow, 1 );
						engine_unlock();
						break;
				}
			}
			else
			{
				__draw_cursor( screen, textWindow, 0);
			}
		}
	}
}


char *_textLocate( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen; 
	struct retroTextWindow *textWindow ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (screen=screens[current_screen])
	{
		textWindow = screen -> currentTextWindow;

		if (textWindow)
		{
			clear_cursor(screen);
			if (next_print_line_feed) textWindow -> locateY++;

			switch (args)
			{
				case 1:
						if (kittyStack[stack].type == type_int ) 
							textWindow -> locateX = kittyStack[stack].integer.value;
						break;

				case 2:
						if (kittyStack[stack-1].type == type_int ) 
							textWindow -> locateX = kittyStack[stack-1].integer.value;
		
						if (kittyStack[stack].type == type_int )
							textWindow -> locateY = kittyStack[stack].integer.value;
						break;

				default:
						setError(22,data->tokenBuffer);

			}
			draw_cursor(screen);
		}
	}

	next_print_line_feed = false;

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
				next_print_line_feed = false;
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

void __print_text(struct retroScreen *screen, struct stringData *txt, int maxchars)
{
	if (engine_ready())
	{
		_my_print_text( screen, (char *) &txt -> ptr, maxchars, underLine, shade, inverse,writing_w1,writing_w2 );
	}
	else
	{
		printf("%s", &txt -> ptr);
	}
}

void __print_char_array(struct retroScreen *screen, const char *txt, int maxchars)
{
	if (engine_ready())
	{
		_my_print_text( screen, (char *) txt, maxchars, underLine, shade, inverse,writing_w1,writing_w2 );
	}
	else
	{
		printf("%s", txt );
	}
}


void __print_num( struct retroScreen *screen, int num )
{
	char tmp[50];
	struct stringData *str = (struct stringData *) tmp;

	if (num>-1)
	{
		sprintf( &(str->ptr)," %d",num);
	}
	else
	{
		sprintf( &(str->ptr),"%d",num);
	}
	__print_text(screen, str ,0);
}

void __print_double( struct retroScreen *screen, double d )
{
	char tmp[50];
	struct stringData *str = (struct stringData *) tmp;

	if (d>=0.0)
	{
		sprintf(&(str->ptr)," %0.3lf",d);
	}
	else
	{
		sprintf(&(str->ptr),"%0.3lf",d);
	}
	__print_text(screen, str,0);
}

char *_print( struct glueCommands *data, int nextToken )
{
	struct retroScreen *screen = screens[current_screen];
	int n;

	flushCmdParaStack( nextToken );
	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;

		for (n=data->stack;n<=stack;n++)
		{
			switch (kittyStack[n].type)
			{
				case type_int:
					__print_num( screen, kittyStack[n].integer.value);
					break;
				case type_float:
					__print_double( screen, kittyStack[n].decimal.value);
					break;
				case type_string:
					if (kittyStack[n].str) __print_text( screen, kittyStack[n].str,0);
					break;
				case type_none:
					if (n>data->stack) next_print_line_feed = false;
					break;
			}

			if ( n < stack ) textWindow -> locateX += textWindow -> locateX % _tab_size ? _tab_size - textWindow -> locateX % _tab_size : 0;
		}

		draw_cursor(screen);

	}

	popStack( stack - data->stack );
	do_breakdata = NULL;	// done doing that.

	return NULL;
}


char *_textCentre( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen; 
	struct stringData *txt = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args!=1) setError(22,data->tokenBuffer);

	if (screen = screens[current_screen])
	{
		if (engine_ready())
		{
			struct retroTextWindow *textWindow = screen -> currentTextWindow;
			txt = getStackString(stack);

			clear_cursor(screen);

			if (next_print_line_feed == true) __print_char_array( screen, "\n",0);

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

	stackCmdOnBreakOrNewCmd( _addDataToText, tokenBuffer, token_add );
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
		if (next_print_line_feed == true) __print_char_array(screen, "\n",0);
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

int memorizeX,memorizeY;

char *textMemorizeX(struct nativeCommand *cmd, char *tokenBuffer)
{
	struct retroScreen *screen; 

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (screen=screens[current_screen])
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;

		if (textWindow)
		{
			clear_cursor(screen);
			if (next_print_line_feed) textWindow -> locateY++;
			memorizeX = textWindow -> locateX;
			next_print_line_feed = false;
			draw_cursor(screen);
		}
	}

	return tokenBuffer;
}

char *textMemorizeY(struct nativeCommand *cmd, char *tokenBuffer)
{
	struct retroScreen *screen; 

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (screen=screens[current_screen])
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;

		if (textWindow)
		{
			clear_cursor(screen);
			if (next_print_line_feed) textWindow -> locateY++;
			memorizeY = textWindow -> locateY;
			next_print_line_feed = false;
			draw_cursor(screen);
		}
	}

	return tokenBuffer;
}

char *textRememberX(struct nativeCommand *cmd, char *tokenBuffer)
{
	struct retroScreen *screen; 

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (screen=screens[current_screen])
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;

		if (textWindow)
		{
			clear_cursor(screen);
			if (next_print_line_feed) textWindow -> locateY++;
			textWindow -> locateX = memorizeX;
			next_print_line_feed = false;
			draw_cursor(screen);
		}
	}
	return tokenBuffer;
}

char *textRememberY(struct nativeCommand *cmd, char *tokenBuffer)
{
	struct retroScreen *screen; 

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (screen=screens[current_screen])
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;

		if (textWindow)
		{
			clear_cursor(screen);
			if (next_print_line_feed) textWindow -> locateY++;
			textWindow -> locateY = memorizeY;
			next_print_line_feed = false;
			draw_cursor(screen);
		}
	}
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	inverse = true;

	return tokenBuffer;
}

char *textInverseOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	inverse = false;

	return tokenBuffer;
}

char *_textBorderStr( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct stringData *newstr = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 2)
	{
		struct stringData *txt = getStackString( stack-1 );
		int border = getStackNum( stack );
		char *ptr;

		if ((txt)&&(border>=0)&&(border<16))
		{
			newstr = alloc_amos_string( txt->size + 7 ); 
			if (newstr)
			{
				ptr = &newstr -> ptr;
				*ptr++ = 27;
				memcpy( ptr, "E0",2); ptr+=2;
				memcpy( ptr, &txt->ptr, txt -> size); ptr += txt -> size;
				*ptr++ = 27;
				*ptr++ = 'R';
				*ptr++ = 48+ border;
				*ptr = 0;
				newstr -> size = 3 + txt -> size + 4;
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
		int type0 = kittyStack[stack-1].type;
		int type1 = kittyStack[stack].type;

		if (( type0 == type_int ) || ( type0 == type_float) )
		{
			x = getStackNum( stack-1 );
			index = 1;
		}

		if (( type1 == type_int ) || ( type1 == type_float) )
		{
			y = getStackNum( stack );
			index |= 2;
		}
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );


	{
		struct stringData *str;
		char *p;

		switch  (index)
		{
			case 1:
				str = alloc_amos_string( 3 );
				p = &str -> ptr;
				*p++ =27;
				*p++ = 'X';
				*p++ = (x>-1) ? '0'+x : '0';
				*p = 0;
				setStackStr( str );
				break;

			case 2:
				str = alloc_amos_string( 3 );
				p = &str -> ptr;
				*p++ =27;
				*p++ = 'Y';
				*p++ = (y>-1) ? '0'+y : '0';
				*p = 0;
				setStackStr( str );
				break;

			case 3:
				str = alloc_amos_string( 6 );
				p = &str -> ptr;
				*p++ =27;
				*p++ = 'X';
				*p++ = (x>-1) ? '0'+x : '0';
				*p++ =27;
				*p++ = 'Y';
				*p++ = (y>-1) ? '0'+y : '0';
				*p = 0;
				setStackStr( str );
				break;

			default:
				str = alloc_amos_string( 0 );
				setStackStr( str );
				break;
		}
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
	struct stringData *str = alloc_amos_string( 3 );

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		int n = getStackNum( stack );
		char *p = &str -> ptr;
		*p++ =27;
		*p++ = 'P';
		*p++ = (n>-1) ? '0'+n : '0';
		*p = 0;
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackStr( str );

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
	struct stringData *str = alloc_amos_string( 3 );

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		int n = getStackNum( stack );
		char *p = &str -> ptr;
		*p++ =27;
		*p++ = 'B';
		*p++ = (n>-1) ? '0'+n : '0';
		*p = 0;
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackStr( str );

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
	int args = stack - data->stack +1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	writing_w1 = getStackNum(stack);
				break;
		case 2:	writing_w1 = getStackNum(stack-1);
				writing_w2 = getStackNum(stack);
				break;
		default:
				setError(22, data -> tokenBuffer);
	}


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
				next_print_line_feed = false;
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
	int n;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (n=0;n<args;n++)
	{
		curs_lines[n] = getStackNum( stack-7+n );
	}

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
//	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	NYI(__FUNCTION__);

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

				retroBAR( screen, screen -> double_buffer_draw_frame,
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

			retroBAR(screen,screen -> double_buffer_draw_frame,x0,y0,x1,y1,screen -> paper);
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
	struct stringData *str = alloc_amos_string(1);	 //  '%' 
	str -> ptr = 31;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackStr(str);
	return ptr;
}

char *textCUpStr(nativeCommand *cmd, char *ptr)
{
	struct stringData *str = alloc_amos_string(1);	 //  '%' 
	str -> ptr = 30;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackStr(str);
	return ptr;
}

char *textCLeftStr(nativeCommand *cmd, char *ptr)
{
	struct stringData *str = alloc_amos_string(1);	 //  '%' 
	str -> ptr = 29;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackStr(str);
	return ptr;
}

char *textCRightStr(nativeCommand *cmd, char *ptr)
{
	struct stringData *str = alloc_amos_string(1);	 //  '%' 
	str -> ptr = 28;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackStr(str);
	return ptr;
}

void _print_using_break( struct nativeCommand *cmd, char *tokenBuffer )
{
	stack++;
	setStackNone();
}

int stringSymbCount(struct stringData *str, char c)
{
	int cnt = 0;
	char *s;
	char *se = (&str -> ptr) + str -> size;
	for (s=&(str->ptr);s<se;s++) if (*s==c) cnt++;
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
	struct stringData *dest = NULL;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==2)
	{
		struct stringData *fmt = getStackString(stack-1);
		dest = amos_strdup(fmt);
		char *d;
		int numPos = 1;
		int _div = 0;

		switch(kittyStack[stack].type)
		{
			case type_string:
					{
						int fmtCount = stringSymbCount(fmt,'~');
						struct stringData *str = getStackString(stack);
						char *s = &str -> ptr;

						for (d=&dest->ptr;*d;d++)
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
						write_format( decimal < 0.0f, buf, &dest -> ptr );
					}
					break;

			case type_int:
					{
						char buf[60];
						int num = getStackNum(stack);
						sprintf(buf,"%d.0",num);
						write_format( num<0, buf, &dest -> ptr );
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
						retroPutBlock( screen, screen -> double_buffer_draw_frame, (*tab) -> saved, (*tab) -> x * 8, (*tab) -> y * 8, 0xFF );
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
							retroPutBlock( screen, screen -> double_buffer_draw_frame, textWindow -> saved, textWindow -> x * 8, textWindow -> y * 8, 0xFF );
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

	retroBAR( screen, screen -> double_buffer_draw_frame, x0,y0,x1,y1,screen -> paper);
}


void __gfx_border1( struct retroScreen *screen, int x0,int y0,int x1,int y1)
{
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y0+7,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y0,x0+7,y1,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x1-7,y0,x1,y1,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y1-7,x1,y1,screen -> paper);

	x0+=3;
	y0+=3;
	x1-=2;
	y1-=2;

	retroLine( screen, screen -> double_buffer_draw_frame,x0+1,y0,x1-1,y0, 2 );
	retroLine( screen, screen -> double_buffer_draw_frame,x0+1,y1,x1-1,y1, 2 );

	retroLine( screen, screen -> double_buffer_draw_frame,x0,y0+1,x0,y1-1, 2 );
	retroLine( screen, screen -> double_buffer_draw_frame,x1,y0+1,x1,y1-1, 2 );
}

void __gfx_border2( struct retroScreen *screen, int x0,int y0,int x1,int y1)
{
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y0+7,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y0,x0+7,y1,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x1-7,y0,x1,y1,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y1-7,x1,y1,screen -> paper);

	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
}


#define _move(xx,yy)  	x0+=xx; y0+=yy; x1-=xx; y1-=yy

void __gfx_border3( struct retroScreen *screen, int x0,int y0,int x1,int y1)
{
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y0+7,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y0,x0+7,y1,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x1-7,y0,x1,y1,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y1-7,x1,y1,screen -> paper);

	_move(2,2);
	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
	_move(1,1);
	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
	_move(2,2);
	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
	_move(1,1);
	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
}

void __gfx_border4( struct retroScreen *screen, int x0,int y0,int x1,int y1)
{
	int x,y;
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y0+7,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y0,x0+7,y1,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x1-7,y0,x1,y1,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y1-7,x1,y1,screen -> paper);

	_move(1,1);
	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
	_move(2,2);
	for (x=x0;x<x1;x+=2)
	{
		retroPixel(screen, screen ->Memory[ screen -> double_buffer_draw_frame ],x,y0, 2);
		retroPixel(screen, screen ->Memory[ screen -> double_buffer_draw_frame ],x,y1, 2);
	}
	for (y=y0;y<y1;y+=2)
	{
		retroPixel(screen, screen ->Memory[ screen -> double_buffer_draw_frame ],x0,y, 2);
		retroPixel(screen, screen ->Memory[ screen -> double_buffer_draw_frame ],x1,y, 2);
	}
	_move(2,2);
	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
}

void __gfx_border5( struct retroScreen *screen, int x0,int y0,int x1,int y1)
{
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y0+7,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y0,x0+7,y1,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x1-7,y0,x1,y1,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y1-7,x1,y1,screen -> paper);

	_move(2,2);
//	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );

	retroLine( screen, screen -> double_buffer_draw_frame,x0+1,y0,x1-1,y0, 2 );
	retroLine( screen, screen -> double_buffer_draw_frame,x0+1,y1,x1-1,y1, 2 );
	retroLine( screen, screen -> double_buffer_draw_frame,x0,y0+1,x0,y1-1, 2 );
	retroLine( screen, screen -> double_buffer_draw_frame,x1,y0+1,x1,y1-1, 2 );

	_move(1,1);
	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
	_move(1,1);
	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
	_move(1,1);
	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
}

void __gfx_border6( struct retroScreen *screen, int x0,int y0,int x1,int y1)
{
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y0+7,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y0,x0+7,y1,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x1-7,y0,x1,y1,screen -> paper);
	retroBAR( screen, screen -> double_buffer_draw_frame,x0,y1-7,x1,y1,screen -> paper);

	_move(2,2);
	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
	_move(2,2);
	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
	_move(1,1);
	retroBox( screen, screen -> double_buffer_draw_frame,x0,y0,x1,y1, 2 );
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

	if (textWindow -> border == 0) return;

	switch (textWindow -> border)
	{
		case 0:
				return;
		case 1:	__gfx_border1( screen, x0,y0,x1,y1);		break;
		case 2:	__gfx_border2( screen, x0,y0,x1,y1);		break;
		case 3:	__gfx_border3( screen, x0,y0,x1,y1);		break;
		case 4:	__gfx_border4( screen, x0,y0,x1,y1);		break;
		case 5:	__gfx_border5( screen, x0,y0,x1,y1);		break;
		case 6:	__gfx_border6( screen, x0,y0,x1,y1);		break;
		default: 	__gfx_border1( screen, x0,y0,x1,y1);		break;
	}


	_x = textWindow -> x + 2;
	_y = textWindow -> y;

	if (textWindow -> title_top)
	{
		retroBAR( screen, screen -> double_buffer_draw_frame,_x*8,y0,(_x + strlen(textWindow -> title_top) )*8,y0+7,screen -> paper);

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
		retroBAR( screen, screen -> double_buffer_draw_frame,_x*8,y1-7,(_x + strlen(textWindow -> title_bottom) )*8,y1,screen -> paper);

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

	switch (args)
	{
		case 5:
				id = getStackNum( stack -4);
				x = getStackNum( stack -3 );
				y = getStackNum( stack -2 );
				w = getStackNum( stack -1 );
				h = getStackNum( stack );
				break;

		case 6:
				id = getStackNum( stack -5 );
				x = getStackNum( stack -4 );
				y = getStackNum( stack -3 );
				w = getStackNum( stack -2 );
				h = getStackNum( stack -1 );
				b = getStackNum( stack );
				break;	

		case 7:
				id = getStackNum( stack -6 );
				x = getStackNum( stack -5 );
				y = getStackNum( stack -4 );
				w = getStackNum( stack -3 );
				h = getStackNum( stack -2 );
				b = getStackNum( stack -1 );
				s = getStackNum( stack );
				break;

		default:
				setError(22,data-> tokenBuffer);
				popStack( stack - data->stack );
				return NULL;
	}

	if ((b<0)||(b>16))
	{
		setError(60,data-> tokenBuffer);
		popStack( stack - data->stack );
		return NULL;
	}

	screen = screens[current_screen];
	if (screen == NULL)
	{
		setError(22,data-> tokenBuffer);
		popStack( stack - data->stack );
		return NULL;
	}

	textWindow = newTextWindow( screen, id );
	if (textWindow)
	{
		textWindow -> x = x / 8;
		textWindow -> y = y / 8; 
		textWindow -> charsPerRow = w;
		textWindow -> rows = h; 
		textWindow -> border = b;
		textWindow -> set = s;

		clear_cursor(screens[current_screen]);
		screen -> currentTextWindow = textWindow;
		draw_cursor(screens[current_screen]);

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
			retroGetBlock(screen, screen -> double_buffer_draw_frame, block, textWindow -> x * 8, textWindow -> y *8 );

			redrawWindowsExceptID( screen, textWindow -> id );

			textWindow -> x = x /8;
			textWindow -> y = y /8; 

			if (textWindow -> saved) retroGetBlock(screen, screen -> double_buffer_draw_frame, textWindow -> saved, textWindow -> x * 8, textWindow -> y *8 );

			retroPutBlock( screen, screen -> double_buffer_draw_frame, block, textWindow -> x * 8, textWindow -> y * 8, 0xFF );
			retroFreeBlock(block);

			clear_cursor(screens[current_screen]);
			textWindow -> locateX = 0;
			textWindow -> locateY = 0;
			next_print_line_feed = false;
			draw_cursor(screens[current_screen]);
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
	int w=0,h=0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	screen = screens[current_screen];
	if (screen)
	{
		switch (args)
		{
			case 2:
					w = getStackNum( stack -1 );
					h = getStackNum( stack  );
					textWindow = screen -> currentTextWindow;
					break;
		}
	}

	if (textWindow)
	{
		struct retroBlock *block = NULL;
		int minW, minH;

		minW = (textWindow -> charsPerRow < w) ? textWindow -> charsPerRow : w;
		minH = (textWindow -> rows < h) ? textWindow -> rows : h;

		block = retroAllocBlock( (minW-1)*8, (minH-1)*8 );
		if (block)	retroGetBlock( screen, screen -> double_buffer_draw_frame, block, textWindow -> x * 8, textWindow -> y *8 );

		textWindow -> charsPerRow = w;
		textWindow -> rows = h;

		redrawWindowsExceptID( screen, textWindow -> id );

		renderWindow( screen, textWindow );

		if (block)
		{
			retroPutBlock( screen, screen -> double_buffer_draw_frame, block, textWindow -> x * 8, textWindow -> y * 8, 0xFF );
			retroFreeBlock(block);
		}

		renderWindowBorder( screen, textWindow );

		clear_cursor(screens[current_screen]);
		textWindow -> locateX = 0;
		textWindow -> locateY = 0;
		next_print_line_feed = false;
		draw_cursor(screens[current_screen]);
	}
	else
	{
		setError(22,data->tokenBuffer);
	}

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
	int b=0;

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
	int b=0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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
				struct stringData *title = getStackString(stack);

				if (textWindow -> title_top) free( textWindow -> title_top );
				textWindow -> title_top = strdup( &title -> ptr );
				
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
				struct stringData *title = getStackString(stack);

				if (textWindow -> title_bottom) free( textWindow -> title_bottom );
				textWindow -> title_bottom = strdup( &title -> ptr );
				
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

				retroBAR( screen, screen -> double_buffer_draw_frame,
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (screen)
	{
		struct retroTextWindow *textWindow = screen -> currentTextWindow;

		if (textWindow)
		{
			if (textWindow -> saved) retroFreeBlock( textWindow -> saved );
			textWindow -> saved = retroAllocBlock( textWindow -> charsPerRow * 8, textWindow -> rows *8 );
			if (textWindow -> saved)	retroGetBlock( screen, screen -> double_buffer_draw_frame,textWindow -> saved, textWindow -> x * 8, textWindow -> y *8 );
		}
	}

	return tokenBuffer;
}

char *textTextBase(struct nativeCommand *cmd, char *tokenBuffer)
{
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	const char *alpha = "abcdefghijklmnopqrstuvwxyz";
	char buffer[ sizeof(struct stringData) + 50 ];

	struct stringData *txt = (struct stringData *) buffer;

	txt -> size = strlen(alpha);
	memcpy( &txt -> ptr, alpha ,txt -> size );
	 
	if (txt)
	{
		ret = os_text_base(txt);
	}

	setStackNum(ret);

	return tokenBuffer;
}

int text_style = 0;

char *_textSetText( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch( args )
	{
		case 1:	
				text_style = getStackNum(stack);

				engine_lock();
				if (engine_ready())
				{
					SetSoftStyle(&font_render_rp,
				             text_style,
				             FSF_BOLD | FSF_UNDERLINED | FSF_ITALIC);
				}
				engine_unlock();
				
				break;

		default:
				setError(22, data->tokenBuffer);
				break;
	}

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *textSetText(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	// some thing to do with drawing, not sure.
	stackCmdParm( _textSetText, tokenBuffer );
	return tokenBuffer;
}

char *textTextStyles(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	// some thing to do with drawing, not sure.
	setStackNum(text_style);
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

char *_textGrWriting( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			GrWritingMode = getStackNum(stack);
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *textGrWriting(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _textGrWriting, tokenBuffer );
	return tokenBuffer;
}

char *_textText( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];
	
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:
			{
				int x = getStackNum( stack-2 );
				int y = getStackNum( stack-1 );
				struct stringData *txt = getStackString( stack );

				if ((txt)&&(screen))
				{
					engine_lock();
					if (engine_ready())
					{

						switch (GrWritingMode)
						{
							case 0:	// 
									os_text_no_outline(screen, x, y, txt, screen -> ink0 );
									break;

							case 1:	//
									os_text(screen, x,y,txt, screen -> ink0, screen -> ink1 );
									break;

							case 2:	//
									os_text_no_outline(screen, x, y, txt, screen -> ink0 );
									break;

							case 3:	//
									os_text(screen, x,y,txt, screen -> ink0, screen -> ink1 );
									break;

							case 4:	//
									os_text_no_outline(screen, x, y, txt, screen -> ink1 );
									break;

							case 5:	//
									os_text(screen, x,y,txt, screen -> ink1, screen -> ink0 );
									break;

							case 6:	//
									os_text_no_outline(screen, x, y, txt, screen -> ink1 );
									break;

							case 7:	//
									os_text(screen, x,y,txt, screen -> ink1, screen -> ink0);
									break;
						}
					}
					engine_unlock();
				}
			}
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *textText(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	// some thing to do with drawing, not sure.
	setStackNone();
	stackCmdNormal( _textText, tokenBuffer );
	return tokenBuffer;
}

char *_textTextLength( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	unsigned short ret = 0;
	struct stringData *txt;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			txt = getStackString( stack );
			if (txt) ret = os_text_width(txt); 
			break;

			default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *textTextLength(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdParm( _textTextLength, tokenBuffer );
	return tokenBuffer;
}

