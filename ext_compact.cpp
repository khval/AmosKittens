
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <string.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#endif

#include "debug.h"
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsBanks.h"
#include "commandsBlitterObject.h"
#include "errors.h"
#include "engine.h"
#include "ext_compact.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern int current_screen;
extern struct retroSprite *sprite ;
extern struct retroSprite *icons ;

extern void _my_print_text(struct retroScreen *screen, char *text, int maxchars);

extern struct retroTextWindow *newTextWindow( struct retroScreen *screen, int id );
extern void freeAllTextWindows(struct retroScreen *screen);

// palette data for RLE
static int r[32]={0},g[32]={0},b[32]={0};

// data to image 
// static unsigned char *data = NULL;

#define get2( pos ) (int(data[pos])*256 + int(data[pos+1]))
#define get4(  pos ) ( ( ( int(data[pos])*256 + int(data[pos+1]) )*256 + int(data[pos+2]) ) * 256 + int(data[pos+3]) )

void getRGB( unsigned char *data, int pos, int &r, int &g, int &b ) { // get RGB converted to 0..255
	r = (data[pos] & 0x0F) * 0x11;
	g = ((data[pos+1] & 0xF0)>>4) * 0x11;
	b = (data[pos+1] & 0x0F) * 0x11;
}

void plotUnpackedContext( struct PacPicContext *context, struct retroScreen *screen, int x0, int y0 )
{
	int row;
	int byte;
	int bit;
	int color;
	int planeOffset;
	int imageWidth = context -> w * 8;
	int imageHeight = context -> h * context -> ll;
	int bytesPerPlan = context -> w * imageHeight;
	unsigned char *mem = screen -> Memory[0];

	for (int y=0; y < imageHeight; y++)
	{
		row = context -> w * y;

		for (int x=0; x < imageWidth; x++)
		{
			byte = x / 8;
			bit = 1<<(7-(x & 7));
			planeOffset = 0;

			color = 0;
			for (int d=0;d<context -> d;d++)
			{
				color += context -> raw[ row + byte + planeOffset ] & bit ? 1<<d: 0 ;
				planeOffset += bytesPerPlan;
			}

			retroPixel( screen, mem, x +x0,y +y0, color );	
		}
	}
}

void openUnpackedScreen(int screen_num, 
//		int bytesPerRow, int height, 
		struct PacPicContext *context )
			
{
	int n;

	int colors = 1 << context -> d;
	unsigned int videomode = retroLowres_pixeld;
	struct retroScreen *screen = NULL;
	struct retroTextWindow *textWindow = NULL;

	printf("mode %08x\n", context -> mode);

	// mode & 0x8000 is Hires
	// mode & 0x7000 is colors 
	// mode & 0x0004 is Laced
	// mode = $6A00 is HAM6

	videomode = 0;
	if (context -> mode & 0x0004) videomode |= retroInterlaced;

	if (context -> mode & 0x8000)
	{
		 videomode |= retroHires; 
	}
	else
	{
		 videomode |= retroLowres_pixeld; 
	}

	if ( (context -> mode & 0x7000) == 0x6000 ) videomode |= retroHam6;

	printf("retromode: %08x\n", videomode);
	getchar();

	engine_lock();

	if (screens[screen_num]) 
	{
		videomode = screens[screen_num] -> videomode;

		freeScreenBobs(screen_num);
		freeAllTextWindows( screens[screen_num] );
		retroCloseScreen(&screens[screen_num]);
	}

	screens[screen_num] = retroOpenScreen(context -> w * 8, context -> h * context -> ll, videomode );

	if (screen = screens[screen_num])
	{
		current_screen = screen_num;

		retroApplyScreen( screen, video, 0, 0,	screen -> realWidth,screen->realHeight );

		if (textWindow = newTextWindow( screen, 0 ))
		{
			textWindow -> charsPerRow = screen -> realWidth / 8;
			textWindow -> rows = screen -> realHeight / 8;
			screen -> pen = 2;
			screen -> paper = 1;
			screen -> autoback = 2;

			screen -> currentTextWindow = textWindow;
		}

		for (n=0;n<colors;n++)	
		{
			retroScreenColor( screen, n,r[n],g[n],b[n]);
		}

		retroBAR( screen, 0, 0,0, screen -> realWidth,screen->realHeight, screen -> paper );

		plotUnpackedContext( context, screen, 0,0 );

	}

	video -> refreshAllScanlines = TRUE;
	engine_unlock();
}

bool convertPacPicData( unsigned char *data, int o , struct PacPicContext *context );

// pac.pic. RLE decompressor
bool convertPacPic( unsigned char *data, struct PacPicContext *context )
{
	//  int o = 20;
	int o=0;

	if( get4(o) == 0x12031990 )
	{
		printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		context -> mode = get2(o+20);

		// fetch palette
		for( int i=0; i<32; ++i )
		{ 
			getRGB( data, o+26+i*2, r[i],g[i],b[i] ); 
		}
		o+=90;
	}

	if( get4(o) != 0x06071963 )
	{
		printf("could not find picture header!\n");
		printf("%08x\n",get4(o));
		printf("%08x\n",get4(o+4));
		printf("%08x\n",get4(o+8));
		return false;
	}

	return convertPacPicData( data, o , context );

}


// pac.pic. RLE decompressor
bool convertPacPicData( unsigned char *data, int o , struct PacPicContext *context )
{
	context -> w  = get2(o+8),
	context -> h  = get2(o+10),
	context -> ll = get2(o+12),
	context -> d  = get2(o+14);

	printf( " w %d, h %d, ll %d, d %d\n",
		context -> w,	context -> h,	context -> ll,	context -> d );

	printf("data w %d,h %d\n",
				context->w*8,
				context->h*context->ll);

	if (context->w*8 > 640) return false;

	if (context->w == 0) return false;
	if ((context->h*context->ll) <0) return false;

	// reserve bitplane memory
	context -> raw = (unsigned char*) malloc( context -> w * context -> h * context -> ll * context -> d );

	unsigned char *picdata = &data[o+24];
	unsigned char *rledata = &data[o+get4(o+16)];
	unsigned char *points  = &data[o+get4(o+20)];

	if (context -> raw)
	{
		unsigned char *&raw = context -> raw;
		int rrbit = 6, rbit = 7;
		int picbyte = *picdata++;
		int rlebyte = *rledata++;
		if (*points & 0x80) rlebyte = *rledata++;

		for( int i = 0; i < context -> d; i++)
		{
			unsigned char *lump_start = &raw[ i * context -> w * context -> h * context -> ll ];

			for( int j = 0; j < context -> h; j++ )
			{
				unsigned char *lump_offset = lump_start;

				for( int k = 0; k < context -> w; k++ )
				{
					unsigned char *dd = lump_offset;

					for( int l = 0; l < context -> ll; l++ )
					{
						/* if the current RLE bit is set to 1, read in a new picture byte */
						if (rlebyte & (1 << rbit--)) picbyte = *picdata++;

						/* write picture byte and move down by one line in the picture */
						*dd = picbyte;
         					dd += context -> w;

						/* if we've run out of RLE bits, check the POINTS bits to see if a new RLE byte is needed */
						if (rbit < 0)
						{
							rbit = 7;
							if (*points & (1 << rrbit--)) rlebyte = *rledata++;
							if (rrbit < 0)  rrbit = 7, points++;
						}
					}

					lump_offset++;
				}

				lump_start += context -> w * context -> ll;
			}
		}

		return true;
	}

	return false;
}

char *_ext_cmd_unpack( struct glueCommands *data, int nextToken )
{
	int n;
	int screen_num;
	struct kittyBank *bank;
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = stack - data -> stack  +1;

	if (args==2)
	{
		n = getStackNum(stack-1);
		screen_num = getStackNum(stack);

		printf("unpack %d to %d\n",n,screen_num);

		bank = findBank(n);
		if (bank)
		{
			if (bank -> start)
			{
				struct PacPicContext context;

				if ( convertPacPic( (unsigned char *) bank -> start, &context ) )
				{
					openUnpackedScreen( screen_num, &context );
					free( context.raw);
				}

				if (screens[screen_num] == NULL) setError(47,data->tokenBuffer );
			}
			else setError(36,data->tokenBuffer);	// Bank not reserved
		}
		else setError(25, data->tokenBuffer);
	}
	else setError(22, data->tokenBuffer);	// wrong number of args.

	popStack( stack - data->stack );
	return NULL;
}

char *ext_cmd_unpack(nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_unpack, tokenBuffer );
	return tokenBuffer;
}

//----

void set_planar_pixel( unsigned char **plain, int bpr, int x, int y, unsigned char c  )
{
	unsigned char mask = 0x80 >> x  % 8;
	unsigned char umask = ~mask;
	int p = 0;
	int xbyte = x / 8;
	unsigned char *byte;
	int pos;

	pos = xbyte + y * bpr;
	
	while (c)
	{
		byte = plain[p] + pos ;
		*byte = (c&1)  ?  (*byte | mask) : (*byte & umask);
		c = c >> 1;
		p++;
	}

}


void get_rle(unsigned char **plains, int bytesPerRow, int h)
{
	unsigned char *row;
	unsigned char last;
	int b;
	int n;
	int y = 0;
	int wy = 0;	// writen y bytes
	int rle = 1;

	for (b=0;b<bytesPerRow;b++)
	{
		row = plains[0] + b;
		last = *row;
		y = 1;
		wy = 0;


		do
		{
			rle = 1;

			printf("w %02x\n",last);

			for (n=1;n<8;n++)
			{
				row += bytesPerRow;
				rle = rle << 1 ;

				if ( last != *row)
				{
					printf("w %02x\n",*row);
					rle |= 1;
					wy++;
				}

				last = *row;
				y++;
			}
		printf("rle %02x\n",rle);

		} while (y < h);


		printf("y %d wy %d h %d\n",y,wy,h);
	}
}

void save_byte( struct PacPicContext *context, unsigned char value )
{
	context -> data[ context -> data_used ] = value;
	context -> data_used ++;
}

void save_rle( struct PacPicContext *context, unsigned char rle )
{
	if ( context -> first_rle )
	{
		context -> rledata[ context -> rledata_used ] = rle;
		context -> rledata_used ++;
		context -> first_rle = false;
	}
	else
	{
		context -> rrle = context -> rrle << 1;

		if ( context -> last_rle != rle)
		{
			context -> rledata[ context -> rledata_used ] = rle;
			context -> rledata_used++;
			context -> rrle |= 1;
		}

		if (context -> rrle & 0x80)
		{
			context -> points[ context -> points_used ] = context -> rrle;
			context -> points_used++;
			context -> rrle = 1;
			context -> first_rle = true;
		}
	}
}

void spack( unsigned char **plains, struct PacPicContext *context )
{
	unsigned char *lump_start;
	unsigned char rle,last;
	bool first;
	int wy;

	lump_start = plains[0];

	for( int j = 0; j < context -> h; j++ )
	{
		unsigned char *lump_offset = lump_start;

		rle = 1;
		first = true;
		last = *lump_offset;

		for( int k = 0; k < context -> w; k++ )
		{
			unsigned char *row = lump_offset;

			for( int l = 0; l < context -> ll; l++ )
			{
				if ( first )
				{
					save_byte( context, last );
					first = false;
				}
				else
				{
					rle = rle << 1;

					if ( last != *row)
					{
						save_byte( context, *row );
						rle |= 1;
						wy++;
					}

					if (rle & 0x80)
					{
						save_rle( context, rle );
						rle = 1;
						first = true;
					}
				}

				last = *row;
				row += context -> w;
			}
			lump_offset++;
		}
		lump_start += context -> w * context -> ll;
	}
}

static void init_context(struct PacPicContext *context, int size)
{
	context -> data = (unsigned char *) malloc( size * 8 );
	context -> rledata = (unsigned char *) malloc( size * 8 );
	context -> points = (unsigned char *) malloc( size * 8 );

	context -> rrle = 1;
	context -> first_rle = true;
	context -> data_used = 0;
	context -> rledata_used = 0;
	context -> points_used = 0;

	context -> ready_to_encode = ((context -> data) && (context -> rledata) && (context -> points));
}

static void clean_up_context(struct PacPicContext *context)
{
	if (context -> data) free(context -> data);
	if (context -> rledata) free(context -> rledata);
	if (context -> points) free(context -> points);

	context -> data = NULL;
	context -> rledata = NULL;
	context -> points = NULL;
	context -> ready_to_encode = false;
}

#define  set2(a,v) *(unsigned short *) (a) = v
#define  set4(a,v) *(unsigned int *) (a) = v

char *_ext_cmd_spack( struct glueCommands *data, int nextToken )
{
	int bank_num;
	int screen_num;
	struct kittyBank *bank;
	unsigned char *plains[8];
	struct retroScreen *screen;
	int args = stack - data -> stack  +1;
	int planarWidth;
	int n;
	int allocated;
	int size;
	struct PacPicContext context;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==2)
	{
		screen_num = getStackNum(stack-1);
		bank_num = getStackNum(stack);

		printf("spack %d to %d\n",screen_num, bank_num);
		freeBank(bank_num);

		screen = screens[screen_num];

		planarWidth = screen -> realWidth;
		planarWidth += planarWidth % 8 ?  7 - screen -> realWidth % 8 : 0;
		context.w = planarWidth / 8;

		size = context.w * screen -> realHeight;
		allocated = 0;

		init_context( &context , size );

		for (n=0;n<8;n++)
		{
			plains[n] = (unsigned char *) malloc( size );
			if (plains[n]) 
			{
				memset( plains[n], 0, size );
				allocated ++;
			}
		}

		if ((allocated == 8) && (context.ready_to_encode))
		{
			int x,y;
			unsigned char c;
			int maxc = 0;
			struct kittyBank *bank;

			for (y = 0 ; y < screen -> realHeight; y++ )
			{
				for ( x = 0 ; x < screen -> realWidth; x++ )
				{
					c = retroPoint( screen, x, y);
					set_planar_pixel( plains, context.w, x, y, c );
					if (c>maxc) maxc = c;
				}
			}

			context.h = screen -> realHeight;
			context.ll = 1;
			context.d = 1;

			while ( ( ( context.h & 1) == 0 ) && (context.ll != 0x80))
			{
				context.ll *= 2; 
				context.h /= 2;
			}

			printf("looks like success to me\n");
			printf("highest color value is %d\n",maxc);
			printf("bytes per row: %d\n",context.w);
			printf("context.h %d, ll %d\n",context.h,context.ll);

			spack( plains, &context );


			if (bank = __ReserveAs( 0, 5, 5000 , "Pic.Pac.", NULL ))
			{
				unsigned char *a = (unsigned char *) bank -> start;
				unsigned int o = 0;
				unsigned int o_data, o_rle, o_points;
				
				set4(a+o, 0x06071963);

				set2(a+o+8, context.w);
				set2(a+o+10,context.h);
				set2(a+o+12,context.ll);
				set2(a+o+14,context.d);

				o_data = 24;
				o_rle = 24+context.data_used;
				o_points = 24+context.data_used+context.rledata_used;

				set4( a+o+16, o_rle );
				set4( a+o+20, o_points );

				memcpy( a+o_data, context.data,  context.data_used );
				memcpy( a+o_rle, context.rledata,  context.rledata_used );
				memcpy( a+o_points, context.points,  context.points_used );


//	unsigned char *picdata = &data[o+24];
//	unsigned char *rledata = &data[o+get4(o+16)];
//	unsigned char *points  = &data[o+get4(o+20)];
			}
		}

		for (n=0;n<8;n++)	if (plains[n]) free(plains[n]);

		printf ("context -> data_used %d\n", context.data_used);
		printf ("context -> rledata_used %d\n", context.rledata_used);
		printf ("context -> points_used %d\n", context.points_used);

		clean_up_context( &context );
	}
	else setError(22, data->tokenBuffer);	// wrong number of args.

	popStack( stack - data->stack );
	return NULL;
}

char *ext_cmd_spack(nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_spack, tokenBuffer );
	return tokenBuffer;
}

