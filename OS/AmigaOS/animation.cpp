
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <iostream>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#include <proto/datatypes.h>
#include <datatypes/pictureclass.h>
#include <datatypes/animationclass.h>
#endif

#include <amosKittens.h>
#include <stack.h>
#include "debug.h"
#include "commandsScreens.h"
#include "commandsBlitterObject.h"
#include "KittyErrors.h"
#include "engine.h"
#include <math.h>

extern int last_var;

extern void convert_bitmap(int bformat, struct RastPort *rp, struct retroScreen *screen );
extern bool kitten_screen_close(int screen_num );
extern void copy_palette(int bformat, struct ColorRegister *cr ,struct RastPort *rp,  struct retroScreen *screen , ULONG *colors );
extern void init_amos_kittens_screen_default_text_window( struct retroScreen *screen, int colors );

void progress(int xx, struct retroScreen *screen )
{
	int x,y;

	for (y=0;y<10;y++)
	{
		for (x=0;x<xx;x++)
		{
			retroPixel( screen, screen -> Memory[0], x,y, 2);
		}
	}
}

bool try_frame(struct DataType *dto,ULONG frame, ULONG bformat, struct RastPort *rp, struct retroScreen *screen)
{
	struct BitMap *bitmap;
	ULONG frame_read = 0;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (SetDTAttrs ( (Object*) dto, NULL, NULL, ADTA_Frame, frame, TAG_END ))
	{
		printf("set frame number\n");
	}
	else
	{
		printf("can't set frame number\n");
	}

	if (GetDTAttrs ( (Object*) dto,ADTA_KeyFrame, (ULONG) &bitmap, TAG_DONE))
	{

		if (GetDTAttrs ( (Object*) dto, NULL, NULL, ADTA_Frame, &frame_read, TAG_END ))
		{
			printf("Got frame number?\n");
		}
		else
		{
			printf("can't get frame number\n");
		}

		printf("set frame: %-8d frame read %-8d\n",frame,frame_read);

		rp -> BitMap = bitmap;

		engine_lock();	
		convert_bitmap( bformat, rp, screen );
		engine_unlock();

	}

	return TRUE;
}

struct animContext
{
	struct DataType *dto;
	ULONG colors;
 	ULONG bformat;
	struct RastPort rp;
	 struct retroScreen *screen;
	ULONG  duration;
	ULONG *rgb_table;
};

void copy_palette24(struct animContext *context)
{
	if (context -> bformat==PIXF_NONE)
	{
		ULONG c,idx;

		for (c=0;c<context -> colors;c++)		
		{
			idx = c*3;

			retroScreenColor(context -> screen,c,
				context->rgb_table[idx+0] >> 24,
				context->rgb_table[idx+1] >> 24,
				context->rgb_table[idx+2] >> 24);
		}
	}
}

bool new_frame(struct animContext *context,ULONG TimeStamp)
{
	struct adtNewFormatFrame new_frame;

	memset( &new_frame, 0 , sizeof(struct adtNewFormatFrame) );

	new_frame.MethodID = ADTM_LOADNEWFORMATFRAME;
	new_frame.alf_TimeStamp = TimeStamp;
	new_frame.alf_Frame = TimeStamp;
	new_frame.alf_Size = sizeof(struct adtNewFormatFrame); 

	if (DoDTMethodA ( (Object*) context -> dto,NULL,NULL, (Msg) &new_frame))
	{
		context -> rp.BitMap = new_frame.alf_BitMap;
		context -> duration = new_frame.alf_Duration;

		if ((new_frame.alf_CMap)&&(context->rgb_table))
		{
			GetRGB32( new_frame.alf_CMap,  0, context -> colors, context->rgb_table );
			copy_palette24( context );
		}

		if (context -> rp.BitMap)
		{
			printf("time: %-8d duration: %-d\n",TimeStamp,context -> duration);

			// to avoid tering...
			engine_lock();	
			convert_bitmap( context -> bformat, &(context -> rp), context -> screen );
			engine_unlock();
		}

		new_frame.MethodID = ADTM_UNLOADNEWFORMATFRAME;
		DoDTMethodA ( (Object*) context -> dto,NULL,NULL, (Msg) &new_frame); 

		return TRUE;
	}

	return FALSE;
}


bool old_frame(struct animContext *context,ULONG TimeStamp )
{
	struct adtFrame frame;

	memset( &frame, 0 , sizeof(struct adtFrame) );

	frame.MethodID = ADTM_LOADFRAME;
	frame.alf_TimeStamp = TimeStamp;
	frame.alf_Frame = TimeStamp;

	if (DoDTMethodA ( (Object*) context -> dto,NULL, NULL, (Msg) &frame))
	{
		context -> rp.BitMap = frame.alf_BitMap;
		context -> duration = frame.alf_Duration;
		
		if ((frame.alf_CMap)&&(context->rgb_table))
		{
			GetRGB32( frame.alf_CMap,  0, context -> colors, context->rgb_table );
			copy_palette24( context );
		}

		if (context -> rp.BitMap)
		{
			// to avoid tering...
			engine_lock();	
			convert_bitmap( context -> bformat, &(context -> rp), context -> screen );
			engine_unlock();
		}
		else
		{
			printf("failed no bitmap\n");
		}

		frame.MethodID = ADTM_UNLOADFRAME;
		DoDTMethodA ( (Object*) context -> dto,NULL,NULL, (Msg) &frame); 

		return TRUE;
	}
	else
	{
		printf("failed\n");
	}

	return FALSE;
}

extern unsigned int AmigaModeToRetro( unsigned int mode );

extern void __wait_vbl();

void IffAnim( char *name, const int sn )
{
	struct DataType *dto = NULL;
	struct BitMapHeader *bm_header;
	struct BitMap *dt_bitmap;
	struct ColorRegister *cr;
	ULONG modeid; 
	ULONG mode;
	ULONG frames;
	ULONG fps;
	struct dtFrameBox dtf;
	struct FrameInfo fri;
	ULONG time = 0;
	struct animContext context;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("name: %s\n",name);

	if(dto = (struct DataType *) NewDTObject( name, DTA_GroupID, GID_ANIMATION, TAG_DONE))
	{

		printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

//		SetDTAttrs ( (Object*) dto, NULL,NULL,	PDTA_DestMode, (ULONG) PMODE_V43,TAG_DONE);

//		DoDTMethod ( (Object*) dto,NULL,NULL,DTM_PROCLAYOUT,NULL,TRUE); 
//		DoDTMethod ( (Object*) dto,NULL,NULL,GM_LAYOUT,NULL,TRUE); 	// this sucks

		dtf.MethodID = DTM_FRAMEBOX;
		dtf.dtf_FrameInfo = &fri;
		dtf.dtf_ContentsInfo = &fri;
		dtf.dtf_SizeFrameInfo = sizeof( struct FrameInfo );

		if (DoDTMethodA( (Object*) dto, NULL, NULL, (Msg) &dtf ))		// Obtain the display environment that the animation requires.
		{
				printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
		}

		printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		if (GetDTAttrs ( (Object*) dto,PDTA_BitMapHeader, (ULONG *) &bm_header, 	
					ADTA_KeyFrame, (ULONG) &dt_bitmap, 
					ADTA_ColorRegisters, &cr,
					ADTA_NumColors, &context.colors,
					ADTA_ModeID, &modeid,
					ADTA_Frames, &frames,
					ADTA_FramesPerSecond, &fps,
					TAG_DONE))
		{
			printf("got values?\n");
		}

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		GetDTAttrs ( (Object*) dto,ADTA_KeyFrame, (ULONG) &dt_bitmap, TAG_DONE);

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		context.bformat = GetBitMapAttr(dt_bitmap,BMA_PIXELFORMAT);

		printf("fps %d\n",fps);
		printf("colors %d\n",context.colors);
		printf("mode id %08x\n",modeid);
		printf("bformat %d\n",context.bformat);
		printf("%d,%d\n",bm_header -> bmh_Width,bm_header -> bmh_Height);
	
		mode = AmigaModeToRetro( modeid );

		if (instance.screens[sn]) 	kitten_screen_close( sn );	// this function locks engine ;-)

		engine_lock();

		instance.screens[sn] = retroOpenScreen(bm_header -> bmh_Width,bm_header -> bmh_Height, mode);

		if (instance.screens[sn])
		{
			ULONG n;
			InitRastPort(&context.rp);

			context.dto = dto;
			context.screen = instance.screens[sn];
			context.rgb_table = (ULONG *) malloc(sizeof(ULONG) * 3 * 256 );

			init_amos_kittens_screen_default_text_window(instance.screens[sn], 256);

			retroApplyScreen( instance.screens[sn], instance.video, 0, 0, instance.screens[sn] -> realWidth,instance.screens[sn]->realHeight );
			retroBAR( instance.screens[sn], 0, 0,0, instance.screens[sn] -> realWidth,instance.screens[sn]->realHeight, instance.screens[sn] -> paper );
			set_default_colors( instance.screens[sn] );
			instance.current_screen = sn;
			engine_unlock();

			context.rp.BitMap = dt_bitmap;
			if (cr) copy_palette( context.bformat, cr ,&context.rp, instance.screens[sn] , &context.colors );

			time = 0;

			for (n=0; n<frames;n++)
			{
				if (new_frame( &context, time )== FALSE)
				{
					old_frame( &context, time );
				}

//				progress(n*10,instance.screens[sn]);

				__wait_vbl();

				time += 1;
			}

			if (context.rgb_table) free(context.rgb_table);
		}
		else
		{
			engine_unlock();
		}

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		DisposeDTObject((Object*) dto);

	}
}
