
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

			retroPixel( screen, x +x0,y +y0, color );	
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

		retroBAR( screen, 0,0, screen -> realWidth,screen->realHeight, screen -> paper );

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

