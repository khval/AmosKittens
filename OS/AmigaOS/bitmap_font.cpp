#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/diskfont.h>
#include <string>
#include <proto/retroMode.h>
#include "AmosKittens.h"
#include "debug.h"

extern struct DiskfontIFace *IDiskfont;
extern struct TextFont *topaz8_font;
extern struct TextFont *gfx_font;

#define effect_args unsigned char data, int bit_offset, int num_bits_in_byte , int _bit_start,int y,struct retroScreen *screen, int buffer, int destx, int desty, int pen, int paper, int w2

void draw_glyph(struct retroScreen *screen, struct TextFont *font, int rp_x, int rp_y, int glyph, int pen);
void draw_glyph_shade(struct retroScreen *screen, struct TextFont *font, int rp_x, int rp_y, int glyph, int pen);

void effect_byte_replace ( effect_args );
void effect_byte_replace_shade ( effect_args );
void draw_glyph_effect(struct retroScreen *screen, int buffer, struct TextFont *font, int destx, int desty, int glyph, int pen, int paper, int w2, void (*heffect) ( effect_args ) );

void freeAllTextWindows(struct retroScreen *screen)
{
	if (screen)
	{
		struct retroTextWindow **tab = screen -> textWindows;
		struct retroTextWindow **eot = screen -> textWindows + screen -> allocatedTextWindows;

		if (screen -> textWindows)
		{
			for (tab = screen -> textWindows; tab < eot ; tab++)
			{
				if (*tab) FreeVec(*tab);
				*tab = NULL;
			}

			FreeVec(screen -> textWindows);
			screen -> textWindows = NULL;
		}
	}
}

struct retroTextWindow **allocRetroTextWindows( int n )
{
	return (struct retroTextWindow **) AllocVecTags( sizeof( struct retroTextWindow * ) * n, 
		AVT_Type, MEMF_SHARED, 	// is used by more then one thread.
		AVT_ClearWithValue, 0,		// should be empty
		TAG_END );
}

struct retroTextWindow *new_text_window ( int id )
{
	struct retroTextWindow *textWindow = (struct retroTextWindow *) AllocVecTags( sizeof( struct retroTextWindow  ) , 
		AVT_Type, MEMF_SHARED, 	// is used by more then one thread.
		AVT_ClearWithValue, 0,		// should be empty
		TAG_END );

	if (textWindow) textWindow -> id = id;

	return textWindow;
}

struct retroTextWindow *newTextWindow( struct retroScreen *screen, int id )
{
	int _to_alloc_ = screen -> allocatedTextWindows +1;

	struct retroTextWindow **_new_tab_ = allocRetroTextWindows( _to_alloc_ );

	if (_new_tab_)
	{
		if (screen -> textWindows) 
		{
			printf("copy old ptrs\n");
			memcpy( _new_tab_, screen -> textWindows, sizeof( struct retroTextWindow * ) * screen -> allocatedTextWindows );
			FreeVec(screen -> textWindows );
			screen -> textWindows = NULL;
		}

		_new_tab_[_to_alloc_-1] = new_text_window( id ) ;
		screen -> textWindows = _new_tab_;
		screen -> allocatedTextWindows = _to_alloc_;

		return _new_tab_[_to_alloc_-1];
	}

	return NULL;
}


void effect_byte_replace ( effect_args )
{
	int x;
	unsigned char *memory = screen -> Memory[ buffer ]; 

	w2=~w2;	// invert

	for (x=0;x< num_bits_in_byte ;x++)
	{
		if (x + bit_offset - _bit_start > -1)
		{
			if (data & (1<<(7-x)) ) 
			{ 
				if (w2&1) retroPixel( screen, memory, destx + x + bit_offset - _bit_start, desty + y, pen);
			}
			else
			{
				if (w2&2) retroPixel( screen, memory, destx + x + bit_offset - _bit_start, desty + y, paper);
			}
		}
	}
}

void effect_byte_replace_shade ( effect_args )
{
	int x;
	unsigned char *memory = screen -> Memory[ buffer ]; 

	w2=~w2;	// invert

	for (x=0;x< num_bits_in_byte ;x++)
	{
		if (x + bit_offset - _bit_start > -1)
		{
			if (data & (((x^y)&1)<<(7-x)) ) 
			{
				if (w2&1)  retroPixel( screen, memory, destx + x + bit_offset - _bit_start, desty + y, pen);
			}
			else
			{
				if (w2&2) retroPixel( screen, memory, destx + x + bit_offset - _bit_start, desty + y, paper);
			}
		}
	}
}

void effect_byte_or ( effect_args )
{
	int x;
	int ix,iy,sp;
	unsigned char *memory = screen -> Memory[ buffer ]; 

	w2=~w2;	// invert

	for (x=0;x< num_bits_in_byte ;x++)
	{
		if (x + bit_offset - _bit_start > -1)
		{
			ix = destx + x + bit_offset - _bit_start;
			iy = desty + y;
	
			sp = retroPoint( screen, ix, iy );			
			
			if (data & (1<<(7-x)) ) 
			{
				if (w2&1)  retroPixel( screen, memory, ix, iy , sp | pen);
			}
			else
			{
				if (w2&2) retroPixel( screen, memory, ix, iy, sp | paper);
			}
		}
	}
}

void effect_byte_or_shade ( effect_args )
{
	int x;
	int ix,iy,sp;
	unsigned char *memory = screen -> Memory[ buffer ]; 

	w2=~w2;	// invert

	for (x=0;x< num_bits_in_byte ;x++)
	{
		ix = destx + x + bit_offset - _bit_start;
		iy = desty + y;
	
		sp = retroPoint( screen, ix, iy );		

		if (x + bit_offset - _bit_start > -1)
		{
			if (data & (((x^y)&1)<<(7-x)) ) 
			{
				if (w2&1)  retroPixel( screen, memory, ix, iy , sp | pen);
			}
			else
			{
				if (w2&2) retroPixel( screen, memory, ix, iy, sp | paper);
			}
		}
	}
}

void effect_byte_xor ( effect_args )
{
	int x;
	int ix,iy,sp;
	unsigned char *memory = screen -> Memory[ buffer ]; 

	w2=~w2;	// invert

	for (x=0;x< num_bits_in_byte ;x++)
	{
		if (x + bit_offset - _bit_start > -1)
		{
			ix = destx + x + bit_offset - _bit_start;
			iy = desty + y;
	
			sp = retroPoint( screen, ix, iy );			
			
			if (data & (1<<(7-x)) ) 
			{
				if (w2&1)  retroPixel( screen, memory, ix, iy , sp ^ pen);
			}
			else
			{
				if (w2&2) retroPixel( screen, memory, ix, iy, sp ^ paper);
			}
		}
	}
}

void effect_byte_xor_shade ( effect_args )
{
	int x;
	int ix,iy,sp;
	unsigned char *memory = screen -> Memory[ buffer ]; 

	w2=~w2;	// invert

	for (x=0;x< num_bits_in_byte ;x++)
	{
		ix = destx + x + bit_offset - _bit_start;
		iy = desty + y;
	
		sp = retroPoint( screen, ix, iy );		

		if (x + bit_offset - _bit_start > -1)
		{
			if (data & (((x^y)&1)<<(7-x)) ) 
			{
				if (w2&1)  retroPixel( screen, memory, ix, iy , sp ^ pen);
			}
			else
			{
				if (w2&2) retroPixel( screen, memory, ix, iy, sp ^ paper);
			}
		}
	}
}

void effect_byte_and ( effect_args )
{
	int x;
	int ix,iy,sp;
	unsigned char *memory = screen -> Memory[ buffer]; 

	w2=~w2;	// invert

	for (x=0;x< num_bits_in_byte ;x++)
	{
		if (x + bit_offset - _bit_start > -1)
		{
			ix = destx + x + bit_offset - _bit_start;
			iy = desty + y;
	
			sp = retroPoint( screen, ix, iy );			
			
			if (data & (1<<(7-x)) ) 
			{
				if (w2&1)  retroPixel( screen, memory, ix, iy , sp & pen);
			}
			else
			{
				if (w2&2) retroPixel( screen, memory, ix, iy, sp & paper);
			}
		}
	}
}

void effect_byte_and_shade ( effect_args )
{
	int x;
	int ix,iy,sp;
	unsigned char *memory = screen -> Memory[ buffer ]; 

	w2=~w2;	// invert

	for (x=0;x< num_bits_in_byte ;x++)
	{
		ix = destx + x + bit_offset - _bit_start;
		iy = desty + y;
	
		sp = retroPoint( screen, ix, iy );		

		if (x + bit_offset - _bit_start > -1)
		{
			if (data & (((x^y)&1)<<(7-x)) ) 
			{
				if (w2&1)  retroPixel( screen, memory, ix, iy , sp & pen);
			}
			else
			{
				if (w2&2) retroPixel( screen, memory, ix, iy, sp & paper);
			}
		}
	}
}

struct _font_loc {
	short bit_start;
	short bit_width;
};

struct _font
{
	void *ln_Succ ;
	void *in_Pred;
	char ln_Type;
	char ln_Pri;
	char *fontName;
	void *mn_ReplyPort;
	short Reserved_for_1_4_system_use;
	short tf_YSize;
	char tf_Style;
	char tf_Flags;
	short tf_XSize;
	short tf_Baseline  ;
    	short tf_BoldSmear ;
	short tf_Accessors;
    	char tf_LoChar;
	char tf_HiChar;
	short *fontData;
	short tf_Modulo;	
	struct _font_loc * tf_CharLoc;
	short *tf_CharSpace;
	short *tf_CharKern;
 };

/*
void draw_bit( struct retroScreen *screen, int x,int y)
{
	struct retroTextWindow *textWindow = screen -> currentTextWindow;
	if (!textWindow) return;

	retroPixel( screen,x,y,screen -> pen);
}
*/

void draw_char(struct retroScreen *screen, struct retroTextWindow *textWindow, int lX, int lY, char c, int pen, int paper, bool shade, int w1, int w2 )
{
	int b = (textWindow -> border ? 1 : 0);
	int x = textWindow -> x + lX + b;
	int y = textWindow -> y + lY + b;
	int buffer = 0;

	if ((screen -> Memory[1]) && (screen -> autoback ==0))
	{
		printf("has DB, is autoback 0\n");
		 buffer = screen -> double_buffer_draw_frame ;
	}

	x *= 8;
	y *= 8;

	if (shade)
	{
//		printf("shade w1: %d\n",w1);

		switch (w1)
		{
			case 0:	draw_glyph_effect( screen, buffer, topaz8_font, x, y, c, pen, paper, w2, effect_byte_replace_shade  );	break;
			case 1:	draw_glyph_effect( screen, buffer, topaz8_font, x, y, c, pen, paper, w2, effect_byte_or_shade  ); break;
			case 2:	draw_glyph_effect( screen, buffer, topaz8_font, x, y, c, pen, paper, w2, effect_byte_xor_shade  ); break;
			case 3:	draw_glyph_effect( screen, buffer, topaz8_font, x, y, c, pen, paper, w2, effect_byte_and_shade  );	break;
		}
	}
	else
	{
//		printf("w1: %d\n",w1);

		switch (w1)
		{
			case 0:	draw_glyph_effect( screen, buffer, topaz8_font, x, y, c, pen, paper, w2, effect_byte_replace  );	break;
			case 1:	draw_glyph_effect( screen, buffer, topaz8_font, x, y, c, pen, paper, w2, effect_byte_or  ); break;
			case 2:	draw_glyph_effect( screen, buffer, topaz8_font, x, y, c, pen, paper, w2, effect_byte_xor  ); break;
			case 3:	draw_glyph_effect( screen, buffer, topaz8_font, x, y, c, pen, paper, w2, effect_byte_and  );	break;
		}
	}
	
	if ((screen -> Memory[1]) && (screen -> autoback != 0))
	{
		retroScreenBlit( screen, 0 ,x, y, 8, 8, screen, 1, x, y);
	}
}

struct TextFont *open_font( char const *filename, int size )
{
	struct TextAttr ta;
	ta.ta_Name = filename;  
	ta.ta_YSize = size; 
	ta.ta_Style = 0; 
	ta.ta_Flags =  FPF_DISKFONT; 
	return OpenDiskFont(&ta);
}

int offset_x = 0;
int offset_y = 0;



void draw_glyph_effect(struct retroScreen *screen, int buffer, struct TextFont *font, int destx, int desty, int glyph, int pen, int paper, int w2, void (*heffect) ( effect_args ) )
{
	int y;
	short bit_start ;
	short bit_width ;
	int start_byte ;
	int num_bytes ;
	int end_byte ; 
	unsigned char data;
	int bit_offset;
	int num_bits_in_byte;
//	int _bit_start;
	int n;

	if ((glyph<font -> tf_LoChar)||(glyph>font -> tf_HiChar))
	{
		return ;
	}

	glyph -= font -> tf_LoChar;

	bit_start =(( struct _font_loc *)  font -> tf_CharLoc) [glyph].bit_start;
	bit_width = (( struct _font_loc *) font -> tf_CharLoc) [glyph].bit_width;
	start_byte = bit_start >> 3;
	num_bytes = (bit_width & 7) ? (bit_width >> 3) + 1 : bit_width >> 3;
	end_byte =start_byte + num_bytes; 

	bit_start = bit_start & 7;	// we don't need to know what byte anymore.
	bit_width += bit_start;

	num_bytes = (bit_width & 7) ? (bit_width >> 3) + 1 : bit_width >> 3;
	end_byte =start_byte + num_bytes; 

	for (y=0;y<font -> tf_YSize ;y++)
	{
		for (n=start_byte;n<end_byte;n++)
		{
			data = ( (char *) font -> tf_CharData) [ n + (y * font -> tf_Modulo)  ];
			bit_offset = (n-start_byte)<<3;
			num_bits_in_byte = (bit_width - bit_offset) >8 ? 8 : (bit_width - bit_offset) ;
			heffect( data,  bit_offset, num_bits_in_byte, bit_start, y, screen, buffer, destx,  desty,  pen,  paper , w2);
		}
	}
}



void draw_glyph(struct retroScreen *screen, struct TextFont *font, int rp_x, int rp_y, int glyph, int pen)
{
	int x,y;
	short bit_start ;
	short bit_width ;
	int start_byte ;
	int num_bytes ;
	int end_byte ; 
	unsigned char data;
	int bit_offset;
	int num_bits_in_byte;
	int _bit_start;
	int n;
	unsigned char *memory = screen -> Memory[screen -> double_buffer_draw_frame]; 

	if ((glyph<font -> tf_LoChar)||(glyph>font -> tf_HiChar))
	{
		return ;
	}

	glyph -= font -> tf_LoChar;

	bit_start =(( struct _font_loc *)  font -> tf_CharLoc) [glyph].bit_start;
	bit_width = (( struct _font_loc *) font -> tf_CharLoc) [glyph].bit_width;
	start_byte = bit_start >> 3;
	num_bytes = (bit_width & 7) ? (bit_width >> 3) + 1 : bit_width >> 3;
	end_byte =start_byte + num_bytes; 

	bit_start = bit_start & 7;	// we don't need to know what byte anymore.
	bit_width += bit_start;
	_bit_start = bit_start;

	num_bytes = (bit_width & 7) ? (bit_width >> 3) + 1 : bit_width >> 3;
	end_byte =start_byte + num_bytes; 

	for (y=0;y<font -> tf_YSize ;y++)
	{
		for (n=start_byte;n<end_byte;n++)
		{
			data = ( (char *) font -> tf_CharData) [ n + (y * font -> tf_Modulo)  ];
			bit_offset = (n-start_byte)<<3;

			num_bits_in_byte = (bit_width - bit_offset) >8 ? 8 : (bit_width - bit_offset) ;

			for (x=0;x< num_bits_in_byte ;x++)
			{
				if (x + bit_offset - _bit_start > -1)
				{
					if (data & (1<<(7-x)) ) 
					{
						retroPixel( screen, memory, rp_x + x + bit_offset - _bit_start, rp_y + y, pen);
					}
				}
			}
			bit_start = 0;
		}
	}
}

void draw_glyph_shade(struct retroScreen *screen, struct TextFont *font, int rp_x, int rp_y, int glyph, int pen)
{
	int x,y;
	short bit_start ;
	short bit_width ;
	int start_byte ;
	int num_bytes ;
	int end_byte ; 
	unsigned char data;
	int bit_offset;
	int num_bits_in_byte;
	int _bit_start;
	int n;
	int _y;
	unsigned char *memory = screen -> Memory[screen -> double_buffer_draw_frame]; 

	if ((glyph<font -> tf_LoChar)||(glyph>font -> tf_HiChar))
	{
		return ;
	}

	glyph -= font -> tf_LoChar;

	bit_start =(( struct _font_loc *)  font -> tf_CharLoc) [glyph].bit_start;
	bit_width = (( struct _font_loc *) font -> tf_CharLoc) [glyph].bit_width;
	start_byte = bit_start >> 3;
	num_bytes = (bit_width & 7) ? (bit_width >> 3) + 1 : bit_width >> 3;
	end_byte =start_byte + num_bytes; 

	bit_start = bit_start & 7;	// we don't need to know what byte anymore.
	bit_width += bit_start;
	_bit_start = bit_start;

	num_bytes = (bit_width & 7) ? (bit_width >> 3) + 1 : bit_width >> 3;
	end_byte =start_byte + num_bytes; 

	for (y=0;y<font -> tf_YSize ;y+=2)
	{
		for (n=start_byte;n<end_byte;n++)
		{
			bit_offset = (n-start_byte)<<3;

			num_bits_in_byte = (bit_width - bit_offset) >8 ? 8 : (bit_width - bit_offset) ;

			for (x=0;x< num_bits_in_byte ;x++)
			{
				_y = y+(x&1);

				if (x + bit_offset - _bit_start > -1)
				{
					data = ( (char *) font -> tf_CharData) [ n + (_y * font -> tf_Modulo)  ];

					if (data & (1<<(7-x)) ) 
					{
						retroPixel( screen, memory, rp_x + x + bit_offset - _bit_start, rp_y + _y, pen);
					}
				}
			}
			bit_start = 0;
		}
	}
}


//struct esc_cmd;	// forward declare.

struct esc_data
{
	const char *esc;
	void (*fn) (struct retroScreen *screen, struct esc_data *, int, int, char );
	int x;
	int y;
};

struct esc_cmd
{
	const char *name;
	void (*fn) (struct retroScreen *screen,struct esc_data *, int, int, char );
};

void esc_zone (struct retroScreen *screen,struct esc_data *data, int x1, int y1, char c )
{
	struct zone *item;
	int z = c - '0';

	if ((z>-1)&&(z<instance.zones_allocated))
	{
		item  = instance.zones + z;
		item -> screen = instance.current_screen ;
		item -> x0 = data -> x * 8;
		item -> y0 = data -> y * 8;
		item -> x1 = x1*8;
		item -> y1 = y1*8+7;

//		retroBAR( screen,zones[z].x0,zones[z].y0,zones[z].x1,zones[z].y1,4);
	}
}


void esc_border (struct retroScreen *screen,struct esc_data *data, int x1, int y1, char c )
{
	int x,y;

	struct retroTextWindow *textWindow = screen -> currentTextWindow;
	if (!textWindow) return;

	data -> y --;
	data -> x --;
	y1 ++;

	if (data->x>-1)
	{
		if (data -> y>-1) draw_char(screen,textWindow,data->x , data -> y , '.', screen -> pen, screen -> paper, false, 0,0);
		if ( y1>-1) draw_char(screen,textWindow,data->x , y1 , '\'', screen -> pen, screen -> paper, false,0,0);
	}

	if (x1>-1)
	{
		if (data -> y>-1) draw_char(screen,textWindow,x1 , data -> y , '.', screen -> pen, screen -> paper,false,0,0);
		if ( y1>-1) draw_char(screen,textWindow, x1 , y1 , '\'', screen -> pen, screen -> paper,false,0,0);
	}

	for (y=data->y+1;y<y1;y++)
	{
		if (data -> x>-1) draw_char(screen,textWindow,data -> x ,  y , '|', screen -> pen, screen -> paper,false,0,0);
		if ( x1>-1) draw_char(screen,textWindow, x1 , y , '|', screen -> pen, screen -> paper,false,0,0);
	}

	for (x=data->x+1;x<x1;x++)
	{
		if (data -> y>-1) draw_char(screen,textWindow, x , data -> y , '-', screen -> pen, screen -> paper,false,0,0);
		if ( y1>-1) draw_char(screen,textWindow, x , y1 , '-', screen -> pen, screen -> paper,false,0,0);
	}
}


void esc_paper (struct retroScreen *screen, char c )
{
	screen -> paper = c-'0';
}

void esc_pen (struct retroScreen *screen, char c )
{
	screen -> pen = c-'0';
}

void esc_x (struct retroScreen *screen, char c )
{
	struct retroTextWindow *textWindow = screen -> currentTextWindow;
	if (!textWindow) return;

	textWindow -> locateX = c-'0';
}

void esc_y (struct retroScreen *screen, char c )
{
	struct retroTextWindow *textWindow = screen -> currentTextWindow;
	if (!textWindow) return;

	textWindow -> locateY = c-'0';
}


struct esc_cmd esc_codes[]=
{
	{"R",NULL},		// 0,return
	{"Z0",esc_zone},	// 1, zone
	{"E0",esc_border},	// 2, border
	{"B",NULL},		// 3
	{"P",NULL},		// 4, pen
	{"X",NULL},		// 5, x pos
	{"Y",NULL},		// 6, y pos
	{NULL,NULL}
};

struct esc_data esc_data_tab[20];

int what_esc_code(const char *txt)
{
	int ret;
	struct esc_cmd *code;

	ret = 0;
	for (code=esc_codes; code->name; code++)
	{
		if (strncmp(txt,code->name,strlen(code->name))==0) return ret;
		ret ++;
	}
	return -1;
}


void limit_textwindow_location(struct retroScreen *screen )
{
 	int x;
	int y;
	struct retroTextWindow *textWindow = screen -> currentTextWindow;
	int _has_border ;
	int charsPerRow ;

	if (!textWindow) return;

	_has_border =(textWindow -> border ? 1 : 0);

	int _x = textWindow -> x + _has_border;
	int _y = textWindow -> y + _has_border;
	int _w = textWindow -> charsPerRow - (2*_has_border);
	int _h = textWindow -> rows - (2*_has_border);


	charsPerRow = textWindow -> charsPerRow - (_has_border * 2);

	if (textWindow -> locateX>=  charsPerRow )
	{
		textWindow -> locateX = textWindow -> locateX % charsPerRow;
		textWindow -> locateY++;
	}

		if (textWindow -> locateY >= textWindow -> rows-(_has_border*2) )
		{

			unsigned char *src = screen -> Memory[ screen -> double_buffer_draw_frame ] ;
			unsigned char *des = screen -> Memory[ screen -> double_buffer_draw_frame ];
			int *isrc;
			int *ides;

			textWindow -> locateY--;

			int intsPerRow = _w * 2;		// 1 char = 8 pixels/bytes  -> 8/4 bytes = 2
			int intsStart = _x * 2;
			int paper_height = _h * 8;

			src += ( screen -> bytesPerRow * (8 * (_y+1)) );
			des += ( screen -> bytesPerRow * (8 * _y) );

			for (y=0;y<paper_height-8;y++)
			{	
				isrc = (int *) src + intsStart;
				ides = (int *) des + intsStart;

				for (x=0;x<intsPerRow; x++)
				{
					ides[x]= isrc[x];
				}

				src += screen -> bytesPerRow;
				des += screen -> bytesPerRow;
			}

			{
				int gx = _x * 8; 
				int gy = _y * 8;
				int gw = _w * 8;

				retroBAR( screen, screen -> double_buffer_draw_frame,
					gx, gy + paper_height-8,
					gx + gw, gy + paper_height,
					screen -> paper);

			}

		}
}

extern int _tab_size ;

void draw_tab(struct retroScreen *screen)
{
	int n = 0;
	struct retroTextWindow *textWindow = screen -> currentTextWindow;
	if (!textWindow) return;


	n = textWindow -> locateX % _tab_size;

	while (n<_tab_size)
	{
		draw_char(screen, textWindow,
			textWindow -> locateX,
			textWindow -> locateY , 20, screen -> pen, screen -> paper, false, 0, 0);

		textWindow -> locateX ++;
		limit_textwindow_location(screen);
		n++;
	}
}

void _my_print_text(struct retroScreen *screen, char *text, int maxchars, bool underLine, bool shade, bool inverse, int w1,int w2 )
{
	struct retroTextWindow *textWindow = screen -> currentTextWindow;
	if (!textWindow) return;

	char c;
	int cnt = 0;
	int esc_count = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	while (c =*text ++) 
	{
		if (maxchars) 
		{
			if (cnt>=maxchars) break;
			cnt++;
		}

		switch (c)
		{
			case 27:	// ESC code sequence.
					{
						int code = what_esc_code( (const char *) text);

						switch (code)
						{
							case -1:	break;
							case 0:	text +=1;		// skip R.
									if (*text)	// not \0
									{
										if (esc_count)
										{
											esc_count --;
											printf("%s: %d,%d to %d,%d, return %c\n",  esc_data_tab[esc_count].esc, esc_data_tab[esc_count].x, esc_data_tab[esc_count].y, textWindow -> locateX, textWindow -> locateY, *text);
											if (esc_data_tab[esc_count].fn) esc_data_tab[esc_count].fn( screen, &esc_data_tab[esc_count], textWindow -> locateX, textWindow -> locateY, *text);
										}
										text++;
									}
									break;

							case 3:	// Paper
									text += strlen(esc_codes[code].name);
									esc_paper( screen, *text);
									if (*text) text++;
									break;

							case 4:	// Pen
									text += strlen(esc_codes[code].name);
									esc_pen( screen, *text);
									if (*text) text++;
									break;

							case 5:	// X
									text += strlen(esc_codes[code].name);
									esc_x( screen,  *text);
									if (*text) text++;
									break;

							case 6:	// Y
									text += strlen(esc_codes[code].name);
									esc_y( screen,  *text);
									if (*text) text++;
									break;

							default: 	
									text += strlen(esc_codes[code].name);
									esc_data_tab[esc_count].esc = esc_codes[code].name;
									esc_data_tab[esc_count].fn = esc_codes[code].fn;
									esc_data_tab[esc_count].x = textWindow -> locateX;
									esc_data_tab[esc_count].y = textWindow -> locateY;
									esc_count++;
									break;
						}
					}
					break;

			case 9:	draw_tab(screen);
					break;

			case 10:	textWindow -> locateX = 0;
					textWindow -> locateY++;
					break;
			default:

					if (inverse)
					{
						draw_char(screen, textWindow,
							textWindow -> locateX ,
							textWindow -> locateY , c, 
							screen -> paper, 
							screen -> pen,
							shade,w1,w2);
					}
					else
					{
						draw_char(screen, textWindow,
							textWindow -> locateX ,
							textWindow -> locateY , c, 
							screen -> pen, 
							screen -> paper,
							shade,w1,w2);
					}

					if (underLine) retroLine(screen,screen -> double_buffer_draw_frame, textWindow -> locateX*8,textWindow -> locateY*8+6,textWindow -> locateX*8+7,textWindow -> locateY*8+6, screen -> pen);

					textWindow -> locateX ++;
		}

		limit_textwindow_location( screen );
	}
}

int strlen_no_esc(struct stringData *txt)
{
	int _l = 0;
	const char *c=&txt -> ptr;
	const char *s_end = &(txt -> ptr) + txt -> size;

	for (;c<s_end;c++)
	{
		switch (*c)
		{
			case 27:	// ESC code sequence.
					{
						int code = what_esc_code( c+1 );

						if (code>-1) c += strlen(esc_codes[code].name);

						switch (code)
						{
							case -1:	break;
							case 0:	if (c<s_end)	// not \0
									{
										c++;
									}
									break;
							case 2:	if (c<s_end) c++;	// border
									break;
							case 3:	if (c<s_end) c++;	// Paper
									break;
							case 4:	if (c<s_end) c++;	// Pen
									break;
							case 5:	if (c<s_end) c++;	// X
									break;
							case 6:	if (c<s_end) c++;	// Y
									break;
							default: 	
									break;
						}
					}
					break;
			default:
					_l++;
		}
	}

	return _l;
}

