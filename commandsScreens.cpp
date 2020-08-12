#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <iostream>


#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#include <proto/datatypes.h>
#include <datatypes/pictureclass.h>

// undo stupid changes in the SDK.

#define xAspect bmh_XAspect
#define yAspect bmh_YAspect
#endif

#ifdef __linux__
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#endif

#include <amosKittens.h>
#include <stack.h>

#include "debug.h"
#include "commandsScreens.h"
#include "commandsBlitterObject.h"
#include "KittyErrors.h"
#include "engine.h"
#include <math.h>

extern struct retroRGB DefaultPalette[256];

extern struct retroTextWindow *newTextWindow( struct retroScreen *screen, int id );
extern void freeAllTextWindows(struct retroScreen *screen);

extern void __erase_bobs_on_screen__(int screen_id);

#define true_lowres 0x0
#define true_hires 0x8000
#define true_laced 0x4

extern struct fileContext *kittensFile;

unsigned int amosModeToRetro(unsigned int aMode)
{
	unsigned int retMode = 0;

	switch (aMode)
	{
		case true_lowres: 
			retMode = retroLowres; break;

		case true_hires:
			 retMode = retroHires; break;

		case (true_lowres | true_laced): 
			retMode = retroLowres | retroInterlaced; break;

		case (true_hires | true_laced): 
			retMode = retroHires | retroInterlaced; break;
	 }

	return retMode;
}

unsigned int retroModeToAmosMode(unsigned int rMode)
{
	unsigned int retMode = 0;

	if (rMode & retroLowres) retMode |= true_lowres;
	if (rMode & retroLowres_pixeld) retMode |= true_lowres;
	if (rMode & retroHires) retMode |= true_hires;
	if (rMode & retroInterlaced) retMode |= true_laced;

	return retMode;
}

void AmigaModeToRetro( unsigned int modeid, ULONG *retMode, float *aspect )
{
	unsigned int encoding = modeid & (0x800 | 0x1000 | 0x8000);
	*retMode = 0;
	float xdpi = 40;
	float ydpi = 40;

	switch (encoding)
	{
		case 0x0800:
				*retMode |= retroHam6;
				break;

		case 0x8800:					// this is set in HAM8 file I'm testing.
				*retMode |= retroHam8;
				break;
		case 0x1000:					// this was set in anim8 file I'm testing.
				*retMode |= retroHam6;
				break;
	}

	if (modeid & (0x0100 | 0x0008 | 0x0004)) 
	{
		*retMode |= retroInterlaced;
		ydpi = 20;
		printf("Interlaced\n");
	}

	if (modeid & 0x8000)
	{
		*retMode |= retroHires;
		xdpi = 20;
		printf("hires\n");
	}
	else
	{
		*retMode |= retroLowres;
		printf("lowres\n");
	}

	*aspect = (1.0f / xdpi) / (1.0f / ydpi);
}


int	physical( retroScreen *screen )
{
	return  (screen ->Memory[1]) ? 1-screen -> double_buffer_draw_frame : 0;
}


int	logical( retroScreen *screen )
{
	return screen -> double_buffer_draw_frame;
}


void init_amos_kittens_screen_default_text_window( struct retroScreen *screen, int colors )
{
	struct retroTextWindow *textWindow = NULL;

	if (colors == 2)
	{
		screen->pen = 1;
		screen->paper = 0;
		screen->ink0 = 1;
		screen->ink1= 0;
		screen->ink2= 0;
	}
	else
	{
		screen->pen = 2;
		screen->paper = 1;
		screen->ink0 = 2;
		screen->ink1= 0;
		screen->ink2= 0;
	}

	if (textWindow = newTextWindow( screen, 0 ))
	{
		textWindow -> charsPerRow = screen -> realWidth / 8;
		textWindow -> rows = screen -> realHeight / 8;
		screen -> currentTextWindow = textWindow;
	}
}

void init_amos_kittens_screen_default_colors(struct retroScreen *screen)
{
	set_default_colors( screen );
	retroFlash( screen, 3, (char *) "(100,5),(200,5),(300,5),(400,5),(500,5),(600,5)(700,5),(800,5),(900,5),(A00,5),(B00,5),(A00,5),(900,5),(800,5),(700,5),(600,5),(500,5)(400,5),(300,5),(200,5)");
	retroBAR( screen, 0, 0,0, screen -> realWidth,screen->realHeight, screen -> paper );
}

char *_gfxScreenOpen( struct glueCommands *data, int nextToken )
{
	int args = instance.stack - data->stack +1 ;
	bool success = false;
	int colors = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==5)
	{
		int screen_num = getStackNum(__stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			int mode = 0;
			struct retroScreen *screen;
			instance.current_screen = screen_num;

			colors = getStackNum(__stack-1 );
			mode = amosModeToRetro(getStackNum(__stack ));

			if (colors == 4096) mode |= retroHam6 ;
			if (colors == -6) mode |= retroHam6 ;
			if (colors == -8) mode |= retroHam8 ;

			engine_lock();
			if (instance.screens[screen_num]) retroCloseScreen(&instance.screens[screen_num]);

			instance.screens[screen_num] = retroOpenScreen(getStackNum(__stack-3 ),getStackNum(__stack-2 ),mode);
			if (screen = instance.screens[screen_num])
			{
				init_amos_kittens_screen_default_text_window(screen, colors);
				init_amos_kittens_screen_default_colors(screen);
				draw_cursor(screen);
				retroApplyScreen( screen, instance.video, 0, 0, screen -> realWidth,screen->realHeight );
			}
			engine_unlock();

			success = true;
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( instance.stack - data->stack );
	return NULL;
}

// make sure there is standard way to close screens, that take care of clean up.


void  __kitten_screen_close( int screen_num,retroScreen **screen )
{
	freeAllTextWindows( *screen  );
	freeScreenBobs( screen_num );
	retroCloseScreen( screen );

	// find a open screen, and set current screen to that.
	if (screen_num == instance.current_screen)
	{
		int n;
		for (n=7; n>-1;n--)
		{
			if (instance.screens[n])
			{
				instance.current_screen = n;
				break;
			}
		}
	}
}

bool kitten_screen_close_atomic(int screen_num )
{
	if ((screen_num>-1)&&(screen_num<8))
	{
		engine_lock();
		__kitten_screen_close( screen_num,&instance.screens[screen_num] );
		engine_unlock();
		return true;
	}

	return false;
}

char *_gfxScreenClose( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = getStackNum(__stack );
		success = kitten_screen_close_atomic( screen_num );
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack(__stack - data->stack );
	return NULL;
}

char *gfxScreenClose(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenClose, tokenBuffer );
	return tokenBuffer;
}

void copy_pal(	struct retroRGB *source_pal,	struct retroRGB *dest_pal)
{
	int n;
	for (n=0; n<256;n++)
	{
		*dest_pal=*source_pal;
		source_pal++;
		dest_pal++;
	}
}

char *_gfxScreenClone( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = getStackNum(__stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (instance.screens[instance.current_screen])	// check if current screen is open.
			{
				engine_lock();
				if (instance.screens[screen_num]) retroCloseScreen(&instance.screens[screen_num]);

				instance.screens[screen_num] = retroScreenClone(instance.screens[instance.current_screen], instance.screens[instance.current_screen] -> videomode );

				if (instance.screens[screen_num])
				{
					instance.screens[screen_num] -> fade_speed = 0;
					instance.screens[screen_num] -> fade_count = 0;

					copy_pal( instance.screens[instance.current_screen] -> orgPalette, instance.screens[screen_num] -> orgPalette );
					copy_pal( instance.screens[instance.current_screen] -> rowPalette, instance.screens[screen_num] -> rowPalette );
					copy_pal( instance.screens[instance.current_screen] -> fadePalette, instance.screens[screen_num] -> fadePalette );

					retroApplyScreen( instance.screens[screen_num], instance.video, 
							instance.screens[screen_num]->scanline_x, 
							instance.screens[screen_num]->scanline_y, 
							instance.screens[screen_num]->displayWidth, 
							instance.screens[screen_num]->displayHeight );

					instance.video -> refreshAllScanlines = TRUE;
				}

				engine_unlock();

				instance.current_screen = screen_num;
				success = true;
			}
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack(__stack - data->stack );
	return NULL;
}


char *_gfxScreenDisplay( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==5)
	{
		int screen_num = getStackNum(__stack-4 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			struct retroScreen *screen; 

			engine_lock();
			if (screen = instance.screens[screen_num])
			{
				// can't change screen scanline_x,y direct its read only.

				int tmp_scanline_x  = screen -> scanline_x;
				int tmp_scanline_y = screen -> scanline_y;
				int tmp_displayWidth  = screen -> displayWidth;
				int tmp_displayHeight = screen -> displayHeight;

				if (kittyStack[__stack-3].type ==  type_int) tmp_scanline_x = (getStackNum(__stack-3 )-128)*2;
				if (kittyStack[__stack-2].type ==  type_int) tmp_scanline_y = (getStackNum(__stack-2 )- 50)*2;
				if (kittyStack[__stack-1].type ==  type_int) tmp_displayWidth = getStackNum(__stack-1 );
				if (kittyStack[__stack].type ==  type_int) tmp_displayHeight = getStackNum(__stack );

				// This function compares input values, with what is stored inside of screen struct, 
				// do not change screen struct values manually... they should be private.

				retroApplyScreen( screen, instance.video, 
					tmp_scanline_x,
					tmp_scanline_y,
					tmp_displayWidth,
					tmp_displayHeight );
			}
			else
			{
				setError(47,data->tokenBuffer);	// Screen not open.
			}

			engine_unlock();
			success = true;
		}
	}

	if (success == false) setError(22,data->tokenBuffer);
	popStack(__stack - data->stack );

	return NULL;
}

char *_gfxScreenOffset( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==3)
	{
		int screen_num = getStackNum(__stack-2 );

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (kittyStack[__stack-1].type ==  type_int) instance.screens[screen_num] -> offset_x = getStackNum(__stack-1 );
			if (kittyStack[__stack].type ==  type_int) instance.screens[screen_num] -> offset_y = getStackNum(__stack );
 			instance.screens[screen_num] -> refreshScanlines = TRUE;
			instance.video -> refreshSomeScanlines = TRUE;
			success = true;
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack(__stack - data->stack );
	setStackNum(ret);
	return NULL;
}



char *_gfxScin( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int ret = -1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==2)
	{
//		int mx = getStackNum(__stack-1 );
		int my = getStackNum(__stack );

		if ((my>-1)&&(my<480))
		{
			struct retroScreen *s = NULL;
			struct retroScanline *scanline = &instance.video -> scanlines[my].scanline[0];
			int n;

			if ( scanline -> data)
			{
				s = scanline -> screen;
			}
			else if (my>0)
			{
				s = scanline -> screen;
			}

			if (s)
			{
				for (n=0;n<8;n++)
				{
					if ( instance.screens[n] == s )
					{
						ret =n;
						break;
					}
				}
			}
		}
	}

	popStack(__stack - data->stack );
	setStackNum( ret );
	return NULL;
}


char *gfxScreenOpen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenOpen, tokenBuffer );
	return tokenBuffer;
}

char *gfxLowres(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum( true_lowres );
	return tokenBuffer;
}

char *gfxHires(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum( true_hires );
	return tokenBuffer;
}

char *gfxScreenDisplay(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenDisplay, tokenBuffer );
	return tokenBuffer;
}

char *gfxScreenOffset(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenOffset, tokenBuffer );
	return tokenBuffer;
}

char *_gfxScreen( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = getStackNum(__stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (instance.screens[screen_num])
			{
				instance.current_screen = screen_num;
				success = true;
			}
		}

		if (success == false) setError(47,data->tokenBuffer);
	}
	else setError(22,data->tokenBuffer);

	popStack(__stack - data->stack );
	return NULL;
}

char *gfxScreen(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( (getLastProgStackToken() == token_trap ) || (instance.token_is_fresh) )
	{
		stackCmdNormal( _gfxScreen, tokenBuffer );
	}
	else
	{
		unsigned short next_token = *((short *) (tokenBuffer) );
		setStackNum( instance.screens[instance.current_screen] ? instance.current_screen : -1 );		// returns -1 if no screen is open.	
		kittyStack[__stack].state = state_none;
		flushCmdParaStack( next_token );
	}

	return tokenBuffer;
}

char *gfxScreenClone(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenClone, tokenBuffer );
	return tokenBuffer;
}

char *gfxScreenColour(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum(256);
	return tokenBuffer;
}

char *_gfxScreenWidth( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = getStackNum(__stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			popStack(__stack - data->stack );
			setStackNum(instance.screens[screen_num] -> realWidth);
			return NULL;
		}

		setError(47,data->tokenBuffer);
	}
	else setError(22,data->tokenBuffer);

	popStack(__stack - data->stack );
	return NULL;
}


char *gfxScreenWidth(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);

	if (next_token == 0x0074)
	{
		stackCmdParm( _gfxScreenWidth, tokenBuffer );
	}
	else if (instance.screens[instance.current_screen])	// check if current screen is open.
	{
		setStackNum(instance.screens[instance.current_screen] -> realWidth);
	}
	return tokenBuffer;
}


char *_gfxScreenHeight( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = getStackNum(__stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			popStack(__stack - data->stack );
			setStackNum(instance.screens[screen_num] -> realHeight);
			return NULL;
		}

		if (success == false) setError(47,data->tokenBuffer);
	}
	else setError(22,data->tokenBuffer);

	popStack(__stack - data->stack );
	return NULL;
}

char *gfxScreenHeight(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);

	if (next_token == 0x0074)
	{
		stackCmdParm( _gfxScreenHeight, tokenBuffer );
	}
	else if (instance.screens[instance.current_screen])	// check if current screen is open.
	{
		setStackNum(instance.screens[instance.current_screen] -> realHeight);
	}
	return tokenBuffer;
}

char *gfxGetScreen(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (instance.screens[instance.current_screen])	// check if current screen is open.
	{
		setStackNum(instance.current_screen);
	}
	else 	setStackNum(-1);

	return tokenBuffer;
}

char *gfxScin(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxScin, tokenBuffer );
	return tokenBuffer;
}

char *_gfxScreenToFront( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = getStackNum(__stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (instance.screens[screen_num])
			{
				retroScreenToFront(instance.screens[screen_num]);
				instance.video -> refreshAllScanlines = TRUE;
				success = true;
			}
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack(__stack - data->stack );
	return NULL;
}

char *gfxScreenToFront(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _gfxScreenToFront, tokenBuffer );
	return tokenBuffer;
}

char *_gfxScreenToBack( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = 0;

		if (kittyStack[__stack].type != type_none)
		{
			screen_num = getStackNum(__stack );
		}
		else screen_num = instance.current_screen;

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (instance.screens[screen_num])
			{
				retroScreenToBack(instance.screens[screen_num]);
				instance.video -> refreshAllScanlines = TRUE;
				success = true;
			}
		}
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack(__stack - data->stack );
	return NULL;
}

char *gfxScreenToBack(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _gfxScreenToBack, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_gfxScreenShow( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num =-1;
		bool success = false;

		switch (kittyStack[__stack].type)
		{ 
			case type_none:
					screen_num = instance.current_screen;
					break;

			case type_int:
					screen_num = getStackNum(__stack );
					break;
		}

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (instance.screens[screen_num])
			{
				instance.screens[screen_num]->flags &= ~retroscreen_flag_hide;
				instance.video -> refreshAllScanlines = TRUE;
				success = true;
			}
		}

		if (success == false) setError(47,data->tokenBuffer);
	}
	else setError( 22, data -> tokenBuffer );

	popStack(__stack - data->stack );
	return NULL;
}

char *_gfxScreenHide( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num = getStackNum(__stack );

		if ((screen_num>-1)&&(screen_num<8))
		{
			if (instance.screens[screen_num])
			{
				instance.screens[screen_num]->flags |= retroscreen_flag_hide;
				instance.video -> refreshAllScanlines = TRUE;
				success = true;
			}
		}

		if (success == false) setError(47,data->tokenBuffer);
	}
	else setError(22,data->tokenBuffer);

	popStack(__stack - data->stack );
	return NULL;
}

char *_gfxScreenCopy( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	struct retroScreen *src_screen = NULL;
	struct retroScreen *dest_screen = NULL;
	int src_buffer, dest_buffer;

	uint32_t src_screen_flags = 0;
	uint32_t dest_screen_flags = 0;

	uint32_t src_screen_nr = 0;
	uint32_t dest_screen_nr = 0;

	int src_x0 = 0;
	int src_y0 = 0;
	int src_x1 = 0;
	int src_y1 = 0;
	int dest_x = 0;
	int dest_y = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);


	switch (args)
	{
		case 2:	// Screen Copy {source} to {dest}

				src_screen_flags = getStackNum(__stack-1 );
				dest_screen_flags = getStackNum(__stack );

				src_screen_nr = src_screen_flags & 0xFF;
				dest_screen_nr = dest_screen_flags & 0xFF;

				if ((src_screen_nr>=0)&&(src_screen_nr<8)&&(dest_screen_nr>=0)&&(dest_screen_nr<8))
				{
					src_screen = instance.screens[src_screen_nr];
					src_x1 = src_screen->realWidth-1;
					src_y1 = src_screen->realHeight-1;
					dest_screen = instance.screens[dest_screen_nr];
				}

				break;

		case 8:	// Screen Copy {source},x0,y0,x1,y1 to {dest},x,y
			
				src_screen_flags = getStackNum(__stack-7 );
				dest_screen_flags = getStackNum(__stack-2 );

				src_screen_nr = src_screen_flags & 0xFF;
				dest_screen_nr = dest_screen_flags & 0xFF;

				src_x0 = getStackNum(__stack-6 );
				src_y0 = getStackNum(__stack-5 );
				src_x1 = getStackNum(__stack-4 );
				src_y1 = getStackNum(__stack-3 );
				dest_x = getStackNum(__stack-1 );
				dest_y = getStackNum(__stack );

				if ((src_screen_nr>=0)&&(src_screen_nr<8)&&(dest_screen_nr>=0)&&(dest_screen_nr<8))
				{
					src_screen = instance.screens[src_screen_nr];
					dest_screen = instance.screens[dest_screen_nr];
				}	
				break;

		default:
				printf("args: %d\n",args);
	 			setError(22,data->tokenBuffer);
				popStack(__stack - data->stack );
				return NULL;
	}

	if ( (src_screen) && (dest_screen) )
	{
		src_buffer = ((src_screen_flags & 0xFFFFFF00) == 0xC0000000) ? physical( src_screen ) : logical( src_screen );
		dest_buffer = ((dest_screen_flags & 0xFFFFFF00) == 0xC0000000) ? physical( dest_screen ) : logical( dest_screen );

		retroScreenBlit( src_screen, src_buffer ,src_x0, src_y0, src_x1-src_x0, src_y1-src_y0, dest_screen, dest_buffer, dest_x, dest_y);
	}
	else setError(47,data->tokenBuffer);

	popStack(__stack - data->stack );
	return NULL;
}

char *gfxScreenShow(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _gfxScreenShow, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *gfxScreenHide(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _gfxScreenHide, tokenBuffer );
	return tokenBuffer;
}

char *gfxScreenCopy(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenCopy, tokenBuffer );
	return tokenBuffer;
}

void grayScalePalette( struct retroScreen *screen, int colors )
{
	int c;
	for (c=0;c<colors;c++)		
	{
		retroScreenColor(screen,c,c,c,c);
	}
}

void argbToGrayScale(struct RastPort *rp, int y, struct retroScreen *screen)
{
	int x;
	int luminate;
	uint32_t argb;
	unsigned char *mem = screen -> Memory[screen -> double_buffer_draw_frame]; 

	for (x=0;x<screen->realWidth;x++)
	{
		argb = ReadPixelColor(rp,x,y);

		luminate = (((argb & 0xFF0000) >> 16)
				+ ((argb & 0xFF00) >> 8)
				+ (argb & 0xFF)) / 3; 

		retroPixel( screen, mem, x,y, luminate );
	}
}

void floydPalette( struct retroScreen *screen, int colors )
{
	int c;
	int r,g,b;

	for (c=0;c<256;c++)		
	{
		r =	(( c >> 0) & 3) * 255 / 3;
		g =	(( c >> 2) & 7) * 255 / 7;
		b =	(( c >> 5) & 3) * 255 / 3;

		retroScreenColor(screen,c,r,g,b);
	}
}

extern void get_most_used_colors(struct RastPort *rp, int w, int h, struct retroScreen *screen);

extern void floydChannel( double *image, int w, int h );

extern void floyd(struct RastPort *rp, int w, int h, struct retroScreen *screen);

void dmode( const char *name, uint64_t mode )
{
	uint64_t n = 1;

	while (n<= 0x80000000)
	{
		if (mode & n) printf("%08llx: %s %08llx\n",n,name, mode );
		n=n<<1;
	}
}


extern char *get_name(char *path,char *name);		// found in include.cpp file

void LoadIff( const char *org_name,  int sn )
{
	struct DataType *dto = NULL;
	struct BitMapHeader *bm_header;
	struct BitMap *dt_bitmap;
	struct ColorRegister *cr;
	ULONG modeid = 0; 
	ULONG colors;
	ULONG bformat;
	ULONG mode;
	char *name;

	// new screen if screen number is set, or if current screen is not open.

	BOOL new_screen = (sn >-1 ) || (instance.screens[instance.current_screen] == NULL);

	// lets make sure SN is valid range.
	if (sn<0) sn = instance.current_screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	name = get_name( kittensFile -> path, (char *) org_name );

	if(dto = (struct DataType *) NewDTObject( name, DTA_GroupID, GID_PICTURE, TAG_DONE))
	{
		SetDTAttrs ( (Object*) dto, NULL,NULL,	PDTA_DestMode, (ULONG) PMODE_V43,TAG_DONE);
		DoDTMethod ( (Object*) dto,NULL,NULL,DTM_PROCLAYOUT,NULL,TRUE); 
		GetDTAttrs ( (Object*) dto,PDTA_BitMapHeader, (ULONG *) &bm_header, 	
					PDTA_BitMap, (ULONG) &dt_bitmap, 
					PDTA_ColorRegisters, &cr,
					PDTA_NumColors, &colors,
					PDTA_ModeID, &modeid,
					TAG_DONE);

		bformat = GetBitMapAttr(dt_bitmap,BMA_PIXELFORMAT);

		printf("colors %d\n",colors);
		printf("mode id %08x\n",modeid);
		printf("bformat %d\n",bformat);
		printf("%d,%d\n",bm_header -> bmh_Width,bm_header -> bmh_Height);
	
		if (modeid != (ULONG) ~0)
		{
			int xAspect = bm_header -> xAspect -  bm_header -> xAspect % 10;	
			int yAspect =  bm_header -> yAspect - bm_header -> yAspect % 10;

			float expected_aspect;
			float image_aspect;


			image_aspect = xAspect ?  ( 1.0f / (float) xAspect) / (1.0f / (float) yAspect) : 0.0f;

			switch (bformat)
			{
				case PIXF_NONE:
				case PIXF_CLUT:
					AmigaModeToRetro (modeid , &mode, &expected_aspect);

					if (image_aspect) if (image_aspect != expected_aspect)
					{
						if ((modeid == 0x8000)&&(image_aspect==1.0f))
						{
							mode = retroLowres;
							break;
						}
					}
					break;	
				default:
					mode = (bm_header -> bmh_Width>=640) ? retroHires : retroLowres;
					mode |= (bm_header -> bmh_Height>256) ? retroInterlaced : 0;
			}
		}
		else
		{
			mode = (bm_header -> bmh_Width>=640) ? retroHires : retroLowres;
			mode |= (bm_header -> bmh_Height>256) ? retroInterlaced : 0;
		}
		engine_lock();

		if (new_screen)
		{
			printf("its a new screen\n");

			if (instance.screens[sn])
			{
				printf("we are closing the old screen\n");
				 __kitten_screen_close( sn, &instance.screens[sn] );
			}

			printf("we are opening a new screen on %d\n",sn);

			instance.screens[sn] = retroOpenScreen(bm_header -> bmh_Width,bm_header -> bmh_Height, mode);
		}
		else sn = instance.current_screen;

		printf("screen id: %d\n", sn);

		if (instance.screens[sn])
		{
			unsigned int c;
			struct RastPort rp;
			int x,y;
			InitRastPort(&rp);

			printf("yes it looks like we can do whit, screen is open\n");

			retroBAR( instance.screens[sn], 0, 0,0, instance.screens[sn] -> realWidth,instance.screens[sn]->realHeight, instance.screens[sn] -> paper );

			if (new_screen)
			{
				printf("this new screen so we need to more stuff\n");

				init_amos_kittens_screen_default_text_window(instance.screens[sn], 256);
				retroApplyScreen( instance.screens[sn], instance.video, 0, 0, instance.screens[sn] -> realWidth,instance.screens[sn]->realHeight );
				set_default_colors( instance.screens[sn] );
				instance.current_screen = sn;
			}

			rp.BitMap = dt_bitmap;

			if (cr)
			{
				if ((bformat==PIXF_NONE) || (bformat==PIXF_CLUT))
				{
					for (c=0;c<colors;c++)		
					{
						retroScreenColor(instance.screens[sn],c,cr[c].red,cr[c].green,cr[c].blue);
					}
				}
				else
				{
					colors = 256;

					grayScalePalette( instance.screens[sn], colors );

					get_most_used_colors( &rp, instance.screens[sn]->realHeight,  instance.screens[sn]->realWidth, instance.screens[sn]);
				}
			}

			if ((bformat==PIXF_NONE) || (bformat==PIXF_CLUT))
			{
				for (y=0;y<instance.screens[sn]->realHeight;y++)
				{
					for (x=0;x<instance.screens[sn]->realWidth;x++)
					{
						retroPixel( instance.screens[sn], instance.screens[sn] -> Memory[0], x,y, ReadPixel(&rp,x,y));
					}
				}
			}
			else
			{
//				floyd( &rp, instance.screens[sn]->realWidth,  instance.screens[sn]-> realHeight , instance.screens[sn] );

				for (y=0;y<instance.screens[sn]->realHeight;y++)
				{
					argbToGrayScale( &rp, y, instance.screens[sn] );
				}
			}
		}

		DisposeDTObject((Object*) dto);
		engine_unlock();
	}

	if (name) free(name);
}

void copy_palette(int bformat, struct ColorRegister *cr ,struct RastPort *rp,  struct retroScreen *screen , ULONG *colors )
{
	ULONG c;

	if (bformat==PIXF_NONE)
	{
		for (c=0;c<*colors;c++)		
		{
			retroScreenColor(screen,c,cr[c].red,cr[c].green,cr[c].blue);
		}
	}
	else
	{
		*colors = 256;
		grayScalePalette( screen, *colors );
		get_most_used_colors( rp, screen->realHeight,  screen->realWidth, screen);
	}
}

void convert_bitmap(int bformat, struct RastPort *rp, struct retroScreen *screen )
{
	int x,y;

	if (bformat==PIXF_NONE)
	{
		for (y=0;y<screen->realHeight;y++)
		{
			for (x=0;x<screen->realWidth;x++)
			{
				retroPixel( screen, screen -> Memory[0], x,y, ReadPixel(rp,x,y));
			}
		}
	}
	else
	{
//		floyd( rp, screen->realWidth,  screen-> realHeight , screen );

		for (y=0;y<screen->realHeight;y++)
		{
			argbToGrayScale( rp, y, screen );
		}
	}
}





void SaveIff( char *name, const int n )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

#if 1

	struct DataType *dto = NULL;
	struct BitMapHeader *bm_header;
	struct BitMap *dt_bitmap;
	struct ColorRegister *cr;
	struct RastPort rp;
	int maxColor = 0;

	printf("we try to create a datatype\n");

	dt_bitmap = AllocBitMap(
				instance.screens[n] -> realWidth,
				instance.screens[n]-> realHeight,
				8,BMF_CLEAR, NULL);

	if (dt_bitmap)
	{
		int col = 0;
		int x,y;
		dto = (struct DataType *) NewDTObject( NULL, 
				DTA_SourceType, DTST_RAM,
				DTA_GroupID,GID_PICTURE,
				PDTA_BitMap, dt_bitmap,
				PDTA_NumColors, 256, 
				TAG_DONE);

		InitRastPort(&rp);
		rp.BitMap = dt_bitmap;
		
		for (y=0;y<instance.screens[n]-> realHeight;y++)
		for (x=0;x<instance.screens[n] -> realWidth;x++)
		{
			col = retroPoint(instance.screens[n],x,y);
			SetAPen(&rp, col );
			WritePixel(&rp,x,y);
			maxColor = col > maxColor ? col : maxColor;
		}
	}

	if ((dt_bitmap)&&(dto))
	{
		GetDTAttrs( (Object *) dto, PDTA_BitMapHeader, &bm_header, PDTA_ColorRegisters, &cr,	TAG_END);

		if (cr)
		{
			int colors = 256;
			int c;
			struct retroRGB *pal = instance.screens[n]->orgPalette;

			for (c=0;c<colors;c++)		
			{
				cr[c].red = pal[c].r ;
				cr[c].green = pal[c].g ;
				cr[c].blue = pal[c].b ;
			}
		}

		if (bm_header)
		{
			bm_header -> bmh_Width = instance.screens[n] -> realWidth;
			bm_header -> bmh_Height = instance.screens[n]-> realHeight;

			bm_header -> bmh_Depth = 1;
			while ( (1 << bm_header -> bmh_Depth) < maxColor ) bm_header -> bmh_Depth <<= 1;

			bm_header -> xAspect = (instance.screens[n]->videomode & retroHires) ? 11 : 22;
			bm_header -> yAspect = (instance.screens[n]->videomode & retroInterlaced) ? 11 : 22;
		}

		SaveDTObject( (Object*) dto, NULL,NULL, name, FALSE, TAG_END );
	}

	if (dto)	// if datatype object is created bitmap is attached to it, and will be freed here.
	{
		 DisposeDTObject((Object*) dto);
	}
	else 	if (dt_bitmap) FreeBitMap(dt_bitmap);

#endif

}

char *_gfxLoadIff( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	// load iff image to current screen.
				{
					struct stringData *name= getStackString(__stack );
					if (name)	LoadIff(&(name->ptr),-1);
				}
				break;
		case 2:	// load iff image to new screen.
				{
					struct stringData *name= getStackString(__stack -1);
					int screen_num = getStackNum(__stack );
					if (name)	LoadIff( &(name->ptr),screen_num);
				}
				break;
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *gfxLoadIff(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxLoadIff, tokenBuffer );
	return tokenBuffer;
}


char *_gfxSaveIff( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	// save iff image from current screen.
				{
					struct stringData *name= getStackString(__stack );
					if (name)	SaveIff( &(name->ptr),instance.current_screen);
				}
				break;
		case 2:	// save iff image from screen X.
				{
					struct stringData *name= getStackString(__stack -1);
					int screen_num = getStackNum(__stack );
					if (name)	SaveIff( &(name->ptr),screen_num);
				}
				break;
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *gfxSaveIff(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxSaveIff, tokenBuffer );
	return tokenBuffer;
}

extern void clearBobs();

char *gfxDoubleBuffer(struct nativeCommand *cmd, char *tokenBuffer)
{
	struct retroScreen *screen = instance.screens[instance.current_screen];

	if (screen)
	{
		engine_lock();
		clearBobs();
		retroAllocDoubleBuffer( screen );
		instance.video -> refreshAllScanlines = TRUE;
		engine_unlock();
	}

	return tokenBuffer;
}

void __swap_screen( struct retroScreen *screen )
{
	if (screen) 
	{
		engine_lock();
		if (screen -> Memory[1])		// have buffer2
		{
			screen -> double_buffer_draw_frame = 1 - screen -> double_buffer_draw_frame ;
		}
		engine_unlock();
	}
}

char *_gfxScreenSwap( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int screen_num;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:

			switch (kittyStack[__stack].type)
			{
				case type_int:
						screen_num = kittyStack[instance.stack].integer.value;
						if ((screen_num>-1)&&(screen_num<8))
						{
							__swap_screen( instance.screens[screen_num] );
							return NULL;
						}
						break;

				case type_none:

						for (screen_num=0; screen_num<8; screen_num++)
						{
							__swap_screen( instance.screens[screen_num] );
						}
						return NULL;
			}
	}

	setError(22, data->tokenBuffer);

	popStack(__stack - data->stack );
	return NULL;
}

char *gfxScreenSwap(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxScreenSwap, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *gfxDefault(struct nativeCommand *cmd, char *tokenBuffer)
{
	int n,screen_num;
	struct retroScreen *screen;

	engine_lock();

	for (n=0; n<8;n++)
	{
		if (instance.screens[n]) retroCloseScreen(&instance.screens[n]);
		instance.screens[n] = 0;
	}

	instance.current_screen = 0;
	screen_num = 0;
	instance.screens[0] = retroOpenScreen(320,200,retroLowres);

	if (screen = instance.screens[screen_num])
	{
		screen -> double_buffer_draw_frame = 0;

		init_amos_kittens_screen_default_text_window(screen, 256);
		init_amos_kittens_screen_default_colors(screen);
		draw_cursor(screen);
	}
	retroApplyScreen( screen, instance.video, 0, 0, screen -> realWidth,screen->realHeight );

	engine_unlock();

	return tokenBuffer;
}

char *_gfxXScreen( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int s,x = 0;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			x = getStackNum(__stack );
			if (screen = instance.screens[instance.current_screen]) x = XScreen_formula( screen, x );
			break;
		case 2:
			s = getStackNum(__stack -1 );			
			x = getStackNum(__stack );
			if (screen = instance.screens[s]) x = XScreen_formula( screen, x );
			break;
	}

	popStack(__stack - data->stack );
	setStackNum(x);
	return NULL;
}

char *gfxXScreen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxXScreen, tokenBuffer );
	return tokenBuffer;
}

char *_gfxYScreen( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int s,y = 0;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			y = getStackNum(__stack );
			if (screen = instance.screens[instance.current_screen]) y = YScreen_formula( screen,y );
			break;
		case 2:
			s = getStackNum(__stack -1 );			
			y = getStackNum(__stack );
			if (screen = instance.screens[s]) y = YScreen_formula( screen,y );
			break;
	}

	popStack(__stack - data->stack );
	setStackNum(y);
	return NULL;
}

char *gfxYScreen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxYScreen, tokenBuffer );
	return tokenBuffer;
}

char *_gfxXHard( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int s=0,x = 0;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			x = getStackNum(__stack );
			if (screen = instance.screens[instance.current_screen]) x = XHard_formula( screen, x );
			break;
		case 2:
			s = getStackNum(__stack -1 );			
			x = getStackNum(__stack );
			if (screen = instance.screens[s]) x = XHard_formula( screen, x );
			break;
	}

	popStack(__stack - data->stack );
	setStackNum(x);
	return NULL;
}

char *gfxXHard(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxXHard, tokenBuffer );
	setStackNum(0);
	return tokenBuffer;
}

char *_gfxYHard( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int s=0,y = 0;
	struct retroScreen *screen;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			y = getStackNum(__stack );
			if (screen = instance.screens[instance.current_screen]) y = YHard_formula( screen,y );
			break;
		case 2:
			s = getStackNum(__stack -1 );			
			y = getStackNum(__stack );
			if (screen = instance.screens[s]) y = YHard_formula( screen,y );
			break;
	}

	popStack(__stack - data->stack );
	setStackNum(y);
	return NULL;
}

char *gfxYHard(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _gfxYHard, tokenBuffer );
	setStackNum(0);
	return tokenBuffer;
}

char *_gfxScreenMode( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		int screen_num =0;

		if (kittyStack[__stack].type == type_int )
		{
			screen_num  = getStackNum(__stack);
		}
		else
		{
			screen_num = instance.current_screen;
		}

		if (instance.screens[screen_num])
		{
			ret = retroModeToAmosMode(instance.screens[screen_num]->videomode);
			setStackNum(ret);
			return NULL;
		}
		setError(22,data->tokenBuffer);
	}
	else 
	{
		popStack(__stack - data->stack );
		setError(22,data->tokenBuffer);
	}

	return NULL;
}

char *gfxScreenMode(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);

	if (next_token == 0x0074)
	{
		stackCmdParm( _gfxScreenMode, tokenBuffer );
		setStackNone();
	}
	else if (instance.screens[instance.current_screen])	// check if current screen is open.
	{
		setStackNum(instance.screens[instance.current_screen] -> videomode);
	}

	return tokenBuffer;
}

char *gfxScreenBase(struct nativeCommand *cmd, char *tokenBuffer)
{
	NYI(__FUNCTION__);

	setError(23,tokenBuffer);
	return tokenBuffer;
}

extern void IffAnim( char *name, const int sn );

char *_gfxIffAnim( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;

	struct stringData *name;
	int screen_num;	

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	switch (args)
	{
		case 2:
			name = getStackString(__stack-1);
			screen_num = getStackNum(__stack);

			if (name)	IffAnim(&(name->ptr),screen_num);
			break;
		case 3:
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	do_to[instance.parenthesis_count] = do_to_default;

	popStack(__stack - data->stack );
	return NULL;
}

char *do_arg_to( struct nativeCommand *cmd, char *tokenBuffer)
{
	__stack++;
	return NULL;
}

char *gfxIffAnim(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _gfxIffAnim, tokenBuffer );
	do_to[instance.parenthesis_count] = do_arg_to;
	return tokenBuffer;
}

char *gfxDualPriority(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setError(1000,tokenBuffer);
	return tokenBuffer;
}

char *_gfxDualPlayfield( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
				{
					int screen0 = getStackNum(__stack-1 );
					int screen1 = getStackNum(__stack );

					if ((instance.screens[screen0])&&(instance.screens[screen1]))
					{
						instance.screens[screen0]->dualScreen = instance.screens[screen1];

						instance.screens[screen1]->flags |= retroscreen_flag_hide;
						instance.video -> refreshAllScanlines = TRUE;
						popStack(__stack - data->stack );
						return NULL;
					}

					setError(22, data->tokenBuffer );
				}
				break;

		default:
				setError(22, data->tokenBuffer );
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *gfxDualPlayfield(struct nativeCommand *cmd, char *tokenBuffer)
{

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _gfxDualPlayfield, tokenBuffer );
	setStackNone();

	return tokenBuffer;
}

char *gfxLaced(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackNum( true_laced );
	return tokenBuffer;
}

char *gfxScreenSize(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setError(16, tokenBuffer);	// should not be called, 
	return tokenBuffer;
}

