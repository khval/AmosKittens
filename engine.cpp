#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>
#include <proto/layers.h>
#include <proto/retroMode.h>

#include "engine.h"
#include "bitmap_font.h"

extern int sig_main_vbl;
extern bool running;			// 
extern bool interpreter_running;	// interprenter is really running.

bool engine_wait_key = false;
bool engine_started = false;
extern bool curs_on;

APTR engine_mx = 0;

 int engine_mouse_key = 0;
 int engine_mouse_x = 0;
 int engine_mouse_y = 0;

struct retroScreen *screens[8] ;
struct retroRGB DefaultPalette[256] = 
{
	{ 0x00, 0x00, 0x00 },
	{ 0xAA, 0x44, 0x00 },
	{ 0xFF, 0xFF, 0xFF },
	{ 0x00, 0x00, 0x00 },
	{ 0xFF, 0x00, 0x00 },
	{ 0x00, 0xFF, 0x00 },
	{ 0x00, 0x00, 0xFF },
	{ 0x66, 0x66, 0x66 },
	{ 0x55, 0x55, 0x55 },
	{ 0x33, 0x33, 0x33 },
	{ 0x77, 0x33, 0x33 },
	{ 0x33, 0x77, 0x33 },
	{ 0x77, 0x77, 0x33 },
	{ 0x33, 0x33, 0x77 },
	{ 0x33, 0x77, 0x77 },

	{ 0x00, 0x00, 0x00 },
	{ 0xEE, 0xCC, 0x88 },
	{ 0xCC, 0x66, 0x00 },
	{ 0xEE, 0xAA, 0x00 },
	{ 0x22, 0x77, 0xFF },
	{ 0x44, 0x99, 0xDD },
	{ 0x55, 0xAA, 0xEE },
	{ 0xAA, 0xDD, 0xFF },
	{ 0xBB, 0xDD, 0xFF },
	{ 0xCC, 0xEE, 0xFF },
	{ 0xFF, 0xFF, 0xFF },
	{ 0x44, 0x00, 0x88 },
	{ 0xAA, 0x00, 0xEE },
	{ 0xEE, 0x00, 0x88 },
	{ 0xEE, 0xEE, 0xEE },

};

#define IDCMP_COMMON IDCMP_MOUSEBUTTONS | IDCMP_INACTIVEWINDOW | IDCMP_ACTIVEWINDOW  | \
	IDCMP_CHANGEWINDOW | IDCMP_MOUSEMOVE | IDCMP_REFRESHWINDOW | IDCMP_RAWKEY | \
	IDCMP_EXTENDEDMOUSE | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE | IDCMP_INTUITICKS

struct retroVideo *video = NULL;
struct Window *My_Window = NULL;

struct Library * IntuitionBase = NULL;
struct IntuitionIFace *IIntuition = NULL;

struct Library * GraphicsBase = NULL;
struct GraphicsIFace *IGraphics = NULL;

struct Library * DiskfontBase = NULL;
struct DiskfontIFace *IDiskfont 	= NULL;

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
			WA_Flags,           WFLG_REPORTMOUSE | WFLG_RMBTRAP,
			TAG_DONE);

	return (My_Window != NULL) ;
}

struct TextFont *topaz8_font = NULL;

bool init_engine()
{
	if ( ! open_lib( "intuition.library", 51L , "main", 1, &IntuitionBase, (struct Interface **) &IIntuition  ) ) return FALSE;
	if ( ! open_lib( "graphics.library", 54L , "main", 1, &GraphicsBase, (struct Interface **) &IGraphics  ) ) return FALSE;
	if ( ! open_lib( "diskfont.library", 50L, "main", 1, &DiskfontBase, (struct Interface **) &IDiskfont  ) ) return FALSE;
	if ( ! open_lib( "layers.library", 54L , "main", 1, &LayersBase, (struct Interface **) &ILayers  ) ) return FALSE;
	if ( ! open_lib( "retromode.library", 1L , "main", 1, &RetroModeBase, (struct Interface **) &IRetroMode  ) ) return FALSE;
	if ( ! open_window(640,480) ) return false;

	if ( (video = retroAllocVideo( My_Window )) == NULL ) return false;

	topaz8_font =  open_font( "topaz.font" ,  8);

	engine_mx = (APTR) AllocSysObjectTags(ASOT_MUTEX, TAG_DONE);
	if ( ! engine_mx) return FALSE;

	return TRUE;
}

void close_engine()
{
	if (topaz8_font) CloseFont(topaz8_font); topaz8_font=0;

	if (My_Window) CloseWindow(My_Window);

	if (IntuitionBase) CloseLibrary(IntuitionBase); IntuitionBase = 0;
	if (IIntuition) DropInterface((struct Interface*) IIntuition); IIntuition = 0;

	if (LayersBase) CloseLibrary(LayersBase); LayersBase = 0;
	if (ILayers) DropInterface((struct Interface*) ILayers); ILayers = 0;

	if (GraphicsBase) CloseLibrary(GraphicsBase); GraphicsBase = 0;
	if (IGraphics) DropInterface((struct Interface*) IGraphics); IGraphics = 0;

	if (RetroModeBase) CloseLibrary(RetroModeBase); RetroModeBase = 0;
	if (IRetroMode) DropInterface((struct Interface*) IRetroMode); IRetroMode = 0;

	if (DiskfontBase) CloseLibrary(DiskfontBase); DiskfontBase = 0;
	if (IDiskfont) DropInterface((struct Interface*) IDiskfont); IRetroMode = 0;

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

void set_default_colors( struct retroScreen *screen )
{
	int n;
	for (n=0;n<256;n++)
		retroScreenColor( screen, n,DefaultPalette[n].r,DefaultPalette[n].g,DefaultPalette[n].b);
}

extern int paper;

void clear_cursor(struct retroScreen *screen)
{
	if (curs_on)
	{
		int gx,gy;
		gx = screen -> locateX * 8;
		gy = screen -> locateY * 8;
		retroBAR( screen, gx,gy,gx+7,gy+7, paper);
	}
}


void draw_cursor(struct retroScreen *screen)
{
	if (curs_on)
	{
		int gx,gy;
		gx = screen -> locateX * 8;
		gy = screen -> locateY * 8;
		retroBAR( screen, gx,gy+6,gx+6,gy+7, 3);
	}
}

char *gfxDefault(struct nativeCommand *cmd, char *tokenBuffer);

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
		gfxDefault(NULL, NULL);

		while (running)
		{
			while (msg = (IntuiMessage *) GetMsg( video -> window -> UserPort) )
			{
				switch (msg -> Class) 
				{
					case IDCMP_CLOSEWINDOW: 
							running = false; break;

					case IDCMP_MOUSEBUTTONS:
							switch (msg -> Code)
							{
								case SELECTDOWN:	engine_mouse_key |= 1; break;
								case SELECTUP:	engine_mouse_key &= ~1; break;
								case MENUDOWN:	engine_mouse_key |= 2; break;
								case MENUUP:		engine_mouse_key &= ~2; break;
							}
							break;

					case IDCMP_MOUSEMOVE:
							engine_mouse_x = msg -> MouseX - video -> window -> BorderLeft;
							engine_mouse_y = msg -> MouseY - video -> window -> BorderTop;
							break;

					case IDCMP_RAWKEY:
							engine_wait_key = false;
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

			for (n=0; n<8;n++)
			{
				if (n==1) if (screens[n])
				{
					retroFadeScreen(screens[n]);
				}
			}

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
	
		// make sure interpenter is close before we start closing screens.
		while (interpreter_running)
		{
			Delay(1);
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
