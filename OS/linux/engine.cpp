#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <vector>

#include <stdint.h>
#include <unistd.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>

#include <thread>
#include <mutex>
#include <signal.h>
#include <pthread.h>

#include "amoskittens.h"
#include "joysticks.h"
#include "engine.h"
#include "bitmap_font.h"
#include "debug.h"
#include "amalcompiler.h"
#include "channel.h"
#include "spawn.h"
#include "init.h"


using namespace std;


FILE *engine_fd = NULL; 

extern int sig_main_vbl;
extern bool running;			// 
extern bool interpreter_running;	// interprenter is really running.
extern int keyState[256];
extern char *F1_keys[20];

extern struct Menu *amiga_menu;

extern struct retroSprite *sprite;

struct TextFont *topaz8_font = NULL;
struct Process *EngineTask = NULL;

bool engine_wait_key = false;
bool engine_stopped = false;

extern bool curs_on;
extern int _keyshift;

static std::mutex engine_mx ;

extern ChannelTableClass *channels;
vector<struct keyboard_buffer> keyboardBuffer;
vector<struct amos_selected> amosSelected;


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

extern struct retroScreen *screens[8] ;

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
	IDCMP_EXTENDEDMOUSE | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE | IDCMP_INTUITICKS | IDCMP_MENUPICK

extern struct retroVideo *video;


bool open_window( int window_width, int window_height )
{

	return NULL ;
}

bool init_engine()
{
	char name[200];

	sprintf(name,"/var/log/AmotKittens_engine_pid_%d.log",getpid());

	engine_fd = fopen(name,"a+");

	return FALSE;
}

void close_engine()
{
	if (engine_fd) fclose(engine_fd);
	engine_fd = NULL;
}

void main_engine();


bool start_engine()
{
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

#define limit_step( step ) \
		if ( step <-0x11) step=-0x11; \
		if ( step >0x11) step=0x11; \


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
			int changed_at = 0;
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

				changed_at = dr | dg | db;

				opal->r += dr;
				opal->g += dg;
				opal->b += db;

				*rpal =*opal;

				opal++;
				rpal++;
				npal++;
			}

			screen -> fade_count = 1;
			if (changed_at == 0)
			{
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
	engine_stopped = true;
}


void engine_lock()
{
	engine_mx.lock();
}

void engine_unlock()
{
	engine_mx.unlock();
}
