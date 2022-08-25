
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <vector>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <proto/keymap.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/gadtools.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/diskfont.h>
#include <diskfont/diskfonttag.h>
#include <workbench/startup.h>

#include "common_screen.h"

struct MsgPort *iconifyPort = NULL;
struct DiskObject *dobj = NULL;
struct AppIcon *appicon;
ULONG iconify_sig;


void save_window_attr(windowclass *self)
{
	GetWindowAttr( self -> win,  WA_Left, &self -> window_left, sizeof(int *));
	GetWindowAttr( self -> win,  WA_Top, &self -> window_top, sizeof(int *));
	GetWindowAttr( self -> win,  WA_InnerWidth, &self -> window_width, sizeof(int *));
	GetWindowAttr( self -> win,  WA_InnerHeight, &self -> window_height, sizeof(int *));
}

extern  struct Screen *fullscreen_screen;

static ULONG had_ModeID;
static bool had_fullscreen = false;


void enable_Iconify( )
{
	int n;

	had_fullscreen = fullscreen_screen ? true : false;

	const char *files[]={
		"progdir:AmosKittens.exe",
		"s:amos",
		NULL};

	for (n=0;files[n];n++)
	{
		Printf("%s\n", files[n] );
		dobj = GetDiskObject( files[n] );
		if (dobj) break;
	}

	if (dobj)
	{
		iconifyPort = (struct MsgPort *) AllocSysObject(ASOT_PORT,NULL);

		if (iconifyPort)
		{
			iconify_sig = 1L<<iconifyPort -> mp_SigBit;

			appicon = AddAppIcon(1, 0, "Amos Kittens", iconifyPort, 0, dobj, 
					WBAPPICONA_SupportsOpen, TRUE,
					TAG_END);

			if (appicon) 
			{
				window_save_state.win = My_Window;
				save_window_attr(&window_save_state);
				close_engine_window();

				if (fullscreen_screen) 
				{
					had_ModeID = GetVPModeID(&fullscreen_screen->ViewPort);
					CloseScreen( fullscreen_screen);
				}
				fullscreen_screen = NULL;
			}
		}
	}
}

void dispose_Iconify()
{
	if (dobj)
	{
		RemoveAppIcon( appicon );
		FreeDiskObject(dobj);
		appicon = NULL;
		dobj = NULL;
	}

	if (iconifyPort)
	{
		FreeSysObject ( ASOT_PORT, iconifyPort ); 
		iconifyPort = NULL;
		iconify_sig  = 0;
	}
}

void	disable_Iconify()
{
	dispose_Iconify();

	if ((had_fullscreen)&&(fullscreen_screen == NULL))	// open screen if its not open.
	{
		open_fullscreen(had_ModeID);
	}

	printf("Open window %d,%d,%d,%d\n",
		window_save_state.window_left,
		window_save_state.window_top,
		window_save_state.window_width,
		window_save_state.window_height);


	open_engine_window(
		window_save_state.window_left,
		window_save_state.window_top,
		window_save_state.window_width,
		window_save_state.window_height);

}


