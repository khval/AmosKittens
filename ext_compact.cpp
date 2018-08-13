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
static unsigned char *data = NULL;

int get2( int pos ) { 
  int d = int(data[pos])*256 + int(data[pos+1]);
  return d; // negative numbers?!
}

int get4( int pos ) { 
  int d = ( ( int(data[pos])*256 + int(data[pos+1]) )*256 + int(data[pos+2]) ) * 256 + int(data[pos+3]);
  return d; // negative numbers?!
}

void getRGB( int pos, int &r, int &g, int &b ) { // get RGB converted to 0..255
  r = (data[pos] & 0x0F) * 17;
  g = ((data[pos+1] & 0xF0)/16) * 17;
  b = (data[pos+1] & 0x0F) * 17;
}

// pac.pic. RLE decompressor
void convertPacPic( const char *base ) {
  int o = 20;
  bool ham = false;
  if( get4(o) == 0x12031990 ) {
    // detect HAM
    if( get2(o+20) & 0x800 ) {
      ham = true;
      printf("HAM data is not yet supported, output will look garbled!\n");
    }
    // fetch palette
    for( int i=0; i<32; ++i ) { 
      getRGB( o+26+i*2, r[i],g[i],b[i] ); 
      printf( "color %d, (%d,%d,%d)\n", i, r[i],g[i],b[i] );
    }
    o+=90;
  }
  if( get4(o) != 0x06071963 ) {
    printf("could not find picture header!\n");
    exit(1);
  }

  int w  = get2(o+8),
      h  = get2(o+10),
      ll = get2(o+12),
      d  = get2(o+14);
  printf("width: %d bytes\nheight: %d linelumps a %d lines\n", w,h,ll);

  // reserve bitplane memory
  unsigned char* raw = (unsigned char*)calloc(w*h*ll*d,1);
  unsigned char *picdata = &data[o+24];
  unsigned char *rledata = &data[o+get4(o+16)];
  unsigned char *points  = &data[o+get4(o+20)];

  int rrbit = 6, rbit = 7;
  int picbyte = *picdata++;
  int rlebyte = *rledata++;
  if (*points & 0x80) rlebyte = *rledata++;

  for( int i = 0; i < d; i++) {
    unsigned char *lump_start = &raw[i*w*h*ll];
    for( int j = 0; j < h; j++ ) {
      unsigned char *lump_offset = lump_start;
      for( int k = 0; k < w; k++ ) {
        unsigned char *dd = lump_offset;
        for( int l = 0; l < ll; l++ ) {
          /* if the current RLE bit is set to 1, read in a new picture byte */
          if (rlebyte & (1 << rbit--)) picbyte = *picdata++;

          /* write picture byte and move down by one line in the picture */
          *dd = picbyte;
          dd += w;

          /* if we've run out of RLE bits, check the POINTS bits to see if a new RLE byte is needed */
          if (rbit < 0) {
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

  // save png
  // char fname[1000];
  // snprintf( fname, 1000, "%s.png", base );
  // FILE* out = fopen( fname, "wb" );
  // saveBitplanesAsPNG( w,h*ll,d, r,g,b, raw, out, false, ham );
  // fclose(out);
}

char *_ext_cmd_unpack( struct glueCommands *data, int nextToken )
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = stack - data -> stack  +1;

	dump_banks();

	if (args==2)
	{
	}

	getchar();

	popStack( stack - data->stack );
	return NULL;
}

char *ext_cmd_unpack(nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_unpack, tokenBuffer );
	getchar();
	return tokenBuffer;
}

