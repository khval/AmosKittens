
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "debug.h"
#include "errors.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;

extern void setStackStr( char *str );
extern void setStackStrDup( const char *str );

using namespace std;

#define NEXT_TOKEN(ptr) *((short *) ptr)
#define NEXT_INT(ptr) *((int *) (ptr+2))

/*********

string names like _xxx is read only (const char), and need to copied.
string names like xxx is new and can saved on stack, with out being copied.

*********/


char *_stackString( int n )
{
	if (kittyStack[n].type == type_string)
	{
		return (kittyStack[n].str);
	}
	return NULL;
}

int _stackInt( int n )
{
	if (kittyStack[n].type == type_int)
	{
		return (kittyStack[n].value);
	}
	return 0;
}

char *_print( struct glueCommands *data )
{
	int n;
	printf("PRINT: ");

	for (n=data->stack;n<=stack;n++)
	{
//		printf("stack %d, type: %d value %d\n",n, kittyStack[n].type, kittyStack[n].value);

		switch (kittyStack[n].type)
		{
			case type_int:
				printf("%d", kittyStack[n].value);
				break;
			case type_float:
				printf("%f", kittyStack[n].decimal);
				break;
			case type_string:
				if (kittyStack[n].str) printf("%s", kittyStack[n].str);
				break;
		}

		if (n<=stack) printf("    ");
	}
	printf("\n");

	dump_stack();

	return NULL;
}

char *cmdPrint(nativeCommand *cmd, char *ptr)
{
	dump_stack();

	stackCmdNormal( _print, ptr );
	return ptr;
}

char *_left( struct glueCommands *data )
{
	int args = stack - data->stack ;
	char *str;
	char *tmp = NULL;
	int _len;

	if (args == 2)
	{
		str = _stackString( data->stack + 1 );
		_len = _stackInt( data->stack + 2 );
		tmp = strndup(str, _len );

		printf("len %d\n", _len);
	}	

	popStack(args);

	if (tmp) setStackStr(tmp);

	return NULL;
}


char *_mid( struct glueCommands *data )
{
	int args = stack - data->stack ;
	char *str;
	char *tmp = NULL;
	int _start, _len;

	printf("%s: args %d\n",__FUNCTION__,args);

	if (args == 3)
	{
		str = _stackString( data->stack + 1 );
		_start = _stackInt( data->stack + 2 ) -1;
		_len = _stackInt( data->stack + 3 );

		if (_start<0) _start = 0;
		if (_start>strlen(str)-1) _start = strlen(str)-1;

		tmp = strndup(str + _start, _len );

		printf("len %d\n", _len);
	}	

	popStack(args);

	if (tmp) setStackStr(tmp);

	return NULL;
}

char *_right( struct glueCommands *data )
{
	int args = stack - data->stack ;
	char *str;
	char *tmp = NULL;
	int _start, _len;

	printf("%s: args %d\n",__FUNCTION__,args);

	if (args == 2)
	{
		str = _stackString( data->stack + 1 );
		_len = _stackInt( data->stack + 2 );
		if (_len>strlen(str)) _start = strlen(str);

		tmp = strdup(str + strlen(str) - _len );
	}	

	popStack(args);

	if (tmp) setStackStr(tmp);

	return NULL;
}

char *_instr( struct glueCommands *data )
{
	int args = stack - data->stack ;
	char *str,*find, *ret;
	char *tmp = NULL;
	int  _pos = 0;

	printf("%s: args %d\n",__FUNCTION__,args);

	dump_stack();

	if (args == 2)
	{
		str = _stackString( data->stack + 1 );
		find = _stackString( data->stack + 2 );

		ret = strstr( str, find );

		printf("%s,%s,%s\n",str,find,ret ? ret : "NULL");

		_pos = ret ? (unsigned int) (ret - str) +1 : 0;
	}	

	popStack(args);

	_num( _pos );

	return NULL;
}

char *_str( struct glueCommands *data )
{
	int args = stack - data->stack ;
	int num;
	char _str[30];

	printf("%s: args %d stack %d\n",__FUNCTION__,args,stack);

	num = _stackInt( data->stack + 1 );

	_str[0]=0;
	sprintf(_str,"%d",num);

	popStack(args);

	printf("%s %d\n",_str,stack);

	setStackStrDup(_str);

	return NULL;
}

char *_hex( struct glueCommands *data )
{
	int args = stack - data->stack ;
	int num;
	char _str[12];

	printf("%s: args %d\n",__FUNCTION__,args);

	num = _stackInt( data->stack + 1 );
	sprintf(_str,"$%X",num);

	popStack(args);

	setStackStrDup(_str);

	return NULL;
}

char *_bin( struct glueCommands *data )
{
	int args = stack - data->stack ;
	int num,len,n;
	char *str,*p;

	printf("%s: args %d\n",__FUNCTION__,args);

	len = 0;
	num = _stackInt( data->stack + 1 );
	for (n= num ; n!=0 ; n >>= 1 ) len++;

	len = len ? len : len + 1;	// always one number in bin number.

	str = (char *) malloc(len+2);	 //  '%' and '\0' symbols

	if (str)
	{
		str[0]='%';
		p = str+1;

		for (n=len;n>0;n--)
		{
			*p++= (num & (1<<(n-1))) ? '1' : '0';
		}
		*p = 0;
	}

	popStack(args);

	setStackStr(str);

	return NULL;
}

char *_flip( struct glueCommands *data )
{
	int args = stack - data->stack ;
	int l,i;
	char *str,*p,t;

	printf("%s: args %d\n",__FUNCTION__,args);

	str = _stackString( data->stack + 1 );

	if (str)
	{
		l = strlen(str);
		for (i=0;i<l/2;i++)
		{
		 	t = str[i] ;
			str[i] = str[l-1-i];
			str[l-1-i] = t;
		}
	}

	popStack(args);

	return NULL;
}

char *_string( struct glueCommands *data )
{
	int args = stack - data->stack ;
	int i,_len;
	char *str;
	char *_str;

	printf("%s: args %d\n",__FUNCTION__,args);

	_str = _stackString( data->stack + 1 );
	_len = _stackInt( data->stack + 2 );

	str = (char *) malloc(_len+1);

	for (i=0;i<_len;i++) str[i]= (_str ? *_str : 0) ;
	str[i]= 0;

	popStack(args);

	setStackStr(str);

	return NULL;
}

char *_asc( struct glueCommands *data )
{
	int args = stack - data->stack ;
	char *_str;

	printf("%s: args %d\n",__FUNCTION__,args);

	_str = _stackString( data->stack + 1 );

	popStack(args);

	_num( _str ? *_str : 0 );

	return NULL;
}

char *_val( struct glueCommands *data )
{
	int args = stack - data->stack ;
	int num;
	char *_str;

	printf("%s: args %d\n",__FUNCTION__,args);

	_str = _stackString( data->stack + 1 );
	if (_str) sscanf(_str,"%d",&num);

	popStack(args);

	_num( num );

	return NULL;
}

char *_chr( struct glueCommands *data )
{
	int args = stack - data->stack ;
	char _str[2];

	printf("%s: args %d\n",__FUNCTION__,args);

	_str[0] = (char) _stackInt( data->stack + 1 );
	_str[1] =0;

	popStack(args);

	setStackStrDup(_str);

	return NULL;
}

char *_len( struct glueCommands *data )
{
	int args = stack - data->stack ;
	int len = 0;
	char *_str;

	printf("%s: args %d\n",__FUNCTION__,args);

	if (kittyStack[data->stack + 1].type == type_string)
	{
		len  = (kittyStack[data->stack + 1].len);
	}

	popStack(args);

	_num( len );

	return NULL;
}

char *_space( struct glueCommands *data )
{
	int args = stack - data->stack ;
	int i,_len;
	char *str;

	printf("%s: args %d\n",__FUNCTION__,args);

	_len = _stackInt( data->stack + 1 );

	str = (char *) malloc(_len+1);

	for (i=0;i<_len;i++) str[i]=' ';
	str[i]= 0;

	popStack(args);

	setStackStr(str);

	return NULL;
}

char *_upper( struct glueCommands *data )
{
	int args = stack - data->stack ;
	char *str,*s;

	printf("%s: args %d\n",__FUNCTION__,args);

	dump_stack();

	str = _stackString( data->stack + 1 );

	if (str)
	{
		for (s=str;*s;s++) if ((*s>='a')&&(*s<='z')) *s+=('A'-'a');
	}

	popStack(args);	// should be only one arg, so this should be 0 ;-)

	return NULL;
}

char *_lower( struct glueCommands *data )
{
	int args = stack - data->stack ;
	char *str,*s;

	printf("%s: args %d\n",__FUNCTION__,args);

	dump_stack();

	str = _stackString( data->stack + 1 );

	if (str)
	{
		for (s=str;*s;s++) if ((*s>='A')&&(*s<='Z')) *s-=('A'-'a');
	}

	popStack(args); // should be only one arg, so this should be 0 ;-)

	return NULL;
}

char *cmdLeft(nativeCommand *cmd, char *ptr)
{
	stackCmdNormal( _left, ptr );
	return ptr;
}

char *cmdMid(nativeCommand *cmd, char *ptr)
{
	stackCmdNormal( _mid, ptr );
	return ptr;
}

char *cmdRight(nativeCommand *cmd, char *ptr)
{
	stackCmdNormal( _right, ptr );
	return ptr;
}

char *cmdHex(nativeCommand *cmd, char *ptr)
{
	stackCmdNormal( _hex, ptr );
	return ptr;
}

char *cmdBin(nativeCommand *cmd, char *ptr)
{
	stackCmdNormal( _bin, ptr );
	return ptr;
}

extern char *cmdInstr(nativeCommand *cmd, char *ptr)
{
	stackCmdNormal( _instr, ptr );
	return ptr;
}

char *cmdFlip(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _flip, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdSpace(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _space, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdUpper(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _upper, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdLower(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _lower, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdString(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _string, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdChr(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _chr, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdAsc(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _asc, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdLen(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _len, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdVal(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdNormal( _val, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s: stack %d\n",__FUNCTION__,stack);

	stackCmdNormal( _str, tokenBuffer );	// we need to store the step counter.

	dump_stack();

	return tokenBuffer;
}

