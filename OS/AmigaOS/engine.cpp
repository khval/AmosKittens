#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <vector>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
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
#include "spawn.h"
#include "init.h"

#include <proto/asl.h>

extern int sig_main_vbl;
extern bool running;			// 
extern bool interpreter_running;	// interprenter is really running.
extern int keyState[256];
extern char *F1_keys[20];

enum
{
	GID_ICONIFY = 1,
	GID_PREFS
};

extern struct Menu *amiga_menu;
extern struct retroSprite *sprite;

struct Process *EngineTask = NULL;
extern UWORD *EmptyPointer;

bool engine_wait_key = false;
bool engine_stopped = false;
bool engine_key_repeat = false;
bool engine_key_down = false;
bool engine_mouse_hidden = false;

uint32_t engine_update_flags = rs_bob_moved | rs_force_swap;

extern bool curs_on;
extern int _keyshift;

extern APTR engine_mx ;

extern ChannelTableClass *channels;
std::vector<struct keyboard_buffer> keyboardBuffer;
std::vector<struct amos_selected> amosSelected;
std::vector<int> engineCmdQue;


int		engine_mouse_key = 0;
int		engine_mouse_x = 0;
int		engine_mouse_y = 0;
uint32_t	engine_back_color = 0x000000;

int autoView = 1;
int bobDoUpdate = 0;			// when we are ready to update bobs.
int bobDoUpdateEnable = 1;
int bobAutoUpdate = 1;
int bobUpdateOnce = 0;
int bobUpdateEvery = 1;
int bobUpdateNextWait = 0;
int cursor_color = 3;

void clearBobs();
void clearBobsOnScreen(retroScreen *screen);
void drawBobs();
void drawBobsOnScreen(retroScreen *screen);

struct Gadeget *IcoGad = NULL;
struct Image * IcoImg = NULL;

extern void channel_amal( struct kittyChannel *self );
extern void channel_anim( struct kittyChannel *self );
extern void channel_movex( struct kittyChannel *self );
extern void channel_movey( struct kittyChannel *self );

struct Window *My_Window = NULL;

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
	IDCMP_EXTENDEDMOUSE | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE | IDCMP_INTUITICKS | IDCMP_MENUPICK | IDCMP_GADGETUP

extern BOOL open_lib( const char *name, int ver , const char *iname, int iver, struct Library **base, struct Interface **interface);

struct Gadeget *add_window_button(struct Image *img, ULONG id)
{
	struct Gadeget *retGad = NULL;

	if (img)
	{
		retGad = (struct Gadeget *) NewObject(NULL , "buttongclass", 
			GA_ID, id, 
			GA_RelVerify, TRUE, 
			GA_Image, img, 
			GA_TopBorder, TRUE, 
			GA_RelRight, 0, 
			GA_Titlebar, TRUE, 
			TAG_END);

		if (retGad)
		{
			AddGadget( My_Window, (struct Gadget *) retGad, ~0 );
		}
	}

	return retGad;
}


bool open_engine_window( int window_left, int window_top, int window_width, int window_height )
{
	My_Window = OpenWindowTags( NULL,
				WA_Left,			window_left,
				WA_Top,			window_top,
				WA_InnerWidth,	window_width,
				WA_InnerHeight,	window_height,
				WA_SimpleRefresh,	TRUE,
				WA_CloseGadget,	TRUE,
				WA_DepthGadget,	TRUE,
				WA_DragBar,		TRUE,
				WA_Borderless,	FALSE,
				WA_SizeGadget,	TRUE,
				WA_SizeBBottom,	TRUE,
				WA_NewLookMenus,	TRUE,
				WA_Title, "Amos Kittens",
				WA_Activate,        TRUE,
				WA_Flags, WFLG_RMBTRAP| WFLG_REPORTMOUSE,
				WA_IDCMP,           IDCMP_COMMON,
			TAG_DONE);

	if (My_Window)
	{
		struct DrawInfo *dri = GetScreenDrawInfo(My_Window -> WScreen);

		if (dri)
		{
			IcoImg = (struct Image *) NewObject(NULL, "sysiclass", SYSIA_DrawInfo, dri, SYSIA_Which, ICONIFYIMAGE, TAG_END );
			if (IcoImg)
			{
				IcoGad = add_window_button(IcoImg, GID_ICONIFY);
			}
		}
	}

	return (My_Window != NULL) ;
}

void close_engine_window( )
{
	if (My_Window)
	{
		if (IcoGad)
		{
			RemoveGadget( My_Window, (struct Gadget *) IcoGad );
			IcoGad = NULL;
		}

		if (IcoImg)
		{
			DisposeObject( (Object *) IcoImg );
			IcoImg = NULL;
		}

		ClearPointer( My_Window );
		CloseWindow( My_Window );
		My_Window = NULL;
	}
}

struct RastPort font_render_rp;
struct retroEngine *engine = NULL;

extern struct TextFont *open_font( char const *filename, int size );
extern struct TextFont *gfx_font;

bool init_engine()
{
	if ( ! open_engine_window( 200, 100, 640,480) ) return false;

	InitRastPort(&font_render_rp);
	font_render_rp.BitMap = AllocBitMapTags( 800, 50, 256, 
				BMATags_PixelFormat, PIXF_CLUT, 
				BMATags_Clear, true,
				BMATags_Displayable, false,
				TAG_END);

	if ( !font_render_rp.BitMap ) return false;

	font_render_rp.Font =  My_Window -> RPort -> Font;
	SetBPen( &font_render_rp, 0 );

	if (gfx_font = open_font( "topaz.font", 8 ))
	{
		 SetFont( &font_render_rp, gfx_font );
	}

	engine =  retroAllocEngine( My_Window, video );

	if ( ! engine) return FALSE;

	return TRUE;
}

bool engine_ready()
{
	if (engine == NULL) return false; 
	if ( font_render_rp.BitMap == NULL ) return false;
	return true;
}

void close_engine()
{
	if ( font_render_rp.BitMap )
	{
		FreeBitMap( font_render_rp.BitMap );
		font_render_rp.BitMap = NULL;
	}

	close_engine_window();

	if (gfx_font)
	{
		CloseFont( gfx_font );
		gfx_font = NULL;
	}

	if (engine)
	{
		retroFreeEngine( engine );
		engine = NULL;
	}
}

void main_engine();


bool start_engine()
{
#ifdef enable_engine_debug_output_yes
	BPTR engine_debug_output = Open("CON:660/50/600/480/Kittens engine",MODE_NEWFILE);
#else
	BPTR engine_debug_output = NULL;
#endif

	main_task = (struct Process *) FindTask(NULL);
	EngineTask = spawn( main_engine, "Amos kittens graphics engine",engine_debug_output);

	Wait(SIGF_CHILD);
	return ((EngineTask) && (engine_stopped == false));
}

void set_default_colors( struct retroScreen *screen )
{
	int n;
	for (n=0;n<256;n++)
		retroScreenColor( screen, n,DefaultPalette[n].r,DefaultPalette[n].g,DefaultPalette[n].b);
}

void atomic_add_key( ULONG eventCode, ULONG Code, ULONG Qualifier, char Char )
{
	struct keyboard_buffer event;

	engine_lock();
	event.event = eventCode;
	event.Code = Code;
	event.Qualifier = Qualifier;
	event.Char = Char;
	keyboardBuffer.push_back(event);
	engine_unlock();
}

#define limit_step( step ) if ( step <-0x11) { step=-0x11; } else if ( step >0x11) { step=0x11; }

void retroFadeScreen_beta(struct retroScreen * screen)
{
	int dr,dg,db;

	if (screen -> fade_speed)
	{
		if (screen -> fade_count < screen -> fade_speed)
		{
			screen -> fade_count++;
		}
		else
		{
			int changed = 0;
			int n = 0;
			struct retroRGB *opal = screen -> orgPalette;
			struct retroRGB *rpal = screen -> rowPalette;
			struct retroRGB *npal = screen -> fadePalette;

			for (n=0;n<256;n++)
			{
				dr = (int) npal->r - (int) opal->r;
				dg = (int) npal->g - (int) opal->g;
				db = (int) npal->b - (int) opal->b;

				limit_step(dr);
				limit_step(dg);
				limit_step(db);

				changed |= dr | dg | db;

				opal->r += dr;
				opal->g += dg;
				opal->b += db;

				*rpal =*opal;

				opal++;
				rpal++;
				npal++;
			}

			screen -> fade_count = 1;
			if (changed == 0) screen -> fade_speed = 0;
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
	struct retroFrameHeader *frame;

	if (image >= sprite -> number_of_frames) image = sprite -> number_of_frames-1;
	if (image < 0) image = 0;

	frame = sprite -> frames + image;
	width = frame -> width ;
	height = frame -> height ;

	x = (item -> x / 2) -  frame -> XHotSpot;
	y = (item -> y / 2) - frame -> YHotSpot;	

	if (y>0)
	{
		if (y+height> (int) (video->height/2)) height = (video->height/2) - y;
	}
	else
	{
		 source_y0 = -y; y = 0; height -= source_y0;
	}

	if (x>0)
	{
		if (x+width> (int) (video->width/2)) width =(video->width/2) - x;
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

		rgb = video -> scanlines[0].scanline[0].orgPalette;

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

extern void enable_Iconify();
extern void disable_Iconify();


	UWORD Code;
	ULONG GadgetID;
	UWORD Qualifier;
	struct MenuItem *item;
	UWORD menuNumber;
	struct amos_selected selected;

void empty_que( struct MsgPort *port )
{
	struct IntuiMessage *msg;

	while (msg = (IntuiMessage *) GetMsg( engine -> window -> UserPort) )
	{
		ReplyMsg( (Message*) msg );
	}
}


void engine_ShowMouse( ULONG enable )
{
	if (engine)
	{
		if(enable)
		{
			ClearPointer( engine -> window );
		}
		else if(EmptyPointer)
		{
			SetPointer( engine -> window, EmptyPointer, 1, 16, 0, 0);
		}
	}

	engine_mouse_hidden = !enable;
}

extern std::vector<struct amosMenuItem *> menuitems;

struct amosMenuItem *find_menu_shortcut_by_rawkey( ULONG code, ULONG qualifier )
{
	int i;

	for ( i=0; i< (int) menuitems.size();i++ )
	{
		if (menuitems[i])
		{
			if ((menuitems[i] -> scancode == code ) &&
				(menuitems[i] -> qualifier == qualifier ) )
			{
				return menuitems[i];
			}
		}
	}

	return NULL;
}

#define MKEYS (IEQUALIFIER_RALT | IEQUALIFIER_LALT | IEQUALIFIER_RSHIFT | IEQUALIFIER_LSHIFT | IEQUALIFIER_RCOMMAND | IEQUALIFIER_LCOMMAND) 

bool menu_shortcut( ULONG Code ,  ULONG Qualifier)
{
	struct amosMenuItem *menuItem ;

	menuItem = find_menu_shortcut_by_rawkey( Code &~IECODE_UP_PREFIX, Qualifier & MKEYS  );

	if (menuItem)
	{
		if (Code & IECODE_UP_PREFIX)
		{
			engine_lock();
			selected.menu = menuItem -> index[0]-1;
			selected.item = menuItem -> index[1]-1;
			selected.sub = menuItem -> index[2]-1;
			amosSelected.push_back(selected);
			engine_unlock();
		}
		return true;
	}

	return false;
}

void handel_window()
{
	ULONG Class;
	struct IntuiMessage *msg;
	int mouse_x, mouse_y;
	int ccode;

			while (msg = (IntuiMessage *) GetMsg( engine -> window -> UserPort) )
			{
				Qualifier = msg -> Qualifier;
				Class = msg -> Class;
				Code = msg -> Code;
				GadgetID = (Class == IDCMP_GADGETUP) ? ((struct Gadget *) ( msg -> IAddress)) -> GadgetID : 0;
				mouse_x = msg -> MouseX;
				mouse_y = msg -> MouseY;
				ReplyMsg( (Message*) msg );

				switch (Class) 
				{
					case IDCMP_CLOSEWINDOW: 
							running = false;
							break;

					case IDCMP_GADGETUP:
							empty_que( engine -> window -> UserPort );
							enable_Iconify(); 
							engine -> window = NULL;
							return;

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

							engine_mouse_x = mouse_x - engine -> window -> BorderLeft;
							engine_mouse_y = mouse_y - engine -> window -> BorderTop;
							
							break;

					case IDCMP_MENUPICK:

							menuNumber =  Code;
							while (menuNumber != MENUNULL)
							{
								item = ItemAddress(amiga_menu, menuNumber);

								if (item)
								{
									selected.menu = MENUNUM(menuNumber);
									selected.item = ITEMNUM(menuNumber);
									selected.sub = SUBNUM(menuNumber);
									amosSelected.push_back(selected);
									menuNumber = item -> NextSelect;

									Printf("selected %ld\n",amosSelected.size());
								} else break;
							}
							

							break;

					case IDCMP_RAWKEY:

							if (menu_shortcut( Code ,  Qualifier)) break;

							_keyshift = Qualifier;
							if (Qualifier & IEQUALIFIER_REPEAT) break;		// repeat done by Amos KIttens...
							 
							ccode = Code & ~IECODE_UP_PREFIX;

							{
								int emu_code = Code &~ IECODE_UP_PREFIX;
								if (emu_code==75) emu_code = 95;
								keyState[ emu_code ] = (Code & IECODE_UP_PREFIX) ? 0 : -1;
							}

							engine_wait_key = false;


							if ((ccode >= RAWKEY_F1) && (ccode <= RAWKEY_F10))
							{
								int idx = ccode - RAWKEY_F1 + (Qualifier & IEQUALIFIER_RCOMMAND ? 10 : 0);

								if (F1_keys[idx])
								{
									char *p;
									for (p=F1_keys[idx];*p;p++) atomic_add_key( (Code & IECODE_UP_PREFIX) ? kitty_key_up : kitty_key_down, ccode, 0, *p );
								}
							}

							if (Code & IECODE_UP_PREFIX)
							{
								atomic_add_key( kitty_key_up, ccode, Qualifier, 0 );
							}
							else
							{
								atomic_add_key( kitty_key_down, ccode, Qualifier, 0 );
							}

							break;
				}
			}
}

extern struct MsgPort *iconifyPort;

void handel_iconify()
{
	struct Message *msg;
	bool disabled = false;

	while (msg = (Message *) GetMsg( iconifyPort ) )
	{
		ReplyMsg( (Message*) msg );
		disabled = true;
	}

	if (disabled)
	{
		disable_Iconify();
		engine -> window = My_Window;
	}
}

void swap_buffer(struct retroScreen *screen )
{
/*
	memcpy( 
		screen -> Memory[1 - screen -> double_buffer_draw_frame], 
		screen -> Memory[screen -> double_buffer_draw_frame],
		screen -> bytesPerRow * screen -> realHeight );
*/

	screen -> double_buffer_draw_frame = 1 - screen -> double_buffer_draw_frame ;
}


void limit_mouse()
{
	if ((engine -> window)&&(iconifyPort == NULL))	
	{
		if (engine -> limit_mouse == true)
		{
			struct IBox mouseLimit = {
				engine -> window -> BorderLeft + engine -> limit_mouse_x0,
				engine -> window -> BorderTop + engine -> limit_mouse_y0,
				engine -> limit_mouse_x1 - engine -> limit_mouse_x0,
				engine -> limit_mouse_y1 - engine -> limit_mouse_y0 };

			ActivateWindow( engine -> window );
			SetWindowAttrs( engine -> window, WA_MouseLimits, &mouseLimit, TAG_END);
		}
		else
		{
			SetWindowAttrs( engine -> window, 	WA_GrabFocus, 0, TAG_END);
		}
	}
}

void main_engine()
{
	int bobIsUpdated = 0; 
	int bobUpdateOnceDone = 0;

	Printf("init engine\n");

	if (init_engine())		// libs open her.
	{
		Printf("init engine done..\n");
		
		struct retroScreen *screen = NULL;

		ULONG sigs;
		ULONG joy_sig;
		ULONG ret;
		int n;
		
		Signal( &main_task->pr_Task, SIGF_CHILD );

		Printf("clear video\n");
		retroClearVideo(video, engine_back_color);

		Printf("init joysticks..\n");
		init_joysticks();

		joy_sig = 1L << (joystick_msgport -> mp_SigBit);

		sigs = SIGBREAKF_CTRL_C;
		sigs |= joy_sig;

		while (running)
		{
			ret = SetSignal(0L, sigs);

			if ( ret & SIGBREAKF_CTRL_C) running = false;

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

			if (My_Window) handel_window();

			if (iconifyPort) handel_iconify();

			engine_lock();

			while (engineCmdQue.size() > 0)
			{
				switch ( engineCmdQue[0] )
				{
					case kitty_to_back:

						if ((engine -> window)&&(iconifyPort == NULL))
						{
							empty_que( engine -> window -> UserPort );
							enable_Iconify(); 
							engine -> window = NULL;
						}
						break;

					case kitty_to_front:

						if (iconifyPort) 
						{
							disable_Iconify(); 
							engine -> window = My_Window;
							limit_mouse();
						}
						break;

					case kitty_limit_mouse:

						limit_mouse();
						break;
				}

				engineCmdQue.erase( engineCmdQue.begin() );
			}

			if (autoView)
			{
				retroClearVideo( video, engine_back_color );

				for (n=0; n<8;n++)
				{
					screen = screens[n];

					if (screen)
					{
						retroFadeScreen_beta(screen);

						Printf("%08lx,%08lx\n",screen -> event_flags , engine_update_flags);

						if (screen -> event_flags & engine_update_flags)
						{
							if (screen -> Memory[1]) 	// has double buffer
							{
								clearBobsOnScreen(screen);
								drawBobsOnScreen(screen);
								swap_buffer( screen );
							}
							else
							{
								clearBobsOnScreen(screen);
								drawBobsOnScreen(screen);
							}
						}
						screen -> event_flags = 0;
					}
				}	// next

				retroDrawVideo( video );

#if 1
				if (channels)
				{
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
#endif

#if 1
				if ((sprite)&&(video -> sprites))
				{
					struct retroSpriteObject *item;

					for (n=0;n<64;n++)
					{
						item = &video -> sprites[n];
						item -> sprite = sprite;

						if (item -> image>0)
						{
							DrawSprite( sprite, item, item -> image -1, 0 );
						}
					}
				}
#endif		
			}	// end if (autoView)

			engine_unlock();

			if (My_Window)
			{
				AfterEffectScanline( video );
//				AfterEffectAdjustRGB( video , 8, 0 , 4);
				retroDmaVideo( video, engine );

				WaitTOF();
				if (sig_main_vbl) Signal( &main_task->pr_Task, 1<<sig_main_vbl );

				BltBitMapTags(BLITA_SrcType, BLITT_BITMAP,
						BLITA_Source, engine->rp.BitMap,
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
			else
			{
				if (sig_main_vbl) Signal( &main_task->pr_Task, 1<<sig_main_vbl );
			}

			Delay(1);
		} // while
	}
	else
	{
		Signal( &main_task->pr_Task, SIGF_CHILD );
	}

	close_joysticks();

	engine_lock();
	close_engine();
	engine_unlock();


	if (sig_main_vbl) Signal( &main_task->pr_Task, 1<<sig_main_vbl );	// signal in case we got stuck in a waitVBL.

	engine_stopped = true;
}


void engine_lock()
{
	MutexObtain(engine_mx);
}

void engine_unlock()
{
	MutexRelease(engine_mx);
}

char *asl()
{
	struct FileRequester	 *filereq;
	char *ret = NULL;
	char c;
	int l;

	if (filereq = (struct FileRequester	 *) AllocAslRequest( ASL_FileRequest, TAG_DONE ))
	{
		if (AslRequestTags( (void *) filereq, ASLFR_DrawersOnly, FALSE,	TAG_DONE ))
		{
			if ((filereq -> fr_File)&&(filereq -> fr_Drawer))
			{
				if (l = strlen(filereq -> fr_Drawer))
				{
					c = filereq -> fr_Drawer[l-1];
					if (ret = (char *) malloc( strlen(filereq -> fr_Drawer) + strlen(filereq -> fr_File) +2 ))
					{
						sprintf( ret, ((c == '/') || (c==':')) ? "%s%s" : "%s/%s",  filereq -> fr_Drawer, filereq -> fr_File ) ;
					}
				}
				else ret = strdup(filereq -> fr_File);
			}
		}
		 FreeAslRequest( filereq );
	}

	return ret;
}

