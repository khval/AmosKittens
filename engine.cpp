#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <vector>

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
extern int keyState[256];

bool engine_wait_key = false;
bool engine_started = false;

extern bool curs_on;
extern int _keyshift;

APTR engine_mx = 0;

std::vector<struct keyboard_buffer> keyboardBuffer;

int engine_mouse_key = 0;
int engine_mouse_x = 0;
int engine_mouse_y = 0;

int autoView = 1;

int cursor_color = 3;

void clearBobs();
void drawBobs();

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
			WA_Activate, TRUE,

			WA_IDCMP,           IDCMP_COMMON,
			WA_Flags,           WFLG_REPORTMOUSE | WFLG_RMBTRAP ,
			TAG_DONE);

	return (My_Window != NULL) ;
}

struct TextFont *topaz8_font = NULL;
struct RastPort font_render_rp;

bool init_engine()
{
	if ( ! open_lib( "intuition.library", 51L , "main", 1, &IntuitionBase, (struct Interface **) &IIntuition  ) ) return FALSE;
	if ( ! open_lib( "graphics.library", 54L , "main", 1, &GraphicsBase, (struct Interface **) &IGraphics  ) ) return FALSE;
	if ( ! open_lib( "layers.library", 54L , "main", 1, &LayersBase, (struct Interface **) &ILayers  ) ) return FALSE;
	if ( ! open_window(640,480) ) return false;

	if ( (video = retroAllocVideo( My_Window )) == NULL ) return false;

	topaz8_font =  open_font( "topaz.font" ,  8);
	if ( ! topaz8_font ) return FALSE;


	InitRastPort(&font_render_rp);
	font_render_rp.BitMap = AllocBitMapTags( 800, 50, 256, 
				BMATags_PixelFormat, PIXF_CLUT, 
				BMATags_Clear, true,
				BMATags_Displayable, false,
				TAG_END);

	if ( !font_render_rp.BitMap ) return false;

	font_render_rp.Font =  My_Window -> RPort -> Font;
	SetBPen( &font_render_rp, 0 );

	engine_mx = (APTR) AllocSysObjectTags(ASOT_MUTEX, TAG_DONE);
	if ( ! engine_mx) return FALSE;

	return TRUE;
}

void close_engine()
{
	if (topaz8_font) CloseFont(topaz8_font); topaz8_font=0;

	if ( font_render_rp.BitMap )
	{
		FreeBitMap( font_render_rp.BitMap );
		font_render_rp.BitMap = NULL;
	}

	if (My_Window) CloseWindow(My_Window);

	if (IntuitionBase) CloseLibrary(IntuitionBase); IntuitionBase = 0;
	if (IIntuition) DropInterface((struct Interface*) IIntuition); IIntuition = 0;

	if (LayersBase) CloseLibrary(LayersBase); LayersBase = 0;
	if (ILayers) DropInterface((struct Interface*) ILayers); ILayers = 0;

	if (GraphicsBase) CloseLibrary(GraphicsBase); GraphicsBase = 0;
	if (IGraphics) DropInterface((struct Interface*) IGraphics); IGraphics = 0;

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
		retroBAR( screen, gx,gy+6,gx+6,gy+7, cursor_color);
	}
}

char *gfxDefault(struct nativeCommand *cmd, char *tokenBuffer);

void atomic_add_to_keyboard_queue( ULONG Code, ULONG Qualifier, char Char )
{
	struct keyboard_buffer event;

	engine_lock();
	event.Code = Code;
	event.Qualifier = Qualifier;
	event.Char = Char;
	keyboardBuffer.push_back(event);
	engine_unlock();
}

void main_engine()
{
	struct RastPort scroll_rp;
	struct IntuiMessage *msg;

	retroRGB color;
	double start_sync;

	if (init_engine())		// libs open her.
	{
		struct retroScreen *screen ;
		ULONG Class;
		UWORD Code;
		UWORD Qualifier;
		int n;
		engine_started = true;
		Signal( &MainTask->pr_Task, SIGF_CHILD );

		retroClearVideo(video);
		gfxDefault(NULL, NULL);

		while (running)
		{
			if (SetSignal(0L, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) running = false;

			while (msg = (IntuiMessage *) GetMsg( video -> window -> UserPort) )
			{
				Qualifier = msg -> Qualifier;
				Class = msg -> Class;
				Code = msg -> Code;

				switch (Class) 
				{
					case IDCMP_CLOSEWINDOW: 
							running = false; break;

					case IDCMP_MOUSEBUTTONS:
							switch (Code)
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

							_keyshift = Qualifier;

							if (Qualifier & IEQUALIFIER_REPEAT)
							{
								printf("we have a repeat key\n");
							}
							 
							{
								int emu_code = Code &~ IECODE_UP_PREFIX;
								if (emu_code==75) emu_code = 95;
								keyState[ emu_code ] = (Code & IECODE_UP_PREFIX) ? 0 : -1;
							}

							if ((Code & IECODE_UP_PREFIX) || (Qualifier & IEQUALIFIER_REPEAT))
							{
								engine_wait_key = false;
								atomic_add_to_keyboard_queue( Code & ~ IECODE_UP_PREFIX, Qualifier, 0 );
							}
							break;
				}

				ReplyMsg( (Message*) msg );
			}

			engine_lock();
			retroClearVideo( video );
			drawBobs();

			retroDrawVideo( video );
			retroDmaVideo( video );
			engine_unlock();

			WaitTOF();
			if (sig_main_vbl) Signal( &MainTask->pr_Task, 1<<sig_main_vbl );

			engine_lock();
			for (n=0; n<8;n++)
			{
				screen = screens[n];

				if (screen)
				{
					retroFadeScreen(screen);
					if (screen -> Memory[1]) screen -> double_buffer_draw_frame = 1 - screen -> double_buffer_draw_frame ;
				}
			}
			clearBobs();
			engine_unlock();

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
