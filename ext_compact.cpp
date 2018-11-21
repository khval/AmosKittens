#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "debug.h"
#include <string>
#include <iostream>
#include <proto/retroMode.h>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsBanks.h"
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

void _my_print_text(struct retroScreen *screen, char *text, int maxchars);

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

void openUnpackedScreen(int screen_num, int bytesPerRow, int height, int depth, int *r, int *g, int *b, unsigned char *raw,bool ham )
{
	int n;
	int row;
	int byte;
	int bit;
	int color;
	int d;
	int planeOffset;
	int bytesPerPlan;
	int bytesPerPlane;
	int colors = 1 << depth;
	struct retroScreen *screen = NULL;

	engine_lock();

	if (screens[screen_num]) retroCloseScreen(&screens[screen_num]);
	screens[screen_num] = retroOpenScreen( bytesPerRow * 8, height, retroLowres_pixeld );

	if (screen = screens[screen_num])
	{
		retroApplyScreen( screen, video, 0, 0,	screen -> realWidth,screen->realHeight );

		for (n=0;n<colors;n++)	
		{
			retroScreenColor( screen, n,r[n],g[n],b[n]);
		}

		retroBAR( screen, 0,0, screen -> realWidth,screen->realHeight, 1 );

		bytesPerPlan = bytesPerRow * height;

		for (int y=0; y < screen -> realHeight; y++)
		{
			row = bytesPerRow * y;

			for (int x=0; x < screen -> realWidth; x++)
			{
				byte = x / 8;
				bit = 1<<(7-(x & 7));
				planeOffset = 0;

				color = 0;
				for (d=0;d<depth;d++)
				{
					color += raw[ row + byte + planeOffset ] & bit ? 1<<d: 0 ;
					planeOffset += bytesPerPlan;
				}

				retroPixel( screen, x,y, color );					
			}
		}
	}

	engine_unlock();
}


// pac.pic. RLE decompressor
int convertPacPic( int screen, unsigned char *data, const char *name )
{
	//  int o = 20;
	int o=0;

	bool ham = false;
	if( get4(o) == 0x12031990 )
	{
		// detect HAM
		if( get2(o+20) & 0x800 )
		{
		      ham = true;
		      printf("HAM data is not yet supported, output will look garbled!\n");
		}
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
		return 1;
	}

  int w  = get2(o+8),
      h  = get2(o+10),
      ll = get2(o+12),
      d  = get2(o+14);

	// reserve bitplane memory
	unsigned char* raw = (unsigned char*) malloc(w*h*ll*d);
	unsigned char *picdata = &data[o+24];
	unsigned char *rledata = &data[o+get4(o+16)];
	unsigned char *points  = &data[o+get4(o+20)];

	if (raw)
	{
		int rrbit = 6, rbit = 7;
		int picbyte = *picdata++;
		int rlebyte = *rledata++;
		if (*points & 0x80) rlebyte = *rledata++;

		for( int i = 0; i < d; i++)
		{
			unsigned char *lump_start = &raw[i*w*h*ll];

			for( int j = 0; j < h; j++ )
			{
				unsigned char *lump_offset = lump_start;

				for( int k = 0; k < w; k++ )
				{
					unsigned char *dd = lump_offset;
					for( int l = 0; l < ll; l++ )
					{
						/* if the current RLE bit is set to 1, read in a new picture byte */
						if (rlebyte & (1 << rbit--)) picbyte = *picdata++;

						/* write picture byte and move down by one line in the picture */
						*dd = picbyte;
         					dd += w;

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
				lump_start += w * ll;
			}
		}

		printf ("%d,%d,%d\n",w*8,h*ll,1<<d);
		openUnpackedScreen( screen, w, h*ll, d, r,g ,b,raw, ham );
		free(raw);
	}

	return 0;
}

char *_ext_cmd_unpack( struct glueCommands *data, int nextToken )
{
	int n;
	int s;
	unsigned char *adr;
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = stack - data -> stack  +1;

	if (args==2)
	{
		n = getStackNum(stack-1);
		s = getStackNum(stack);

		if ((n>0)&&(n<16))
		{
			adr = (unsigned char *) kittyBanks[n-1].start;

			if (adr)
			{
				convertPacPic( s, adr, "dump" );
			}
			else setError(36,data->tokenBuffer);
		} 
	}

	popStack( stack - data->stack );
	return NULL;
}

char *ext_cmd_unpack(nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_unpack, tokenBuffer );
	return tokenBuffer;
}

