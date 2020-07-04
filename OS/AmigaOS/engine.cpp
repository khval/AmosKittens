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

#include "common_screen.h"

#include <proto/asl.h>

extern int sig_main_vbl;
extern bool running;			// 
extern bool interpreter_running;	// interprenter is really running.

extern char *F1_keys[20];
extern struct Menu *amiga_menu;

extern void BackFill_Func(struct RastPort *ArgRP, struct BackFillArgs *MyArgs);

struct Process *EngineTask = NULL;
struct Screen *fullscreen_screen = NULL;
extern UWORD *EmptyPointer;

bool synchro_on = true;

uint32_t engine_update_flags = rs_bob_moved | rs_force_swap;

extern bool curs_on;
extern int _keyshift;

extern APTR engine_mx ;
struct windowclass window_save_state;
struct windowclass window_normal_saved;

extern ChannelTableClass *channels;
std::vector<struct keyboard_buffer> keyboardBuffer;
std::vector<struct amos_selected> amosSelected;
std::vector<int> engineCmdQue;
std::vector<struct kittyVblInterrupt> kittyVblInterrupts;

int autoView = 1;
int bobDoUpdate = 0;			// when we are ready to update bobs.
int bobDoUpdateEnable = 1;
int bobAutoUpdate = 1;
int bobUpdateOnce = 0;
int bobUpdateEvery = 1;
int bobUpdateNextWait = 0;
int cursor_color = 3;

void clearBobsOnScreen(retroScreen *screen);
void drawBobsOnScreenExceptBob( struct retroScreen *screen, struct retroSpriteObject *exceptBob );

extern void draw_comp_bitmap(struct BitMap *the_bitmap,struct BitMap *the_bitmap_dest, int width,int height, int wx,int wy,int ww, int wh);

struct kIcon
{
	struct Gadeget *gadget ;
	struct Image *image ;
};

struct kIcon iconifyIcon = { NULL, NULL };
struct kIcon zoomIcon = { NULL, NULL };


extern void channel_amal( struct kittyChannel *self );
extern void channel_anim( struct kittyChannel *self );
extern void channel_movex( struct kittyChannel *self );
extern void channel_movey( struct kittyChannel *self );

struct Window *My_Window = NULL;
struct BitMap *comp_bitmap = NULL;

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

void open_icon(struct DrawInfo *dri, ULONG imageID, ULONG gadgetID, struct kIcon *icon )
{
	icon -> image = (struct Image *) NewObject(NULL, "sysiclass", SYSIA_DrawInfo, dri, SYSIA_Which, imageID, TAG_END );
	if (icon -> image)
	{
		icon -> gadget = add_window_button( icon -> image, gadgetID);
	}
}

void dispose_icon(struct Window *win, struct kIcon *icon)
{
	if (icon -> gadget)
	{
		RemoveGadget( win, (struct Gadget *) icon -> gadget );
		icon -> gadget = NULL;
	}

	if (icon -> image)
	{
		DisposeObject( (Object *) icon -> image );
		icon -> image = NULL;
	}
}


bool open_engine_window( int window_left, int window_top, int window_width, int window_height )
{
	if (fullscreen_screen)
	{
		window_left = 	(fullscreen_screen -> Width/2) - (window_width/2);
	}

	My_Window = OpenWindowTags( NULL,
				WA_PubScreen,       (ULONG) fullscreen_screen,
				WA_Left,			window_left,
				WA_Top,			fullscreen_screen ? 0 : window_top,
				WA_InnerWidth,		window_width,
				WA_InnerHeight,	window_height,

				WA_MinWidth, 	instance.video -> width, 
				WA_MinHeight,   instance.video -> height,
				WA_MaxWidth,      ~0,
				WA_MaxHeight,       ~0,  

				WA_SimpleRefresh,	TRUE,
				WA_CloseGadget,	fullscreen_screen ? FALSE : TRUE,
				WA_DepthGadget,	fullscreen_screen ? FALSE : TRUE,
				WA_DragBar,		fullscreen_screen ? FALSE : TRUE,
				WA_Borderless,	fullscreen_screen ? TRUE : FALSE,
				WA_SizeGadget,	fullscreen_screen ? FALSE : TRUE,
				WA_SizeBBottom,	fullscreen_screen ? FALSE : TRUE,
				WA_NewLookMenus,	TRUE,
				WA_Title,			fullscreen_screen ? NULL : "Amos Kittens",
				WA_Activate,		TRUE,
				WA_Flags,			WFLG_RMBTRAP| WFLG_REPORTMOUSE,
				WA_IDCMP,		IDCMP_COMMON,
			TAG_DONE);

	if ((My_Window) && ( fullscreen_screen == NULL ))
	{
		struct DrawInfo *dri = GetScreenDrawInfo(My_Window -> WScreen);

		if (dri)
		{
			open_icon( dri, GUPIMAGE, GID_FULLSCREEN, &zoomIcon );
			open_icon( dri, ICONIFYIMAGE, GID_ICONIFY, &iconifyIcon );
		}
	}

	return (My_Window != NULL) ;
}

void close_engine_window( )
{
	if (My_Window)
	{
		dispose_icon( My_Window, &zoomIcon);
		dispose_icon( My_Window, &iconifyIcon);
		ClearPointer( My_Window );
		CloseWindow( My_Window );
		My_Window = NULL;
	}

	if (comp_bitmap)
	{
		FreeBitMap(comp_bitmap);
		comp_bitmap = NULL;
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

	engine =  retroAllocEngine( My_Window, instance.video );

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
	BPTR engine_debug_output = Open("CON:850/100/600/480/Kittens engine",MODE_NEWFILE);
#else
	BPTR engine_debug_output = NULL;
#endif

	main_task = (struct Process *) FindTask(NULL);
	EngineTask = spawn( main_engine, "Amos kittens graphics engine",engine_debug_output);

	Wait(SIGF_CHILD);
	return ((EngineTask) && (instance.engine_stopped == false));
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
		for (int n=0;n<8;n++)
		{
			if (instance.screens[n] == screen)
			{
				Printf("fade screen %ld\n",n);
				break;
			}
		}

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
			struct retroRGB *fpal = screen -> fadePalette;

			for (n=0;n<256;n++)
			{
				dr = (int) fpal->r - (int) opal->r;
				dg = (int) fpal->g - (int) opal->g;
				db = (int) fpal->b - (int) opal->b;

//				if (n<32) Printf("%-3ld: %04lx,%04lx,%0l4x\n",n,fpal->r,fpal->g,fpal->b);

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
				fpal++;
			}

			screen -> fade_count = 1;
			if (changed == 0) screen -> fade_speed = 0;
		}
	}
}

// idea, use macros to range check if needed...

#if 1
	#define set(adr,v) *(adr)=v
	#define get(adr) *(adr)
#else
	#define set(adr,v) if ((( (char *) adr)>=(set_min)) && (( (char *) adr)<=(set_max))) { *(adr)=(v); } else { Printf("set out of range, min %08lx value %08lx max %08lx - ypos %ld\n",set_min,adr,set_max, ypos); }
	#define get(adr) ((( (char *) adr)>=(get_min)) &&(( (char *) adr)<=(get_max))) ? *(adr) : get_error_code;
	#define set_min ( (char *) instance.video -> Memory)
	#define set_max ( (char *)  instance.video -> Memory + (instance.video -> BytesPerRow * instance.video -> height))
	#define get_min ((char *) (frame -> data))
	#define get_max ((char *) frame -> data + ( frame -> bytesPerRow * frame -> height))
	#define get_error_code 0
#endif



void DrawSprite(
	int num,
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
	unsigned short c;
	unsigned short color_min = 0;
	unsigned int *destination_row_ptr;
	unsigned int *destination_row_ptr2;
	unsigned int *destination_row_start;
	unsigned char *source_row_start;
	unsigned char *source_row_ptr;
	unsigned char *source_row_end ;
	struct retroRGB *rgb = NULL;
	struct retroRGB *Nrgb = NULL;
	struct retroRGB *rgb2;
	unsigned int color;
	struct retroFrameHeader *frame;

	switch (num)
	{
		case 0:
		case 1: color_min = 16; break;
		case 2:
		case 3: color_min = 20; break;
		case 4:
		case 5: color_min = 24; break;
		case 6:
		case 7: color_min = 28; break;
	}

	if (image >= sprite -> number_of_frames) image = instance.sprites -> number_of_frames-1;
	if (image < 0) image = 0;

	frame = sprite -> frames + image;
	width = frame -> width ;
	height = frame -> height ;

	x = (item -> x / 2) -  frame -> XHotSpot;
	y = (item -> y / 2) - frame -> YHotSpot;	

	if (y>0)
	{
		if (y+height>= (int) (instance.video->height/2)) height = (instance.video->height/2) - y;
	}
	else
	{
		source_y0 = -y; y = 0; height -= source_y0;
		if (height>= (int) (instance.video->height/2)) height = (instance.video->height/2) ;	
	}

	if (x>0)
	{
		if (x+width>= (int) (instance.video->width/2)) width =(instance.video->width/2) - x;
	}
	else
	{
		source_x0 = -x; x = 0; width -= source_x0;
		if (width>= (int) (instance.video->width/2)) width =(instance.video->width/2) ;
	}

	destination_row_start =(unsigned int *) ((char *) instance.video -> Memory + (instance.video -> BytesPerRow * (y*2)) ) + (x*2);
	source_row_start = (unsigned char *) frame -> data + (source_y0 * frame -> bytesPerRow ) + source_x0;
	source_row_end = source_row_start + width;

	for ( ypos = 0; ypos < height; ypos++ )
	{
		Nrgb = instance.video -> scanlines[ypos + (y*2) ].scanline[0].orgPalette;
		if (Nrgb) rgb = Nrgb;

		if (rgb)
		{
			destination_row_ptr = destination_row_start;
			destination_row_ptr2 =  (unsigned int *) ((char *) destination_row_start + instance.video -> BytesPerRow );

			for ( source_row_ptr = source_row_start;  source_row_ptr < source_row_end ; source_row_ptr++ )
			{	
				c = get(source_row_ptr);
				if (c) 
				{
					c += color_min;
					if ( c < 256)
					{
						rgb2 = rgb + c;
						color = (rgb2->r << 16) | (rgb2->g<<8) | rgb2->b;

						set(destination_row_ptr,color);
						set(destination_row_ptr+1,color);
						set(destination_row_ptr2,color);
						set(destination_row_ptr2+1,color);
					}
				}

				destination_row_ptr+=2;
				destination_row_ptr2+=2;
			}
		}

		destination_row_start = (unsigned int *) ( (char *) destination_row_start + (instance.video -> BytesPerRow * 2));	// every 2en row
		source_row_start += frame -> bytesPerRow;
		source_row_end += frame -> bytesPerRow;
	}
}

#undef set
#undef get

extern void enable_Iconify();
extern void disable_Iconify();
extern void dispose_Iconify();

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

	instance.engine_mouse_hidden = !enable;
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


void open_fullscreen(ULONG ModeID)
{
	fullscreen_screen = OpenScreenTags ( NULL,
			SA_DisplayID,  ModeID,
			SA_Type, PUBLICSCREEN,
			SA_PubName, "kittens Screen",
			SA_Title, "Kittens Screen",
			SA_ShowTitle, FALSE,
			SA_Quiet, 	TRUE,
			SA_LikeWorkbench, TRUE,
		TAG_DONE);
}

void enable_fullscreen()
{
	double window_aspect;
	ULONG ModeID = 0x0;
	int max_w,max_h;

	max_w = 0;
	max_h = 0;

	engine_lock();

	window_save_state.win = My_Window;
	save_window_attr(&window_save_state);

	window_normal_saved.win = My_Window;
	save_window_attr(&window_normal_saved);

	close_engine_window();

	window_aspect = (double) window_save_state.window_width / (double) window_save_state.window_height;

	struct Screen *screen = LockPubScreen(NULL);
	if (screen)
	{
		if (ModeID == 0x0) ModeID = GetVPModeID(&screen->ViewPort);
		window_save_state.window_height = screen -> Height;
		window_save_state.window_width = window_aspect * (double) window_save_state.window_height;
		UnlockPubScreen(NULL,screen);
	}

	open_fullscreen(ModeID);

	open_engine_window(
		window_save_state.window_left,
		window_save_state.window_top,
		window_save_state.window_width,
		window_save_state.window_height);

	engine -> window = My_Window;
	engine_unlock();
}

void disable_fullscreen()
{
	engine_lock();
	close_engine_window();

	CloseScreen( fullscreen_screen);
	fullscreen_screen = NULL;

	open_engine_window(
		window_normal_saved.window_left,
		window_normal_saved.window_top,
		window_normal_saved.window_width,
		window_normal_saved.window_height);

	engine -> window = My_Window;
	engine_unlock();
}

void handel_engine_keys(ULONG ccode)
{
	switch (ccode)
	{
		case RAWKEY_F11:
			instance.engine_pal_mode = instance.engine_pal_mode ? false : true;
			break;					

		case RAWKEY_F12:
			if (fullscreen_screen) 
			{
				disable_fullscreen();
			}
			else
			{
				enable_fullscreen();
			}
			break;				
	}	
}

bool handel_keyboard_joypad(ULONG code, ULONG Qualifier)
{
	ULONG ccode = code & ~IECODE_UP_PREFIX;

	Printf("joy_keyboard_index: %lx Qualifier %0lx code %ld\n",joy_keyboard_index, Qualifier, ccode);

	if (joy_keyboard_index == -1) return false;

	if (Qualifier & 0x100 ) 
	{
		int joy_b = 0;
		int joy_d = 0;

		switch (ccode)
		{

			case 62:
			case 76: joy_d = joy_up; break;

			case 46: joy_d = joy_down; break;

			case 45: 
			case 79: joy_d = joy_left; break;

			case 47:
			case 78: joy_d = joy_right; break;

			case 67: joy_b = 1; break;
			case 15: joy_b = 2; break;
			case 60: joy_b = 3; break;
		}



		if (joy_d)
		{
			if (code & IECODE_UP_PREFIX)
			{
				amiga_joystick_dir[joy_keyboard_index] &= ~joy_d;
			}
			else
			{
				amiga_joystick_dir[joy_keyboard_index] |= joy_d;
			}
			return true;
		}

		if (joy_b)
		{
			if (code & IECODE_UP_PREFIX)
			{
				amiga_joystick_button[joy_keyboard_index] &= ~joy_b;
			}
			else
			{
				amiga_joystick_button[joy_keyboard_index] |= joy_b;
			}
			return true;
		}
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

							switch (GadgetID)
							{
								case GID_ICONIFY:

									empty_que( engine -> window -> UserPort );
									enable_Iconify(); 
									engine -> window = NULL;
									break;

								case GID_FULLSCREEN:
									enable_fullscreen(); 
									break;
							}
							return;

					case IDCMP_MOUSEBUTTONS:

							switch (Code)
							{
								case SELECTDOWN:	instance.engine_mouse_key |= 1; break;
								case SELECTUP:	instance.engine_mouse_key &= ~1; break;
								case MENUDOWN:	instance.engine_mouse_key |= 2; break;
								case MENUUP:		instance.engine_mouse_key &= ~2; break;
							}
							break;

					case IDCMP_MOUSEMOVE:

							{
								int ww;
								int wh;
								int ih;

								ih = instance.engine_pal_mode ? instance.video -> height : instance.video -> height * 5 / 6;

								if (fullscreen_screen)
								{
									ww = My_Window->Width ;
									wh = My_Window->Height ;
									instance.engine_mouse_x = mouse_x * instance.video -> width / ww;
									instance.engine_mouse_y = mouse_y * ih / wh;
								}
								else
								{
									ww = My_Window->Width - My_Window->BorderLeft - My_Window->BorderRight;
									wh = My_Window->Height - My_Window->BorderTop - My_Window->BorderBottom;
									mouse_x -= engine -> window -> BorderLeft;
									mouse_y -= engine -> window -> BorderTop;
									instance.engine_mouse_x = mouse_x * instance.video -> width / ww;
									instance.engine_mouse_y = mouse_y * ih / wh;
								}
							}

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

							ccode = Code & ~IECODE_UP_PREFIX;
							{
								ULONG engine_key = 0;

								switch (ccode)
								{
									case RAWKEY_F11:		
									case RAWKEY_F12:
										engine_key = ccode;
										break;
								}							

								if (handel_keyboard_joypad( Code, Qualifier )) break;

								if (engine_key) 
								{
									if (Code & IECODE_UP_PREFIX) handel_engine_keys(ccode);
									break;	// exit case..
								}
							}

							instance.engine_wait_key = false;

							if (menu_shortcut( Code ,  Qualifier)) break;

							_keyshift = Qualifier;
							if (Qualifier & IEQUALIFIER_REPEAT) break;		// repeat done by Amos KIttens...
							 
							{
								int emu_code = Code &~ IECODE_UP_PREFIX;
								if (emu_code==75) emu_code = 95;
								instance.engine_key_state[ emu_code ] = (Code & IECODE_UP_PREFIX) ? 0 : -1;
							}

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

void run_amal_scripts()
{
	if (channels)
	{
		struct kittyChannel *item;
		unsigned int i;

		for (i=0;i<channels -> _size();i++)
		{
			if (item = channels -> item(i))
			{
				if (item->amal_script)
				{

#ifdef show_debug_amal_yes

Printf("debug AMAL channel %ld object nr %ld\n",item -> id, item -> number);

if (item -> amalStatus & channel_status::active)
{
	Printf("script start>>\n%s\n<<script end\n",&(item->amal_script->ptr));
}
#endif
					 channel_amal( item );
				}

				if (item->anim_script) channel_anim( item );
				if (item->movex_script) channel_movex( item );
				if (item->movey_script) channel_movey( item );
			}
		}
	}
}

void main_engine()
{
	Printf("init engine\n");

	if (init_engine())		// libs open her.
	{
		Printf("init engine done..\n");
		


		ULONG sigs;
		ULONG joy_sig;
		ULONG ret;
		int n;
		
		Signal( &main_task->pr_Task, SIGF_CHILD );

		retroClearVideo(instance.video, instance.engine_back_color);

		init_usb_joysticks();

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
					if (joysticks[n].connected)
					{
						joy_stick(n,joysticks[n].controller);
					}
				}
			}
			else
			{
				struct joystick *joy;

				for (joy=joysticks;joy<joysticks+4;joy++)
				{
					switch (joy -> type)
					{
						case joy_usb:

								if (joy -> device_id)
								{
									AIN_Query(
										joy -> controller, 
										joy -> device_id,
										AINQ_CONNECTED,0,
										&joy -> connected,4 );
								}
								break;
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
				retroClearVideo( instance.video, instance.engine_back_color );

					engine_draw_vbl_Interrupts();
					engine_draw_bobs_and_do_vbl();

				retroDrawVideo( instance.video );

				if (synchro_on == true) 
				{
					run_amal_scripts();
				}

#if 1
				if ((instance.sprites)&&(instance.video -> sprites))
				{
					struct retroSpriteObject *item;

					for (n=0;n<64;n++)
					{
						item = &instance.video -> sprites[n];
						item -> sprite = instance.sprites;

						if (item -> image>0)
						{
							DrawSprite( n, instance.sprites, item, item -> image -1, 0 );
						}
					}
				}
#endif		
			}	// end if (autoView)

			engine_unlock();

			if (My_Window)
			{
				AfterEffectScanline( instance.video );
//				AfterEffectAdjustRGB( instance.video , 8, 0 , 4);
				retroDmaVideo( instance.video, engine );

				WaitTOF();
				if (sig_main_vbl) Signal( &main_task->pr_Task, 1<<sig_main_vbl );

#if	0
					BltBitMapTags(BLITA_SrcType, BLITT_BITMAP,
						BLITA_Source, engine->rp.BitMap,
						BLITA_SrcX, 0,
						BLITA_SrcY, 0,
						BLITA_Width,  instance.video -> width, 
						BLITA_Height, instance.video -> height,
						BLITA_DestType,  BLITT_RASTPORT,
						BLITA_Dest, My_Window->RPort,
						BLITA_DestX, My_Window->BorderLeft,
						BLITA_DestY, My_Window->BorderTop,
						TAG_END);
#else
					BackFill_Func( My_Window -> RPort, NULL );
#endif

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

	if (iconifyPort)  dispose_Iconify();

	if (fullscreen_screen) CloseScreen(fullscreen_screen);
	fullscreen_screen = NULL;

	instance.engine_stopped = true;
}

void engine_lock()
{
	MutexObtain(engine_mx);
}

void engine_unlock()
{
	MutexRelease(engine_mx);
}

bool engine_have_vbl_Interrupt( void (*fn) VBL_FUNC_ARGS )		// is not atomic.
{
	engine_lock();
	unsigned int i;
	for (i = 0; i < kittyVblInterrupts.size(); i++)
	{
		if (kittyVblInterrupts[i].fn == fn) 
		{
			engine_unlock();
			return true;
		}
	}
	engine_unlock();
	return false;
}

void engine_add_vbl_Interrupt( void (*fn) VBL_FUNC_ARGS, void *custom )
{
	if (engine_have_vbl_Interrupt(fn) == false)
	{
		struct kittyVblInterrupt new_Interrupt;

		engine_lock();
		new_Interrupt.fn = fn;
		new_Interrupt.custom = custom;
		kittyVblInterrupts.push_back(new_Interrupt);
		engine_unlock();
	}
}


void engine_remove_vbl_Interrupt( void (*fn) VBL_FUNC_ARGS )
{
	unsigned int i;
	engine_lock();
	for (i = 0; i < kittyVblInterrupts.size(); i++)
	{
		if (kittyVblInterrupts[i].fn == fn)
		{
			kittyVblInterrupts.erase(kittyVblInterrupts.begin()+i);
			break;
		}
	}
	engine_unlock();
}

void engine_draw_vbl_Interrupts()
{
	unsigned int i;
	for (i = 0; i < kittyVblInterrupts.size(); i++)
	{
		kittyVblInterrupts[i].fn( kittyVblInterrupts[i].custom );
	}
}

void engine_draw_bobs_and_do_vbl()
{
	int n;
	struct retroScreen *screen;

	for (n=0; n<8;n++)
	{
		screen = instance.screens[n];

		if (screen)
		{
			retroFadeScreen_beta(screen);

//			Printf("screen id: %ld, flags %08lx,%08lx\n",n, screen -> event_flags , engine_update_flags);
//			dump_channels();
//			dump_anim();

			if (screen -> event_flags & engine_update_flags)
			{
				// dump_bobs_on_screen( n );
				clearBobsOnScreen(screen);
				drawBobsOnScreenExceptBob(screen,NULL);

				if (screen -> Memory[1]) 	// has double buffer
				{
					swap_buffer( screen );
				}
			}
			screen -> event_flags = 0;
		}
	}	// next
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

