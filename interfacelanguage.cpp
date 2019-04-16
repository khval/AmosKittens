
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

void _read_gfx( char *bnk_adr, int offset_gfx, int &pn )
{
//	int tpos = offset_gfx+2+pn*4;
//	int offset_picture = offset_gfx + getLong( bnk_adr, tpos  );		// picture stored here.

	pn++;
}

struct __attribute__((__packed__))  __header__
{
	uint32_t	what;
	uint16_t	x;
	uint16_t	y;
	uint16_t	w;
	uint16_t	h1;
	uint16_t	h2;
};

bool get_resource_block( struct kittyBank *bank1, int block_nr, int x0, int y0 )
{
	struct resourcebank_header *header = (resourcebank_header*) bank1->start;
	int chunks,end_offset,_len,type,id;
	int pos,pupics;
	int xx,yy;
	int is_signed;
	int offset_gfx;
	int tpos;
	int colors;
	int x1,y1;
	struct __header__ *head;
	struct retroScreen *screen = screens[current_screen];

	if (!screen) return false;

	pos = 0;
	chunks = getWord( bank1->start, pos );	// get chunks.

	offset_gfx = pos = 2+chunks*4; 
	end_offset = getLong( bank1->start, pos );

	pos = header -> img_offset;
	 
	pupics = getWord( bank1->start, pos );

	if ((block_nr<0) ||  (block_nr >= pupics)) return false;


	printf("pupics: %d\n",pupics);

	pos = header -> img_offset + 2 + block_nr*4;
	pos = getLong( bank1->start, pos );

	if (pos)
	{
		pos += header -> img_offset;

		head = (struct __header__ *) (bank1->start + pos);


			printf("pos %d, %d block x %d,y %d,w %d,h %d\n",
				x0,
				y0,
				head->x*8,
				head->y,
				head->w*8,
				head->h1*head->h2);

		x1 = x0 + (head->w*8);
		y1 = y0 + (head->h1*head->h2);

		is_signed =  getByte( bank1->start, pos );
		type = getByte( bank1->start, pos );

		printf("%x,%x\n",is_signed, type);

		type = is_signed ? -type : type;
		id = getWord( bank1->start, pos );

		printf("pn: % 2d, type: % 4d, id: %04x\n", block_nr, type, id);

		retroBox( screen, x0,y0,x1,y1, 3 );

		return true;		
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


extern void os_text(struct retroScreen *screen,int x, int y, char *txt);
void os_text_outline(struct retroScreen *screen,int x, int y, char *txt, uint16_t pen,uint16_t outline);

void _icmd_Print( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

#if defined(__amigaos4__) || defined(__morphos__) || defined(__aros__)
	if (( sig_main_vbl )&&( EngineTask ))
	{

		if (context -> stackp>=4)
		{
			struct retroScreen *screen = screens[current_screen];

			if (screen)
			{
				int x = context -> stack[context -> stackp-4].num;
				int y = context -> stack[context -> stackp-3].num;
//				int o = context -> stack[context -> stackp-1].num;

				x+=get_dialog_x(context);
				y+=get_dialog_y(context);

				switch ( context -> stack[context -> stackp-2].type )
				{
					case type_string:
						{
							char *txt  = context -> stack[context -> stackp-2].str;
							if (txt) os_text(screen, x,y,txt);
						}
						break;

					case type_int:
						{
							char txt[30];
							int n = context -> stack[context -> stackp-2].num;
							sprintf( txt, "%d", n);
							os_text(screen, x,y,txt);
						}
						break;
				}
			}
		}
	}
#endif

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

void _icmd_PrintOutline( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

#if defined(__amigaos4__) || defined(__morphos__) || defined(__aros__)
	if (( sig_main_vbl )&&( EngineTask ))
	{

		if (context -> stackp>=5)
		{
			struct retroScreen *screen = screens[current_screen];

			if (screen)
			{
				int x = context -> stack[context -> stackp-5].num;
				int y = context -> stack[context -> stackp-4].num;
				char *txt = context -> stack[context -> stackp-3].str;
				uint16_t pen = context -> stack[context -> stackp-2].num;
				uint16_t outline = context -> stack[context -> stackp-1].num;

				x+=get_dialog_x(context);
				y+=get_dialog_y(context);

				if (txt)	os_text_outline(screen, x,y,txt,pen,outline);
			}
		}
	}
#endif

	pop_context( context, 5);
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

		if ( arg1.type == type_int ) 
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

// ----

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

			dialog.x = arg1.num;
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

	if (context -> stackp>=1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg1.type == type_int ))
		{

		}

		pop_context( context, 1);
	}
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

			if (screen)
			{
				char txt[30];

				int mx = XScreen_formula( screen, engine_mouse_x );
				int my = YScreen_formula( screen, engine_mouse_y );

				if (	(mx>=x)&&(mx<=(x+_w.num))	&&
					(my>=y)&&(my<=(y+_h.num))	)
				{
					context -> block = 2;
				}

#if defined(__amigaos4__) || defined(__morphos__) || defined(__aros__)
				if (( sig_main_vbl )&&( EngineTask ))
				{
					sprintf( txt, "    %d,    %d     ",	mx,	my );
					os_text(screen, 0,20,txt);

					sprintf( txt, "    %d,    %d     ",	engine_mouse_x,	engine_mouse_y );
					os_text(screen, 160,20,txt);
				}
#endif
			}
		}

		pop_context( context, 8);
	}

	context -> selected_dialog = 1;
	context -> cmd_done = NULL;
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

void icmd_Button( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	context -> cmd_done = _icmd_Button;
	context -> args = 8;
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
		struct ivar &arg1 = context -> stack[context -> stackp-3];
		struct ivar &arg2 = context -> stack[context -> stackp-2];
		struct ivar &arg3 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ) && ( arg3.type == type_int ) )
		{
			struct kittyBank *bank1;

			printf("unpack %d,%d,%d\n", arg1.num, arg2.num, arg3.num);

			bank1 = findBank(16);
	
			if (bank1)
			{
				printf("**** we found a bank\n");

				if (get_resource_block( bank1, arg3.num, arg1.num, arg2.num ) == false )
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
			context -> image_offset = arg1.num;
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


void icmd_ButtonNoWait( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
}

void icmd_ButtonPosition( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	push_context_num( context, context -> dialog[1].x );

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

		if (index>-1) push_context_var(context, index);
	}
}

void pop_context( struct cmdcontext *context, int pop )
{
	printf("pop(%d)\n",pop);

	while ((pop)&&(context->stackp))
	{
		struct ivar &p = context -> stack[--context -> stackp];

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


void icmd_TextWidth( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>0)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_string ) 
		{
			ret = strlen(arg1.str);
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
				int tl = TextLength(&font_render_rp, arg1.str, l );

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
}

void icmd_ScreenWidth( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	push_context_num( context, -999 );
}

void icmd_ScreenHeight( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	push_context_num( context, -999 );
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
	unsigned char *at = (unsigned char *) context -> at;
	int ret;

	printf("%s\n", at);

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

	{NULL,i_normal,NULL,NULL}
};

struct cmdinterface commands[]=
{

	{"BA",i_normal,NULL,icmd_Base},
	{"BP",i_normal,NULL,icmd_ButtonPosition},
	{"BO",i_normal,NULL,NULL},
	{"BR",i_normal,NULL,icmd_ButtonReturn},
	{"BQ",i_normal,NULL,NULL },
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
	{"HT",i_normal,NULL,NULL},
	{"IF",i_normal,NULL,icmd_If},
	{"IN",i_normal,NULL,icmd_Ink},
	{"JP",i_normal,NULL,icmd_Jump},
	{"JS",i_normal,NULL,icmd_JumpSubRutine},
	{"LA",i_normal,ipass_label, icmd_label },
	{"KY",i_normal,NULL,NULL},
	{"MI",i_parm,NULL,icmd_Min},
	{"NW",i_normal,NULL,icmd_ButtonNoWait},
	{"PR",i_normal,NULL,icmd_Print},
	{"PO",i_normal,NULL,icmd_PrintOutline},
	{"PU",i_normal,NULL,icmd_PushImage},
	{"ME",i_parm,NULL,NULL},
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
	{"TH",i_parm,NULL,NULL},
	{"TL",i_parm,NULL,NULL },
	{"TW",i_parm,NULL,icmd_TextWidth},
	{"UN",i_normal,NULL,icmd_Unpack},
	{"VA",i_parm,NULL,icmd_Var},
	{"VT",i_normal,NULL,NULL},
	{"XB",i_parm,NULL,NULL},
	{"XY",i_parm,NULL,NULL},
	{"YB",i_parm,NULL,NULL},
	{"ZN",i_parm,NULL,NULL},
	{"=",i_parm,NULL,icmd_Equal },
	{";",i_normal,icmd_NextCmd,icmd_NextCmd},
	{"[",i_normal,NULL,icmd_block_start},
	{"]",i_normal,NULL,icmd_block_end},
	{",",i_parm,NULL,icmd_Comma},
	{"+",i_parm,NULL,icmd_Plus},
	{"-",i_parm,NULL,icmd_Minus},
	{"*",i_parm,NULL,icmd_Mul},
	{"/",i_parm,NULL,icmd_Div},
//	{"%",i_parm,NULL,NULL},

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
	struct ivar &var = context -> vars[index];

	self.type = var.type;
	if (var.type == type_string)
	{
		self.str = strdup( var.str );
	}
	else self.num = var.num;

	context -> stackp++;

	printf("push VAR[%d]\n",index);
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

void init_interface_context( struct cmdcontext *context, int id, char *script, int x, int y )
{
	struct dialog &dialog = context -> dialog[0];

	bzero( context, sizeof( struct cmdcontext ) );

	remove_lower_case( script );

	context -> id = id;
	context -> stackp = 0;
	context -> script = strdup( script );
	context -> at = context -> script;

	dialog.x = x;
	dialog.y = y;
}


void test_interface_script( struct cmdcontext *context)
{
	int sym,cmd;
	int num;
	char *str = NULL;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> at == 0)
	{
		setError( 22, context -> tokenBuffer );
		return ;
	}

	while ((*context -> at != 0) && (context -> error == false))
	{
		while (*context -> at==' ') context -> at++;

		printf("TEST SCRIPT {%s}\n",context -> at);

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

	context -> error = false;
	context -> stackp = 0;

	if (context -> tested == false)
	{
	 	test_interface_script( context );
	}

	printf("%s:%d\n",__FUNCTION__,__LINE__);


	if (label == -1)
	{
		context -> at = context -> script;
	}
	else
	{
		context -> at = context -> labels[ label ];
	}

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> at == 0)
	{
		setError( 22, context -> tokenBuffer );
		return ;
	}

	while ((*context -> at != 0) && (context -> error == false))
	{
		while (*context -> at==' ') context -> at++;

		printf("EXECUTE SCRIPT {%s}\n",context -> at);

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
				if (icmd -> cmd)
				{
					icmd -> cmd( context, icmd );
				}
				else printf("ignored %s\n", icmd -> name);
			}
		}

		context -> at += context -> l;
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

