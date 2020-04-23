
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
#include <proto/asl.h>

extern bool open_engine_window( int window_left, int window_top, int window_width, int window_height );
extern void close_engine_window( );

extern struct Window *My_Window;

struct MsgPort *iconifyPort = NULL;
struct DiskObject *dobj = NULL;
struct AppIcon *appicon;
ULONG iconify_sig;

struct windowclass
{
	struct Window *win;
	ULONG window_left;
	ULONG window_top;
	ULONG window_width;
	ULONG window_height;
};

struct windowclass window_save_state;

void save_window_attr(windowclass *self)
{
	GetWindowAttr( self -> win,  WA_Left, &self -> window_left, sizeof(int *));
	GetWindowAttr( self -> win,  WA_Top, &self -> window_top, sizeof(int *));
	GetWindowAttr( self -> win,  WA_InnerWidth, &self -> window_width, sizeof(int *));
	GetWindowAttr( self -> win,  WA_InnerHeight, &self -> window_height, sizeof(int *));
}


void enable_Iconify()
{
	int n;

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
			}
		}
	}
}

void	disable_Iconify()
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

	open_engine_window(
		window_save_state.window_left,
		window_save_state.window_top,
		window_save_state.window_width,
		window_save_state.window_height);
}


