#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "debug.h"
#include <string>
#include <iostream>
#include <proto/retroMode.h>
#include <proto/datatypes.h>
#include <datatypes/pictureclass.h>

#include "stack.h"
#include "amosKittens.h"
#include "commandsScreens.h"
#include "errors.h"
#include "engine.h"

extern int last_var;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;

int current_screen = 0;
extern struct retroRGB DefaultPalette[256];

char *_gfxScreenOpen( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==5)
	{
		int screen_num = _stackInt( stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			current_screen = screen_num;

			// Kitty ignores colors we don't care, allways 256 colors.

			engine_lock();
			if (screens[screen_num]) retroCloseScreen(&screens[screen_num]);
			screens[screen_num] = retroOpenScreen(_stackInt( stack-3 ),_stackInt( stack-2 ),_stackInt( stack ));

			if (screens[screen_num])
			{
				retroApplyScreen( screens[screen_num], video, 0, 0,
					screens[screen_num] -> realWidth,screens[screen_num]->realHeight );

				set_default_colors( screens[screen_num] );
				retroFlash( screens[screen_num], 3, (char *) "(100,5),(200,5),(300,5),(400,5),(500,5),(600,5)(700,5),(800,5),(900,5),(A00,5),(B00,5),(A00,5),(900,5),(800,5),(700,5),(600,5),(500,5)(400,5),(300,5),(200,5)");

				retroBAR( screens[screen_num], 0,0, screens[screen_num] -> realWidth,screens[screen_num]->realHeight, 1 );
				draw_cursor(screens[0]);
			}

			engine_unlock();

			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxScreenClose( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = _stackInt( stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			engine_lock();
			if (screens[screen_num]) retroCloseScreen(&screens[screen_num]);
			engine_unlock();

			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxScreenClone( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = _stackInt( stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (screens[current_screen])	// check if current screen is open.
			{
				engine_lock();
				if (screens[screen_num]) retroCloseScreen(&screens[screen_num]);

				screens[screen_num] = retroScreenClone(screens[current_screen], screens[current_screen] -> videomode );

				if (screens[screen_num])
				{
					set_default_colors( screens[screen_num] );

					retroApplyScreen( screens[screen_num], video, 0, 100, screens[screen_num]->displayWidth, screens[screen_num]->displayHeight );
					video -> refreshAllScanlines = TRUE;
				}

				engine_unlock();

				printf("screen clone %d at %04x org screen %d\n",screen_num, screens[screen_num], current_screen);
//				getchar();

				current_screen = screen_num;
				success = true;
			}
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}


char *_gfxScreenDisplay( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==5)
	{
		int screen_num = _stackInt( stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (kittyStack[stack-3].type ==  type_int) screens[screen_num] -> scanline_x = _stackInt( stack-3 );
			if (kittyStack[stack-2].type ==  type_int) screens[screen_num] -> scanline_y = _stackInt( stack-2 );
			if (kittyStack[stack-1].type ==  type_int) screens[screen_num] -> displayWidth = _stackInt( stack-1 );
			if (kittyStack[stack].type ==  type_int) screens[screen_num] -> displayHeight = _stackInt( stack );

			engine_lock();

			if (screens[screen_num])
				retroApplyScreen( screens[screen_num], video, 
					screens[screen_num] -> scanline_x,
					screens[screen_num] -> scanline_y,
					screens[screen_num] -> displayWidth,
					screens[screen_num] -> displayHeight );

			video -> refreshAllScanlines = TRUE;

			engine_unlock();

			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_gfxScreenOffset( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==3)
	{
		int screen_num = _stackInt( stack-2 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (kittyStack[stack-1].type ==  type_int) screens[screen_num] -> offset_x = _stackInt( stack-1 );
			if (kittyStack[stack].type ==  type_int) screens[screen_num] -> offset_y = _stackInt( stack );
 			screens[screen_num] -> refreshScanlines = TRUE;
			video -> refreshSomeScanlines = TRUE;
			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_gfxScreen( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = _stackInt( stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			current_screen = screen_num;
			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxScin( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = -1;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==2)
	{
		int mx = _stackInt( stack-1 );
		int my = _stackInt( stack );

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
			printf("ret: %d\n",ret);
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
	setStackNum(retroLowres);
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

char *gfxScreenClose(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenClose, tokenBuffer );
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
	return tokenBuffer;
}

char *gfxScin(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxScin, tokenBuffer );
	return tokenBuffer;
}

char *_gfxScreenToFront( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = _stackInt( stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			retroScreenToFront(screens[screen_num]);
			video -> refreshAllScanlines = TRUE;
			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxScreenToBack( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = _stackInt( stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			retroScreenToBack(screens[screen_num]);
			video -> refreshAllScanlines = TRUE;
			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *gfxScreenToFront(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _gfxScreenToFront, tokenBuffer );
	return tokenBuffer;
}

char *gfxScreenToBack(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _gfxScreenToBack, tokenBuffer );
	return tokenBuffer;
}

char *_gfxScreenShow( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = _stackInt( stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			printf("screen_num %d\n",screen_num);

			if (screens[screen_num]) screens[screen_num]->flags &= ~retroscreen_flag_hide;
			video -> refreshAllScanlines = TRUE;
			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxScreenHide( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = _stackInt( stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			printf("screen_num %d\n",screen_num);

			if (screens[screen_num]) screens[screen_num]->flags |= retroscreen_flag_hide;
			video -> refreshAllScanlines = TRUE;
			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	return NULL;
}

char *_gfxScreenCopy( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			{
				int src_screen = _stackInt( stack-1 );
				int dest_screen = _stackInt( stack );

				if ((src_screen>-1)&&(src_screen<8)&&(dest_screen>-1)&&(dest_screen<8))
				{
					retroScreenBlit( screens[src_screen], 0, 0, screens[src_screen]->realWidth, screens[src_screen]->realHeight,
							screens[dest_screen],0, 0);
				}
			}
			break;

		case 8:	// Screen Copy 1,x,y,w,h to 2,x,y
			{
				int src_screen = _stackInt( stack-7 );
				int src_x = _stackInt( stack-6 );
				int src_y = _stackInt( stack-5 );
				int src_w = _stackInt( stack-4 );
				int src_h = _stackInt( stack-3 );
				int dest_screen = _stackInt( stack-2 );
				int dest_x = _stackInt( stack-1 );
				int dest_y = _stackInt( stack );

				if ((src_screen>-1)&&(src_screen<8)&&(dest_screen>-1)&&(dest_screen<8))
				{
					retroScreenBlit( screens[src_screen], src_x, src_y, src_w, src_h,
							screens[dest_screen],dest_x, dest_y);
				}
			}
			break;

		default:
 			setError(22);
			break;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *gfxScreenShow(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _gfxScreenShow, tokenBuffer );
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

void LoadIff( char *name, int n )
{
	struct DataType *dto = NULL;
	struct BitMapHeader *bm_header;
	struct BitMap *dt_bitmap;
		
	if(dto = (struct DataType *) NewDTObject( name, DTA_GroupID, GID_PICTURE, TAG_DONE))
	{
		SetDTAttrs ( (Object*) dto, NULL,NULL,	PDTA_DestMode, (ULONG) PMODE_V43,TAG_DONE);
		DoDTMethod ( (Object*) dto,NULL,NULL,DTM_PROCLAYOUT,NULL,TRUE); 
		GetDTAttrs ( (Object*) dto,PDTA_BitMapHeader, (ULONG *) &bm_header, 	PDTA_BitMap, (ULONG) &dt_bitmap, TAG_DONE);

		printf("Wdith %d, Height %d, Depth %d\n",bm_header -> bmh_Width,bm_header -> bmh_Height,bm_header -> bmh_Depth);

		DisposeDTObject((Object*) dto);
	}
}

char *_gfxLoadIff( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	// load iff image to current screen.
				{
					char *name= _stackString( stack );
					if (name)	LoadIff(name,2);
				}
				break;
		case 2:	// load iff image to new screen.
				{
					char *name= _stackString( stack -1);
					int screen_num = _stackInt( stack );
					if (name)	LoadIff(name,screen_num);
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

