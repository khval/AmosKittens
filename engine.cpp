#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/retroMode.h>

#include "engine.h"

extern int sig_main_vbl;
extern bool running;

bool engine_started = false;

APTR engine_mx = 0;

 int engine_mouse_key = 0;
 int engine_mouse_x = 0;
 int engine_mouse_y = 0;

struct retroScreen *screens[8] ;

#define IDCMP_COMMON IDCMP_MOUSEBUTTONS | IDCMP_INACTIVEWINDOW | IDCMP_ACTIVEWINDOW  | \
	IDCMP_CHANGEWINDOW | IDCMP_MOUSEMOVE | IDCMP_REFRESHWINDOW | IDCMP_RAWKEY | \
	IDCMP_EXTENDEDMOUSE | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE | IDCMP_INTUITICKS

struct retroVideo *video = NULL;
struct Window *My_Window = NULL;

struct Library * IntuitionBase = NULL;
struct IntuitionIFace *IIntuition = NULL;

struct Library * GraphicsBase = NULL;
struct GraphicsIFace *IGraphics = NULL;

struct Library * RetroModeBase = NULL;
struct RetroModeIFace *IRetroMode = NULL;

struct Library * LayersBase = NULL;
struct LayersIFace *ILayers = NULL;


extern BOOL open_lib( const char *name, int ver , const char *iname, int iver, struct Library **base, struct Interface **interface);


bool open_window( int window_width, int window_height )
{
		My_Window = OpenWindowTags( NULL,

			WA_Title,         "Amos Kittens",

			WA_InnerWidth,      window_width,
			WA_InnerHeight,     window_height,

			WA_SimpleRefresh,		FALSE,
			WA_CloseGadget,     TRUE,
			WA_DepthGadget,     TRUE,

			WA_DragBar,         TRUE,
			WA_Borderless,      FALSE,
			WA_SizeGadget,      FALSE,
			WA_SizeBBottom,	FALSE,

			WA_IDCMP,           IDCMP_COMMON,
			WA_Flags,           WFLG_REPORTMOUSE,
			TAG_DONE);

	return (My_Window != NULL) ;
}

bool init_engine()
{
	if ( ! open_lib( "intuition.library", 51L , "main", 1, &IntuitionBase, (struct Interface **) &IIntuition  ) ) return FALSE;
	if ( ! open_lib( "graphics.library", 54L , "main", 1, &GraphicsBase, (struct Interface **) &IGraphics  ) ) return FALSE;
	if ( ! open_lib( "layers.library", 54L , "main", 1, &LayersBase, (struct Interface **) &ILayers  ) ) return FALSE;
	if ( ! open_lib( "retromode.library", 1L , "main", 1, &RetroModeBase, (struct Interface **) &IRetroMode  ) ) return FALSE;
	if ( ! open_window(640,480) ) return false;
	if ( (video = retroAllocVideo( My_Window )) == NULL ) return false;

	engine_mx = (APTR) AllocSysObjectTags(ASOT_MUTEX, TAG_DONE);
	if ( ! engine_mx) return FALSE;

	return TRUE;
}

void close_engine()
{
	if (My_Window) CloseWindow(My_Window);

	if (IntuitionBase) CloseLibrary(IntuitionBase); IntuitionBase = 0;
	if (IIntuition) DropInterface((struct Interface*) IIntuition); IIntuition = 0;

	if (LayersBase) CloseLibrary(LayersBase); LayersBase = 0;
	if (ILayers) DropInterface((struct Interface*) ILayers); ILayers = 0;

	if (GraphicsBase) CloseLibrary(GraphicsBase); GraphicsBase = 0;
	if (IGraphics) DropInterface((struct Interface*) IGraphics); IGraphics = 0;

	if (RetroModeBase) CloseLibrary(RetroModeBase); RetroModeBase = 0;
	if (IRetroMode) DropInterface((struct Interface*) IRetroMode); IRetroMode = 0;

	if (engine_mx) FreeSysObject(ASOT_MUTEX, engine_mx); engine_mx = 0;
}

void main_engine();

static struct Process *MainTask = NULL;
static struct Process *gfx_engine = NULL;


bool start_engine()
{
	MainTask = (struct Process *) FindTask(NULL);
	gfx_engine = CreateNewProcTags(
				NP_Name, "Amos kittens graphics engine" ,
				NP_Entry, main_engine, 
				NP_Priority, 0, 
				NP_Child, TRUE,
				TAG_END);

	Wait(SIGF_CHILD);
	return engine_started;
}

void wait_engine()
{
	do
	{
		Delay(1);
	} while (engine_started);
}

void set_default_colors( int screen )
{
	retroScreenColor( screens[screen], 0, 0, 0, 0 );
	retroScreenColor( screens[screen], 1, 255, 100, 50 );
	retroScreenColor( screens[screen], 2, 255, 255, 255 );
	retroScreenColor( screens[screen], 3, 255, 0, 0 );
	retroScreenColor( screens[screen], 4, 0, 255, 0 );

	retroScreenColor( screens[screen], 5, 0, 0, 0 );
	retroScreenColor( screens[screen], 6, 255, 255, 255 );
	retroScreenColor( screens[screen], 7, 0, 0, 0 );
	retroScreenColor( screens[screen], 8, 255, 0, 0 );
}

void main_engine()
{
	struct RastPort scroll_rp;
	struct IntuiMessage *msg;

	retroRGB color;
	double start_sync;

	if (init_engine())		// libs open her.
	{
		int n;
		engine_started = true;
		Signal( &MainTask->pr_Task, SIGF_CHILD );

		retroClearVideo(video);

		screens[0] = retroOpenScreen(320,200,retroLowres);

		if (screens[0])
		{
			set_default_colors( 0 );
			retroBAR( screens[0], 0,0, screens[0] -> realWidth, screens[0] -> realHeight, 1 );
		}

		if (screens[0])	retroApplyScreen( screens[0], video, 0, 0,320,200 );

		while (running)
		{
			while (msg = (IntuiMessage *) GetMsg( video -> window -> UserPort) )
			{
				switch (msg -> Class) 
				{
					case IDCMP_CLOSEWINDOW: 
							running = false; break;

					case IDCMP_MOUSEBUTTONS:
							engine_mouse_key = msg -> Code;
							break;

					case IDCMP_MOUSEMOVE:
							engine_mouse_x = msg -> MouseX;
							engine_mouse_y = msg -> MouseY;
							break;
				}

				ReplyMsg( (Message*) msg );
			}

			engine_lock();
			retroClearVideo( video );
			retroDrawVideo( video );
			retroDmaVideo(video);
			engine_unlock();

			WaitTOF();
			if (sig_main_vbl) Signal( &MainTask->pr_Task, 1<<sig_main_vbl );

			BltBitMapTags(BLITA_SrcType, BLITT_BITMAP,
						BLITA_Source, video->rp.BitMap,
						BLITA_SrcX, 0,
						BLITA_SrcY, 0,
						BLITA_Width,  video -> width, 
						BLITA_Height, video -> height,
						BLITA_DestType,  BLITT_RASTPORT,
						BLITA_Dest, My_Window->RPort,
						BLITA_DestX, My_Window->BorderLeft,
						BLITA_DestY, My_Window->BorderTop,
						TAG_END);
		}
		
		for (n=0; n<8;n++)
		{
			if (screens[n]) retroCloseScreen(&screens[n]);
		}
		retroFreeVideo(video);
	}
	else
	{
		engine_started = false;
		Signal( &MainTask->pr_Task, SIGF_CHILD );
	}

	close_engine();
	engine_started = false;
	if (sig_main_vbl) Signal( &MainTask->pr_Task, 1<<sig_main_vbl );	// signal in case we got stuck in a waitVBL.

}

void engine_lock()
{
	MutexObtain(engine_mx);
}

void engine_unlock()
{
	MutexRelease(engine_mx);
}
