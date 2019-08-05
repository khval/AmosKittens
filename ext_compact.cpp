
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

int get_pac_pic_option( int bank_num, unsigned int block_id, int offset);

// palette data for RLE
static int r[32]={0},g[32]={0},b[32]={0};

// data to image 
// static unsigned char *data = NULL;

#define get2( pos ) (int(data[pos])*256 + int(data[pos+1]))
#define get4(  pos ) ( ( ( int(data[pos])*256 + int(data[pos+1]) )*256 + int(data[pos+2]) ) * 256 + int(data[pos+3]) )

static void getRGB( unsigned char *data, int pos, int &r, int &g, int &b ) { // get RGB converted to 0..255
	r = (data[pos] & 0x0F) * 0x11;
	g = ((data[pos+1] & 0xF0)>>4) * 0x11;
	b = (data[pos+1] & 0x0F) * 0x11;
}

static void setRGB( unsigned char *data, int pos, int r, int g, int b )
{
	data[pos] |= 0x01 * (r >> 4);
	data[pos+1] |= 0x10 * (g >> 4);
	data[pos+1] |= 0x01 * (b >> 4);
}


#define _debug_spack

#ifdef debug_spack

unsigned char d_data[40*200];
int d_data_pos;

unsigned char d_rle[40*200];
int d_rle_pos;

unsigned char d_rrle[40*200];
int d_rrle_pos;

void _sd(unsigned char d)
{
	printf("save DATA %02X at %d\n",d,d_data_pos);
	d_data[d_data_pos] = d;
	d_data_pos++;

	if (d) getchar();
}

void _srle(unsigned char d)
{
	printf("save RLE %02X at %d\n",d,d_data_pos);
	d_rle[d_rle_pos] = d;
	d_rle_pos++;

	if (d) getchar();
}

void _srrle(unsigned char d)
{
	printf("save RRLE %02X at %d\n",d,d_data_pos);
	d_rrle[d_rle_pos] = d;
	d_rrle_pos++;

	if (d) getchar();
}

void _cd(unsigned char d)
{
	if (d_data[d_data_pos] != d)
	{
		printf("DATA pos %d, expected %02x, got %02X\n",d_data_pos,d_data[d_data_pos],d);
		getchar();
	}
	d_data_pos++;
}

void _crle(unsigned char d)
{
	if (d_rle[d_rle_pos] != d)
	{
		printf("RLE pos %d, expected %02x, got %02X\n",d_rle_pos,d_rle[d_rle_pos],d);
		getchar();
	}
	d_rle_pos++;
}

void _crrle(unsigned char d)
{
	if (d_rrle[d_rle_pos] != d)
	{
		printf("RRLE pos %d, expected %02x, got %02X\n",d_rrle_pos,d_rrle[d_rrle_pos],d);
		getchar();
	}
	d_rrle_pos++;
}

#endif

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

	printf(" plotUnpackedContext %d,%d\n ",x0,y0);

	for (int y=0; y < imageHeight; y++)
	{
		row = context -> w * y;

		for (int x=0; x < imageWidth; x++)
		{
			byte = x / 8;
			bit = 0x80 >> (x&7);
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

unsigned int toAmosMode(int retromode)
{
	int mode = 0;

	if (retromode & retroInterlaced)	
		mode |= 0x0004;

	if (retromode & retroHires)
		mode |= 0x8000;

	if (retromode & retroLowres_pixeld)
		mode |= 0x0000;

	if (retromode & retroHam6)
		mode |= 0x6000;

	return mode;
}

void __close_screen( int screen_num )
{
	if (screens[screen_num]) 
	{
//		videomode = screens[screen_num] -> videomode;

		freeScreenBobs(screen_num);
		freeAllTextWindows( screens[screen_num] );
		retroCloseScreen(&screens[screen_num]);
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

//	printf("retromode: %08x\n", videomode);
//	getchar();

	engine_lock();

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

#ifdef debug_spack
		yy = 0;
		for (y=0;y<context->h;y++)
		{
			for (x=0;x<context->w*8;x++)
				if (x&16) if (x&1) retroPixel( screen, screen -> Memory[0], x, yy,4);

			yy += context -> ll;
		}
#endif

	}
	else
	{
		printf("no screen opened\n");
		printf("Screen Open %d,%d,%d,%08x\n", screen_num, context -> w * 8, context -> h * context -> ll, videomode);
		getchar();

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

#ifdef debug_spack
	d_data_pos = 0;
	d_rle_pos = 0;
	d_rrle_pos = 0;
#endif

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

#ifdef debug_spack
	printf("picdata %d, rledata %d points %d\n", o+24, get4(o+16), get4(o+20));
	printf(" *picdata = %02x, *rledata = %02x,  *points %02x\n ",*picdata, *rledata,  *points);
	getchar();
#endif

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
//						printf("rlebyte %2x rbit %02x\n", rlebyte, (1 << rbit));

						/* if the current RLE bit is set to 1, read in a new picture byte */
						if (rlebyte & (1 << rbit--)) picbyte = *picdata++;

						/* write picture byte and move down by one line in the picture */
#ifdef debug_spack
						_cd( picbyte );
						if (d_data_pos == 10) return true; 
#endif

						*dd = picbyte;
         					dd += context -> w;

						/* if we've run out of RLE bits, check the POINTS bits to see if a new RLE byte is needed */

						if (rbit < 0)
						{
							rbit = 7;

							if (*points & (1 << rrbit--)) 
							{
								rlebyte = *rledata++;
#ifdef debug_spack
								_crle(rlebyte);
#endif
							}
							if (rrbit < 0)  
							{
								rrbit = 7;
								points++;
#ifdef debug_spack
								_crrle( *points );
#endif
							}
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

int get_pac_pic_option( int bank_num, unsigned int block_id, int offset)
{
	struct kittyBank *bank;
	unsigned char *ptr;
	unsigned int this_id;

	bank = findBank(bank_num);
	if (bank)
	{
		int o=0;
		ptr = (unsigned char *) bank -> start;
				
		while(true)
		{
			this_id = *((unsigned int *) (ptr+o));	
			if (block_id != this_id )
			{
				switch (this_id)
				{
					case 0x12031990:	o+= 90;	break;	// known size of block, move to next.
					default:	return 0;
				}
			}
			else
			{
				o+=offset;
				return (int) *((unsigned short *) (ptr+o));
			}
		}
	}

	return 0;
}

void unpack( struct glueCommands *data, int bank_num, int screen_num, int x0, int y0 )
{
	struct kittyBank *bank;

	x0 -= x0 %8; 

	bank = findBank(bank_num);
	if (bank)
	{
		if (bank -> start)
		{
			struct PacPicContext context;

			if ( convertPacPic( (unsigned char *) bank -> start, &context ) )
			{
				if (screens[screen_num] == NULL)	// no screen, open new screen.
				{
					openUnpackedScreen( screen_num, &context );
				}
				else
				{
					plotUnpackedContext( &context, screens[screen_num], x0,y0 );
				}

				free( context.raw);
			}

			if (screens[screen_num] == NULL) setError(47,data->tokenBuffer );
		}
		else setError(36,data->tokenBuffer);	// Bank not reserved
	}
	else setError(25, data->tokenBuffer);
}

char *_ext_cmd_unpack( struct glueCommands *data, int nextToken )
{
	int bank_num = 0;
	int screen_num = 0;
	int x0 = 0, y0 = 0;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = stack - data -> stack  +1;

	switch (args)
	{
		case 1:
			bank_num = getStackNum(stack);
			screen_num = current_screen;

			x0 = get_pac_pic_option( bank_num, 0x06071963, 4 ) << 2;
			y0 = get_pac_pic_option( bank_num, 0x06071963, 6 );
			break;

		case 2:
			bank_num = getStackNum(stack-1);
			screen_num = getStackNum(stack);
			__close_screen( screen_num );
			break;

		case 3:
			bank_num = getStackNum(stack-2);

			x0 = get_pac_pic_option( bank_num, 0x06071963, 4 ) << 2;
			y0 = get_pac_pic_option( bank_num, 0x06071963, 6 );

			stack_get_if_int(stack-1,&x0);
			stack_get_if_int(stack,&y0);
			screen_num = current_screen;
			break;

		case 4:
			bank_num = getStackNum(stack-3);
			screen_num = getStackNum(stack-2);

			x0 = get_pac_pic_option( bank_num, 0x06071963, 4 ) << 2;
			y0 = get_pac_pic_option( bank_num, 0x06071963, 6 );

			stack_get_if_int(stack-1,&x0);
			stack_get_if_int(stack,&y0);
			__close_screen( screen_num );
			break;

		default:
			setError(22, data->tokenBuffer);	// wrong number of args.
			popStack( stack - data->stack );
			return NULL;
	}

	printf( "unpack( %08x, %d  %d, %d, %d )\n" ,  data, bank_num, screen_num, x0, y0 );

	unpack( data, bank_num, screen_num, x0, y0 );

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


void save_byte( struct PacPicContext *context, unsigned char value )
{
	context -> data[ context -> data_used ] = value;
	context -> data_used ++;
}

void save_rle( struct PacPicContext *context, unsigned char rle )
{
	if ( context -> first_rle )
	{
#ifdef debug_spack
		_srle( rle );
#endif
		context -> rledata[ context -> rledata_used ] = rle;
		context -> rledata_used ++;
		context -> first_rle = false;
	}
	else
	{
		context -> rrle_bit --;

		if ( context -> last_rle != rle)
		{
#ifdef debug_spack
			_srle( rle );
#endif
			context -> rledata[ context -> rledata_used ] = rle;
			context -> rledata_used++;
			context -> rrle |= (1 << context -> rrle_bit);
		}

		if (context -> rrle_bit == 0)	// run out bits..
		{
#ifdef debug_spack
			_srrle( context -> rrle );
#endif
			context -> points[ context -> points_used ] = context -> rrle;
			context -> points_used++;
			context -> rrle = 0;
			context -> rrle_bit = 8;		// msb is first rle byte
		}
	}

	context -> last_rle = rle;
}

void spack_image( unsigned char **plains, struct PacPicContext *context )
{
	unsigned char *lump_start;
	unsigned char rle,last;
	int rle_bit;
	bool first;

	last = *plains[0];

	rle = 0;
	rle_bit = 7;
	first = true;

#ifdef debug_spack
	d_data_pos = 0;
	d_rle_pos = 0;
	d_rrle_pos = 0;
#endif


	for (int d = 0 ; d < context -> d; d++)
	{
		lump_start = plains[d];

		for( int j = 0; j < context -> h; j++ )
		{
			unsigned char *lump_offset = lump_start;

			for( int k = 0; k < context -> w; k++ )
			{
				unsigned char *row = lump_offset;

				for( int l = 0; l < context -> ll; l++ )
				{
					if ( first )
					{
#ifdef debug_spack
						_sd( *row );
#endif
						save_byte( context, *row );
						first = false;
					}
					else
					{
						rle_bit --;
	
						if ( last != *row)
						{
#ifdef debug_spack
							_sd( *row );
#endif
							save_byte( context, *row );
							rle |= 1 << rle_bit;
						}

						if (rle_bit == 0)
						{
							save_rle( context, rle );
							rle = 0;
							rle_bit = 8;
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
}

static void init_context(struct PacPicContext *context, int size)
{

	context -> data = (unsigned char *) malloc( size * 8 );
	context -> rledata = (unsigned char *) malloc( size * 8 );
	context -> points = (unsigned char *) malloc( size * 8 );

	memset( context -> data, 0, size * 8 );
	memset( context -> rledata, 0, size * 8 );
	memset( context -> points, 0, size * 8 );
	

	context -> rrle_bit = 7;	// msb is first byte.
	context -> rrle = 0;
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

bool spack(struct retroScreen *screen, int bank_num, int x0, int y0, int x1, int y1, bool include_screen )
{
	int imageWidth;
	int imageHeight;
	int planarWidth;
	int n;
	int allocated;
	int size;
	struct PacPicContext context;
	unsigned char *plains[8];

	if (screen == NULL) return false;

		freeBank(bank_num);

		imageWidth = x1 - x0 + 1;
		imageHeight = y1 - y0 + 1;

		planarWidth = imageWidth;
		planarWidth -= planarWidth % 8 ;
		context.w = planarWidth / 8;

		size = context.w * imageHeight;
		allocated = 0;

		init_context( &context , size );

		for (n=0;n<8;n++)
		{
			plains[n] = (unsigned char *) malloc( size );
			if (plains[n]) 
			{
				memset( plains[n], 0x00, size );
				allocated ++;
			}
		}

		printf("allocated plains %d\n",allocated);

		if ((allocated == 8) && (context.ready_to_encode))
		{
			int x,y;
			unsigned char c;
			int maxc = 0;
			struct kittyBank *bank;
			int _size;

			for (y = 0 ; y < imageHeight; y++ )
			{
				for ( x = 0 ; x < planarWidth; x++ )
				{
					c = retroPoint( screen, x+x0, y+y0);
					set_planar_pixel( plains, context.w, x, y, c );
					if (c>maxc) maxc = c;
				}
			}

			context.h = imageHeight;
			context.ll = 1;

			while ( ( ( context.h & 1) == 0 ) && (context.ll != 0x80))
			{
				context.ll *= 2; 
				context.h /= 2;
			}

			context.d = 1;
			while ( (1L<<context.d) < maxc ) context.d++;

			printf("looks like success to me\n");
			printf("highest color value is %d\n",maxc);
			printf("bytes per row: %d\n",context.w);
			printf("context.h %d, ll %d\n",context.h,context.ll);

			spack_image( plains, &context );

			_size = 90 + 24 + context.data_used + context.rledata_used + context.points_used;

			if (bank = __ReserveAs( 9, bank_num, _size , "Pac.Pic.", NULL ))
			{

				unsigned char *a = (unsigned char *) bank -> start;
				unsigned int o = 0;
				unsigned int o_data, o_rle, o_points;
			
				context.mode = toAmosMode( screen -> videomode );

				// bitmask 0x8000 is Hires, so can't give it that high depth, most limit it.
				// Better to get depth from the image data.

				context.mode |= context.d <6 ? (context.d * 0x1000) : 0x6000 ;		
				context.mode |= 0x0200;		// not sure but this bit is set while testing in AMOS PRO.

				if (include_screen)
				{
					set4(a+o,0x12031990 );
					set2(a+o+4, screen -> realWidth );
					set2(a+o+6, screen -> realHeight );
					set2(a+o+8, (screen -> scanline_x +128) / 2 );
					set2(a+o+10, (screen -> scanline_y + (50*2)) / 2);
					set2(a+o+12, screen -> displayWidth );
					set2(a+o+14, screen -> displayHeight );
					set2(a+o+16, screen -> offset_x );
					set2(a+o+18, screen -> offset_y );
					set2(a+o+20,context.mode);
					set2(a+o+22,2624);	// ??
					set2(a+o+24,context.d);

					// set palette
					for( int i=0; i<32; ++i )
					{ 
						setRGB( a, o+26+i*2, 
							screen -> orgPalette[i].r,
							screen -> orgPalette[i].g,
							screen -> orgPalette[i].b ); 
					}
					o+=90;
				}

				set4(a+o, 0x06071963);
				set2(a+o+4, x0 >> 2 );
				set2(a+o+6, y0 );
				set2(a+o+8, context.w);
				set2(a+o+10,context.h);
				set2(a+o+12,context.ll);
				set2(a+o+14,context.d);

				o_data = o+24;
				o_rle = o+24+context.data_used;
				o_points = o+24+context.data_used+context.rledata_used;

				set4( a+o+16, o_rle - o );
				set4( a+o+20, o_points - o);

				memcpy( a+o_data, context.data,  context.data_used );
				memcpy( a+o_rle, context.rledata,  context.rledata_used );
				memcpy( a+o_points, context.points,  context.points_used );

			}
		}

		for (n=0;n<8;n++)
		{
			if (plains[n])
			{
				free(plains[n]);
				plains[n] = NULL;
			}
		}

		printf ("context -> data_used %d\n", context.data_used);
		printf ("context -> rledata_used %d\n", context.rledata_used);
		printf ("context -> points_used %d\n", context.points_used);

		clean_up_context( &context );

	return true;
}

char *_ext_cmd_spack( struct glueCommands *data, int nextToken )
{
	int x0=0,y0=0,x1=0,y1=0;
	int bank_num;
	int screen_num;

	int args = stack - data -> stack  +1;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			screen_num = getStackNum(stack-1);
			bank_num = getStackNum(stack);

			if (struct retroScreen *screen = screens[screen_num])
			{
				x1 = screen -> realWidth-1;
				y1 = screen -> realHeight-1;
			}
			break;

		case 6:
			screen_num = getStackNum(stack-5);
			bank_num = getStackNum(stack-4);
			x0 = getStackNum(stack-3);
			y0 = getStackNum(stack-2);
			x1 = getStackNum(stack-1);
			y1 = getStackNum(stack);
			break;

		default:
			setError(22, data->tokenBuffer);	// wrong number of args.
			popStack( stack - data->stack );
			return NULL;
			break;
	}

	printf("spack %d to %d,%d,%d,%d,%d\n",screen_num, bank_num,x0,y0,x1,y1);

	spack( screens[screen_num] , bank_num, x0,y0,x1,y1, true );

	popStack( stack - data->stack );
	return NULL;
}

char *ext_cmd_spack(nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_spack, tokenBuffer );
	return tokenBuffer;
}

char *_ext_cmd_pack( struct glueCommands *data, int nextToken )
{
	int x0=0,y0=0,x1=0,y1=0;
	int bank_num;
	int screen_num;

	int args = stack - data -> stack  +1;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			screen_num = getStackNum(stack-1);
			bank_num = getStackNum(stack);

			if (struct retroScreen *screen = screens[screen_num])
			{
				x1 = screen -> realWidth-1;
				y1 = screen -> realHeight-1;
			}
			break;

		case 6:
			screen_num = getStackNum(stack-5);
			bank_num = getStackNum(stack-4);
			x0 = getStackNum(stack-3);
			y0 = getStackNum(stack-2);
			x1 = getStackNum(stack-1);
			y1 = getStackNum(stack);
			break;

		default:
			setError(22, data->tokenBuffer);	// wrong number of args.
			popStack( stack - data->stack );
			return NULL;
			break;
	}

	dprintf("pack %d to %d,%d,%d,%d,%d\n",screen_num, bank_num,x0,y0,x1,y1);

	spack( screens[screen_num] , bank_num, x0,y0,x1,y1, false );

	popStack( stack - data->stack );
	return NULL;
}

char *ext_cmd_pack(nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_pack, tokenBuffer );
	return tokenBuffer;
}


