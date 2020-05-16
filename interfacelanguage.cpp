
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <vector>

#include "debug.h"

#if defined(__amigaos4__) || defined(__amigaos)
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/retroMode.h>
#include <proto/kittyCompact.h>
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

#include "AmalCompiler.h"
#include "pass1.h"
#include "AmosKittens.h"
#include "interfacelanguage.h"
#include "commandsBanks.h"
#include "kittyErrors.h"
#include "engine.h"
#include "bitmap_font.h"
#include "amosstring.h"

extern struct TextFont *topaz8_font;

extern int sig_main_vbl;

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
void push_context_string(struct cmdcontext *context, struct stringData *str);
void push_context_var(struct cmdcontext *context, int num);
userDefined *push_context_ui( struct cmdcontext *context );

extern uint8_t getByte( char *adr, int &pos );
extern uint16_t getWord( char *adr, int &pos );
extern uint32_t getLong( char *adr, int &pos );

extern int os_text_height(struct stringData *txt);
extern int os_text_base(struct stringData *txt);
extern int os_text_width(struct stringData *txt);
extern void os_text(struct retroScreen *screen,int x, int y, struct stringData *txt, int ink0, int ink1);
extern void os_text_outline(struct retroScreen *screen,int x, int y, struct stringData *txt, int pen,int outline);
extern void os_text_no_outline(struct retroScreen *screen,int x, int y, struct stringData *txt, int pen);

extern bool breakpoint ;

#define set_block_fn(name) context -> block_fn[ context -> block_level ] = name
#define has_block_fn() context -> block_fn[ context -> block_level ]
#define call_block_fn(context,self) context -> block_fn[ context -> block_level ](context,self)
#define inc_block() { context -> block_level++ ; context -> block_fn[ context -> block_level ] = NULL; }

void execute_interface_sub_script( struct cmdcontext *context, int zone,  char *at);

#define ierror(nr)  { context -> error = nr; printf("Error at %s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__); getchar(); }

userDefined::userDefined()
{
	*((int *) &name ) =0;
	args = 0;
	action = NULL;
}

struct userDefined *cmdcontext::findUserDefined( const char *name )
{
	unsigned int n;
	uint16_t twoChars = * ((uint16_t *)  name);	// name is char and myabe a \0, name should be two chars.

	// command is always two chars. buffer is 4 so no issue.
	for (n=0;n<userDefineds.size();n++)
	{
		if ( *((uint16_t *) userDefineds[n].name)  != twoChars) continue;
		return &userDefineds[n];
	}

	return NULL;
}

void cmdcontext::dumpUserDefined()
{
	unsigned int n;
	for (n=0;n<userDefineds.size();n++)
	{
		struct userDefined *ud = &userDefineds[n];	// don't need check...
		printf("name: %s args %d action: %08x\n", ud -> name, ud -> args, ud -> action );

	}
}

void block_hypertext_action( struct cmdcontext *context, struct cmdinterface *self );

void il_set_zone( struct cmdcontext *context, int id, int type, struct zone_base *custom )
{
	if (id>=20) return;

	if (context -> zones[id].custom == custom)
	{
		printf("bad code\n");
		return;
	}

	if (context -> zones[id].custom ) free (context -> zones[id].custom);
	context -> zones[id].id = id;
	context -> zones[id].type = type;
	context -> zones[id].custom = custom;
}


void il_dump_vars( struct cmdcontext *context)
{
	int n;

	printf("----- %s ------\n",__FUNCTION__);

	for (n=0;n<4;n++)
	{
		switch (context -> vars[n].type)
		{
			case type_string:
					printf("var[%d]=[%08x] {%s}\n",n,context -> vars[n].str, context -> vars[n].str);
					break;

			case type_int:
					printf("var[%d]={%d}\n",n,context -> vars[n].num);
					break;

		 }
	}
}



void block_skip( struct cmdcontext *context, struct cmdinterface *self )
{
	char *at = context -> at;
	int block_count = 0;
	int size = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	while (*at != 0)
	{
		if (*at == '[')
		{
			block_count ++;
		}
		else if (*at == ']')
		{		
			block_count --;
		}

		if ((*at == ']') && (block_count == 0)) break;

		at++;
		size ++;
	}

	context -> at = at;
	context -> l = 0;
	set_block_fn(NULL);
}


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
	struct retroScreen *screen = instance.screens[instance.current_screen];

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
				sys_free( context.raw);
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

		if (( arg1.type == type_int ) && (arg1.num == 0)) 
		{
			set_block_fn(block_skip);
		}

		pop_context( context, 1);

	} else ierror(1);
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
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp >= 2)
	{
		struct ivar &eventCode = context -> stack[context -> stackp-2];
		struct ivar &shiftCode = context -> stack[context -> stackp-1];

		if ((eventCode.type == type_int)&&(shiftCode.type == type_int))
		{

		}

		pop_context( context, 2);

	} else ierror(1);
}

void icmd_KeyShortCut( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_KeyShortCut;
	context -> lstackp = context -> stackp;
	context -> args = 2;
}


void _icmd_ZoneChange( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp >= 2)
	{
		struct ivar &zn = context -> stack[context -> stackp-2];
		struct ivar &data = context -> stack[context -> stackp-1];

		if ((zn.type == type_int) && ( data.type == type_int ))
		{
			struct zone_base *zone =  context -> zones[zn.num].custom;
			if (zone) 
			{
				zone -> value = data.num;
				pop_context( context, 2);

				if (zone -> render) zone -> render( zone );

				return;		// return success ;-)
			}
		}

		pop_context( context, 2);
	} 

	ierror(1);	// if we have not retuned success before.
}

void icmd_ZoneChange( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	context -> cmd_done = _icmd_ZoneChange;
	context -> lstackp = context -> stackp;
	context -> args = 2;
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
	else ierror(1);

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
	else ierror(1);
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
	else ierror(1);
}

void icmd_label( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_label;
	context -> lstackp = context -> stackp;
	context -> args = 1;
}

void __print_one_line__( struct retroScreen *screen, int x, int y, struct stringData *txt, int pen)
{
	int n;
	char *c = &txt -> ptr;

	for (n=0;n<txt -> size;n++)
	{
		switch (*c)
		{
			case 0:
			case 10:
			case 12:
				return;
		}

		draw_glyph( screen, topaz8_font, x, y, *c, pen );
		c++;
		x+=8;
	}
}

void _icmd_Print( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=4)
	{
		struct retroScreen *screen = instance.screens[instance.current_screen];

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
							struct stringData *txt  = context -> stack[context -> stackp-2].str;
							if (txt)
							{
								int th = os_text_height( txt );
								int tb = os_text_base( txt );

								__print_one_line__(screen, x,y,txt,pen);
								context -> xgc += os_text_width( txt ) ;
								context -> ygc += th;
							}
						}
						break;

					case type_int:
						{
							char tmp[50];
							struct stringData *str = (struct stringData *) tmp;
							int n = context -> stack[context -> stackp-2].num;


							sprintf( &str -> ptr, "%d", n);
							if (str)
							{
								int th = os_text_height( str );
								int tb = os_text_base( str );

								os_text_no_outline(screen, x,y+tb ,str,pen);
								context -> xgc += os_text_width( str ) ;
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

void gfx_debug_print(int x, int y, char *txt)
{
	struct retroScreen *screen = instance.screens[instance.current_screen];
	struct stringData *strd = toAmosString( txt, strlen(txt) );
	os_text_no_outline(screen, x,y ,strd,0);
	sys_free(strd);
}

void hypertext_render(struct zone_hypertext *zh)
{
	struct retroScreen *screen = instance.screens[instance.current_screen];
	int _x = 0, _y=-zh->pos;
	int x0,y0,x1,y1;
	char *c;
	bool is_link = false;
	bool is_id = false;

	if (screen)
	{
		retroBAR( screen, screen -> double_buffer_draw_frame,  zh -> x0, zh -> y0, zh -> x1, zh -> y1, zh -> paper );

		c = (char *) zh -> address;

		if (c)
		{
			while (*c)				
			{
				switch (*c)
				{
					case 10:	
					case 13:	_y++; _x=0;	break;		// Amos expects lines to end with char 13 (char return), not char 10
					case '{':	is_link = true;
							break;
					case '}':	is_link = false;
							break;
					case '[':	if (is_link) is_id = true;
							break;
					case ']':	if (is_link) is_id = false;
							break;

					default:
							if (is_id)  break;

							x0 = _x*8+zh->x0;
							y0 = _y*8+zh->y0;
							x1 = x0 + 8;
							y1 = y0 + 8;

							if (is_link) 	retroBAR( screen, screen -> double_buffer_draw_frame,  x0, y0, x1, y1, is_link ? zh->pen : zh -> paper );

							if ((_y>=0)&&(_y<zh->h)&&(_x>=0)&&(_x<zh->w))
							{
								draw_glyph( screen, topaz8_font,x0,y0,*c, is_link ? zh->paper : zh -> pen );
								_x++;
							}
				}
				c++;
			}
		}
	}
}

int get_id_hypertext(struct zone_hypertext *zh, int chr_x, int chr_y)
{
	struct retroScreen *screen = instance.screens[instance.current_screen];
	int _x = 0, _y=-zh->pos;
	char *c;
	bool is_link = false;
	bool is_id = false;
	bool clicked = false;
	int id = 0;

	if (screen)
	{
		c = (char *) zh -> address;

		if (c)
		{
			while (*c)				
			{
				switch (*c)
				{
					case 10:	
					case 13:	_y++; _x=0;	

							if ( _y > chr_y ) return 0;

							break;		// Amos expects lines to end with char 13 (char return), not char 10
					case '{':	is_link = true;
							break;
					case '}':	is_link = false;
							if (clicked) return id;
							break;
					case '[':	if (is_link)
							{
								id = 0;
								is_id = true;
							}
							break;
					case ']':	if (is_link)	is_id = false;
							break;

					default:
							if (is_id) 
							{
								if ((*c>='0')&&(*c<='9'))	id = id * 10 + (*c-'0');
							}
							else
							{
								if (is_link)
								{
									if ((chr_x == _x) && ( chr_y == _y)) clicked = true;
								}
								_x ++;
							}
							break;
				}
				c++;
			}
		}
	}
	return 0;
}

void hypertext_mouse_event(zone_hypertext *base,struct cmdcontext *context, int mx, int my, int zid)
{
	int chr_x,  chr_y;
	if ((mx<base -> x0)||(mx>base -> x1)||(my<base ->y0)||(my>base->y1)) return ;

	chr_x = (mx - base -> x0) / 8;
	chr_y = (my - base -> y0) / 8;

	base -> value = get_id_hypertext( base, chr_x, chr_y );
	base -> event = base -> value;
	context -> has_return_value = true;
	context -> return_value = zid;
}


void _icmd_HyperText( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	engine_lock();

	if (engine_ready())
	{
		if (context -> stackp>=10)
		{
			struct zone_hypertext *zh;
			int ox = get_dialog_x(context);
			int oy = get_dialog_y(context);

			struct ivar &zn = context -> stack[context -> stackp-10];

			context -> last_zone = zn.num;

			zh = new zone_hypertext();
			if (zh)
			{
				zh -> x0 = context -> stack[context -> stackp-9].num + ox;
				zh -> y0 = context -> stack[context -> stackp-8].num + oy;
				zh -> w = context -> stack[context -> stackp-7].num;
				zh -> h = context -> stack[context -> stackp-6].num;
				zh -> x1 = zh -> x0+(zh->w*8);
				zh -> y1 = zh -> y0+(zh->h*8);

				zh -> address = (void *) context -> stack[context -> stackp-5].num;
				zh -> pos = context -> stack[context -> stackp-4].num;
				zh -> buffer = context -> stack[context -> stackp-3].num;
				zh -> paper = context -> stack[context -> stackp-2].num;
				zh -> pen = context -> stack[context -> stackp-1].num;

				il_set_zone( context, zn.num, iz_hypertext, zh );

				zh -> render(zh);
			}

			pop_context( context, 10);

			set_block_fn( block_hypertext_action );
		}
	}

	engine_unlock();

	context -> cmd_done = NULL;
}

void icmd_HyperText( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_HyperText;
	context -> args = 10;
}

void _icmd_ct( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	pop_context( context, 6);
	context -> cmd_done = NULL;
}


void icmd_ct( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_ct;
	context -> args = 6;
}


//-----


void _icmd_PrintOutline( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=5)
	{
		struct retroScreen *screen = instance.screens[instance.current_screen];

		if (screen)
		{
			int tb;
			int x = context -> stack[context -> stackp-5].num;
			int y = context -> stack[context -> stackp-4].num;
			struct stringData *txt = context -> stack[context -> stackp-3].str;
			uint16_t outline = context -> stack[context -> stackp-2].num;
			uint16_t pen = context -> stack[context -> stackp-1].num;

			x+=get_dialog_x(context);
			y+=get_dialog_y(context);

			engine_lock();
			if (engine_ready())
			{
				if (txt)
				{
					tb = os_text_base( txt );
					os_text_outline(screen, x,y+tb,txt,pen,outline);
				}
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
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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
					break;
			}
		}
		else ierror(1);
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

void _icmd_SetZone( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (context -> stackp>=1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_int )
		{
			struct zone_base *zb = context -> zones[context -> last_zone].custom;
			if (zb) zb -> value = arg1.num;
		}
		else ierror(1);
		pop_context( context, 1);
	}

	context -> cmd_done = NULL;
}

void icmd_SetZone( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_SetZone;
	context -> lstackp = context -> stackp;
	context -> args = 1;
}

void _icmd_ImageBox( struct cmdcontext *context, struct cmdinterface *self )
{
	struct retroScreen *screen = instance.screens[instance.current_screen];
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

			bank1 = findBank( instance.current_resource_bank );

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

				if (get_resource_block( bank1, _image + 6 , x0.num, y1.num - h, &w,&h ) == false )
				{
					setError( 22, context -> tokenBuffer );
					context -> error = true;
				}

				if (get_resource_block( bank1, _image +8,  ew*w+ x0.num, y1.num - h, &w,&h ) == false )
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

					if (get_resource_block( bank1, _image +7, x*w + x0.num, y1.num - h, &_w,&_h ) == false )
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
	struct retroScreen *screen = instance.screens[instance.current_screen];
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

			bank1 = findBank(instance.current_resource_bank);

			if (bank1)
			{
				int xp;
				int w=0,h=0;
				int _image = image.num - 1 + context -> image_offset ;

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
	struct retroScreen *screen = instance.screens[instance.current_screen];
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

			bank1 = findBank(instance.current_resource_bank);

			if (bank1)
			{
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
			struct stringData *ret ;
			ret = getResourceStr( arg1.num );
			pop_context( context, 1);

			if (ret)
			{
				push_context_string(context, ret );
			}
			else
			{
				push_context_string(context, toAmosString("NULL",4) );
			}
		}
	}
}

void _icmd_GraphicLine( struct cmdcontext *context, struct cmdinterface *self )
{
	struct retroScreen *screen = instance.screens[instance.current_screen];

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

		if (screen) retroLine( screen, screen -> double_buffer_draw_frame, x0,y0,x1,y1,context -> ink0 );
	}

	pop_context( context, 4);
	context -> cmd_done = NULL;
}

void icmd_GraphicLine( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_GraphicLine;
	context -> args = 4;
}

void _icmd_GraphicBox( struct cmdcontext *context, struct cmdinterface *self )
{
	struct retroScreen *screen = instance.screens[instance.current_screen];

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

		if (screen) retroBAR( screen, screen -> double_buffer_draw_frame,  x0,y0,x1,y1,context -> ink0 );
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

void _icmd_RenderButton( struct cmdcontext *context, struct cmdinterface *self )
{
	struct retroScreen *screen = instance.screens[instance.current_screen];

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=5)
	{
		int ox,oy;

		int x0 = context -> stack[context -> stackp-5].num;
		int y0 = context -> stack[context -> stackp-4].num;
		int x1 = context -> stack[context -> stackp-3].num;
		int y1 = context -> stack[context -> stackp-2].num;
		int buttonPos = context -> stack[context -> stackp-1].num;

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

		if (screen) retroBox( screen, screen -> double_buffer_draw_frame,  x0,y0,x1,y1,context -> ink0 );
	}

	pop_context( context, 5);
	context -> cmd_done = NULL;
}

void icmd_RenderButton( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_RenderButton;
	context -> args = 5;
}

void button_render(struct zone_button *zl)
{
}

void hslider_render(struct zone_hslider *zl)
{
	int t0;
	int t1;
	struct retroScreen *screen = instance.screens[instance.current_screen];

	t0 = zl->w * zl -> pos / zl->total;
	t1 = zl->w * (zl->pos+zl->trigger) / zl->total;
	
	if (screen) 
	{
		if (zl->x0+t0 != zl->x0 ) retroBAR( screen, screen -> double_buffer_draw_frame,  zl->x0,zl->y0,zl->x0+t0-1,zl->y1, 0 );
		retroBAR( screen, screen -> double_buffer_draw_frame,  zl->x0+t0,zl->y0,zl->x0+t1,zl->y1, 4 );
		if (zl->x0+t1 != zl->x1 ) retroBAR( screen, screen -> double_buffer_draw_frame,  zl->x0+t1+1,zl->y0,zl->x1,zl->y1, 0 );
	}
}

void vslider_render(zone_vslider *base)
{
	int t0;
	int t1;
	struct retroScreen *screen = instance.screens[instance.current_screen];

	t0 = base -> h *  base -> pos / base -> total;
	t1 = base -> h * (base -> pos+base -> trigger) / base -> total;
	
	if (screen) 
	{
		if (base -> y0+t0 != base -> y0 ) retroBAR( screen, screen -> double_buffer_draw_frame,  
				base -> x0,
				base -> y0,
				base -> x1,
				base -> y0+t0-1, 0 );

		retroBAR( screen, screen -> double_buffer_draw_frame, base -> x0,base -> y0+t0,base -> x1,base -> y0+t1, 4 );
		if (base -> y0+t1 != base -> y1 ) retroBAR( screen, screen -> double_buffer_draw_frame, base -> x0,base -> y0+t1+1,base -> x1,base -> y1, 0 );
	}
}


void hslider_mouse_event(struct zone_hslider *base, struct cmdcontext *context, int mx, int my, int zid)
{
	int t0;
	int t1;
	int tpos = base -> pos;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	t0 = base -> w *  base -> pos / base -> total;
	t1 = base -> w * (base -> pos+base -> trigger) / base -> total;

	if ((my>=base -> y0)&&(my<=base -> y1))
	{
		mx -= base -> x0;

		if ((mx>=0)&&(mx<=t0))
		{
			tpos -= ( tpos - base -> step < 0) ? 1: base -> step;
			if (tpos < 0 )	tpos =0;
			base -> pos = tpos;
			base -> render( base );

			execute_interface_sub_script( context, zid , (char *) base -> script_action);
			return;
		}

		if ((mx>=t0)&&(mx<=t1))
		{
			struct retroScreen *screen = instance.screens[instance.current_screen];
			int dx = mx-t0;
			int x;
			int handel = base -> w * base -> trigger / base -> total;
			int tpos;

			while (instance.engine_mouse_key)
			{
				mx = XScreen_formula( screen, hw_mouse_x );

				x = mx - base -> x0 - dx;
				if (x<0) x=0;
				if (x>base -> w) x = base -> w;

				tpos = x * (base -> total - base -> trigger) / (base -> w - handel);

				if (tpos > (base -> total - base -> trigger)) tpos = base -> total - base -> trigger; 

				base -> pos = tpos;

				printf("mx  %d, mouse x  %d, x %d, dx %d, t0 %d tpos %d\n",
						mx,
						hw_mouse_x, 
						x, 
						dx,
						t0, tpos );

				base -> render( base );

				Delay(1);
			}

			execute_interface_sub_script( context, zid, base -> script_action);
			return;
		}

		if ((mx>=t1)&&(mx<=base -> w))
		{
			tpos += ( tpos + base -> trigger + base -> step > base -> total) ? 1: base -> step;
			if ( tpos + base -> trigger >  base -> total )	tpos = base -> total - base -> trigger;
			base -> pos = tpos;
			base -> event = tpos;
			base -> render( base );

			execute_interface_sub_script( context, zid, base -> script_action);
			return;
		}
	}

	printf("%s:%d\n",__FUNCTION__,__LINE__);
}

void vslider_mouse_event(zone_vslider *base,struct cmdcontext *context, int mx, int my, int zid)
{
	int t0;
	int t1;
	int tpos = base -> pos;

	t0 = base -> h *  base -> pos / base -> total;
	t1 = base -> h * (base -> pos+base -> trigger) / base -> total;


//	printf("%s:%d -> min %d, is %d, max %d\n",__FUNCTION__,__LINE__, base -> x0,mx,base -> x1);

	if ((mx>=base -> x0)&&(mx<=base -> x1))
	{
		my -= base -> y0;

		if ((my>=0)&&(my<=t0))
		{
			tpos -= ( tpos - base -> step < 0) ? 1: base -> step;
			if (tpos < 0 )	tpos =0;
			base -> pos = tpos;
			base -> render( base );
			execute_interface_sub_script( context, zid, base -> script_action);
			return;
		}

		if ((my>=t0)&&(my<=t1))
		{
			struct retroScreen *screen = instance.screens[instance.current_screen];
			int dy = my-t0;
			int y;
			int handel = base -> h * base -> trigger / base -> total;
			int tpos;

			while (instance.engine_mouse_key)
			{
				my = YScreen_formula( screen, hw_mouse_y );

				y = my - base->y0 - dy;
				if (y<0) y=0;
				if (y>base -> h) y = base -> h;

				tpos = y * (base -> total - base -> trigger) / (base -> h - handel);

				if (tpos > (base -> total - base -> trigger)) tpos = base -> total - base -> trigger; 

				base -> pos = tpos;
				base -> event = tpos;
				base -> render( base );

				Delay(1);
			}
			execute_interface_sub_script( context, zid,  base -> script_action);

			return;
		}

		if ((my>=t1)&&(my<=base -> h))
		{
			tpos += ( tpos + base -> trigger + base -> step > base -> total) ? 1: base -> step;
			if ( tpos + base -> trigger > base -> total )	tpos = base -> total - base -> trigger;
			base -> pos = tpos;
			base -> render( base );
			execute_interface_sub_script( context, zid, base -> script_action);
			return;
		}
	}
}

void block_slider_action( struct cmdcontext *context, struct cmdinterface *self )
{
	struct zone_hslider *zs = (struct zone_hslider *) context -> zones[context -> last_zone].custom;

	if (zs)
	{
		zs -> script_action = context -> at;
	}

	set_block_fn(block_skip);
}

void edit_mouse_event(zone_edit *base,struct cmdcontext *context, int mx, int my, int zid)
{
}


void edit_render(struct zone_edit *ze)
{
	struct retroScreen *screen = instance.screens[instance.current_screen];

	if (screen) 
	{
		retroBAR( screen, screen -> double_buffer_draw_frame,  ze->x0,ze->y0,ze->x1,ze->y1, ze -> paper );

		if (ze -> string)
		{
			int th = os_text_height( ze -> string );
			int tb = os_text_base( ze -> string );

			os_text_no_outline(screen, ze->x0,ze->y0+tb+2 ,ze -> string ,ze -> pen);
		}
	}

}

void _icmd_Edit( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=8)
	{
		struct zone_edit *ze = NULL;

		int zn = context -> stack[context -> stackp-8].num;
		context -> last_zone = zn;

		if (context -> zones)
		{
			ze = (struct zone_edit *) context -> zones[zn].custom;
		}
		else
		{
			printf("your so fucked\n");
			getchar();
			return;
		}

		if (ze == NULL)
		{
			ze = new zone_edit();
			ze -> pos = 0;
			context -> zones[zn].custom = ze;
		}

		if (ze)
		{
			int ox,oy;
			ox = get_dialog_x(context);
			oy = get_dialog_y(context);

			ze -> x0 = context -> stack[context -> stackp-7].num + ox;
			ze -> y0 = context -> stack[context -> stackp-6].num + oy;
			ze -> w = context -> stack[context -> stackp-5].num;
			ze -> max = context -> stack[context -> stackp-4].num;
			ze -> string = context -> stack[context -> stackp-3].str;
			ze -> paper = context -> stack[context -> stackp-2].num;
			ze -> pen = context -> stack[context -> stackp-1].num;

			ze -> x1 = ze -> x0+(ze->w*8);
			ze -> y1 = ze -> y0+8;

			ze -> render(ze);
		}

		set_block_fn(block_slider_action);

	}

	pop_context( context, 8);
	context -> cmd_done = NULL;
}

void icmd_Edit( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_Edit;
	context -> args = 8;
}

void icmd_param( struct cmdcontext *context, struct cmdinterface *self )
{
	char c = *(context -> at + 1);
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	push_context_num( context, context -> param[ c-'0' ] );
}

void _icmd_VerticalSlider( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=9)
	{
		struct zone_vslider *zs = NULL;

		int zn = context -> stack[context -> stackp-9].num;
		context -> last_zone = zn;

		if (context -> zones)
		{
			zs = (struct zone_vslider *) context -> zones[zn].custom;
		}
		else
		{
			printf("your so fucked\n");
			getchar();
			return;
		}

		if (zs == NULL)
		{
			zs = new zone_vslider();
			zs -> pos = context -> stack[context -> stackp-4].num;
			il_set_zone( context, zn, iz_vslider, zs);
		}

		if (zs)
		{
			int ox,oy;
			ox = get_dialog_x(context);
			oy = get_dialog_y(context);

			zs -> x0 = context -> stack[context -> stackp-8].num + ox;
			zs -> y0 = context -> stack[context -> stackp-7].num + oy;
			zs -> w = context -> stack[context -> stackp-6].num;
			zs -> h = context -> stack[context -> stackp-5].num;
			zs -> x1 = zs -> x0+zs->w;
			zs -> y1 = zs -> y0+zs->h;
			zs -> trigger = context -> stack[context -> stackp-3].num;
			zs -> total = context -> stack[context -> stackp-2].num;
			zs -> step = context -> stack[context -> stackp-1].num;

			zs -> render(zs);
		}

		set_block_fn(block_slider_action);

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


void _icmd_HorizontalSlider( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=9)
	{
		struct zone_hslider *zs = NULL;

		int zn = context -> stack[context -> stackp-9].num;
		context -> last_zone = zn;

		if (context -> zones)
		{
			zs = (struct zone_hslider *) context -> zones[zn].custom;
		}
		else
		{
			printf("your so fucked\n");
			getchar();
			return;
		}

		if (zs == NULL)
		{
			zs = new zone_hslider();
			zs -> pos = context -> stack[context -> stackp-4].num;
			il_set_zone( context, zn, iz_hslider, zs);
		}

		if (zs)
		{
			int ox,oy;
			ox = get_dialog_x(context);
			oy = get_dialog_y(context);

			zs -> x0 = context -> stack[context -> stackp-8].num + ox;
			zs -> y0 = context -> stack[context -> stackp-7].num + oy;
			zs -> w = context -> stack[context -> stackp-6].num;
			zs -> h = context -> stack[context -> stackp-5].num;
			zs -> x1 = zs -> x0+zs->w;
			zs -> y1 = zs -> y0+zs->h;

			zs -> trigger = context -> stack[context -> stackp-3].num;
			zs -> total = context -> stack[context -> stackp-2].num;
			zs -> step = context -> stack[context -> stackp-1].num;

			zs -> render(zs);
		}

		set_block_fn(block_slider_action);

	}

	pop_context( context, 9);
	context -> cmd_done = NULL;
}

void icmd_HorizontalSlider( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmd_HorizontalSlider;
	context -> args = 9;
}


void _icmd_GraphicSquare( struct cmdcontext *context, struct cmdinterface *self )
{
	struct retroScreen *screen = instance.screens[instance.current_screen];

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

		if (screen) retroBox( screen, screen -> double_buffer_draw_frame, x0,y0,x1,y1,context -> ink0 );
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

void icmd_BaseX( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	push_context_num( context, context -> dialog[context -> selected_dialog].x );
}

void icmd_BaseY( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	push_context_num( context, context -> dialog[context -> selected_dialog].y );
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

void icmd_ScreenMove( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	struct retroScreen *screen = instance.screens[instance.current_screen];
	int hw_screen_y;
	int hw_delta;
	int my;

	if (screen == NULL) return;

	hw_screen_y = screen -> scanline_y / 2 + 50;
	hw_delta = hw_mouse_y - hw_screen_y;
	
	while (instance.engine_mouse_key)
	{
		my = hw_mouse_y ;
	
		screen -> scanline_y = (hw_mouse_y - hw_delta- 50)*2;

		engine_lock();
		retroApplyScreen( screen, instance.video, 
			screen -> scanline_x,
			screen -> scanline_y,
			screen -> displayWidth,
			screen -> displayHeight );

		instance.video -> refreshAllScanlines = TRUE;
		engine_unlock();

		Delay(1);
	}
}

void do_events_interface_script(  struct cmdcontext *context, int event, int delay )
{
	int n;
	struct zone_base *base;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	context -> exit_run = false;
	context -> has_return_value = false;

	if (event & 4) engine_wait_key = true;

	for (n=0;n<20;n++)
	{
		if (base = context -> zones[n].custom) base -> event = 0;
	}

	for (;;)
	{
		if (event & 2)	// mouse key
		{
			if (instance.engine_mouse_key)
			{
				struct retroScreen *screen = instance.screens[instance.current_screen];

				context -> mouse_key = true;

				if (screen)
				{

					int mx = XScreen_formula( screen, hw_mouse_x );
					int my = YScreen_formula( screen, hw_mouse_y );

					for (n=0;n<20;n++)
					{
						if (base = context -> zones[n].custom)
						{
							base -> mouse_event(base, context, mx,my, n);
						}
					}
				}

				if (context -> has_return_value) break;
				if (context -> exit_run) break;
			}
		}

		if (event & 4)	// key press
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
			int event = arg2.num;
			int delay = arg1.num;
			pop_context( context, 2);
			do_events_interface_script( context,  event, delay );
		}

		return;	// return success
	}

	pop_context( context, 2);
	ierror(1);
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
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ( has_block_fn() ) call_block_fn( context, self );

	inc_block();

	context -> args = 0;
}

void icmd_block_end( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	context -> selected_dialog = 0;

	if (context -> block_level > 0) context -> block_level --;

	context -> args = 0;
}

void block_hypertext_action( struct cmdcontext *context, struct cmdinterface *self )
{
	struct zone_hypertext *zh = (struct zone_hypertext *) context -> zones[context -> last_zone].custom;

	if (zh)
	{
		zh -> script_action = context -> at;
	}

	set_block_fn(block_skip);
}

void block_button_action( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	set_block_fn(NULL);

	struct zone_button *zb = (struct zone_button *) context -> zones[context -> last_zone].custom;

	if (zb)
	{
		zb -> script_action = context -> at;
	}

	block_skip(context,self);		// does purge set_block_fn
	set_block_fn(NULL);
}

void block_button_render( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	struct zone_button *zb = (struct zone_button *) context -> zones[context -> last_zone].custom;

	if (zb)
	{
		zb -> script_render = context -> at;
	}

	set_block_fn(block_button_action);
}

void button_mouse_event( zone_button *base, struct cmdcontext *context, int mx, int my, int zid)
{
	if ((mx<base -> x0)||(mx>base -> x1)||(my<base -> y0)||(my>base -> y1)) return;

	set_block_fn(NULL);

	if ( base -> script_render)
	{
		for ( base -> value = 1; base -> value >=0  ; base -> value --)
		{
			context -> selected_dialog = 0;
			context -> dialog[0].x =  base -> x0;
			context -> dialog[0].y =  base -> y0;

			execute_interface_sub_script( context, zid, base -> script_render);

			if (( base -> script_action) && ( base -> value == 1))
			{
				execute_interface_sub_script( context, zid, base -> script_action);
			}

			while (instance.engine_mouse_key)	Delay(1);
		}

		base -> event = 1;
		context -> has_return_value = true;
		context -> return_value = zid;
	}
}

void _icmd_Button( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=8)
	{
		struct ivar &zn = context -> stack[context -> stackp-8];
		struct ivar &_x = context -> stack[context -> stackp-7];
		struct ivar &_y = context -> stack[context -> stackp-6];
		struct ivar &_w = context -> stack[context -> stackp-5];
		struct ivar &_h = context -> stack[context -> stackp-4];
		struct ivar &arg6 = context -> stack[context -> stackp-3];
		struct ivar &_min = context -> stack[context -> stackp-2];
		struct ivar &_max = context -> stack[context -> stackp-1];

		if (( zn.type == type_int ) && ( _x.type == type_int )  && ( _y.type == type_int )  && ( _w.type == type_int )  && ( _h.type == type_int )  
				&& ( arg6.type == type_int ) && ( _min.type == type_int ) && ( _max.type == type_int ))
		{
			struct dialog &button = context -> dialog[1];
			struct zone_button *zb = NULL;

			context -> last_zone = zn.num;

			_x.num -= _x.num % 8;

			button.x = _x.num;
			button.y = _y.num;
			button.width = _w.num;
			button.height = _h.num;

			if (context -> zones)
			{
				zb = (struct zone_button *) context -> zones[zn.num].custom;
			}
			else
			{
				return;
			}

			if (zb == NULL)
			{
				zb = new zone_button();

				if (zb)
				{
					zb -> value = 0;
					zb -> script_render = NULL;
					zb -> script_action = NULL;
					il_set_zone( context, zn.num, iz_button,  zb);
				}
			}

			if (zb)
			{
				zb -> x0 = _x.num + get_dialog_x(context);
				zb -> y0 = _y.num + get_dialog_y(context);
				zb -> w = _w.num;
				zb -> h = _h.num;
				zb -> x1 = zb -> x0+zb->w;
				zb -> y1 = zb -> y0+zb->h;
			}

			context -> xgcl = _x.num;
			context -> ygcl =_y.num;
			context -> xgc = _x.num + _w.num;
			context -> ygc = _y.num + _h.num;
		}

		pop_context( context, 8);
	}

	set_block_fn(block_button_render);

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
	context -> exit_run = true;
	context -> at = &(context -> script) -> ptr + context -> script -> size;
	context -> l = 0;
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

			bank1 = findBank(instance.current_resource_bank);
	
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
	retroScreen *screen = instance.screens[instance.current_screen];

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_int ) 
		{	
			if (context -> saved_block) retroFreeBlock( context -> saved_block );

			context -> saved_block = retroAllocBlock( context -> dialog[0].width, context -> dialog[0].height );

			if (context -> saved_block)
			{
				retroGetBlock( screen, screen -> double_buffer_draw_frame, context -> saved_block, context -> dialog[0].x, context -> dialog[0].y );
			} 
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

void _icmd_ui_cmd( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d - %s\n",__FUNCTION__,__LINE__, context -> ui_current -> name);

	if (context -> stackp>= context -> ui_current -> args )
	{
		int n;
		int p = 0;
		struct ivar *arg;


		for (n=-context -> ui_current -> args; n<=-1;n++)
		{
			printf("n: %d\n",n);
			arg = context -> stack + context -> stackp-n;	
			if (arg -> type == type_int ) context -> param[ p ] = arg -> num;
			p ++;
		}

		pop_context( context, context -> ui_current -> args);

		execute_interface_sub_script( context, 0, (char *) context -> ui_current -> action );

	}

	context -> cmd_done = NULL;
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

void icmd_ButtonNoWait( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
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
			push_context_var(context, index);
		}
		else ierror(1) ;
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
						printf("--pop frees(%08x)\n",p.str);

						sys_free (p.str);
						p.str = NULL;
					}
					break;
		}

		pop--;
		context -> stackp--;
	}

	printf("stackp is %d\n",context -> stackp);
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
	else ierror(1);
}

void icmd_NotEqual( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			ret = arg1.num != arg2.num ? ~0 : 0 ;
		}

		pop_context( context, 2);
		push_context_num( context, ret );
	}
	else ierror(1);
}

void icmd_More( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			ret = arg1.num > arg2.num ? ~0 : 0 ;
		}

		pop_context( context, 2);
		push_context_num( context, ret );
	}
	else ierror(1);
}

void icmd_Less( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>1)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			ret = arg1.num < arg2.num ? ~0 : 0 ;
		}

		pop_context( context, 2);
		push_context_num( context, ret );
	}
	else ierror(1);
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
	else ierror(1);
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
	else ierror(1);
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
	else ierror(1);
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
	else ierror(1);
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
	else ierror(1);
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
	else ierror(1);
}

void icmd_TextWidth( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	char tmp[30];
	struct stringData *str = (struct stringData *) tmp;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>0)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		switch ( arg1.type )
		{
			case type_string:
					ret = os_text_width( arg1.str );
					break;
			case type_int:
					sprintf( &str -> ptr, "%d", arg1.num );
					str -> size = strlen(&str -> ptr);
					ret = os_text_width( str );
					break;
			default:
					ret = 0;
		}

		pop_context( context, 1);
		push_context_num( context, ret );
	}
	else ierror(1);
}


void icmd_TextLength( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>0)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		if ( arg1.type == type_string ) 
		{
			ret = os_text_width(arg1.str);
		}
		else ret = 0;

		pop_context( context, 1);
		push_context_num( context, ret );
	}
	else ierror(1);
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

void icmd_Max( struct cmdcontext *context, struct cmdinterface *self )	// Max
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);	

	if (context -> stackp>=2)
	{
		int ret = 0;
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			ret =  (arg1.num > arg2.num) ? arg1.num : arg2.num ;
		}

		pop_context( context, 2);
		push_context_num( context, ret );

			dump_context_stack( context );

	}
	else ierror(1);
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
			struct retroScreen *screen = instance.screens[instance.current_screen];

			if (screen)
			{
				int tl = os_text_width( arg1.str );

				ret = (context -> dialog[ context -> selected_dialog ] .width / 2) - (tl/2)  ;
			}
		}

		pop_context( context, 1);
		push_context_num( context, ret );
	}
	else ierror(1);
}

void icmd_Exit( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> at = &(context -> script -> ptr) + context -> script -> size;
	context -> cmd_done = NULL;
	context -> args = 0;
	context -> l = 0;
}

void icmd_ScreenWidth( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (instance.screens[instance.current_screen])	// check if current screen is open.
	{
		push_context_num( context,instance.screens[instance.current_screen] -> realWidth);
	}
}

void icmd_ScreenHeight( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (instance.screens[instance.current_screen])	// check if current screen is open.
	{
		push_context_num( context,instance.screens[instance.current_screen] -> realHeight);
	}
}

void icmd_NextCmd( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> cmd_done)
	{
		context ->cmd_done(context, self);
		context ->cmd_done = NULL;
	}
}

void isetvarstr( struct cmdcontext *context,int index, struct stringData *str)
{
	struct ivar &var = context -> vars[index];

	if (var.type == type_string)
	{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
		if (var.str) sys_free(var.str);
	}
	else var.type = type_string;
	var.str = amos_strdup(str);
}

void isetvarnum( struct cmdcontext *context,int index,int num)
{
	struct ivar &var = context -> vars[index];

	if (var.type == type_string)
	{
		if (var.str) sys_free(var.str);
		var.str = NULL;
	}

	var.type = type_int;
	var.num = num;
}

struct stringData *igetvarstr( struct cmdcontext *context, int index)
{
	struct ivar &var = context -> vars[index];
	return (var.type == type_string) ? amos_strdup(var.str) : NULL;
}

int igetvarnum( struct cmdcontext *context,int index )
{
	struct ivar &var = context -> vars[index];
	return (var.type == type_int) ? var.num : 0;
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

	struct zone_base *zb = context -> zones[context -> last_zone].custom;

	if (zb)
	{
		push_context_num( context, zb -> value );
	}
}

void icmd_ZonePosition( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	struct zone_base *zb = context -> zones[context -> last_zone].custom;

	if (zb)
	{
		push_context_num( context, zb -> pos );
	}
}

void icmd_ZoneNumber( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	push_context_num( context, context -> last_zone );
}

void icmd_ButtonPosition( struct cmdcontext *context, struct cmdinterface *self )
{
	struct zone_base *zb;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("context -> last_zone: %d\n", context -> last_zone);

	zb = context -> zones[context -> last_zone].custom;

	push_context_num( context, zb ? zb -> value : 0 );
}

#define skip_spaces while ( *context -> at == ' ' ) context -> at++
#define is(sym) (*context -> at == sym )

int get_num( struct cmdcontext *context )
{
	int r =0;
	char c = *context -> at;

	while ((c>='0')&&(c<='9'))
	{
		printf("%c\n",c);
		r = (r*10) + (c - '0');
		c = *(++context -> at);
	}
	return r;
}

void test_UserInstruction( struct cmdcontext *context, struct cmdinterface *self )
{
	userDefined *ud;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	context -> at += 2;

	skip_spaces;

	ud = push_context_ui( context  );

	// if can't create it, maybe its in the list from before.
	if (ud == NULL) ud = context -> findUserDefined( context -> at );

	if (ud)
	{
		bool success = false;
		context -> at += strlen(ud -> name);

		skip_spaces;

		printf("%c\n",*context -> at);

		if (is(','))
		{
			context -> at++;
			skip_spaces;
			ud -> args = get_num(context);
			skip_spaces;
			success = true;
			if (is(';'))
			{
				context -> at++;
				skip_spaces;
				ud -> action = context -> at;
			}
		}
		
		if (success== false)
		{
			context -> error = true;
		}

		printf("%s\n", ud -> name);
	}

	context -> at += 2;	// we skip the command name, this is a hack!!!

	context -> at -= 2;
}

void icmd_UserInstruction( struct cmdcontext *context, struct cmdinterface *self )
{
	const char *at;
	int block = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	// skip this command, should not be executed, its defined under test

	context -> at += 2;	// skip current command name "UI".

	// find the end of code block for the UI command.

	at = context -> at;
	while ( *at != 0 )
	{
		switch (*at) 
		{
			case '[':
				block++;
				break;

			case ']':
				at++;
				block--;
				break;
		}
		at++;
	}

	context -> at = (char *) at;

	// next should be the new command.
	printf("---%.5s\n", context -> at);

	context -> at -= 2;	// undo skip of "UI" command name.
}

struct cmdinterface symbols[]=
{

	{"=",i_parm,NULL,icmd_Equal },
	{"\\",i_parm,NULL,icmd_NotEqual},
	{">",i_parm,NULL,icmd_More },
	{"<",i_parm,NULL,icmd_Less },
	{";",i_normal,icmd_NextCmd,icmd_NextCmd},
	{"[",i_normal,NULL,icmd_block_start},
	{"]",i_normal,NULL,icmd_block_end},
	{",",i_parm,NULL,icmd_Comma},
	{"+",i_parm,NULL,icmd_Plus},
	{"-",i_parm,NULL,icmd_Minus},
	{"*",i_parm,NULL,icmd_Mul},
	{"/",i_parm,NULL,icmd_Div},
	{"%",i_parm,icmd_Bin,icmd_Bin},
	{"$",i_parm,icmd_Hex,icmd_Hex},

	{NULL,i_normal,NULL,NULL}
};

struct cmdinterface commands[]=
{
	{"BA",i_normal,NULL,icmd_Base},
	{"BP",i_parm,NULL,icmd_ButtonPosition },
	{"BO",i_normal,NULL,icmd_ImageBox },
	{"BR",i_normal,NULL,icmd_ButtonReturn },
	{"BQ",i_normal,NULL,icmd_ButtonQuit },
	{"BU",i_normal,NULL,icmd_Button},
	{"BX",i_parm,NULL,icmd_BaseX},
	{"BY",i_parm,NULL,icmd_BaseY},
	{"CX",i_parm,NULL,icmd_cx},
	{"CT",i_normal,NULL,icmd_ct},
	{"ED",i_normal,NULL,icmd_Edit},
	{"EX",i_normal,NULL,icmd_Exit},
	{"GB",i_normal,NULL,icmd_GraphicBox},
	{"GS",i_normal,NULL,icmd_GraphicSquare},
	{"GE",i_normal,NULL,NULL},
	{"GL",i_normal,NULL,icmd_GraphicLine},
	{"HS",i_normal,NULL,icmd_HorizontalSlider },
	{"HT",i_normal,NULL,icmd_HyperText},
	{"IF",i_normal,NULL,icmd_If},
	{"IN",i_normal,NULL,icmd_Ink},
	{"JP",i_normal,NULL,icmd_Jump},
	{"JS",i_normal,NULL,icmd_JumpSubRutine},
	{"LA",i_normal,ipass_label, icmd_label },
	{"LI",i_normal,NULL,icmd_Imagehline },
	{"VL",i_normal,NULL,icmd_imagevline },
	{"KY",i_normal,NULL,icmd_KeyShortCut},
	{"MA",i_parm,NULL,icmd_Max },	
	{"MI",i_parm,NULL,icmd_Min},
	{"NW",i_normal,NULL,icmd_ButtonNoWait},
	{"P1",i_parm,NULL,icmd_param },
	{"P2",i_parm,NULL,icmd_param },
	{"P3",i_parm,NULL,icmd_param },
	{"P4",i_parm,NULL,icmd_param },
	{"P5",i_parm,NULL,icmd_param },
	{"P6",i_parm,NULL,icmd_param },
	{"P7",i_parm,NULL,icmd_param },
	{"P8",i_parm,NULL,icmd_param },
	{"P9",i_parm,NULL,icmd_param },
	{"PR",i_normal,NULL,icmd_Print},
	{"PO",i_normal,NULL,icmd_PrintOutline},
	{"PU",i_normal,NULL,icmd_PushImage},
	{"ME",i_parm,NULL,icmd_Message},
	{"SA",i_normal,NULL,icmd_Save},
	{"SH",i_parm,NULL,icmd_ScreenHeight},
	{"SI",i_normal,NULL,icmd_Dialogsize},
	{"SM",i_parm,NULL,icmd_ScreenMove},
	{"SP",i_normal,NULL,NULL},
	{"SV",i_normal,NULL,icmd_SetVar },
	{"SW",i_parm,NULL,icmd_ScreenWidth},
	{"SX",i_parm,NULL,icmd_SizeX},
	{"SY",i_parm,NULL,icmd_SizeY},
	{"SZ",i_normal,NULL,icmd_SetZone},
	{"RB",i_normal,NULL,icmd_RenderButton},
	{"RT",i_normal,NULL,icmd_Return},
	{"RU",i_normal,NULL,icmd_Run},
	{"TH",i_parm,NULL,icmd_TextHeight},
	{"TL",i_parm,NULL,icmd_TextLength},
	{"TW",i_parm,NULL,icmd_TextWidth},
	{"UN",i_normal,NULL,icmd_Unpack},
	{"UI",i_normal,test_UserInstruction, icmd_UserInstruction },
	{"VA",i_parm,NULL,icmd_Var},
	{"VS",i_normal,NULL,icmd_VerticalSlider },
	{"VT",i_normal,NULL,NULL},
	{"XA",i_parm,NULL,icmd_XGCL},
	{"YA",i_parm,NULL,icmd_YGCL},
	{"XB",i_parm,NULL,icmd_XGC},
	{"YB",i_parm,NULL,icmd_YGC},
	{"XY",i_parm,NULL,NULL},
	{"ZN",i_parm,NULL,icmd_ZoneNumber},
	{"ZP",i_parm,NULL,icmd_ZonePosition},
	{"ZC",i_normal,NULL,icmd_ZoneChange},
	{"ZV",i_parm,NULL,icmd_ZoneValue},
	{"=",i_parm,NULL,icmd_Equal},
	{";",i_parm,icmd_NextCmd,icmd_NextCmd},		// next command or end of command.
	{"[",i_normal,NULL,icmd_block_start},
	{"]",i_normal,NULL,icmd_block_end},
	{",",i_parm,NULL,icmd_Comma},
	{"+",i_parm,NULL,icmd_Plus},
	{"-",i_parm,NULL,icmd_Minus},
	{"*",i_parm,NULL,icmd_Mul},
	{"/",i_parm,NULL,icmd_Div},
	{"\\",i_parm,NULL,icmd_NotEqual},
	{">",i_parm,NULL,icmd_More },
	{"<",i_parm,NULL,icmd_Less },

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
			while (((*c>='a')&&(*c<='z'))||(*c=='#')||(*c=='\n'))	{ c++;  }

			if (d!=txt)
			{
				char ld = *(d-1);
				if ( ((ld==' ')||(ld==',')||(ld==';'))&&(*c==' ') )	space_repeat = true;
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

			printf("%s, '%c'\n", cmd -> name,c);

			if ((c == ' ')||(c=='\'')||(c == 0)) return num;
			if ((c>='0')&&(c<='9')) return num;
			if (is_command(at+l)) return num;
		}
		num++;
	}
	return -1;
}

bool is_string( char *at, struct stringData *&str, int &l )
{
	l=0;

	if (*at!='\'') return false;

	at++;
	{
		char *p;
		for (p = at; ((*p !='\'') && (*p!=0));p++ ) l++;
		str =toAmosString( at, l );
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

void push_context_string(struct cmdcontext *context, struct stringData *str)
{
	struct ivar &self = context -> stack[context -> stackp];
	self.type = type_string;
	self.str = str;
	context -> stackp++;

	printf("push %s\n",&str -> ptr);
}

void push_context_var(struct cmdcontext *context, int index)
{
	struct ivar &stackd = context -> stack[context -> stackp];

	if (index < 0) return;
	if (index >= 20 ) return;

	if ( context -> vars)
	{
		struct ivar &var = context -> vars[index];

		switch (var.type )
		{
			case type_string:

					stackd.type = type_string;
					stackd.str = amos_strdup( var.str );
					stackd.num = 0;
					context -> stackp++;
					break;

			case type_int:

					stackd.type = type_int;
					stackd.str = NULL;
					stackd.num = var.num;
					context -> stackp++;
					break;
		}
	}
	else
	{
		printf("interface context not initialized\n");
		ierror(1);
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

void init_interface_context( struct cmdcontext *context, int id, struct stringData *script, int x, int y, int varSize, int bufferSize  )
{
	int n;
	struct dialog &dialog = context -> dialog[0];

	bzero( context, sizeof( struct cmdcontext ) );

	remove_lower_case( &script->ptr );

	context -> id = id;
	context -> stackp = 0;
	context -> script = amos_strdup( script );
	context -> at = &(context -> script -> ptr);
	context -> max_vars = varSize;
	context -> selected_dialog = 0;
	context -> block_level = 0;
	context -> saved_block = NULL;

	context -> zones = (struct izone *) malloc( sizeof(struct izone) * 20 );

	context -> vars = (struct ivar *) malloc( sizeof(struct ivar) * varSize  );

	if (context -> vars)
	{
		for (n =0;n<varSize;n++)
		{
			context -> vars[n].type = 0;
			context -> vars[n].num = 0;
		}
	}

	context -> block_fn = (void (**)( struct cmdcontext *, struct cmdinterface * )) malloc( sizeof(void *) * 20  );

	dialog.x = x - (x % 16) ;
	dialog.y = y;

	for (n=0;n<20;n++) 
	{
		context -> zones[n].type = iz_none;
		context -> zones[n].custom = NULL;
	}

	for (n=0;n<20;n++) context -> block_fn[n] = NULL;

}

void cleanup_interface_context( struct cmdcontext *context )
{
	context -> at = NULL;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> saved_block)
	{
		retroFreeBlock( context -> saved_block );
		context -> saved_block = NULL;
	}

	if (context -> zones)
	{
		int n=0;
		for (n=0;n<20;n++)
		{
			context -> zones[n].type = 0;
			if (context -> zones[n].custom) 
			{
				delete context -> zones[n].custom;
				context -> zones[n].custom = NULL;
			}
		}
		free(context -> zones);
		context -> zones = NULL;
	}

	if (context -> script)
	{
		sys_free( context -> script );
		context -> script = NULL;
	}

	if (context -> block_fn)
	{
		free(	context -> block_fn );
		context -> block_fn = NULL;
	}

	if (context -> vars) 
	{
		free(context -> vars);
		context -> vars = NULL;
	}
}

userDefined *push_context_ui( struct cmdcontext *context )
{
	userDefined ud;
	strncpy( ud.name, context -> at, 2 );

	if (context -> findUserDefined( ud.name ) == NULL)
	{
		ud.len = strlen( ud.name );	 // just nice to store the length, we know it should 2 chars, but way not also support 1 char commands.
		ud.action = NULL;
		printf("storing possible UI command %s\n", ud.name);
		context -> userDefineds.push_back(ud);
		return &(context -> userDefineds.back());
	}

	return NULL;
}

void test_interface_script( struct cmdcontext *context)
{
	int sym,cmd;
	int num;
	struct stringData *str = NULL;

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
				else 	// Must be a user defined command, so we keep it. if not we know it its not when the test is done.
				{
					userDefined *ud = push_context_ui( context );
					if (ud)
					{
						context -> l = strlen(ud -> name);
					}
					else // if we can't create it, it most exist before, and if not its a error.
					{
						userDefined *ud = context -> findUserDefined( context -> at );

						if ( ud )
						{ context -> l = ud -> len; }
						else
						{ context -> error = true; }
					}
				}
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

		if (context -> zones  == NULL)
		{
			printf("context zones died here -> context stack %d\n", context -> stackp );
			getchar();
			break;
		}

		if (context -> vars  == NULL)
		{
			printf("context vars died here -> context stack %d\n", context -> stackp );
			getchar();
			break;
		}

		context -> at += context -> l;
	}
	pop_context( context, context -> stackp );


	if (*context -> at == 0) 		printf("test exited becouse its at \\0 char symbol\n");
	if (context -> error != false)	printf("test exited becouse of error\n");

	context -> tested = true;
}

void execute_interface_sub_script( struct cmdcontext *context, int zone, char *at)
{
	int sym,cmd;
	int num;
	int initial_block_level = context -> block_level;
	int initial_command_length = context -> l;

	struct stringData *str = NULL;
	char *backup_at = context -> at;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	context -> last_zone = zone;
	context -> at = at;

	if (context -> at == 0) 
	{
		ierror(1);
		context -> at = backup_at;
		context -> l = initial_command_length;
		return ;
	}

	if (*context -> at == '[') 
	{
		context -> at ++;
	}
	else context -> error =1;

	while ((*context -> at != 0) && (context -> error == false) && (context -> has_return_value == false))
	{
		if (initial_block_level == context -> block_level)
		{
			if (*context -> at == ']') break;		// time to exit block.
		}

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
				else 	
				{
					struct userDefined *ud = context -> findUserDefined( context -> at );

					if (ud)
					{
						Printf("found this ud command\n");
					}
					else
					{
						context -> error = true;
						break;
					}
				}
			}
			else 	
			{
				struct cmdinterface *icmd = &commands[cmd];

				if (icmd -> type == i_normal)
				{
					if (context -> stackp >0)
					{
						printf("can't execute command '%s'\n",icmd -> name);
						printf("Interface language: there is stuff on the stack, there shoud be none.\n");
						dump_context_stack( context );
						getchar();
						context -> error = true;
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
	}

	if (context -> error)
	{
		printf("error at: {%s}\n",context -> at);
		getchar();
	}

	// return to where we left off, restore damaged data.

	context -> at = backup_at;	
	context -> l = initial_command_length;
}


void execute_interface_script( struct cmdcontext *context, int32_t label)
{
	int sym,cmd;
	int num;
	struct stringData *str = NULL;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( context -> zones == NULL )
	{
		printf("%s:%s context zones damged\n",__FILE__,__FUNCTION__);
		getchar();
		return;
	}

	if ( context -> vars == NULL )
	{
		printf("%s:%s context vars damged\n",__FILE__,__FUNCTION__);
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
		context -> dumpUserDefined();
		getchar();
	}

	context -> at = &(context -> script -> ptr);	// default

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

//		printf("%s\n", context -> at);

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
				else 	
				{
					context -> ui_current = context -> findUserDefined( context -> at );

					if (context -> ui_current)
					{
						Printf("found this ud command - %s\n", context -> ui_current -> name);
						context -> l = strlen(context -> ui_current -> name);
						context -> args = context -> ui_current -> args;
						context -> cmd_done = _icmd_ui_cmd;
					}
					else
					{
						printf("not a command, not string, not a number, not a user defined command\n");

						context -> error = true;
						break;
					}
				}
			}
			else 	
			{
				struct cmdinterface *icmd = &commands[cmd];

				printf("%s:%s\n",__FUNCTION__,icmd -> name);

				if (icmd -> type == i_normal)
				{
					if (context -> stackp >0)
					{
						printf("can't execute command '%s'\n",icmd -> name);
						printf("at location %d\n", (unsigned int) ((context -> at) - (&context -> script -> ptr)) );
						printf("Interface language: there is stuff on the stack, there shoud be none.\n");
						dump_context_stack( context );
 						ierror(1);
					}
				}

				if (icmd -> cmd)
				{
					icmd -> cmd( context, icmd );
					printf("%s:context -> error %d\n", __FUNCTION__, context -> error );
				}
				else
				{
					printf("ignored %s\n", icmd -> name);
					getchar();
				}
			}
		}

		context -> at += context -> l;

		if (context -> zones  == NULL)
		{
			printf("context zones died here -> context stack %d\n", context -> stackp );
			getchar();
			break;
		}

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
		if (context -> at)

		printf("ended at: {%s} %d\n",context -> at,*context -> at);
		dump_context_stack( context );
		getchar();
	}
}

