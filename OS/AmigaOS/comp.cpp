
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/retroMode.h>

#include "amoskittens.h"
#include "engine.h"

extern struct Window *My_Window;
extern struct retroEngine *engine ;

extern bool engine_pal_mode;

struct XYSTW_Vertex3D { 
	float x, y; 
	float s, t, w; 
}; 


void BackFill_Func(struct RastPort *ArgRP, struct BackFillArgs *MyArgs);

struct Hook BackFill_Hook =
{
	{NULL, NULL},
	(HOOKFUNC) &BackFill_Func,
	NULL,
	NULL
};


void draw_comp_bitmap(struct BitMap *the_bitmap,struct BitMap *the_bitmap_dest, int width,int height, int wx,int wy,int ww, int wh)
{
	#define STEP(a,xx,yy,ss,tt,ww)   P[a].x= xx; P[a].y= yy; P[a].s= ss; P[a].t= tt; P[a].w= ww;  

	int error;
	struct XYSTW_Vertex3D P[6];

	STEP(0, wx, wy ,0 ,0 ,1);
	STEP(1, wx+ww,wy,width,0,1);
	STEP(2, wx+ww,wy+wh,width,height,1);

	STEP(3, wx,wy, 0,0,1);
	STEP(4, wx+ww,wy+wh,width,height,1);
	STEP(5, wx, wy+wh ,0 ,height ,1);

	if (the_bitmap)
	{
		LockLayer(0, My_Window->RPort->Layer);

		error = CompositeTags(COMPOSITE_Src, 
			the_bitmap, the_bitmap_dest,

			COMPTAG_VertexArray, P, 
			COMPTAG_VertexFormat,COMPVF_STW0_Present,
		    	COMPTAG_NumTriangles,2,

			COMPTAG_SrcAlpha, (uint32) (0x0010000 ),
			COMPTAG_Flags, COMPFLAG_SrcAlphaOverride | COMPFLAG_HardwareOnly | COMPFLAG_SrcFilter ,
			TAG_DONE);

		UnlockLayer(My_Window->RPort->Layer);
	}
}


void BackFill_Func(struct RastPort *ArgRP, struct BackFillArgs *MyArgs)
{
	struct BitMap *bitmap;
	int ww;
	int wh;

	ww = My_Window->Width - My_Window->BorderLeft - My_Window->BorderRight;
	wh = My_Window->Height - My_Window->BorderTop - My_Window->BorderBottom;

	bitmap = AllocBitMap( ww , wh, 32, BMF_DISPLAYABLE, My_Window ->RPort -> BitMap);
	if (bitmap)
	{
		draw_comp_bitmap(engine->rp.BitMap, bitmap, 
						instance.video -> width, 
						engine_pal_mode ? instance.video -> height : instance.video -> height * 5 / 6 ,
						0,0,ww,wh);

		BltBitMapRastPort(bitmap, 0, 0, My_Window -> RPort,
			My_Window -> BorderLeft,
			My_Window -> BorderTop,
			ww, wh,0xc0);

		FreeBitMap(bitmap);
	}
}


