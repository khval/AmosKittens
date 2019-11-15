
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#include <proto/datatypes.h>
#include <datatypes/pictureclass.h>
#include <datatypes/animationclass.h>
#endif

#include "debug.h"
#include <string>
#include <iostream>
#include "stack.h"
#include "amosKittens.h"
#include "commandsScreens.h"
#include "commandsBlitterObject.h"
#include "KittyErrors.h"
#include "engine.h"
#include <math.h>

extern int last_var;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern int current_screen ;

extern void convert_bitmap(int bformat, struct RastPort *rp, struct retroScreen *screen );
extern bool kitten_screen_close(int screen_num );
extern void copy_palette(int bformat, struct ColorRegister *cr ,struct RastPort *rp,  struct retroScreen *screen , ULONG &colors );
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

bool new_frame(struct DataType *dto,ULONG TimeStamp, ULONG bformat, struct RastPort *rp, struct retroScreen *screen, ULONG  &duration)
{
	struct adtNewFormatFrame new_frame;

	memset( &new_frame, 0 , sizeof(struct adtNewFormatFrame) );

	new_frame.MethodID = ADTM_LOADNEWFORMATFRAME;
	new_frame.alf_TimeStamp = TimeStamp;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (DoDTMethodA ( (Object*) dto,NULL,NULL, (Msg) &new_frame))
	{
		rp -> BitMap = new_frame.alf_BitMap;
		duration = new_frame.alf_Duration;

		printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		if (rp -> BitMap)
		{
			printf("time: %-8d duration: %-d\n",TimeStamp,duration);

			// to avoid tering...
			engine_lock();	
			convert_bitmap( bformat, rp, screen );
			engine_unlock();
		}

		printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		new_frame.MethodID = ADTM_UNLOADNEWFORMATFRAME;
		DoDTMethodA ( (Object*) dto,NULL,NULL, (Msg) &new_frame); 

		return TRUE;
	}
	else
	{
		printf("failed\n");
	}

	return FALSE;
}


bool old_frame(struct DataType *dto,ULONG TimeStamp, ULONG bformat, struct RastPort *rp, struct retroScreen *screen, ULONG  &duration )
{
	struct adtFrame frame;

	memset( &frame, 0 , sizeof(struct adtFrame) );

	frame.MethodID = ADTM_LOADFRAME;
	frame.alf_TimeStamp = TimeStamp;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (DoDTMethodA ( (Object*) dto,NULL, NULL, (Msg) &frame))
	{
		rp -> BitMap = frame.alf_BitMap;
		duration = frame.alf_Duration;
		
		printf("TimeStamp: %d\n", frame.alf_TimeStamp);
		printf("%s:%s:%d - frame %d\n",__FILE__,__FUNCTION__,__LINE__, frame.alf_Frame);

		if (rp -> BitMap)
		{
			printf("time: %-8d duration: %-d\n",TimeStamp,duration);

			// to avoid tering...
			engine_lock();	
			convert_bitmap( bformat, rp, screen );
			engine_unlock();
		}
		else
		{
			printf("failed no bitmap\n");
		}

		printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		frame.MethodID = ADTM_UNLOADFRAME;
		DoDTMethodA ( (Object*) dto,NULL,NULL, (Msg) &frame); 

		return TRUE;
	}
	else
	{
		printf("failed\n");
	}

	return FALSE;
}

void IffAnim( char *name, const int sn )
{
	struct DataType *dto = NULL;
	struct BitMapHeader *bm_header;
	struct BitMap *dt_bitmap;
	struct ColorRegister *cr;
	ULONG modeid; 
	ULONG colors;
	ULONG bformat;
	ULONG mode;
	ULONG frames;
	ULONG fps;
	struct dtFrameBox dtf;
	struct FrameInfo fri;
	ULONG time = 0;
	ULONG duration = 0;

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
					ADTA_NumColors, &colors,
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

		bformat = GetBitMapAttr(dt_bitmap,BMA_PIXELFORMAT);

		printf("colors %d\n",colors);
		printf("mode id %08x\n",modeid);
		printf("bformat %d\n",bformat);
		printf("%d,%d\n",bm_header -> bmh_Width,bm_header -> bmh_Height);
	
		switch (modeid)
		{
			case 0x800: 
				mode = retroLowres | retroHam6;
				break;

			default:
				mode = (bm_header -> bmh_Width>=640) ? retroHires : retroLowres;
				mode |= (bm_header -> bmh_Height>256) ? retroInterlaced : 0;
		 }

		if (screens[sn]) 	kitten_screen_close( sn );	// this function locks engine ;-)

		printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		engine_lock();

		screens[sn] = retroOpenScreen(bm_header -> bmh_Width,bm_header -> bmh_Height, mode);

		if (screens[sn])
		{
			ULONG n;
			struct RastPort rp;
			InitRastPort(&rp);

			init_amos_kittens_screen_default_text_window(screens[sn], 256);

			retroApplyScreen( screens[sn], video, 0, 0, screens[sn] -> realWidth,screens[sn]->realHeight );
			retroBAR( screens[sn], 0, 0,0, screens[sn] -> realWidth,screens[sn]->realHeight, screens[sn] -> paper );
			set_default_colors( screens[sn] );
			current_screen = sn;
			engine_unlock();

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

			rp.BitMap = dt_bitmap;
			if (cr) copy_palette( bformat, cr ,&rp, screens[sn] , colors );

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

			time = 0;

			for (n=0; n<frames;n++)
			{
//				try_frame( dto, n, bformat, &rp, screens[sn] );


				if (new_frame( dto, time, bformat, &rp, screens[sn], duration )== FALSE)
				{
					old_frame( dto, time, bformat,  &rp, screens[sn], duration );
				}

				progress(n*10,screens[sn]);
				Delay(20);

				time += 1000;
			}
		}
		else
		{
			engine_unlock();
		}

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		DisposeDTObject((Object*) dto);

	}
}
