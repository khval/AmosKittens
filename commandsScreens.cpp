#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#include <proto/datatypes.h>
#include <datatypes/pictureclass.h>
#endif

#ifdef __linux__
#include <stdint.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#endif

#include "debug.h"
#include <string>
#include <iostream>
#include "stack.h"
#include "amosKittens.h"
#include "commandsScreens.h"
#include "commandsBlitterObject.h"
#include "errors.h"
#include "engine.h"

extern int last_var;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;

int current_screen = 0;
extern struct retroRGB DefaultPalette[256];

extern struct retroTextWindow *newTextWindow( struct retroScreen *screen, int id );
extern void freeAllTextWindows(struct retroScreen *screen);

void init_amos_kittens_screen_default_text_window( struct retroScreen *screen, int colors )
{
	struct retroTextWindow *textWindow = NULL;

	if (colors == 2)
	{
		screen->pen = 1;
		screen->paper = 0;
		screen->ink0 = 1;
		screen->ink1= 0;
		screen->ink2= 0;
	}
	else
	{
		screen->pen = 2;
		screen->paper = 1;
		screen->ink0 = 2;
		screen->ink1= 0;
		screen->ink2= 0;
	}

	if (textWindow = newTextWindow( screen, 0 ))
	{
		textWindow -> charsPerRow = screen -> realWidth / 8;
		textWindow -> rows = screen -> realHeight / 8;
		screen -> currentTextWindow = textWindow;
	}
}

void init_amos_kittens_screen_default_colors(struct retroScreen *screen)
{
	set_default_colors( screen );
	retroFlash( screen, 3, (char *) "(100,5),(200,5),(300,5),(400,5),(500,5),(600,5)(700,5),(800,5),(900,5),(A00,5),(B00,5),(A00,5),(900,5),(800,5),(700,5),(600,5),(500,5)(400,5),(300,5),(200,5)");
	retroBAR( screen, 0,0, screen -> realWidth,screen->realHeight, screen -> paper );
}

char *_gfxScreenOpen( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int colors = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==5)
	{
		int screen_num = getStackNum( stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			struct retroScreen *screen;
			current_screen = screen_num;

			// Kitty ignores colors we don't care, allways 256 colors.

			colors = getStackNum( stack-1 );

			engine_lock();
			if (screens[screen_num]) retroCloseScreen(&screens[screen_num]);
			screens[screen_num] = retroOpenScreen(getStackNum( stack-3 ),getStackNum( stack-2 ),getStackNum( stack ));
	
			if (screen = screens[screen_num])
			{
				init_amos_kittens_screen_default_text_window(screen, colors);
				init_amos_kittens_screen_default_colors(screen);
				draw_cursor(screen);
			}

			retroApplyScreen( screen, video, 0, 0, screen -> realWidth,screen->realHeight );
			engine_unlock();

			success = true;
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

// make sure there is standard way to close screens, that take care of clean up.

bool kitten_screen_close(int screen_num)
{
	if ((screen_num>-1)&&(screen_num<8))
	{
		engine_lock();
		freeAllTextWindows( screens[screen_num]  );
		freeScreenBobs( screen_num );
		if (screens[screen_num]) retroCloseScreen(&screens[screen_num]);

		// find a open screen, and set current screen to that.
		if (screen_num == current_screen)
		{
			int n;
			for (n=8; n>-1;n--)
			{
				if (screens[n])
				{
					current_screen = n;
					break;
				}
			}
		}
		engine_unlock();
		return true;
	}

	return false;
}

char *_gfxScreenClose( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = getStackNum( stack );
		success = kitten_screen_close( screen_num );
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *gfxScreenClose(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenClose, tokenBuffer );
	return tokenBuffer;
}

void copy_pal(	struct retroRGB *source_pal,	struct retroRGB *dest_pal)
{
	int n;
	for (n=0; n<256;n++)
	{
		*dest_pal=*source_pal;
		source_pal++;
		dest_pal++;
	}
}

char *_gfxScreenClone( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = getStackNum( stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (screens[current_screen])	// check if current screen is open.
			{
				engine_lock();
				if (screens[screen_num]) retroCloseScreen(&screens[screen_num]);

				screens[screen_num] = retroScreenClone(screens[current_screen], screens[current_screen] -> videomode );

				if (screens[screen_num])
				{
					screens[screen_num] -> fade_speed = 0;
					screens[screen_num] -> fade_count = 0;

					copy_pal( screens[current_screen] -> orgPalette, screens[screen_num] -> orgPalette );
					copy_pal( screens[current_screen] -> rowPalette, screens[screen_num] -> rowPalette );
					copy_pal( screens[current_screen] -> fadePalette, screens[screen_num] -> fadePalette );

					retroApplyScreen( screens[screen_num], video, 0, 100, screens[screen_num]->displayWidth, screens[screen_num]->displayHeight );
					video -> refreshAllScanlines = TRUE;
				}

				engine_unlock();

				current_screen = screen_num;
				success = true;
			}
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}


char *_gfxScreenDisplay( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==5)
	{
		int screen_num = getStackNum( stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			struct retroScreen *screen; 

			engine_lock();

			if (screen = screens[screen_num])
			{
				if (kittyStack[stack-3].type ==  type_int) screen -> scanline_x = getStackNum( stack-3 );
				if (kittyStack[stack-2].type ==  type_int) screen -> scanline_y = (getStackNum( stack-2 ) *2) - 80;
				if (kittyStack[stack-1].type ==  type_int) screen -> displayWidth = getStackNum( stack-1 );
				if (kittyStack[stack].type ==  type_int) screen -> displayHeight = getStackNum( stack );

				printf( "retroApplyScreen( screen %08x, video %08x, screen -> scanline_x %d, screen -> scanline_y, %d,	screen -> displayWidth %d, screen -> displayHeight  %d );\n",
					screen, video, 
					screen -> scanline_x,
					screen -> scanline_y,
					screen -> displayWidth,
					screen -> displayHeight );

				retroApplyScreen( screen, video, 
					screen -> scanline_x,
					screen -> scanline_y,
					screen -> displayWidth,
					screen -> displayHeight );
			}
			else
			{
				setError(47,data->tokenBuffer);	// Screen not open.
			}

			video -> refreshAllScanlines = TRUE;
			engine_unlock();
			success = true;
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxScreenOffset( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==3)
	{
		int screen_num = getStackNum( stack-2 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (kittyStack[stack-1].type ==  type_int) screens[screen_num] -> offset_x = getStackNum( stack-1 );
			if (kittyStack[stack].type ==  type_int) screens[screen_num] -> offset_y = getStackNum( stack );
 			screens[screen_num] -> refreshScanlines = TRUE;
			video -> refreshSomeScanlines = TRUE;
			success = true;
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_gfxScreen( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = getStackNum( stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			current_screen = screen_num;
			success = true;
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxScin( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = -1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==2)
	{
		int mx = getStackNum( stack-1 );
		int my = getStackNum( stack );

		if ((my>-1)&&(my<480))
		{
			struct retroScreen *s = NULL;
			int n;

			if ( video -> scanlines[my].data)
			{
				s = video -> scanlines[my].screen;
			}
			else if (my>0)
			{
				s = video -> scanlines[my-1].screen;
			}

			if (s)
			{
				for (n=0;n<8;n++)
				{
					if ( screens[n] == s )
					{
						ret =n;
						break;
					}
				}
			}
		}
	}

	popStack( stack - data->stack );
	setStackNum( ret );
	return NULL;
}


char *gfxScreenOpen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenOpen, tokenBuffer );
	return tokenBuffer;
}

char *gfxLowres(struct nativeCommand *cmd, char *tokenBuffer)
{
//	setStackNum(retroLowres); 
	setStackNum(retroLowres_pixeld);
	return tokenBuffer;
}

char *gfxHires(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(retroHires);
	return tokenBuffer;
}

char *gfxScreenDisplay(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenDisplay, tokenBuffer );
	return tokenBuffer;
}



char *gfxScreenOffset(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenOffset, tokenBuffer );
	return tokenBuffer;
}

char *gfxScreen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreen, tokenBuffer );
	return tokenBuffer;
}

char *gfxScreenClone(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenClone, tokenBuffer );
	return tokenBuffer;
}

char *gfxScreenColour(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(256);
	return tokenBuffer;
}

char *gfxScreenWidth(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (screens[current_screen])	// check if current screen is open.
	{
		setStackNum(screens[current_screen] -> realWidth);
	}
	return tokenBuffer;
}

char *gfxScreenHeight(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (screens[current_screen])	// check if current screen is open.
	{
		setStackNum(screens[current_screen] -> realHeight);
	}
	return tokenBuffer;
}

char *gfxGetScreen(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (screens[current_screen])	// check if current screen is open.
	{
		setStackNum(current_screen);
	}
	else 	setStackNum(-1);

	return tokenBuffer;
}

char *gfxScin(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxScin, tokenBuffer );
	return tokenBuffer;
}

char *_gfxScreenToFront( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = getStackNum( stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			retroScreenToFront(screens[screen_num]);
			video -> refreshAllScanlines = TRUE;
			success = true;
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxScreenToBack( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = 0;

		if (kittyStack[stack].type != type_none)
		{
			screen_num = getStackNum( stack );
		}
		else screen_num = current_screen;

		if ((screen_num>-1)&&(screen_num<8))
		{
			retroScreenToBack(screens[screen_num]);
			video -> refreshAllScanlines = TRUE;
			success = true;
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *gfxScreenToFront(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _gfxScreenToFront, tokenBuffer );
	return tokenBuffer;
}

char *gfxScreenToBack(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _gfxScreenToBack, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_gfxScreenShow( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		if (kittyStack[stack].type == type_none)
		{
			if (screens[current_screen]) screens[current_screen]->flags &= ~retroscreen_flag_hide;
			video -> refreshAllScanlines = TRUE;
			success = true;
		}
		else
		{
			int screen_num = getStackNum( stack );

			if ((screen_num>-1)&&(screen_num<8))
			{
				if (screens[screen_num]) screens[screen_num]->flags &= ~retroscreen_flag_hide;
				video -> refreshAllScanlines = TRUE;
				success = true;
			}
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxScreenHide( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = getStackNum( stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			printf("screen_num %d\n",screen_num);

			if (screens[screen_num]) screens[screen_num]->flags |= retroscreen_flag_hide;
			video -> refreshAllScanlines = TRUE;
			success = true;
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxScreenCopy( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			{
				int src_screen = getStackNum( stack-1 );
				int dest_screen = getStackNum( stack );
				unsigned int src_mode = ((src_screen & 0xFFFFFF00) == 0xC0000000) ? 1 : 0;
				unsigned int dest_mode = ((dest_screen & 0xFFFFFF00) == 0xC0000000) ? 1 : 0;

				src_screen &=0xFF;
				dest_screen &= 0xFF;

				if ((src_screen>-1)&&(src_screen<8)&&(dest_screen>-1)&&(dest_screen<8))
				{
					retroScreenBlit( screens[src_screen],src_mode, 0, 0, screens[src_screen]->realWidth, screens[src_screen]->realHeight,
							screens[dest_screen],dest_mode, 0, 0);
				}
			}
			break;

		case 8:	// Screen Copy 1,x0,y0,x1,y1 to 2,x,y
			{
				int src_screen = getStackNum( stack-7 );
				int src_x0 = getStackNum( stack-6 );
				int src_y0 = getStackNum( stack-5 );
				int src_x1 = getStackNum( stack-4 );
				int src_y1 = getStackNum( stack-3 );
				int dest_screen = getStackNum( stack-2 );
				int dest_x = getStackNum( stack-1 );
				int dest_y = getStackNum( stack );
				unsigned int src_mode = ((src_screen & 0xFFFFFF00) == 0xC0000000) ? 1 : 0;
				unsigned int dest_mode = ((dest_screen & 0xFFFFFF00) == 0xC0000000) ? 1 : 0;

				src_screen &=0xFF;
				dest_screen &= 0xFF;

				if ((src_screen>-1)&&(src_screen<8)&&(dest_screen>-1)&&(dest_screen<8))
				{
					retroScreenBlit( screens[src_screen], src_mode ,src_x0, src_y0, src_x1-src_x0, src_y1-src_y0,
							screens[dest_screen], dest_mode, dest_x, dest_y);
				}
			}
			break;

		default:
 			setError(22,data->tokenBuffer);
			break;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxScreenShow(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _gfxScreenShow, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *gfxScreenHide(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _gfxScreenHide, tokenBuffer );
	return tokenBuffer;
}

char *gfxScreenCopy(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenCopy, tokenBuffer );
	return tokenBuffer;
}

void LoadIff( char *name, const int n )
{
#if 0
	struct DataType *dto = NULL;
	struct BitMapHeader *bm_header;
	struct BitMap *dt_bitmap;
	struct ColorRegister *cr;
	ULONG modeid; 
	ULONG colors;		

	if(dto = (struct DataType *) NewDTObject( name, DTA_GroupID, GID_PICTURE, TAG_DONE))
	{
		SetDTAttrs ( (Object*) dto, NULL,NULL,	PDTA_DestMode, (ULONG) PMODE_V43,TAG_DONE);
		DoDTMethod ( (Object*) dto,NULL,NULL,DTM_PROCLAYOUT,NULL,TRUE); 
		GetDTAttrs ( (Object*) dto,PDTA_BitMapHeader, (ULONG *) &bm_header, 	
					PDTA_BitMap, (ULONG) &dt_bitmap, 
					PDTA_ColorRegisters, &cr,
					PDTA_NumColors, &colors,
					PDTA_ModeID, &modeid,
					TAG_DONE);

		engine_lock();

		if (screens[n]) 	kitten_screen_close( n );
		screens[n] = retroOpenScreen(bm_header -> bmh_Width,bm_header -> bmh_Height,1);

		if (screens[n])
		{
			struct RastPort rp;
			int x,y,c;

			init_amos_kittens_screen_default_text_window(screens[n], 256);

			retroApplyScreen( screens[n], video, 0, 20, screens[n] -> realWidth,screens[n]->realHeight );
			retroBAR( screens[n], 0,0, screens[n] -> realWidth,screens[n]->realHeight, screens[n] -> paper );
			set_default_colors( screens[n] );

			current_screen = n;

			if (cr)
			{
				for (c=0;c<colors;c++)		
				{
					retroScreenColor(screens[n],c,cr[c].red,cr[c].green,cr[c].blue);
				}
			}

			InitRastPort(&rp);
			rp.BitMap = dt_bitmap;

			for (y=0;y<screens[n]->realHeight;y++)
			{
				for (x=0;x<screens[n]->realWidth;x++)
				{
					retroPixel( screens[n], x,y, ReadPixel(&rp,x,y));
				}
			}
		}

		DisposeDTObject((Object*) dto);
		engine_unlock();
	}

#endif
}

char *_gfxLoadIff( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	// load iff image to current screen.
				{
					char *name= getStackString( stack );
					if (name)	LoadIff(name,current_screen);
				}
				break;
		case 2:	// load iff image to new screen.
				{
					char *name= getStackString( stack -1);
					int screen_num = getStackNum( stack );
					if (name)	LoadIff(name,screen_num);
				}
				break;
	}

	popStack( stack - data->stack );
	return NULL;
}

void SaveIff( char *name, const int n )
{

#if 0

	struct DataType *dto = NULL;
	struct BitMapHeader *bm_header;
	struct BitMap *dt_bitmap;
	struct ColorRegister *cr;
	ULONG modeid; 
	struct RastPort rp;

	printf("we try to create a datatype\n");

	dt_bitmap = AllocBitMap(screens[n] -> realWidth,screens[n]-> realHeight,8,BMF_CLEAR, NULL);

	if (dt_bitmap)
	{
		int x,y;
		dto = (struct DataType *) NewDTObject( NULL, 
				DTA_SourceType, DTST_RAM,
				DTA_GroupID,GID_PICTURE,
				PDTA_BitMap, dt_bitmap,
				PDTA_NumColors, 256, 
				TAG_DONE);

		InitRastPort(&rp);
		rp.BitMap = dt_bitmap;
		
		for (y=0;y<screens[n]-> realHeight;y++)
		for (x=0;x<screens[n] -> realWidth;x++)
		{
			SetAPen(&rp, retroPoint(screens[n],x,y));
			WritePixel(&rp,x,y);
		}
	}

	if ((dt_bitmap)&&(dto))
	{
		GetDTAttrs( (Object *) dto, PDTA_BitMapHeader, &bm_header, PDTA_ColorRegisters, &cr,	TAG_END);

		if (cr)
		{
			int colors = 256;
			int c;
			struct retroRGB *pal = screens[n]->orgPalette;

			for (c=0;c<colors;c++)		
			{
				cr[c].red = pal[c].r ;
				cr[c].green = pal[c].g ;
				cr[c].blue = pal[c].b ;
			}
		}

		if (bm_header)
		{
			bm_header -> bmh_Width = 320;
			bm_header -> bmh_Height = 200;
			bm_header -> bmh_Depth = 8;
			bm_header -> bmh_XAspect = 22;
			bm_header -> bmh_YAspect = 22;
		}

		SaveDTObject( (Object*) dto, NULL,NULL, name, FALSE, TAG_END );
	}

	if (dto)	// if datatype object is created bitmap is attached to it, and will be freed here.
	{
		 DisposeDTObject((Object*) dto);
	}
	else 	if (dt_bitmap) FreeBitMap(dt_bitmap);

#endif

}


char *_gfxSaveIff( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	// save iff image from current screen.
				{
					char *name= getStackString( stack );
					if (name)	SaveIff(name,current_screen);
				}
				break;
		case 2:	// save iff image from screen X.
				{
					char *name= getStackString( stack -1);
					int screen_num = getStackNum( stack );
					if (name)	SaveIff(name,screen_num);
				}
				break;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxLoadIff(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxLoadIff, tokenBuffer );
	return tokenBuffer;
}

char *gfxSaveIff(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxSaveIff, tokenBuffer );
	return tokenBuffer;
}

char *gfxDoubleBuffer(struct nativeCommand *cmd, char *tokenBuffer)
{
	struct retroScreen *screen = screens[current_screen];

	if (screen)
	{
		freeScreenBobs( current_screen );

		engine_lock();
		retroAllocDoubleBuffer( screen );
		video -> refreshAllScanlines = TRUE;
		engine_unlock();
	}

	return tokenBuffer;
}

char *_gfxScreenSwap( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int screen_num;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:

			if (kittyStack[stack].type == type_int)
			{
				screen_num = kittyStack[stack].value;
			}
			else screen_num = current_screen;

			if (screen = screens[screen_num]) 
			{
				engine_lock();
				if (screen -> Memory[1])		// have buffer2
				{
					screen -> double_buffer_draw_frame = 1 - screen -> double_buffer_draw_frame ;
				}
				engine_unlock();
			}
			break;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxScreenSwap(struct nativeCommand *cmd, char *tokenBuffer)
{
//	struct retroScreen *screen = screens[current_screen];

	stackCmdNormal( _gfxScreenSwap, tokenBuffer );
	setStackNone();

	return tokenBuffer;
}

char *gfxDefault(struct nativeCommand *cmd, char *tokenBuffer)
{
	int n,screen_num;
	struct retroScreen *screen;

	engine_lock();

	for (n=0; n<8;n++)
	{
		if (screens[n]) retroCloseScreen(&screens[n]);
		screens[n] = 0;
	}

	current_screen = 0;
	screen_num = 0;
	screens[0] = retroOpenScreen(320,200,retroLowres);

	if (screen = screens[screen_num])
	{
		screen -> double_buffer_draw_frame = 0;

		init_amos_kittens_screen_default_text_window(screen, 256);
		init_amos_kittens_screen_default_colors(screen);
		draw_cursor(screen);
	}
	retroApplyScreen( screen, video, 0, 0, screen -> realWidth,screen->realHeight );

	engine_unlock();

	return tokenBuffer;
}

char *_gfxXScreen( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int s,x = 0;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			x = getStackNum( stack );
			if (screen = screens[current_screen]) x = XScreen_formula( screen, x );
			break;
		case 2:
			s = getStackNum( stack -1 );			
			x = getStackNum( stack );
			if (screen = screens[s]) x = XScreen_formula( screen, x );
			break;
	}

	popStack( stack - data->stack );
	setStackNum(x);
	return NULL;
}

char *gfxXScreen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxXScreen, tokenBuffer );
	return tokenBuffer;
}

char *_gfxYScreen( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int s,y = 0;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			y = getStackNum( stack );
			if (screen = screens[current_screen]) y = YScreen_formula( screen,y );
			break;
		case 2:
			s = getStackNum( stack -1 );			
			y = getStackNum( stack );
			if (screen = screens[s]) y = YScreen_formula( screen,y );
			break;
	}

	popStack( stack - data->stack );
	setStackNum(y);
	return NULL;
}

char *gfxYScreen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxYScreen, tokenBuffer );
	return tokenBuffer;
}

char *_gfxXHard( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int s=0,x = 0;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			x = getStackNum( stack );
			if (screen = screens[current_screen]) x = XHard_formula( screen, x );
			break;
		case 2:
			s = getStackNum( stack -1 );			
			x = getStackNum( stack );
			if (screen = screens[s]) x = XHard_formula( screen, x );
			break;
	}

	popStack( stack - data->stack );
	setStackNum(x);
	return NULL;
}

char *gfxXHard(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxXHard, tokenBuffer );
	return tokenBuffer;
}

char *_gfxYHard( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int s=0,y = 0;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			y = getStackNum( stack );
			if (screen = screens[current_screen]) y = YHard_formula( screen,y );
			break;
		case 2:
			s = getStackNum( stack -1 );			
			y = getStackNum( stack );
			if (screen = screens[s]) y = YHard_formula( screen,y );
			break;
	}

	popStack( stack - data->stack );
	setStackNum(y);
	return NULL;
}

char *gfxYHard(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxYHard, tokenBuffer );
	return tokenBuffer;
}

char *_gfxScreenMode( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
	}
	else setError(22,data->tokenBuffer);

	if (screens[current_screen])
	{
		ret = screens[current_screen]->videomode;
	}

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *gfxScreenMode(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdParm( _gfxScreenMode, tokenBuffer );
	return tokenBuffer;
}



