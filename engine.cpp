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

struct XYSTW_Vertex3D { 
float x, y; 
float s, t, w; 
}; 

typedef struct CompositeHookData_s {
	struct BitMap *srcBitMap; // The source bitmap
	int32 srcWidth, srcHeight; // The source dimensions
	int32 offsetX, offsetY; // The offsets to the destination area relative to the window's origin
	int32 scaleX, scaleY; // The scale factors
	uint32 retCode; // The return code from CompositeTags()
} CompositeHookData;

#ifdef amigaos4
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
		error = CompositeTags(COMPOSITE_Src, 
			the_bitmap, the_bitmap_dest,

			COMPTAG_VertexArray, P, 
			COMPTAG_VertexFormat,COMPVF_STW0_Present,
		    	COMPTAG_NumTriangles,2,

			COMPTAG_SrcAlpha, (uint32) (0x0010000 ),
			COMPTAG_Flags, COMPFLAG_SrcAlphaOverride | COMPFLAG_HardwareOnly | COMPFLAG_SrcFilter ,
			TAG_DONE);
	}
}
#endif


static ULONG compositeHookFunc(struct Hook *hook, struct RastPort *rastPort, struct BackFillMessage *msg) {

	struct Window *the_win = video -> window;	

#ifdef amigaos4

	draw_comp_bitmap(video->rp.BitMap, the_win->RPort -> BitMap, video -> width, video -> height,
		the_win->BorderLeft ,
		the_win->BorderTop ,
		the_win->Width - the_win->BorderLeft - the_win->BorderRight,
		the_win->Height -  the_win->BorderTop - the_win->BorderBottom);

#endif

	return 0;
}

static CompositeHookData hookData;

static struct Rectangle rect;
static struct Hook hook;

static void set_target_hookData( void )
{
 	rect.MinX = My_Window->BorderLeft;
 	rect.MinY = My_Window->BorderTop;
 	rect.MaxX = My_Window->Width - My_Window->BorderRight - 1;
 	rect.MaxY = My_Window->Height - My_Window->BorderBottom - 1;

 	float destWidth = rect.MaxX - rect.MinX + 1;
 	float destHeight = rect.MaxY - rect.MinY + 1;
 	float scaleX = (destWidth + 0.5f) / (double) video -> width;
 	float scaleY = (destHeight + 0.5f) / (double) video -> height;

	hookData.srcWidth = video -> width;
	hookData.srcHeight = video -> height;
	hookData.offsetX = video -> window->BorderLeft;
	hookData.offsetY = video -> window->BorderTop;
	hookData.scaleX = COMP_FLOAT_TO_FIX(scaleX);
	hookData.scaleY = COMP_FLOAT_TO_FIX(scaleY);
	hookData.retCode = COMPERR_Success;

	hook.h_Entry = (HOOKFUNC)compositeHookFunc;
	hook.h_Data = &hookData;
}

static void BackFill_Func(struct RastPort *ArgRP, struct BackFillArgs *MyArgs)
{
	set_target_hookData();

//	LockLayer(0,video -> window -> RPort -> Layer);
	DoHookClipRects(&hook, video -> window -> RPort, &rect);
//	UnlockLayer(video -> window -> RPort -> Layer);
}


extern BOOL open_lib( const char *name, int ver , const char *iname, int iver, struct Library **base, struct Interface **interface);


bool open_window( int window_width, int window_height )
{
		My_Window = OpenWindowTags( NULL,

#ifdef __amigaos4__
					WA_Title,         "retroVideo window mode",
#endif
			WA_InnerWidth,      window_width,
			WA_InnerHeight,     window_height,

			WA_SimpleRefresh,		TRUE,
			WA_CloseGadget,     TRUE,
			WA_DepthGadget,     TRUE,

			WA_DragBar,         TRUE,
			WA_Borderless,      FALSE,
			WA_SizeGadget,      TRUE,
			WA_SizeBBottom,	TRUE,

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
//	APTR output = IDOS->Open("CON:",MODE_NEWFILE);

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
			int x;

			retroScreenColor( screens[0], 0, 255, 100, 50 );
			retroScreenColor( screens[0], 1, 255, 255, 255 );
			retroScreenColor( screens[0], 2, 0, 0, 0 );
			retroScreenColor( screens[0], 3, 255, 0, 0 );

			retroScreenColor( screens[0], 4, 0, 0, 0 );
			retroScreenColor( screens[0], 5, 255, 255, 255 );
			retroScreenColor( screens[0], 6, 0, 0, 0 );
			retroScreenColor( screens[0], 7, 255, 0, 0 );
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
				}

				ReplyMsg( (Message*) msg );
			}

			engine_lock();
			retroClearVideo( video );
			retroDrawVideo( video );
			engine_unlock();

			retroDmaVideo(video);

			WaitTOF();
			BackFill_Func(NULL, NULL );

			if (sig_main_vbl) Signal( &MainTask->pr_Task, 1<<sig_main_vbl );
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
