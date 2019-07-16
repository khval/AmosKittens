

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <vector>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#include <proto/datatypes.h>
#include <datatypes/pictureclass.h>
#endif

#ifdef __linux__
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#endif

#include "amosKittens.h"

#define index(x,y) (w * (y) + (x))


struct sortColor
{
	unsigned short RGB;
	unsigned int used;
	unsigned int uniqueness;
};

std::vector<struct sortColor> sortColors;

bool CheckAndCountColor(unsigned short RGB)
{
	unsigned int n;
	for (n=0;n<sortColors.size();n++)
	{
		if (sortColors[n].RGB == RGB) 
		{
			sortColors[n].used ++;
			return true;
		}
	}
	return false;
}



void get_most_used_colors(struct RastPort *rp, int w, int h, struct retroScreen *screen)
{
	unsigned int argb;
	unsigned short rgb;
	int x,y;
	unsigned int n,nn;
	bool sorted;
	struct sortColor color;
	int r,g,b;
	int rr,gg,bb;
	
	sortColors.clear();

	for (y=0;y<h;y++)
	{
		for (x=0;x<w;x++) 
		{
			argb = ReadPixelColor(rp,x,y);
			rgb = (unsigned short) ((argb & 0x00F00000) >> 12)  || ((argb & 0x00F000) >> 8 ) || ((argb & 0x0000F0) >> 4 );

			if (CheckAndCountColor(rgb) == false)
			{
				color.RGB = rgb;
				color.used = 0;
				color.uniqueness = 0;
				sortColors.push_back(color);
			}
		}
	}

	for (n=0;n<sortColors.size()-1;n++)
	{
		r = sortColors[n].RGB >> 8;
		g = (sortColors[n].RGB & 0xF0) >> 4;
		b = (sortColors[n].RGB & 0xF) ;


		for (nn=0;nn<sortColors.size()-1;nn++)
		{
			rr = sortColors[nn].RGB >> 8;
			gg = (sortColors[nn].RGB & 0xF0) >> 4;
			bb = (sortColors[nn].RGB & 0xF) ;

			sortColors[n].uniqueness = abs(r - rr)+abs(g-gg)+abs(b-bb);
		}
	}

	do
	{
		sorted = false;
		for (n=0;n<sortColors.size()-1;n++)
		{
			if (sortColors[n].uniqueness < sortColors[n+1].uniqueness)
			{
//				if (sortColors[n].used < sortColors[n+1].used)
				{
					sorted = true;
					color = sortColors[n];
					sortColors[n] = sortColors[n+1];
					sortColors[n+1] = color;
				}
			}
		}
	}
	while (sorted);

	for (n=0;n<sortColors.size();n++)
	{
		rgb = sortColors[n].RGB;
		retroScreenColor(screen,n,(rgb >> 8) * 0x11, ((rgb & 0xF0) >> 4) * 0x11, (rgb & 0xF) * 0x11 );
		if (n==255) break;
	}

	sortColors.clear();
}

int findBestColor(
		struct retroScreen *screen, 
		short r, short g, short b ,
		double *dr,double *dg,double *db )
{
	int found = 0;



	if (screen)
	{
		struct retroRGB *rgb = screen -> orgPalette;
		int _dr,_dg,_db;
		int n, d, bestd = 256 * 3;

		for (n=0;n<256;n++)
		{
			_dr =  abs((int) rgb[n].r - (int) r);
			_dg = abs((int) rgb[n].g - (int) g);
			_db = abs((int) rgb[n].b - (int) b);

			d = _dr+_dg+_db;

			if (d<bestd)
			{
				bestd = d;
				found = n;
			}
		}

		*dr = (double) (rgb[found].r);
		*dg = (double) (rgb[found].g);
		*db = (double) (rgb[found].b);
	}
	else
	{
		*dr = r;
		*dg = g;
		*db = b;
	}

	return found;
}

#define pixel(x,y) array[ w*(y)+(x) ]

void qerror( double *array, double qerror, int x, int y, int w, int h )
{
	if (qerror < -255.0) qerror = -255.0;
	if (qerror > 255.0) qerror = 255.0;

	if (x<w-1) pixel( x+1, y ) = pixel( x+1 , y ) + qerror * 7.0 / 16.0;

	if (y<h-1)
	{
		pixel( x, y +1 ) = pixel( x, y + 1 ) + qerror * 5.0 / 16.0;
		if (x>0)	pixel( x-1	, y +1 ) = pixel( x-1	, y + 1 ) + qerror * 3.0 / 16.0;
		if (x<w-1)	pixel( x+1	, y +1 ) = pixel( x+1	, y + 1 ) + qerror * 1.0 / 16.0;
	}
}

#undef pixel

void ToFloyd(
		struct retroScreen *screen, 
 		double *R,double *G, double *B,
		int w, int h )
{
	int x,y;
	double qerrorR, qerrorG, qerrorB;
	double r,g,b;
	int i;

	for( y=0; y<h;y++)
	{
		for (x = 0; x<w;x++)
		{
			i = index(x,y);

			findBestColor( screen, R[i], G[i], B[i] , &r,&g,&b );

			qerrorR = R[i] - r; 
			qerrorG = G[i] - g; 
			qerrorB = B[i] - b;

			qerror( R, qerrorR, x, y, w, h );
			qerror( G, qerrorG, x, y, w, h );
			qerror( B, qerrorB, x, y, w, h );

			R[i] = r;	
			G[i] = g;	
			B[i] = b;
		}
	}
}


void floyd(struct RastPort *rp, int w, int h, struct retroScreen *screen)
{
	int x,y;
	uint32_t argb;
	double *R = (double*) malloc( sizeof(double) * w* h ); 
	double *G = (double*) malloc( sizeof(double) * w* h ); 
	double *B = (double*) malloc( sizeof(double) * w* h ); 
	int c,i;
	double r,g,b;	

	if ( (R) && (G) && (B) )
	{
		printf("RGB to buffer\n");

		for (y=0;y<h;y++)
		{
			for (x=0;x<w;x++) 
			{
				argb = ReadPixelColor(rp,x,y);
				i = y*w+x;
				R[i] = (double) ((argb & 0xFF0000) >> 16);
				G[i] = (double) ((argb & 0xFF00) >> 8);
				B[i] = (double) (argb & 0xFF) ;
			}
		}

		printf("to floyd\n");

		ToFloyd(screen, R,G,B,w,h );

		printf("RGB to colors\n");

		for (y=0;y<h;y++)
		{
			for (x=0;x<w;x++) 
			{
				i = index(x,y);
				c = findBestColor( screen, (int) R[i], (int) G[i], (int) B[i] , &r,&g,&b );
				retroPixel( screen, x, y, c );
			}
		}
	}

	if (R) free(R);
	if (G) free(G);
	if (B) free(B);
}

