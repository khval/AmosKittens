
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

extern int current_screen;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];


void isetvarstr( struct cmdcontext *context, int index, char *str );
void isetvarnum( struct cmdcontext *context, int index, int num );

void dump_context_stack( struct cmdcontext *context );
void pop_context( struct cmdcontext *context, int pop );
void push_context_num(struct cmdcontext *context, int num);
void push_context_string(struct cmdcontext *context, char *str);
void push_context_var(struct cmdcontext *context, int num);


void _icmdif( struct cmdcontext *context, struct cmdinterface *self )
{
	char *at;
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	at = context->at;

	if (context -> stackp > 0)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-1];

		// check if value is number and is false.

		if  (( arg1.type == type_int ) && (arg1.num == 0))
		{
			int count = 0;

			if (*at)
			{
				at++;
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
				else	// skip next command.
				{
					while (*at)
					{
						if (*at == ';')
						{
							context->at =at+1;		// set new location.
							context->l = 0;		// reset length of command.
							break;
						}
						at ++;
					}
				}
			}
		}	else context -> error = 1;

		pop_context( context, 1);

	} else context -> error = 1;
}

void icmdif( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmdif;
	context -> lstackp = context -> stackp;
	context -> args = 1;
}

void _icmddialogsize( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=2)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			context -> dialog.width = arg1.num;
			context -> dialog.height = arg2.num;
		}

		pop_context( context, 2 );
	}
	else context -> error = 1;

	pop_context( context, 2);
	context -> cmd_done = NULL;
}

void icmddialogsize( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmddialogsize;
	context -> lstackp = context -> stackp;
	context -> args = 2;
}

extern void os_text(struct retroScreen *screen,int x, int y, char *txt);
void os_text_outline(struct retroScreen *screen,int x, int y, char *txt, uint16_t pen,uint16_t outline);

void _icmdPrint( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=4)
	{
		struct retroScreen *screen = screens[current_screen];

		if (screen)
		{
			int x = context -> stack[context -> stackp-4].num;
			int y = context -> stack[context -> stackp-3].num;
			int o = context -> stack[context -> stackp-1].num;

			x+=context -> dialog.x;
			y+=context -> dialog.y;

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

	pop_context( context, 4 );
	context -> cmd_done = NULL;
}

void icmdPrint( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmdPrint;
	context -> lstackp = context -> stackp;
	context -> args = 4;
}

void icmdComma( struct cmdcontext *context, struct cmdinterface *self )
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

void _icmdPrintOutline( struct cmdcontext *context, struct cmdinterface *self )
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
			uint16_t pen = context -> stack[context -> stackp-2].num;
			uint16_t outline = context -> stack[context -> stackp-1].num;

			x+=context -> dialog.x;
			y+=context -> dialog.y;

			if (txt)	os_text_outline(screen, x,y,txt,pen,outline);

		}
	}

	pop_context( context, 5);
	context -> cmd_done = NULL;
}

void icmdPrintOutline( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmdPrintOutline;
	context -> lstackp = context -> stackp;
	context -> args = 5;
}

// icmdSetVar

void _icmdSetVar( struct cmdcontext *context, struct cmdinterface *self )
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

void icmdSetVar( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmdSetVar;
	context -> lstackp = context -> stackp;
	context -> args = 2;
}

// ----

void _icmdInk( struct cmdcontext *context, struct cmdinterface *self )
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

void icmdInk( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmdInk;
	context -> args = 3;
}

void _icmdGraphicBox( struct cmdcontext *context, struct cmdinterface *self )
{
	struct retroScreen *screen = screens[current_screen];

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=4)
	{
		int x0 = context -> stack[context -> stackp-4].num;
		int y0 = context -> stack[context -> stackp-3].num;
		int x1 = context -> stack[context -> stackp-2].num;
		int y1 = context -> stack[context -> stackp-1].num;

		x0+=context -> dialog.x;
		y0+=context -> dialog.y;
		x1+=context -> dialog.x;
		y1+=context -> dialog.y;

		if (screen) retroBAR( screen, x0,y0,x1,y1,context -> ink0 );
	}

	pop_context( context, 4);
	context -> cmd_done = NULL;
}

void icmdGraphicBox( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmdGraphicBox;
	context -> args = 4;
}

void _icmdBase( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=2)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-2];
		struct ivar &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			context -> dialog.x = arg1.num;
			context -> dialog.y = arg2.num;
		}

		pop_context( context, 2);
	}

	context -> cmd_done = NULL;
}

void icmdBase( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmdBase;
	context -> args = 2;
}

void _icmdUnpack( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>=3)
	{
		struct ivar &arg1 = context -> stack[context -> stackp-3];
		struct ivar &arg2 = context -> stack[context -> stackp-2];
		struct ivar &arg3 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ) && ( arg3.type == type_int ) )
		{
			printf("unpack %d,%d,%d\n", arg1.num, arg2.num, arg3.num);
		}

		pop_context( context, 3);
	}

	context -> cmd_done = NULL;
}

void icmdUnpack( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmdUnpack;
	context -> args = 3;
}


void _icmdSave( struct cmdcontext *context, struct cmdinterface *self )
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

void icmdSave( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmdSave;
	context -> args = 1;
}

void icmdvar( struct cmdcontext *context, struct cmdinterface *self )
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

void icmdequal( struct cmdcontext *context, struct cmdinterface *self )
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

void icmdplus( struct cmdcontext *context, struct cmdinterface *self )
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

void icmdminus( struct cmdcontext *context, struct cmdinterface *self )
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

void icmdmul( struct cmdcontext *context, struct cmdinterface *self )
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

void icmddiv( struct cmdcontext *context, struct cmdinterface *self )
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

void icmdMin( struct cmdcontext *context, struct cmdinterface *self )
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


void icmdTextWidth( struct cmdcontext *context, struct cmdinterface *self )
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

void icmdSizeX( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	push_context_num( context, context -> dialog.width );
}

void icmdSizeY( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	push_context_num( context, context -> dialog.height );
}

void icmdScreenWidth( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	push_context_num( context, -999 );
}

void icmdScreenHeight( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	push_context_num( context, -999 );
}

void icmdnextcmd( struct cmdcontext *context, struct cmdinterface *self )
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

struct cmdinterface symbols[]=
{
	{"=",i_parm,icmdequal },
	{";",i_normal,icmdnextcmd},
	{"[",i_normal,NULL},
	{"]",i_normal,NULL},
	{",",i_parm,icmdComma},
	{"+",i_parm,icmdplus},
	{"-",i_parm,icmdminus},
	{"*",i_parm,icmdmul},
	{"/",i_parm,icmddiv},
//	{"%",i_parm,NULL},
	{NULL,i_normal, NULL}
};

struct cmdinterface commands[]=
{
	{"BA",i_normal,icmdBase},
//	{"BP",
	{"BO",i_normal,NULL},
	{"BR",i_normal,NULL},
//	{"BQ",
	{"BU",i_normal,NULL},
	{"BX",i_parm,NULL},
	{"BY",i_parm,NULL},
//	{"CX",
	{"ED",i_normal,NULL},
	{"EX",i_normal,NULL},
	{"GB",i_normal,icmdGraphicBox},
	{"GE",i_normal,NULL},
	{"GL",i_normal,NULL},
	{"HT",i_normal,NULL},
	{"IF",i_normal,icmdif},
	{"IN",i_normal,icmdInk},
	{"JS",i_normal,NULL},
	{"LA",i_normal,NULL},
	{"KY",i_normal,NULL},
	{"MI",i_parm,icmdMin},
	{"PR",i_normal,icmdPrint},
	{"PO",i_normal,icmdPrintOutline},
	{"ME",i_parm,NULL},
	{"SA",i_normal,icmdSave},
	{"SH",i_parm,icmdScreenHeight},
	{"SI",i_normal,icmddialogsize},
	{"SM",i_parm,NULL},
	{"SP",i_normal,NULL},
	{"SV",i_normal,icmdSetVar },
	{"SW",i_parm,icmdScreenWidth},
	{"SX",i_parm,icmdSizeX},
	{"SY",i_parm,icmdSizeY},
	{"RT",i_normal,NULL},
//	{"RU",
	{"TH",i_parm,NULL},
	{"TL",i_parm,NULL },
	{"TW",i_parm,icmdTextWidth},
	{"UN",i_normal,icmdUnpack},
	{"VA",i_parm,icmdvar},
	{"VT",i_normal,NULL},
	{"XB",i_parm,NULL},
	{"XY",i_parm,NULL},
	{"YB",i_parm,NULL},
	{"ZN",i_parm,NULL},
	{"=",i_parm,icmdequal },
	{";",i_normal,icmdnextcmd},
	{"[",i_normal,NULL},
	{"]",i_normal,NULL},
	{",",i_parm,icmdComma},
	{"+",i_parm,icmdplus},
	{"-",i_parm,icmdminus},
	{"*",i_parm,icmdmul},
	{"/",i_parm,icmddiv},
//	{"%",i_parm,NULL},
	{NULL,i_normal, NULL}
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
	char c;

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
	int n;
	bzero( context, sizeof( struct cmdcontext ) );

//	for (n=0;n<10;n++) isetvarnum(context,n,n*10);

	remove_lower_case( script );

	context -> id = id;
	context -> stackp = 0;
	context -> script = strdup( script );
	context -> at = context -> script;

	context -> dialog.x = x;
	context -> dialog.y = y;
}

void execute_interface_script( struct cmdcontext *context)
{
	int sym,cmd;
	int n;
	int num;
	char *str = NULL;

	isetvarnum(context,0,2); 
	isetvarstr(context,1,"Hello World");

	while (*context -> at != 0)
	{
		while (*context -> at==' ') context -> at++;

		printf("{%s}\n",context -> at);

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

		dump_context_stack( context );
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

