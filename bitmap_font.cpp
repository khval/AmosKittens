#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/diskfont.h>
#include <string>
#include <proto/retroMode.h>
#include "AmosKittens.h"

extern struct DiskfontIFace *IDiskfont;
extern struct TextFont *topaz8_font;

extern int pen;
extern int paper;

void draw_glyph(struct retroScreen *screen, struct TextFont *font, int rp_x, int rp_y, int glyph);

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

void draw_bit( struct retroScreen *screen, int x,int y)
{
	retroPixel( screen,x,y,pen);
}

void draw_char(struct retroScreen *screen, int lX, int lY, char c )
{
	int x = lX * 8;
	int y = lY * 8 ;
	retroBAR( screen, x,y,x+7,y+7, paper);
	draw_glyph( screen, topaz8_font, x, y, c );
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

void draw_glyph(struct retroScreen *screen, struct TextFont *font, int rp_x, int rp_y, int glyph)
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
						draw_bit( screen, rp_x + x + bit_offset - _bit_start, rp_y + y ) ;
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

extern int current_screen;
extern struct retroScreen *screens[8] ;

void esc_zone (struct retroScreen *screen,struct esc_data *data, int x1, int y1, char c )
{
	int z = c - '0';

	if ((z>-1)&&(z<zones_allocated))
	{
		zones[z].screen = current_screen ;
		zones[z].x0 = data -> x * 8;
		zones[z].y0 = data -> y * 8;
		zones[z].x1 = x1*8;
		zones[z].y1 = y1*8+7;

//		retroBAR( screen,zones[z].x0,zones[z].y0,zones[z].x1,zones[z].y1,4);
	}
}


void esc_border (struct retroScreen *screen,struct esc_data *data, int x1, int y1, char c )
{
	int x,y;
	data -> y --;
	data -> x --;
	y1 ++;

	if (data->x>-1)
	{
		if (data -> y>-1) draw_char(screen, data->x , data -> y , '.');
		if ( y1>-1) draw_char(screen, data->x , y1 , '\'');
	}

	if (x1>-1)
	{
		if (data -> y>-1) draw_char(screen, x1 , data -> y , '.');
		if ( y1>-1) draw_char(screen, x1 , y1 , '\'');
	}


	for (y=data->y+1;y<y1;y++)
	{
		if (data -> x>-1) draw_char(screen, data -> x ,  y , '|');
		if ( x1>-1) draw_char(screen, x1 , y , '|');
	}

	for (x=data->x+1;x<x1;x++)
	{
		if (data -> y>-1) draw_char(screen, x , data -> y , '-');
		if ( y1>-1) draw_char(screen, x , y1 , '-');
	}
}

extern int pen;
extern int paper;

void esc_paper (struct retroScreen *screen, char c )
{
	paper = c-'0';
}

void esc_pen (struct retroScreen *screen, char c )
{
	pen = c-'0';
}

void esc_x (struct retroScreen *screen, char c )
{
	if (screens[current_screen]) screens[current_screen] -> locateX = c-'0';
}


void esc_y (struct retroScreen *screen, char c )
{
	if (screens[current_screen]) screens[current_screen] -> locateY = c-'0';

}


struct esc_cmd esc_codes[]=
{
	{"R",NULL},		// 0,return
	{"Z0",esc_zone},	// 1, zone
	{"E0",esc_border},	// 2, border
	{"B",NULL},		// 3
	{"P",NULL},		// 4
	{"X",NULL},		// 5
	{"Y",NULL},		// 6
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


void _my_print_text(struct retroScreen *screen, char *text, int maxchars)
{
 	int x;
	int y;
	char c;
	int cnt = 0;
	int esc_count = 0;

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
						bool not_found = false;
						switch (code)
						{
							case -1:	break;
							case 0:	text +=1;		// skip R.
									if (*text)	// not \0
									{
										if (esc_count)
										{
											esc_count --;
											printf("%s: %d,%d to %d,%d, return %c\n",  esc_data_tab[esc_count].esc, esc_data_tab[esc_count].x, esc_data_tab[esc_count].y, screen -> locateX, screen -> locateY, *text);
											if (esc_data_tab[esc_count].fn) esc_data_tab[esc_count].fn( screen, &esc_data_tab[esc_count], screen -> locateX, screen -> locateY, *text);
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
									esc_data_tab[esc_count].x = screen -> locateX;
									esc_data_tab[esc_count].y = screen -> locateY;
									esc_count++;
									break;
						}
					}
					break;

			case 10:	screen -> locateX = 0;
					screen -> locateY++;
					break;
			default:
					draw_char(screen, screen -> locateX , screen -> locateY , c);
					screen -> locateX ++;
		}

		if (screen-> locateX>=screen -> realWidth /8)
		{
			screen -> locateX = 0;
			screen -> locateY++;
		}

		if (screen-> locateY>=screen -> realHeight /8)
		{
			unsigned char *src = screen -> Memory + ( screen -> bytesPerRow * 8 );
			unsigned char *des = screen -> Memory;
			screen -> locateY--;
			int intsPerRow = screen ->bytesPerRow / 4;
			int *isrc;
			int *ides;
			int paper_rows = screen -> realHeight / 8;
			int paper_height = paper_rows * 8;

			for (y=0;y<paper_height-8;y++)
			{	
				isrc = (int *) src;
				ides = (int *) des;

				for (x=0;x<intsPerRow; x++)
				{
					ides[x]=isrc[x];
				}

				src += screen -> bytesPerRow;
				des += screen -> bytesPerRow;
			}

			retroBAR( screen,
					0,paper_height-8,
					screen->realWidth, screen-> realHeight, 
					paper);
		}
	}
}

