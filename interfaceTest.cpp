
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

struct stack
{
	int type;
	int num;
	char *str;
};

struct cmdcontext
{
	int stackp;
	struct stack stack[10];
	uint32_t vars[512];	
	uint32_t labels[512];
	void (*cmd_done)( struct cmdcontext *context, struct cmdinterface *self );
	int args;
	int error;
	char *at;
	int l;
};

struct cmdinterface
{
	const char *name;
	int type;
	void (*cmd)( struct cmdcontext *context, struct cmdinterface *self );
};

enum
{
	i_normal,
	i_parm
};


void push_context_num(struct cmdcontext *context, int num);


void _icmdif( struct cmdcontext *context, struct cmdinterface *self )
{
	char *at;
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	at = context->at;

	if (context -> stackp > 0)
	{
		struct stack &arg1 = context -> stack[context -> stackp-1];

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
	} else context -> error = 1;
}

void icmdif( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	context -> cmd_done = _icmdif;
	context -> args = 1;
}

void icmdvar( struct cmdcontext *context, struct cmdinterface *self )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>0)
	{
		struct stack &self = context -> stack[context -> stackp-1];

		if ( self.type == type_int)
		{
			self.num = context -> vars[ self.num ];
		}
	}
	else context -> error = 1;
}

void pop_context( struct cmdcontext *context, int pop )
{
	while ((pop)&&(context->stackp))
	{
		struct stack &p = context -> stack[--context -> stackp];

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
	}
}

void icmdequal( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> stackp>1)
	{
		struct stack &arg1 = context -> stack[context -> stackp-2];
		struct stack &arg2 = context -> stack[context -> stackp-1];

		if (( arg1.type == type_int ) && ( arg2.type == type_int ))
		{
			ret = arg1.num == arg2.num ? ~0 : 0 ;
		}

		pop_context( context, 1);
		push_context_num( context, ret );
	}
	else context -> error = 1;
}

void icmdnextcmd( struct cmdcontext *context, struct cmdinterface *self )
{
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (context -> cmd_done)
	{
		return context ->cmd_done(context, self);
		context ->cmd_done = NULL;
	}
}



struct cmdinterface commands[]=
{
	{"BA",i_normal,NULL},
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
	{"GB",i_normal,NULL},
	{"GE",i_normal,NULL},
	{"GL",i_normal,NULL},
	{"HT",i_normal,NULL},
	{"IF",i_normal,icmdif},
	{"IN",i_normal,NULL},
	{"JS",i_normal,NULL},
	{"LA",i_normal,NULL},
	{"KY",i_normal,NULL},
//	{"MI",
	{"PR",i_normal,NULL},
	{"PO",i_normal,NULL},
	{"ME",i_parm,NULL},
	{"SA",i_normal,NULL},
	{"SH",i_parm,NULL},
	{"SI",i_normal,NULL},
	{"SM",i_parm,NULL},
	{"SP",i_normal,NULL},
	{"SW",i_parm,NULL},
	{"SX",i_parm,NULL},
	{"SY",i_parm,NULL},
	{"RT",i_normal,NULL},
//	{"RU",
	{"TH",i_parm,NULL},
	{"TL",i_parm,NULL },
	{"TW",i_parm,NULL},
	{"UN",i_normal,NULL},
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
	{",",i_parm,NULL},
	{"+",i_parm,NULL},
	{"-",i_parm,NULL},
	{"/",i_parm,NULL},
//	{"%",i_parm,NULL},
	{NULL,i_normal, NULL}
};



void remove_lower_case(char *txt)
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
			if ( ! ((c <= 'A') && (c >= 'Z')) )	return num;
		}
		num++;
	}
	return -1;
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
	struct stack &self = context -> stack[context -> stackp];
	self.type = 0;
	self.num = num;
	context -> stackp++;

	printf("push %d\n",num);
}

void read_script(char *script)
{
	int cmd;
	int n;
	int num;
	struct cmdcontext context;

	for (n=0;n<10;n++) context.vars[n]=n*10;

	 context.vars[0] = 2; 

	context.stackp = 0;
	context.at = script;

	while (*context.at != 0)
	{
		while (*context.at==' ') context.at++;

		cmd = find_command( context.at, context.l );
		if (cmd == -1) 
		{
			if (is_number(context.at, num, context.l))
			{
				push_context_num( &context, num );
			}
			else 	break;
		}
		else 	
		{
			struct cmdinterface *icmd = &commands[cmd];
			if (icmd -> cmd)
			{
				icmd -> cmd( &context, icmd );
			}
			else printf("ignored %s\n", icmd -> name);
		}

		context.at += context.l;
	}

}

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
		remove_lower_case( script_mem );
		printf("%s\n",script_mem);
		
		printf("\n\n");

		read_script(script_mem);

		free(script_mem);
	}

	return 0;
}


