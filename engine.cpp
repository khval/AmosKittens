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
#include <proto/keymap.h>
#include <proto/Amigainput.h>

#include "amoskittens.h"
#include "joysticks.h"
#include "engine.h"
#include "bitmap_font.h"
#include "debug.h"
#include "amalcompiler.h"
#include "channel.h"

extern int sig_main_vbl;
extern bool running;			// 
extern bool interpreter_running;	// interprenter is really running.
extern int keyState[256];
extern char *F1_keys[20];


extern struct retroSprite *sprite;

static struct Process *MainTask = NULL;
struct Process *EngineTask = NULL;

bool engine_wait_key = false;
bool engine_stopped = false;

extern bool curs_on;
extern int _keyshift;

extern APTR engine_mx ;

extern ChannelTableClass *channels;
std::vector<struct keyboard_buffer> keyboardBuffer;

int engine_mouse_key = 0;
int engine_mouse_x = 0;
int engine_mouse_y = 0;

int autoView = 1;
int bobUpdate = 1;

int cursor_color = 3;

void clearBobs();
void drawBobs();

extern void channel_amal( struct kittyChannel *self );
extern void channel_anim( struct kittyChannel *self );
extern void channel_movex( struct kittyChannel *self );
extern void channel_movey( struct kittyChannel *self );

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

	retroAllocSpriteObjects(video,64);

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
}

void main_engine();


bool start_engine()
{
#ifdef enable_engine_debug_output_yes
	BPTR engine_debug_output = Open("CON:660/50/600/480/Kittens engine",MODE_NEWFILE);
#endif


	MainTask = (struct Process *) FindTask(NULL);
	EngineTask = CreateNewProcTags(
				NP_Name, "Amos kittens graphics engine" ,
				NP_Entry, main_engine, 
				NP_Priority, 0, 
				NP_Child, TRUE,

#ifdef enable_engine_debug_output_yes
				NP_Output, engine_debug_output,
#endif
				TAG_END);

	Wait(SIGF_CHILD);
	return ((EngineTask) && (engine_stopped == false));
}

void set_default_colors( struct retroScreen *screen )
{
	int n;
	for (n=0;n<256;n++)
		retroScreenColor( screen, n,DefaultPalette[n].r,DefaultPalette[n].g,DefaultPalette[n].b);
}

void clear_cursor( struct retroScreen *screen )
{
	struct retroTextWindow *textWindow = screen -> currentTextWindow;

	if ((curs_on)&&(textWindow))
	{
		int gx,gy;
		int x = (textWindow -> x + textWindow -> locateX) + (textWindow -> border ? 1 : 0);
		int y = (textWindow -> y + textWindow -> locateY) + (textWindow -> border ? 1 : 0);
		gx=8*x;	gy=8*y;

		retroBAR( screen, gx,gy,gx+7,gy+7, screen->paper);
	}
}

void draw_cursor(struct retroScreen *screen)
{
	struct retroTextWindow *textWindow = screen -> currentTextWindow;

	if ((curs_on)&&(textWindow))
	{
		int gx,gy;
		int x = (textWindow -> x + textWindow -> locateX) + (textWindow -> border ? 1 : 0);
		int y = (textWindow -> y + textWindow -> locateY) + (textWindow -> border ? 1 : 0);
		gx=8*x;	gy=8*y;

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

void retroFadeScreen_beta(struct retroScreen * screen)
{
	if (screen -> fade_speed)
	{
		printf("is fading\n");

		if (screen -> fade_count < screen -> fade_speed)
		{
			screen -> fade_count++;
		}
		else
		{
			printf("is here\n");
			int changed_at = -1;
			int n = 0;
			struct retroRGB *opal = screen -> orgPalette;
			struct retroRGB *rpal = screen -> rowPalette;
			struct retroRGB *npal = screen -> fadePalette;

			for (n=0;n<256;n++)
			{

				if (npal -> r < 256)	// valid colour most be set.
				{
					if (npal->r > opal->r)
					{
						opal -> r +=  0x11;	
						changed_at = n | 0x1000;
					}
					else if ((npal->r < opal -> r))
					{
						opal -> r -=  0x11;	
						changed_at = n | 0x1000;
					}
				}

				if (npal -> g < 256)	// valid colour most be set.
				{
					if (npal->g > opal->g)
					{
						opal->g += 0x11;
						changed_at = n | 0x2000;
					}
					else if ((npal->g < opal -> g))
					{
						opal->g -= 0x11;
						changed_at = n | 0x2000;
					}
				}

				if (npal -> b < 256)	// valid colour most be set.
				{
					if (npal->b > opal->b)
					{
						opal->b +=  0x11;
						changed_at = n | 0x3000;
					}
					else if ((npal->b < opal->b))
					{
						opal->b -=  0x11;
						changed_at = n | 0x3000;
					}
				}

				*rpal =*opal;

				opal++;
				rpal++;
				npal++;
			}

			screen -> fade_count = 0;
			if (changed_at == -1)
			{
				printf("fading done....\n");
				screen -> fade_speed = 0;
			}
		}
	}
}

void DrawSprite(
	struct retroSprite * sprite,
	struct retroSpriteObject *item,
	int image,
	int flags)
{
	int x,y;
	int width;
	int height;
	int ypos;
	int source_x0 = 0,source_y0 = 0;
	unsigned int *destination_row_ptr;
	unsigned int *destination_row_ptr2;
	unsigned int *destination_row_start;
	unsigned char *source_row_start;
	unsigned char *source_row_ptr;
	unsigned char *source_row_end ;
	struct retroRGB *rgb;
	struct retroRGB *rgb2;
	unsigned int color;

	if (image > sprite -> number_of_frames) image = sprite -> number_of_frames;
	if (image < 0) image = 0;

	struct retroFrameHeader *frame = sprite -> frames + image;

	width = frame -> Width ;
	height = frame -> Height ;

	x = (item -> x / 2) -  frame -> XHotSpot;
	y = (item -> y / 2) - frame -> YHotSpot;	

	if (y>0)
	{
		if (y+height>(video->height/2)) height = (video->height/2) - y;
	}
	else
	{
		 source_y0 = -y; y = 0; height -= source_y0;
	}

	if (x>0)
	{
		if (x+width>(video->width/2)) width =(video->width/2) - x;
	}
	else
	{
		source_x0 = -x; x = 0; width -= source_x0;
	}

	destination_row_start = video -> Memory + (video -> width * (y*2)) + (x*2);
	source_row_start = (unsigned char *) frame -> data + (source_y0 * frame -> bytesPerRow ) + source_x0;
	source_row_end = source_row_start + width;

	for ( ypos = 0; ypos < height; ypos++ )
	{
		destination_row_ptr = destination_row_start;
		destination_row_ptr2 = destination_row_start + video -> width;

		rgb = video -> scanlines[0].orgPalette;

		for ( source_row_ptr = source_row_start;  source_row_ptr < source_row_end ; source_row_ptr++ )
		{
			if (rgb) 
			{
				rgb2 = &rgb[*source_row_ptr];
				color = (rgb2->r << 16) | (rgb2->g<<8) | rgb2->b;
			}
			else color = 0;

			if (*source_row_ptr) 
			{
				*destination_row_ptr= color;
				*(destination_row_ptr+1)= color;
				*destination_row_ptr2= color;
				*(destination_row_ptr2+1)= color;
			}
			destination_row_ptr+=2;
		}

		destination_row_start += (video -> width*2);
		source_row_start += frame -> bytesPerRow;
		source_row_end += frame -> bytesPerRow;
	}
}



void main_engine()
{
	struct RastPort scroll_rp;
	struct IntuiMessage *msg;

	retroRGB color;
	double start_sync;

	Printf("init engine\n");

	if (init_engine())		// libs open her.
	{
		Printf("init engine done..\n");
		
		struct retroScreen *screen ;
		ULONG Class;
		UWORD Code;
		UWORD Qualifier;
		ULONG sigs;
		ULONG joy_sig;
		ULONG ret;
		int n;
		
		Signal( &MainTask->pr_Task, SIGF_CHILD );

		Printf("clear video\n");
		retroClearVideo(video);
		gfxDefault(NULL, NULL);

		Printf("init joysticks..\n");
		init_joysticks();

		joy_sig = 1L << (joystick_msgport -> mp_SigBit);

		sigs = SIGBREAKF_CTRL_C;
		sigs |= joy_sig;

		Printf("main loop\n");

		while (running)
		{
			ret = SetSignal(0L, sigs);

			if ( ret & SIGBREAKF_CTRL_C) running = false;

//			Printf("joy events\n");
			if ( ret & joy_sig )
			{
				for (n=0;n<4;n++)
				{
					if (joysticks[n].id>0)
					{
						joy_stick(n,joysticks[n].controller);
					}
				}
			}

//			Printf("window events\n");
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
								Code = Code & ~IECODE_UP_PREFIX;

								if ((Qualifier & IEQUALIFIER_LCOMMAND) || (Qualifier & IEQUALIFIER_RCOMMAND))
								{
									if ((Code >= RAWKEY_F1) && (Code <= RAWKEY_F10))
									{
										int idx = Code - RAWKEY_F1 + (Qualifier & IEQUALIFIER_RCOMMAND ? 10 : 0);

										if (F1_keys[idx])
										{
											char *p;
											for (p=F1_keys[idx];*p;p++) atomic_add_to_keyboard_queue( 0, 0, *p );
										}
									}
								}
								else	atomic_add_to_keyboard_queue( Code, Qualifier, 0 );
							}
							break;
				}

				ReplyMsg( (Message*) msg );
			}

//			Printf("autoview\n");
			if (autoView)
			{
				retroClearVideo( video );

				engine_lock();

//				Printf("draw bobs\n");

				if (bobUpdate==1)	drawBobs();

//				Printf("draw screens\n");

				for (n=0; n<8;n++)
				{
					screen = screens[n];

					if (screen)
					{
//						Printf("check fade count %ld speed %ld\n", screen -> fade_count, screen -> fade_speed);
						retroFadeScreen(screen);

						if (screen -> autoback!=0)
						{
							if (screen -> Memory[1]) 
							{
								memcpy( 
									screen -> Memory[1 - screen -> double_buffer_draw_frame], 
									screen -> Memory[screen -> double_buffer_draw_frame],
									screen -> bytesPerRow * screen -> realHeight );

								screen -> double_buffer_draw_frame = 1 - screen -> double_buffer_draw_frame ;
							}
						}
					}
				}

//				Printf("draw video\n");

				retroDrawVideo( video );

//				Printf("clear bobs\n");

				if (bobUpdate==1) clearBobs();

				if (channels)
				{
//					Printf("do channels %ld\n",channels -> _size());

					struct kittyChannel *item;
					int i;
					for (i=0;i<channels -> _size();i++)
					{
						if (item = channels -> item(i))
						{
							if (item->amal_script) channel_amal( item );
							if (item->anim_script) channel_anim( item );
							if (item->movex_script) channel_movex( item );
							if (item->movey_script) channel_movey( item );
						}
					}
				}

				if ((sprite)&&(video -> sprites))
				{
					struct retroSpriteObject *item;

					Printf("draw sprites\n");

					for (n=0;n<64;n++)
					{
						item = &video -> sprites[n];

						item -> sprite = sprite;

						if (item -> image>0)
						{
							//retroSprite( video, n, item -> x, item -> y, item -> image );

							DrawSprite( sprite, item, item -> image -1, 0 );

							Printf("sprite %ld,%ld,%ld,%ld\n",n, item -> x, item -> y, item -> image);
						}
					}
				}
		
				engine_unlock();
			}

//			Printf("WaitTOF\n");

			WaitTOF();
			if (sig_main_vbl) Signal( &MainTask->pr_Task, 1<<sig_main_vbl );

			AfterEffectScanline( video );
//			AfterEffectAdjustRGB( video , 8, 0 , 4);
			retroDmaVideo( video );

			Delay(1);

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
		Signal( &MainTask->pr_Task, SIGF_CHILD );
	}

	close_joysticks();
	close_engine();

	if (sig_main_vbl) Signal( &MainTask->pr_Task, 1<<sig_main_vbl );	// signal in case we got stuck in a waitVBL.

	engine_stopped = true;
}

void wait_engine()
{
	do
	{
		Delay(1);
	} while ((EngineTask)&&(engine_stopped == false));
	Delay(1);

	Printf("EngineTask %04lx, engine stopped: %s\n", EngineTask, engine_stopped ? "True" : "False" );
}


void engine_lock()
{
	MutexObtain(engine_mx);
}

void engine_unlock()
{
	MutexRelease(engine_mx);
}
