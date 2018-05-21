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

void _my_print_text(struct retroScreen *screen, char *text, int maxchars)
{
 	int x;
	int y;
	char c;
	int cnt = 0;

	while (c =*text ++) 
	{
		if (maxchars) 
		{
			if (cnt>=maxchars) break;
			cnt++;
		}

		switch (c)
		{
			case 10:	screen -> locateX = 0;
					screen -> locateY++;
					break;
			default:
					x = screen -> locateX * 8;
					y = (screen -> locateY * 8) ;

					retroBAR( screen, x,y,x+7,y+7, paper);
					draw_glyph( screen, topaz8_font, x, y, c );
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

