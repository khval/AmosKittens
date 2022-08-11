
#define pixel(x,y) image[ w * (y) + (x) ]

#define pixel(x,y,c) c = (int) image_ ## c[w*y+x]  ; if (c<0) c=0; if (c>255) c=255; c = c >> 6;

void floydChannel( double *image, int w, int h )
{
	int x,y;
	double newPixel,oldPixel;
	double qerror;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for( y=0; y<h;y++)
	{
		for (x = 0; x<w;x++)
		{
			oldPixel = pixel(x,y);
			newPixel = round( oldPixel / 64.0f ) * 64.0f;
			pixel( x , y ) = newPixel;
			qerror = oldPixel - newPixel;

			if (x<w-1) pixel( x+1, y ) = pixel( x+1 , y ) + qerror * 7.0 / 16;

			if (y<h-1)
			{
				pixel( x, y +1 ) = pixel( x, y + 1 ) + qerror * 5.0 / 16;
				if (x>0)	pixel( x-1	, y +1 ) = pixel( x-1	, y + 1 ) + qerror * 3.0 / 16;
				if (x<w-1)	pixel( x+1	, y +1 ) = pixel( x+1	, y + 1 ) + qerror * 1.0 / 16;
			}
		}
	}
}

unsigned findbestColor(
		struct retroScreen *screen, 
		unsigned char r, 
		unsigned char g,
		unsigned char b )
{
	int found = 0;

	if (screen)
	{
		struct retroRGB *rgb = screen -> orgPalette;
		int dr,dg,db;
		int n;
		int d;
		int bestd = 256 * 3;

		for (n=0;n<256;n++)
		{
			dr =  rgb -> r - r;
			dg = rgb -> g - g;
			db = rgb -> b - b;
			d = abs(dr+dg+db);

			if (d<bestd)
			{
				bestd = d;
				found = n;
			}
		}
	}

	return found;
}


void floyd(struct RastPort *rp, int w, int h, struct retroScreen *screen)
{
	int x,y;
	int off;
	uint32_t argb;
	double *image_r = (double*) malloc( sizeof(double) * w* h ); 
	double *image_g = (double*) malloc( sizeof(double) * w* h ); 
	double *image_b = (double*) malloc( sizeof(double) * w* h ); 
	int r,g,b;
	int v;
	

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( (image_r) && (image_g) && (image_b) )
	{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		for (y=0;y<h;y++)
		{
			for (x=0;x<w;x++) 
			{
				argb = ReadPixelColor(rp,x,y);
				off = y*w+x;
				image_r[off] = (double) ((argb & 0xFF0000) >> 16);
				image_g[off] = (double) ((argb & 0xFF00) >> 8);
				image_b[off] = (double) (argb & 0xFF) ;
			}
		}

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		floydChannel( image_r, w, h );
		floydChannel( image_g, w, h );
		floydChannel( image_b, w, h );

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		for (y=0;y<h;y++)
		{
			for (x=0;x<w;x++) 
			{
				pixel(x,y,r);
				pixel(x,y,g);
				pixel(x,y,b);
				retroPixel( screen, x, y,  (r<<4) | (g<<2) | b);
			}
		}
	}

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (image_r) free(image_r);
	if (image_g) free(image_g);
	if (image_b) free(image_b);
}

