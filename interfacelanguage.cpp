
#include "stdafx.h"
#include <stdint.h>

#ifdef _MSC_VER
#include <string.h>
#include "vs_missing_string_functions.h"
#define strdup _strdup
#define Printf printf
#endif

#include "debug.h"

#if defined(__amigaos4__) || defined(__amigaos)
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/retroMode.h>
#include <string.h>
extern struct RastPort font_render_rp;
#endif

#ifdef __linux__
#include <string.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#define Printf printf
#ifdef test_app
#define engine_fd stdout
#else
extern FILE *engine_fd;
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "AmalCompiler.h"
#include "pass1.h"
#include "AmosKittens.h"
#include "interfacelanguage.h"
#include "commandsBanks.h"
#include "errors.h"
#include "engine.h"

extern int sig_main_vbl;

extern int current_screen;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];

int XScreen_formula( struct retroScreen *screen, int x );
int YScreen_formula( struct retroScreen *screen, int y );
int XHard_formula( struct retroScreen *screen, int x );
int YHard_formula( struct retroScreen *screen, int y );

void isetvarstr( struct cmdcontext *context, int index, char *str );
void isetvarnum( struct cmdcontext *context, int index, int num );

void dump_context_stack( struct cmdcontext *context );
void pop_context( struct cmdcontext *context, int pop );
void push_context_num(struct cmdcontext *context, int num);
void push_context_string(struct cmdcontext *context, char *str);
void push_context_var(struct cmdcontext *context, int num);

extern uint8_t getByte( char *adr, int &pos );
extern uint16_t getWord( char *adr, int &pos );
extern uint32_t getLong( char *adr, int &pos );

extern bool convertPacPic( unsigned char *data, struct PacPicContext *context );
extern bool convertPacPicData( unsigned char *data, int o , struct PacPicContext *context );
extern void plotUnpackedContext( struct PacPicContext *context, struct retroScreen *screen, int x0, int y0 );

extern int os_text_height(char *txt);
extern int os_text_width(char *txt);
extern void os_text(struct retroScreen *screen,int x, int y, char *txt, int ink0, int ink1);
extern void os_text_outline(struct retroScreen *screen,int x, int y, char *txt, int pen,int outline);
extern void os_text_no_outline(struct retroScreen *screen,int x, int y, char *txt, int pen);

extern bool breakpoint ;

void _read_gfx( char *bnk_adr, int offset_gfx, int &pn )
{
//	int tpos = offset_gfx+2+pn*4;
//	int offset_picture = offset_gfx + getLong( bnk_adr, tpos  );		// picture stored here.

	pn++;
}

bool resource_bank_has_pictures( struct kittyBank *bank1, int block_nr )
{
	struct resourcebank_header *header; 
	int pos,pupics;

	if (bank1 == NULL) return false;

	header = (resourcebank_header*) bank1->start;

	if (header -> img_offset == 0) return false;

	pos = header -> img_offset;
	pupics = getWord( bank1->start, pos );

	if ((block_nr<0) ||  (block_nr >= pupics)) return false;
	return true;
}

bool get_resource_block( struct kittyBank *bank1, int block_nr, int x0, int y0, int *out_width, int *out_height )
{
	struct resourcebank_header *header = (resourcebank_header*) bank1->start;
	int pos;
	struct retroScreen *screen = screens[current_screen];

	if (!screen) return false;

	pos = 0;

	if (resource_bank_has_pictures( bank1, block_nr ) == false)
	{
		bank1 = findBank(-2);

		if (bank1)
		{
			if (resource_bank_has_pictures( bank1, block_nr ) == false)
			{
				printf("Can't find picture in default resource\n");
				getchar();
				return false;
			}

			header = (resourcebank_header*) bank1->start;
			if (header -> img_offset == 0) 
			{
				printf("resource has no images\n");
				getchar();
				return false;
			}
		}
		else
		{
			printf( "Can't find default resource\n");
			getchar();
			return false;
		}
	}

	pos = header -> img_offset + 2 + block_nr*4;
	pos = getLong( bank1->start, pos );

	if (pos)
	{
		struct PacPicContext context;
		context.raw = NULL;
		pos += header -> img_offset;

		if (convertPacPicData( (unsigned char *) (bank1->start + pos), 0, &context ))
		{
			*out_width = context.w * 8;
			*out_height = context.h *context.ll;

			if (context.raw)
			{
				plotUnpackedContext( &context, screen , x0, y0 );
				free( context.raw);
				return true;	
			}
		}
	}

	return false;
}


void _icmd_If( struct cmdcontext *context, struct cmdinterface *self )
{
	char *at;
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	at = context->at;

	if (context -> stackp > 0)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		// check if value is number and is false.
		context -> block = (( arg1.type == type_int ) && (arg1.num == 0)) ? -2 : 2;

		pop_context( context, 1);

	} else context -> error = 1;
}

void icmd_If( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_If;
	context -> lstackp = context -> stackp;
	context -> args = 1;
}

void _icmd_KeyShortCut( struct cmdcontext *context, struct cmdinterface *self )
{
	char *at;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp >= 2)
	{
		struct ivar &eventCode = context -> stack[context -> stackp-2];
		struct ivar &shiftCode = context -> stack[context -> stackp-1];

		if ((eventCode.type == type_int)&&(shiftCode.type == type_int))
		{

		}

		pop_context( context, 2);

	} else context -> error = 1;
}

void icmd_KeyShortCut( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_KeyShortCut;
	context -> lstackp = context -> stackp;
	context -> args = 2;
}


//icmd_ZoneChange

void _icmd_ZoneChange( struct cmdcontext *context, struct cmdinterface *self )
{
	char *at;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp >= 2)
	{
		struct ivar &zoneNr = context -> stack[context -> stackp-2];
		struct ivar &data = context -> stack[context -> stackp-1];

		if (zoneNr.type == type_int)
		{
			switch ( data.type )
			{
				case type_int:
						break;
				case type_string:
						break; 
			}
		}

		pop_context( context, 2);

	} else context -> error = 1;
}

void icmd_ZoneChange( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_ZoneChange;
	context -> lstackp = context -> stackp;
	context -> args = 2;
}

// ---



static int get_dialog_x(struct cmdcontext *context)
{
	int x=0;
	for (int n=0;n<=context -> selected_dialog;n++) x+=context -> dialog[n].x;
	return x;
}

static int get_dialog_y(struct cmdcontext *context)
{
	int y=0;
	for (int n=0;n<=context -> selected_dialog;n++) y+=context -> dialog[n].y;
	return y;
}

void _icmd_Dialogsize( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=2)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			context -> dialog[0].width = arg1.num;
			context -> dialog[0].height = arg2.num;
		}

		pop_context( context, 2 );
	}
	else context -> error = 1;

	pop_context( context, 2);
	context -> cmd_done = NULL;
}

void icmd_Dialogsize( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_Dialogsize;
	context -> lstackp = context -> stackp;
	context -> args = 2;
}

void _ipass_label( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = NULL;

	if (context -> stackp>=1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_int )
		{
			context -> labels[arg1.num] = context -> at;
		}

		pop_context( context, 1 );
	}
	else context -> error = 1;
}

void ipass_label( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _ipass_label;
	context -> lstackp = context -> stackp;
	context -> args = 1;
}

void _icmd_label( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = NULL;

	if (context -> stackp>=1)
	{
		pop_context( context, 1 );
	}
	else context -> error = 1;
}

void icmd_label( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_label;
	context -> lstackp = context -> stackp;
	context -> args = 1;
}

void _icmd_Print( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=4)
	{
		struct retroScreen *screen = screens[current_screen];

		if (screen)
		{
			int x = context -> stack[context -> stackp-4].num;
			int y = context -> stack[context -> stackp-3].num;
			int pen = context -> stack[context -> stackp-1].num;

			context -> xgcl = x;
			context -> ygcl = y;
			context -> xgc = x;
			context -> ygc = y;

			x+=get_dialog_x(context);
			y+=get_dialog_y(context);

			engine_lock();
			if (engine_ready())
			{

				switch ( context -> stack[context -> stackp-2].type )
				{
					case type_string:
						{
							char *txt  = context -> stack[context -> stackp-2].str;
							if (txt)
							{
								int th = os_text_height( txt );

								os_text_no_outline(screen, x,y,txt,pen);
								context -> xgc += os_text_width( txt ) ;
								context -> ygc += th;
							}
						}
						break;

					case type_int:
						{
							char txt[30];
							int n = context -> stack[context -> stackp-2].num;
							sprintf( txt, "%d", n);
							if (txt)
							{
								int th = os_text_height( txt );

								os_text_no_outline(screen, x,y ,txt,pen);
								context -> xgc += os_text_width( txt ) ;
								context -> ygc += th;
							}
						}
						break;
				}
			}
			engine_unlock();
		}
	}

	pop_context( context, 4 );
	context -> cmd_done = NULL;
}

void icmd_Print( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_Print;
	context -> lstackp = context -> stackp;
	context -> args = 4;
}

void icmd_Comma( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> cmd_done)
	{
		printf("args %d found args %d\n",context -> stackp - context -> lstackp, context -> args );

		if ((context -> stackp - context -> lstackp) == context -> args)
		{
			context ->cmd_done(context, self);
			context ->cmd_done = NULL;	
		}
	}
}


//icmd_HyperText

void _icmd_HyperText( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	engine_lock();

	if (engine_ready())
	{
		if (context -> stackp>=10)
		{
			struct retroScreen *screen = screens[current_screen];
/*
			struct ivar &ZN = context -> stack[context -> stackp-10];
			struct ivar &x = context -> stack[context -> stackp-9];
			struct ivar &y = context -> stack[context -> stackp-8];
			struct ivar &w = context -> stack[context -> stackp-7];
			struct ivar &h = context -> stack[context -> stackp-6];
			struct ivar &address = context -> stack[context -> stackp-5];
			struct ivar &lineNr = context -> stack[context -> stackp-4];
			struct ivar &buffer = context -> stack[context -> stackp-3];
			struct ivar &paper = context -> stack[context -> stackp-2];
			struct ivar &pen = context -> stack[context -> stackp-1];
*/
			if (screen)
			{

			}

			pop_context( context, 10);
		}
	}

	engine_unlock();

	context -> cmd_done = NULL;
}

void icmd_HyperText( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_HyperText;
	context -> lstackp = context -> stackp;
	context -> args = 10;
}

//-----


void _icmd_PrintOutline( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=5)
	{
		struct retroScreen *screen = screens[current_screen];

		if (screen)
		{
			int x = context -> stack[context -> stackp-5].num;
			int y = context -> stack[context -> stackp-4].num;
			char *txt = context -> stack[context -> stackp-3].str;
			uint16_t outline = context -> stack[context -> stackp-2].num;
			uint16_t pen = context -> stack[context -> stackp-1].num;

			x+=get_dialog_x(context);
			y+=get_dialog_y(context);

			engine_lock();
			if (engine_ready())
			{
				if (txt)	os_text_outline(screen, x,y,txt,pen,outline);
			}
			engine_unlock();
		}

		pop_context( context, 5);
	}

	context -> cmd_done = NULL;
}

void icmd_PrintOutline( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_PrintOutline;
	context -> lstackp = context -> stackp;
	context -> args = 5;
}

// icmdSetVar

void _icmd_SetVar( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=2)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg1.num < context -> max_vars ))
		{
			switch ( arg2.type )
			{
				case type_int:
					isetvarnum( context, arg1.num, arg2.num );
					break;

				case type_string:
					isetvarstr( context, arg1.num, arg2.str);
					arg2.str = NULL;		// move not free ;-)
					break;

			}
		}
		else context -> error = 1;

		pop_context( context, 2);
	}

	context -> cmd_done = NULL;
}

void icmd_SetVar( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_SetVar;
	context -> lstackp = context -> stackp;
	context -> args = 2;
}

void _icmd_ImageBox( struct cmdcontext *context, struct cmdinterface *self )
{
	struct retroScreen *screen = screens[current_screen];
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ( (screen) && (context -> stackp>=5) )
	{
		struct ivar &x0 = context -> stack[context -> stackp-5];
		struct ivar &y0 = context -> stack[context -> stackp-4];
		struct ivar &image = context -> stack[context -> stackp-3];
		struct ivar &x1 = context -> stack[context -> stackp-2];
		struct ivar &y1 = context -> stack[context -> stackp-1];

		if ( ( x0.type == type_int ) && ( y0.type == type_int )  && ( image.type == type_int )  && ( x1.type == type_int ) && ( y1.type == type_int ) )
		{
			struct kittyBank *bank1;
			int ox = get_dialog_x(context);
			int oy = get_dialog_y(context);

			x0.num = x0.num - (x0.num % 8);
			x1.num = x1.num - (x0.num % 8);

			x0.num+=ox;
			y0.num+=oy;
			x1.num+=ox;
			y1.num+=oy;

			bank1 = findBank(16);

			if (bank1)
			{
				int x,y;
				int w,h;
				int ew, eh;
				int _w,_h;	// temp, ignore.
				int _image =  image.num - 1 + context -> image_offset ;

				if (get_resource_block( bank1, _image , x0.num, y0.num, &w,&h ) == false )
				{
					setError( 22, context -> tokenBuffer );
					context -> error = true;
				}

				ew = (x1.num - x0.num) / w  ;
				eh = (y1.num - y0.num) / h  ;


				if (get_resource_block( bank1, _image +2, ew*w + x0.num, y0.num, &w,&h ) == false )
				{
					setError( 22, context -> tokenBuffer );
					context -> error = true;
				}

				if (get_resource_block( bank1, _image + 6 , x0.num, eh*h+y0.num, &w,&h ) == false )
				{
					setError( 22, context -> tokenBuffer );
					context -> error = true;
				}

				if (get_resource_block( bank1, _image +8,  ew*w+ x0.num, eh*h + y0.num, &w,&h ) == false )
				{
					setError( 22, context -> tokenBuffer );
					context -> error = true;
				}
		

				for (y=1; y<eh;y++)
				{

					if (get_resource_block( bank1, _image +3, 0 *w + x0.num, y*h+y0.num,&_w,&_h ) == false )
					{
						setError( 22, context -> tokenBuffer );
						context -> error = true;
					}

					if (get_resource_block( bank1, _image +5, ew *w + x0.num, y*h+y0.num,&_w,&_h ) == false )
					{
						setError( 22, context -> tokenBuffer );
						context -> error = true;
					}
				}

				for (x=1; x<ew;x++)
				{
					if (get_resource_block( bank1, _image +1, x *w + x0.num, y0.num, &_w,&_h ) == false )
					{
						setError( 22, context -> tokenBuffer );
						context -> error = true;
					}

					for (y=1; y<eh;y++)
					{
						if (get_resource_block( bank1, _image +4, x *w + x0.num, y*h+y0.num,&_w,&_h ) == false )
						{
							setError( 22, context -> tokenBuffer );
							context -> error = true;
						}
					}

					if (get_resource_block( bank1, _image +7, x*w + x0.num, eh*h + y0.num, &_w,&_h ) == false )
					{
						setError( 22, context -> tokenBuffer );
						context -> error = true;
					}
				}

			}

//			retroBox(screen, x0.num, y0.num, x1.num, y1.num, 2 );
		}
	}

	pop_context( context, 5);
	context -> cmd_done = NULL;
}

void icmd_ImageBox( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_ImageBox;
	context -> args = 5;
}

void _icmd_Imagehline( struct cmdcontext *context, struct cmdinterface *self )
{
	struct retroScreen *screen = screens[current_screen];
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ( (screen) && (context -> stackp>=4) )
	{
		struct ivar &x0 = context -> stack[context -> stackp-4];
		struct ivar &y0 = context -> stack[context -> stackp-3];
		struct ivar &image = context -> stack[context -> stackp-2];
		struct ivar &x1 = context -> stack[context -> stackp-1];

		if ( ( x0.type == type_int ) && ( y0.type == type_int )  && ( image.type == type_int )  && ( x1.type == type_int ) )
		{
			struct kittyBank *bank1;
			int ox = get_dialog_x(context);
			int oy = get_dialog_y(context);

			x0.num = x0.num - (x0.num % 8);

			x0.num+=ox;
			y0.num+=oy;
			x1.num+=ox;

			bank1 = findBank(16);

			if (bank1)
			{
				int x;
				int xp;
				int w=0,h=0;
				int _image =  image.num -1  + context -> image_offset ;

				xp = x0.num;

				if (get_resource_block( bank1, _image , xp, y0.num, &w,&h ) == false )
				{
					setError( 22, context -> tokenBuffer );
					context -> error = true;
				}

				xp += w;

				for (; xp<x1.num-w;xp+=w)
				{
					if (get_resource_block( bank1, _image+1, xp, y0.num, &w,&h ) == false )
					{
						setError( 22, context -> tokenBuffer );
						context -> error = true;
					}
				}

				xp = x1.num - w;

				if (get_resource_block( bank1, _image+2 , xp, y0.num, &w,&h ) == false )
				{
					setError( 22, context -> tokenBuffer );
					context -> error = true;
				}
			}
		}
	}

	pop_context( context, 4);
	context -> cmd_done = NULL;
}

void icmd_Imagehline( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_Imagehline;
	context -> args = 4;
}

void _icmd_imagevline( struct cmdcontext *context, struct cmdinterface *self )
{
	struct retroScreen *screen = screens[current_screen];
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ( (screen) && (context -> stackp>=4) )
	{
		struct ivar &x0 = context -> stack[context -> stackp-4];
		struct ivar &y0 = context -> stack[context -> stackp-3];
		struct ivar &image = context -> stack[context -> stackp-2];
		struct ivar &y1 = context -> stack[context -> stackp-1];

		if ( ( x0.type == type_int ) && ( y0.type == type_int )  && ( image.type == type_int )  && ( y1.type == type_int ) )
		{
			struct kittyBank *bank1;
			int ox = get_dialog_x(context);
			int oy = get_dialog_y(context);

			x0.num+=ox;
			y0.num+=oy;
			y1.num+=ox;

			bank1 = findBank(16);

			if (bank1)
			{
				int y;
				int yp;
				int w=0,h=0;
				int _image =  image.num -1 + context -> image_offset ;

				yp = y0.num;

				if (get_resource_block( bank1, _image, x0.num, yp, &w,&h ) == false )
				{
					setError( 22, context -> tokenBuffer );
					context -> error = true;
				}

				yp +=h;

				for (; yp<y1.num-h;yp+=h)
				{
					if (get_resource_block( bank1, _image+1, x0.num, yp, &w,&h ) == false )
					{
						setError( 22, context -> tokenBuffer );
						context -> error = true;
					}
				}

				if (get_resource_block( bank1, _image+2 , x0.num, yp, &w,&h ) == false )
				{
					setError( 22, context -> tokenBuffer );
					context -> error = true;
				}
			}
		}
	}

	pop_context( context, 4);
	context -> cmd_done = NULL;
}

void icmd_imagevline( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_imagevline;
	context -> args = 4;
}


void _icmd_Ink( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=3)
	{
		context -> ink0  = context -> stack[context -> stackp-3].num;
		context -> ink1 -= context ->  stack[context -> stackp-2].num;
		context -> ink3 = context -> stack[context -> stackp-1].num;
	}

	pop_context( context, 3);
	context -> cmd_done = NULL;
}

void icmd_Ink( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_Ink;
	context -> args = 3;
}

void icmd_Message( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_int )
		{
			char *ret ;
			ret = getResourceStr( arg1.num );
			pop_context( context, 1);

			if (ret)
			{
				push_context_string(context, ret );
			}
			else
			{
				push_context_string(context, strdup("NULL") );
			}
		}
	}
}

void _icmd_GraphicBox( struct cmdcontext *context, struct cmdinterface *self )
{
	struct retroScreen *screen = screens[current_screen];

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=4)
	{
		int ox,oy;

		int x0 = context -> stack[context -> stackp-4].num;
		int y0 = context -> stack[context -> stackp-3].num;
		int x1 = context -> stack[context -> stackp-2].num;
		int y1 = context -> stack[context -> stackp-1].num;

		context -> xgcl = x0;
		context -> ygcl = y0;
		context -> xgc = x1;
		context -> ygc = y1;

		ox = get_dialog_x(context);
		oy = get_dialog_y(context);
		x0+=ox;
		y0+=oy;
		x1+=ox;
		y1+=oy;

		if (screen) retroBAR( screen, x0,y0,x1,y1,context -> ink0 );
	}

	pop_context( context, 4);
	context -> cmd_done = NULL;
}

void icmd_GraphicBox( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_GraphicBox;
	context -> args = 4;
}

// icmd_VerticalSlider

void _icmd_VerticalSlider( struct cmdcontext *context, struct cmdinterface *self )
{
	struct retroScreen *screen = screens[current_screen];

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=9)
	{
		int x1,y1;
		int ox,oy;

		int zn = context -> stack[context -> stackp-9].num;
		int x0 = context -> stack[context -> stackp-8].num;
		int y0 = context -> stack[context -> stackp-7].num;
		int w = context -> stack[context -> stackp-6].num;
		int h = context -> stack[context -> stackp-5].num;
		int position = context -> stack[context -> stackp-4].num;
		int trigger = context -> stack[context -> stackp-3].num;
		int total = context -> stack[context -> stackp-2].num;
		int step = context -> stack[context -> stackp-1].num;

		ox = get_dialog_x(context);
		oy = get_dialog_y(context);

		x1=x0+w;
		y1=y0+h;

		x0+=ox;
		y0+=oy;
		x1+=ox;
		y1+=oy;

		if (screen) retroBox( screen, x0,y0,x1,y1,context -> ink0 );
	}

	pop_context( context, 9);
	context -> cmd_done = NULL;
}

void icmd_VerticalSlider( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_VerticalSlider;
	context -> args = 9;
}

void _icmd_GraphicSquare( struct cmdcontext *context, struct cmdinterface *self )
{
	struct retroScreen *screen = screens[current_screen];

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=4)
	{
		int ox,oy;

		int x0 = context -> stack[context -> stackp-4].num;
		int y0 = context -> stack[context -> stackp-3].num;
		int x1 = context -> stack[context -> stackp-2].num;
		int y1 = context -> stack[context -> stackp-1].num;

		context -> xgcl = x0;
		context -> ygcl = y0;
		context -> xgc = x1;
		context -> ygc = y1;

		ox = get_dialog_x(context);
		oy = get_dialog_y(context);
		x0+=ox;
		y0+=oy;
		x1+=ox;
		y1+=oy;

		if (screen) retroBox( screen, x0,y0,x1,y1,context -> ink0 );
	}

	pop_context( context, 4);
	context -> cmd_done = NULL;
}

void icmd_GraphicSquare( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_GraphicSquare;
	context -> args = 4;
}

void _icmd_Base( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=2)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			struct dialog &dialog = context -> dialog[0];

			dialog.x = arg1.num -  (arg1.num % 16);
			dialog.y = arg2.num;
		}

		pop_context( context, 2);
	}

	context -> cmd_done = NULL;
}

void icmd_Base( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_Base;
	context -> args = 2;
}

void _icmd_Jump( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = NULL;

	if (context -> stackp>=1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_int ) 
		{
			char *at = context -> labels[ arg1.num ];
			if (at)
			{
				context -> at =  at;
				context -> l = 0;
			}
		}

		pop_context( context, 1);
	}
}


void icmd_Jump( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_Jump;
	context -> args = 1;
}

// icmd_Run

void _icmd_Run( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = NULL;

	if (context -> stackp>=2)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg1.type == type_int ))
		{
			if (arg2.num & 4) engine_wait_key = true;

			for (;;)
			{
				if (arg2.num & 2)	// mouse key
				{
					if (engine_mouse_key)
					{
						context -> mouse_key = true;
						break;
					}
				}

				if (arg2.num & 4)	// key press
				{
					if (engine_wait_key == false)
					{
 						engine_wait_key = false;
						break;
					}
				}

				if (engine_ready() == false)	break;

				Delay(1);
			}
		}

		pop_context( context, 2);
	}
	else context -> error = 1;
}


void icmd_Run( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_Run;
	context -> args = 2;
}


void _Jump_SubRoutine( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_int ) 
		{
			char *at = context -> labels[ arg1.num ];
			if (at)
			{
				context -> programStack[ context -> programStackCount] = context -> at;
				context -> programStackCount ++;
				context -> at =  at;
				context -> l = 0;
			}
		}

		pop_context( context, 1);
	}

	context -> cmd_done = NULL;
}

void icmd_JumpSubRutine( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	context -> cmd_done = _Jump_SubRoutine;
	context -> args = 1;
}


void icmd_block_start( struct cmdcontext *context, struct cmdinterface *self )
{
	char *at = context -> at;
	int count = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch  ( context -> block )
	{
		case 2:
		case -1:
		case 0:

				if (*at=='[')	// next is a block.
				{
					count = 0;
					while (*at)
					{
						switch (*at )
						{
							case '[': count ++;	break;
							case ']': count --;	
								break;
						}

						if (count == 0)
						{
							context->at =at+1;		// set new location.
							context->l = 0;		// reset length of command.
			
							printf("%s\n",context->at);
							break;
						}
						at ++;
					}
				}

			break;
	}

	if (context -> block<0)
	{
		context -> block++;
	}
	else 	if (context -> block>0) context -> block--;

	context -> args = 0;
}

void icmd_block_end( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	context -> selected_dialog = 0;

	context -> args = 0;
}

void _icmd_Button( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	context -> block = -2;

	if (context -> stackp>=8)
	{
		struct ivar &nr = context -> stack[context -> stackp-8];
		struct ivar &_x = context -> stack[context -> stackp-7];
		struct ivar &_y = context -> stack[context -> stackp-6];
		struct ivar &_w = context -> stack[context -> stackp-5];
		struct ivar &_h = context -> stack[context -> stackp-4];
		struct ivar &arg6 = context -> stack[context -> stackp-3];
		struct ivar &_min = context -> stack[context -> stackp-2];
		struct ivar &_max = context -> stack[context -> stackp-1];

		if (( nr.type == type_int ) 
			&& ( _x.type == type_int )  
			&& ( _y.type == type_int )  
			&& ( _w.type == type_int )  
			&& ( _h.type == type_int )  
			&& ( arg6.type == type_int ) 
 			&& ( _min.type == type_int ) 
			&& ( _max.type == type_int ))
		{
			struct dialog &dialog = context -> dialog[0];
			struct dialog &button = context -> dialog[1];

			struct retroScreen *screen = screens[current_screen];
			int x = _x.num + dialog.x;
			int y = _y.num + dialog.y;

			button.x = _x.num;
			button.y = _y.num;
			button.width = _w.num;
			button.height = _h.num;

			context -> return_value = nr.num;
			context -> xgcl = _x.num;
			context -> ygcl =_y.num;
			context -> xgc = _x.num + _w.num;
			context -> ygc = _y.num + _h.num;

			if ((screen)&&(engine_mouse_key || context -> mouse_key))
			{
				char txt[30];

				int mx = XScreen_formula( screen, engine_mouse_x );
				int my = YScreen_formula( screen, engine_mouse_y );

				if (	(mx>=x)&&(mx<=(x+_w.num))	&&
					(my>=y)&&(my<=(y+_h.num))  )
				{
					context -> block = 2;
				}
			}
		}

		pop_context( context, 8);
	}

	context -> selected_dialog = 1;
	context -> cmd_done = NULL;
}

void icmd_Button( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	context -> cmd_done = _icmd_Button;
	context -> args = 8;
}


void icmd_ButtonQuit( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> has_return_value = true;
}

void _icmd_ButtonReturn( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=1)
	{
		pop_context( context, 1);
	}

	context -> cmd_done = NULL;
}


void icmd_ButtonReturn( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	context -> cmd_done = _icmd_ButtonReturn;
	context -> args = 1;
}


void icmd_Return( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> programStackCount)
	{
		printf(" jump to %s\n ", context -> programStack[ context -> programStackCount -1 ] );
		context -> at = context -> programStack[ context -> programStackCount - 1 ];
 		context -> programStackCount--;
		context -> l = 0;
	}
	else 	context -> error = true;
}



void _icmd_Unpack( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=3)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-3];	// x
		struct ivar &arg2 = context -> stack[context -> stackp-2];	// y
		struct ivar &arg3 = context -> stack[context -> stackp-1];	// image

		if (( arg1.type == type_int ) && ( arg2.type == type_int ) && ( arg3.type == type_int ) )
		{
			struct kittyBank *bank1;

			printf("unpack %d,%d,%d + (%d )\n", arg1.num, arg2.num, arg3.num, context -> image_offset);

			bank1 = findBank(16);
	
			if (bank1)
			{
				int w,h;

				arg1.num += get_dialog_x(context);
				arg2.num += get_dialog_y(context);

				if (get_resource_block( bank1, arg3.num + context -> image_offset - 1, arg1.num, arg2.num, &w, &h ) == false )
				{
					setError( 22, context -> tokenBuffer );
					context -> error = true;
				}
			}
		}

		pop_context( context, 3);
	}

	context -> cmd_done = NULL;
}

void icmd_Unpack( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_Unpack;
	context -> args = 3;
}


void _icmd_Save( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_int ) 
		{		
		}

		pop_context( context, 1);
	}

	context -> cmd_done = NULL;
}

void icmd_Save( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_Save;
	context -> args = 1;
}

void _icmd_PushImage( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_int ) 
		{		
			context -> image_offset = arg1.num  ;
		}

		pop_context( context, 1);
	}

	context -> cmd_done = NULL;
}

void icmd_PushImage( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_PushImage;
	context -> args = 1;
}

// undocumented not in AmosPro manuall, used in AmosPro_Help.amos

void _icmd_bb( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=5)
	{
		struct kittyBank *bank1;
		struct retroScreen *screen = screens[current_screen]; 
		struct ivar &nr = context -> stack[context -> stackp-5];	// id
		struct ivar &x = context -> stack[context -> stackp-4];	// x
		struct ivar &y = context -> stack[context -> stackp-3];	// y
		struct ivar &width = context -> stack[context -> stackp-2];	// ?
		struct ivar &txt = context -> stack[context -> stackp-1];	// txt

		if (( nr.type == type_int ) 
			&& ( x.type == type_int )  
			&& ( y.type == type_int )  
			&& ( width.type == type_int )  
			&& ( txt.type == type_string ))
		{
			x.num+=get_dialog_x(context);
			y.num+=get_dialog_y(context);
	
			retroBox( screen, x.num, y.num, x.num+width.num-2, y.num+9,2 );

			engine_lock();
			if (engine_ready())
			{
				if (txt.str)
				{
					os_text_no_outline(screen, x.num+(width.num/2)- (os_text_width( txt.str ) / 2),y.num+os_text_height( txt.str ),txt.str,screen -> ink0 );
				}							
			}
			engine_unlock();
		}
		else context -> error = 1;


		pop_context( context, 5);
	}

	context -> cmd_done = NULL;
}

void icmd_bb( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_bb;
	context -> args = 5;
}

void icmd_ButtonNoWait( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
}

void icmd_ButtonPosition( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	push_context_num( context, context -> button_active );

}

void icmd_Var( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>0)
	{
		int index = -1;
		struct ivar &self = context -> stack[context -> stackp-1];
		if ( self.type == type_int) index =  self.num;

		pop_context( context, 1);

		if ((index>-1)  && (index < context -> max_vars))
		{

			printf("index: %d\n",index);

			push_context_var(context, index);
		}
		else context -> error = 1 ;
	}
}

void pop_context( struct cmdcontext *context, int pop )
{
	printf("pop(%d)\n",pop);

	while ((pop>0)&&(context->stackp>0))
	{
		struct ivar &p = context -> stack[context -> stackp-1];

		switch (p.type)
		{
			case type_string:
					if (p.str)
					{
						free (p.str);
						p.str = NULL;
					}
					break;
		}

		pop--;
		context -> stackp--;
	}
}

void icmd_Equal( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			ret = arg1.num == arg2.num ? ~0 : 0 ;
		}

		pop_context( context, 2);
		push_context_num( context, ret );
	}
	else context -> error = 1;
}

void icmd_Plus( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			ret = arg1.num + arg2.num  ;
		}

		pop_context( context, 2 );
		push_context_num( context, ret );
	}
	else context -> error = 1;
}

void icmd_Minus( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			ret = arg1.num - arg2.num  ;
		}

		pop_context( context, 2 );
		push_context_num( context, ret );
	}
	else context -> error = 1;
}

void icmd_Mul( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			ret = arg1.num * arg2.num  ;
		}

		pop_context( context, 2 );
		push_context_num( context, ret );
	}
	else context -> error = 1;
}

void icmd_Div( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			ret = arg1.num / arg2.num  ;
		}

		pop_context( context, 2 );
		push_context_num( context, ret );
	}
	else context -> error = 1;
}

void icmd_Min( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			ret = arg1.num < arg2.num ? arg1.num : arg2.num ;
		}

		pop_context( context, 2 );
		push_context_num( context, ret );
	}
	else context -> error = 1;
}



void icmd_TextHeight( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>0)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_string ) 
		{
			ret = os_text_height( arg1.str );
		}
		else ret = 0;

		pop_context( context, 1);
		push_context_num( context, ret );
	}
	else context -> error = 1;
}

void icmd_TextWidth( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>0)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_string ) 
		{
			ret = os_text_width( arg1.str );
		}
		else ret = 0;

		pop_context( context, 1);
		push_context_num( context, ret );
	}
	else context -> error = 1;
}

void icmd_SizeX( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	push_context_num( context, context -> dialog[context -> selected_dialog].width );
}

void icmd_SizeY( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	push_context_num( context, context -> dialog[context -> selected_dialog].height );
}

void icmd_cx( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);


	if (context -> stackp>0)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_string ) 
		{
			struct retroScreen *screen = screens[current_screen];

			if (screen)
			{
				int l = strlen( arg1.str );
				int tl = os_text_width( arg1.str );

				ret = (context -> dialog[ context -> selected_dialog ] .width / 2) - (tl/2)  ;
			}
		}

		pop_context( context, 1);
		push_context_num( context, ret );
	}
	else context -> error = 1;
}

void icmd_Exit( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> at = context -> script + strlen(context -> script);
	context -> cmd_done = NULL;
	context -> args = 0;
	context -> l = 0;
}

void icmd_ScreenWidth( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (screens[current_screen])	// check if current screen is open.
	{
		push_context_num( context,screens[current_screen] -> realWidth);
	}
}

void icmd_ScreenHeight( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (screens[current_screen])	// check if current screen is open.
	{
		push_context_num( context,screens[current_screen] -> realHeight);
	}
}

void icmd_NextCmd( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> cmd_done)
	{
		return context ->cmd_done(context, self);
		context ->cmd_done = NULL;
	}
}

void isetvarstr( struct cmdcontext *context,int index, char *str)
{
	struct ivar &var = context -> vars[index];

	if (var.type == type_string)
	{
		if (var.str) free(var.str);
	}
	else var.type = type_string;
	var.str = strdup(str);
}

void isetvarnum( struct cmdcontext *context,int index,int num)
{
	struct ivar &var = context -> vars[index];

	if (var.type == type_string)
	{
		if (var.str) free(var.str);
	}
	else var.type = type_int;
	var.num = num;
}

void icmd_Bin( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	unsigned char *at = (unsigned char *) context -> at;
	int ret;

	if (*at != 37) return;	// not binaray

	 at++;

	ret = 0;
	while ((*at=='0')||(*at=='1'))
	{
		ret=ret<<1;
		if (*at=='1') ret++;
		context -> l++;
		at++;
	}

	push_context_num( context, ret );
}

void icmd_Hex( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	unsigned char *at = (unsigned char *) context -> at;
	int ret;
	char symb;

	if (*at != '$') return;	// not binaray
	 at++;

	ret = 0;
	symb = *at;
	while (
			((symb>='0')&&(symb<='9')) ||
			((symb>='A')&&(symb<='F')) 
		)
	{
		ret = ret << 4;
		if ((symb>='0')&&(symb<='9')) ret += symb -'0';
		if ((symb>='A')&&(symb<='F')) ret += symb -'A'+10;
		
		context -> l++;
		at++;
		symb = *at;
	}

	push_context_num( context, ret );
}

void icmd_XGCL( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	push_context_num( context, context -> xgcl );
}

void icmd_YGCL( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	push_context_num( context, context -> ygcl );
}

void icmd_XGC( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	push_context_num( context, context -> xgc );
}

void icmd_YGC( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	push_context_num( context, context -> ygc );
}

void icmd_ZoneValue( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	push_context_num( context, 0 );
}


struct cmdinterface symbols[]=
{

	{"=",i_parm,NULL,icmd_Equal },
	{";",i_normal,icmd_NextCmd,icmd_NextCmd},
	{"[",i_normal,NULL,icmd_block_start},
	{"]",i_normal,NULL,icmd_block_end},
	{",",i_parm,NULL,icmd_Comma},
	{"+",i_parm,NULL,icmd_Plus},
	{"-",i_parm,NULL,icmd_Minus},
	{"*",i_parm,NULL,icmd_Mul},
	{"/",i_parm,NULL,icmd_Div},
	{"%",i_parm,NULL,icmd_Bin},
	{"$",i_parm,NULL,icmd_Hex},

	{NULL,i_normal,NULL,NULL}
};

struct cmdinterface commands[]=
{
	{"BA",i_normal,NULL,icmd_Base},
	{"BB",i_normal,NULL,icmd_bb },	// 6 args
	{"BP",i_parm,NULL,icmd_ButtonPosition },
	{"BO",i_normal,NULL,icmd_ImageBox },
	{"BR",i_normal,NULL,icmd_ButtonReturn },
	{"BQ",i_normal,NULL,icmd_ButtonQuit },
	{"BU",i_normal,NULL,icmd_Button},
	{"BX",i_parm,NULL,NULL},
	{"BY",i_parm,NULL,NULL},
	{"CX",i_parm,NULL,icmd_cx},
	{"ED",i_normal,NULL,NULL},
	{"EX",i_normal,NULL,icmd_Exit},
	{"GB",i_normal,NULL,icmd_GraphicBox},
	{"GS",i_normal,NULL,icmd_GraphicSquare},
	{"GE",i_normal,NULL,NULL},
	{"GL",i_normal,NULL,NULL},
	{"HT",i_normal,NULL,icmd_HyperText},
	{"IF",i_normal,NULL,icmd_If},
	{"IN",i_normal,NULL,icmd_Ink},
	{"JP",i_normal,NULL,icmd_Jump},
	{"JS",i_normal,NULL,icmd_JumpSubRutine},
	{"LA",i_normal,ipass_label, icmd_label },
	{"LI",i_normal,NULL,icmd_Imagehline },
	{"VL",i_normal,NULL,icmd_imagevline },
	{"KY",i_normal,NULL,icmd_KeyShortCut},
	{"MI",i_parm,NULL,icmd_Min},
	{"NW",i_normal,NULL,icmd_ButtonNoWait},
	{"PR",i_normal,NULL,icmd_Print},
	{"PO",i_normal,NULL,icmd_PrintOutline},
	{"PU",i_normal,NULL,icmd_PushImage},
	{"ME",i_parm,NULL,icmd_Message},
	{"SA",i_normal,NULL,icmd_Save},
	{"SH",i_parm,NULL,icmd_ScreenHeight},
	{"SI",i_normal,NULL,icmd_Dialogsize},
	{"SM",i_parm,NULL,NULL},
	{"SP",i_normal,NULL,NULL},
	{"SV",i_normal,NULL,icmd_SetVar },
	{"SW",i_parm,NULL,icmd_ScreenWidth},
	{"SX",i_parm,NULL,icmd_SizeX},
	{"SY",i_parm,NULL,icmd_SizeY},
	{"RT",i_normal,NULL,icmd_Return},
	{"RU",i_normal,NULL,icmd_Run},
	{"TH",i_parm,NULL,icmd_TextHeight},
	{"TL",i_parm,NULL,NULL },
	{"TW",i_parm,NULL,icmd_TextWidth},
	{"UN",i_normal,NULL,icmd_Unpack},
	{"VA",i_parm,NULL,icmd_Var},
	{"VS",i_normal,NULL,icmd_VerticalSlider },
	{"VT",i_normal,NULL,NULL},
	{"XA",i_parm,NULL,icmd_XGCL},
	{"YA",i_parm,NULL,icmd_YGCL},
	{"XB",i_parm,NULL,icmd_XGC},
	{"YB",i_parm,NULL,icmd_YGC},
	{"XY",i_parm,NULL,NULL},
	{"ZN",i_parm,NULL,NULL},
	{"ZC",i_normal,NULL,icmd_ZoneChange },
	{"ZV",i_parm,NULL,icmd_ZoneValue },
	{"=",i_parm,NULL,icmd_Equal },
	{";",i_parm,icmd_NextCmd,icmd_NextCmd},		// next command or end of command.
	{"[",i_normal,NULL,icmd_block_start},
	{"]",i_normal,NULL,icmd_block_end},
	{",",i_parm,NULL,icmd_Comma},
	{"+",i_parm,NULL,icmd_Plus},
	{"-",i_parm,NULL,icmd_Minus},
	{"*",i_parm,NULL,icmd_Mul},
	{"/",i_parm,NULL,icmd_Div},

	{NULL,i_normal,NULL,NULL}
};


static void remove_lower_case(char *txt)
{
	char *c;
	char *d;
	bool space_repeat;
	bool is_text = false;

	d=txt;
	for (c=txt;*c;c++)
	{
		space_repeat = false;

		if (*c=='\'') is_text = is_text ? false : true;

		if (is_text == false)
		{
			// remove noice.
			while (((*c>='a')&&(*c<='z'))||(*c=='#'))	{ c++;  }

			if (d!=txt)
			{
				char ld = *(d-1);
				if (	((ld==' ')||(ld==',')||(ld==';'))	&&	(*c==' ')	)	space_repeat = true;
			}
		}

		if (space_repeat == false)
		{
			*d++=*c;
		}
	}
	*d = 0;
}

bool is_command( char *at )
{
	struct cmdinterface *cmd;
	int l;

	for (cmd = commands; cmd -> name; cmd++)
	{
		l = strlen(cmd -> name);
		if (strncmp(cmd -> name,at,l)==0) return true;
	}
	return false;
}


int find_symbol( char *at, int &l )
{
	struct cmdinterface *cmd;
	int num = 0;

	for (cmd = symbols; cmd -> name; cmd++)
	{
		l = 1;
		if ( *(cmd -> name)  == *at ) return num;
		num++;
	}
	return -1;
}


int find_command( char *at, int &l )
{
	struct cmdinterface *cmd;
	int num = 0;
	char c;

	for (cmd = commands; cmd -> name; cmd++)
	{
		l = strlen(cmd -> name);
		if (strncmp(cmd -> name,at,l)==0)
		{
			c = *(at+l);

			if ((c == ' ')||(c=='\'')||(c == 0)) return num;
			if ((c>='0')&&(c<='9')) return num;
			if (is_command(at+l)) return num;
		}
		num++;
	}
	return -1;
}

bool is_string( char *at, char *&str, int &l )
{
	l=0;

	if (*at!='\'') return false;

	printf("{%s}\n",at);

	at++;
	{
		char *p;
		for (p = at; ((*p !='\'') && (*p!=0));p++ ) l++;
		str =strndup( at, l );
		l+=2;
	}

	return true;
}

bool is_number( char *at, int &num, int &l )
{
	l=0;
	num = 0;
	while ( ((*at>='0') && (*at<='9')) )
	{		
		num = (num*10) + ( *at - '0');
		l++;
		at++;
	}

	return (l>0) ? true : false;
}

void dump(char *txt)
{
	int n=0;
	printf("----------->");
	while ((n<10)&&(*txt))
	{
		printf("%c",*txt++);
		n++;
	}
	printf("\n");
}

void push_context_num(struct cmdcontext *context, int num)
{
	struct ivar &self = context -> stack[context -> stackp];
	self.type = type_int;
	self.num = num;
	context -> stackp++;

	printf("push %d\n",num);
}

void push_context_string(struct cmdcontext *context, char *str)
{
	struct ivar &self = context -> stack[context -> stackp];
	self.type = type_string;
	self.str = str;
	context -> stackp++;

	printf("push %s\n",str);
}

void push_context_var(struct cmdcontext *context, int index)
{
	struct ivar &self = context -> stack[context -> stackp];

	if ( context -> vars)
	{
		struct ivar &var = context -> vars[index];

		self.type = var.type;
		if (var.type == type_string)
		{
			self.str = strdup( var.str );
		}
		else self.num = var.num;

		context -> stackp++;
	}
	else
	{
		printf("interface context not initialized\n");
		context -> error = 1;
	}
}

void dump_context_stack( struct cmdcontext *context )
{
	int n;

	for (n=0; n<context -> stackp;n++)
	{
		switch ( context -> stack[n].type)
		{
			case type_string:
				printf("     stack[%d]='%s'\n",n,context -> stack[n].str);
				break;
			case type_int:
				printf("     stack[%d]=%d\n",n,context -> stack[n].num);
				break;
		}
	}
}

void init_interface_context( struct cmdcontext *context, int id, char *script, int x, int y, int varSize, int bufferSize  )
{
	struct dialog &dialog = context -> dialog[0];

	bzero( context, sizeof( struct cmdcontext ) );

	remove_lower_case( script );

	context -> id = id;
	context -> stackp = 0;
	context -> script = strdup( script );
	context -> at = context -> script;
	context -> max_vars = varSize;
	context -> selected_dialog = 0;

	context -> vars = (struct ivar *) malloc( sizeof(struct ivar) * varSize  );

	dialog.x = x - (x % 16) ;
	dialog.y = y;
}

void cleanup_inerface_context( struct cmdcontext *context )
{
	context -> at = NULL;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> script)
	{
		free(	context -> script );
		context -> script = NULL;
	}

	if (context -> vars) 
	{
		free(context -> vars);
		context -> vars = NULL;
	}
}

void test_interface_script( struct cmdcontext *context)
{
	int sym,cmd;
	int num;
	char *str = NULL;

	if (context -> at == 0)
	{
		setError( 22, context -> tokenBuffer );
		return ;
	}

	while ((*context -> at != 0) && (context -> error == false))
	{
		while (*context -> at==' ') context -> at++;

		sym = find_symbol( context -> at, context -> l );
		if (sym != -1)
		{
			struct cmdinterface *icmd = &symbols[sym];
			if (icmd -> pass)
			{
				printf("found %s\n", icmd -> name);
				icmd -> pass( context, icmd );
			}
		}
		else
		{
			cmd = find_command( context -> at, context -> l );
			if (cmd == -1) 
			{
				if (is_string(context -> at, str, context -> l) )
				{
					push_context_string( context, str );
				}
				else 	if (is_number(context -> at, num, context -> l))
				{
					push_context_num( context, num );
				}
				else 	break;
			}
			else 	
			{
				struct cmdinterface *icmd = &commands[cmd];

				if (icmd -> type == i_normal) pop_context( context, context -> stackp );

				if (icmd -> pass)
				{
					printf("found %s\n", icmd -> name);
					icmd -> pass( context, icmd );
				}
			}
		}

		context -> at += context -> l;
		dump_context_stack( context );
	}
	context -> tested = true;
}

void execute_interface_script( struct cmdcontext *context, int32_t label)
{
	int sym,cmd;
	int num;
	char *str = NULL;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ( context -> vars == NULL )
	{
		printf("%s:%s context damged\n",__FILE__,__FUNCTION__);
		getchar();
		return;
	}

	context -> error = false;
	context -> stackp = 0;
	context -> image_offset = 0;
	context -> has_return_value = false;
	context -> selected_dialog = 0;

	if (context -> tested == false)
	{
	 	test_interface_script( context );
	}

	context -> at = context -> script;	// default

	if (label != -1)
	{
		if (context -> labels[ label ])
		{
			context -> at = context -> labels[ label ];
		}
	}

	if (context -> at == 0)
	{
		setError( 22, context -> tokenBuffer );
		return ;
	}

	while ((*context -> at != 0) && (context -> error == false) && (context -> has_return_value == false))
	{
		while (*context -> at==' ') context -> at++;

		if (breakpoint)
		{
			printf("EXECUTE SCRIPT {%s}\n",context -> at);
			printf("<< breakpoint, press enter >>\n");
		}

		sym = find_symbol( context -> at, context -> l );

		if (sym != -1)
		{
			struct cmdinterface *icmd = &symbols[sym];
			if (icmd -> cmd)
			{
				icmd -> cmd( context, icmd );
			}
			else printf("ignored %s\n", icmd -> name);
		}
		else
		{
			cmd = find_command( context -> at, context -> l );
			if (cmd == -1) 
			{
				if (is_string(context -> at, str, context -> l) )
				{
					push_context_string( context, str );
				}
				else 	if (is_number(context -> at, num, context -> l))
				{
					push_context_num( context, num );
				}
				else 	break;
			}
			else 	
			{
				struct cmdinterface *icmd = &commands[cmd];

				if (icmd -> type == i_normal)
				{
					if (context -> stackp >0)
					{
						printf("there is stuff on the stack, there shoud be none.\n");
						dump_context_stack( context );
						getchar();
					}
				}

				if (icmd -> cmd)
				{
					icmd -> cmd( context, icmd );
				}
				else
				{
					printf("ignored %s\n", icmd -> name);
					getchar();
				}
			}
		}

		context -> at += context -> l;

		if (context -> vars  == NULL)
		{
			printf("context vars died here -> context stack %d\n", context -> stackp );
			getchar();
			break;
		}
	}

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> error)
	{
		printf("error at: {%s}\n",context -> at);
		getchar();
	}
	else if (*context -> at != 0)
	{
		printf("ended at: {%s}\n",context -> at);
		dump_context_stack( context );
		getchar();
	}
}

#ifdef test_app

int main(int args, char **arg)
{
	int n;
	char *script_mem;
	const char *script =    "IF     0VA 1=;" 
   "[" 
   "SIze   1VATW160+ SW MI,40;" 
   "BAse   SWidth SX -2/,SHeight SY- 2/;" 
   "SAve   1;" 
   "BOx    0,0,1,SX,SY;" 
   "PRint  1VACX,SY2/ TH2/ -,1VA,3;" 
   "RUn    0,%1111;" 
   "]" 
   "IF     0VA 2=;" 
   "[" 
   "SIze   SWidth 1VATW80+ MIn,64;" 
   "BAse   SWidth SX -2/,SHeight SY- 2/;" 
   "SAve   1;" 
   "BOx    0,0,1,SX,SY;" 
   "PRint  1VACX,16,1VA,3;" 
   "BUtton 1,16,SY24-,64,16,0,0,1;[UNpack 0,0,13BP+; PRint 12ME CX BP+,4,12ME,3;][BR0;BQ;]" 
   "KY     27,0;" 
   "BUtton 2,SX72-,SY24-,64,16,0,0,1;[UNpack 0,0,13BP+; PRint 11ME CX BP+,4,11ME,3;][BR0;BQ;]" 
   "KY     13,0;" 
   "RUn    0,3;" 
   "]" 
   "IF     0VA 3=;" 
   "[" 
   "SIze   1VATW160+ SW MI,40;" 
   "BAse   SWidth SX -2/,SHeight SY- 2/;" 
   "BOx    0,0,1,SX,SY;" 
   "PRint  1VACX,SY2/ TH2/ -,1VA,3;" 
   "]" 
   "EXit;" ;

	script_mem = strdup(script);

	if (script_mem)
	{

		printf("%s\n",script_mem);
		
		printf("\n\n");

		read_script(script_mem);

		free(script_mem);
	}

	return 0;
}

#endif

