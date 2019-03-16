
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
	struct stack *stack;
	uint32_t vars[512];	
	uint32_t labels[512];
};

struct cmdinterface
{
	const char *name;
	int type;
	char *cmd( struct cmdcontext *context, struct cmdinterface *self );
};

enum
{
	i_normal,
	i_parm
};

struct cmdinterface *commands[]=
{
	{"BA",i_normal,NULL},
//	{"BP",
	{"BO",i_normal,NULL},
	{"BR",i_normal,NULL},
	{"BQ",
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
	{"IF",i_normal,NULL},
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
	{"TL",i_param,NULL },
	{"TW",i_parm,NULL},
	{"UN",i_normal,NULL},
	{"VA",i_parm,NULL},
	{"VT",i_normal,NULL},
	{"XB",i_parm,NULL},
	{"XY",i_parm,NULL},
	{"YB",i_parm,NULL},
	{"ZN",i_parm,NULL},
	{"=",i_parm,NULL},
	{";",i_normal,NULL},
	{"[",i_normal,NULL},
	{"]",i_normal,NULL},
	{",",i_parm,NULL},
	{"+",i_parm,NULL},
	{"-",i_parm,NULL},
	{"/",i_parm,NULL},
	{"%",i_parm,NULL},
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
	const char **cmd;
	int num = 0;
	char c;

	for (cmd = commands; *cmd ; cmd++)
	{
		l = strlen(*cmd);
		if (strncmp(*cmd,at,l)==0)
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

void read_script(char *script)
{
	int cmd;
	int l;
	int num;
	char *at = script;

	while (*at != 0)
	{
		while (*at==' ') at++;

//		dump( at );

		cmd = find_command( at, l );
		if (cmd == -1) 
		{
			if (is_number(at, num, l))
			{
				printf("%d\n",num);
			}
			else 	break;
		}
		else 	printf ("%s\n", commands[cmd] );

		at += l;
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


