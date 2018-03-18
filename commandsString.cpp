
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


/*********

string names like _xxx is read only (const char), and need to copied.
string names like xxx is new and can saved on stack, with out being copied.

*********/

char *_print( struct glueCommands *data )
{
	int n;

	printf("PRINT: ");

	for (n=data->stack;n<=stack;n++)
	{
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
	
	popStack( stack - data->stack );

	return NULL;
}

char *cmdPrint(nativeCommand *cmd, char *ptr)
{
	stackCmdNormal( _print, ptr );
	return ptr;
}


/*
char *_cmdMatch( struct glueCommands *data )
{
	int args = stack - data->stack ;
	char *str;
	char *tmp = NULL;
	int _len;

	printf("%s: stack %d\n",__FUNCTION__,stack);

	dump_stack();

	if (args == 2)
	{

	}	

	popStack(args);

	_num(0);

	return NULL;
}
*/

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
	stackCmdParm( _left, ptr );
	return ptr;
}

char *cmdMid(nativeCommand *cmd, char *ptr)
{
	stackCmdParm( _mid, ptr );
	return ptr;
}

char *cmdRight(nativeCommand *cmd, char *ptr)
{
	stackCmdParm( _right, ptr );
	return ptr;
}

char *cmdHex(nativeCommand *cmd, char *ptr)
{
	stackCmdParm( _hex, ptr );
	return ptr;
}

char *cmdBin(nativeCommand *cmd, char *ptr)
{
	stackCmdParm( _bin, ptr );
	return ptr;
}

extern char *cmdInstr(nativeCommand *cmd, char *ptr)
{
	stackCmdParm( _instr, ptr );
	return ptr;
}

char *cmdFlip(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _flip, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdSpace(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _space, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdUpper(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _upper, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdLower(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _lower, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdString(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _string, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdChr(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _chr, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdAsc(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _asc, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdLen(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _len, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdVal(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _val, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s: stack %d\n",__FUNCTION__,stack);

	stackCmdParm( _str, tokenBuffer );	// we need to store the step counter.

	dump_stack();

	return tokenBuffer;
}

void	_match_int( struct kittyData *array, int value )
{
	int n;
	int closest = INT_MAX;
	int new_delta;
	int delta =INT_MAX;

	for (n =0; n< array -> count; n++)
	{
		new_delta = abs(array -> int_array[n] - value) ;

		if ( new_delta < delta )
		{
			if (new_delta == 0)
			{
				_num(0);
				return;
			}
			else
			{
				delta = new_delta;
				closest = n;
			}
		}
	}

	_num( -closest );
}

void _match_float( struct kittyData *array, double decimal )
{
	int n;
	int closest = INT_MAX;
	int new_delta;
	double delta =INT_MAX;	// yes I know double supports larger numbers, but this should work here.

	for (n =0; n< array -> count; n++)
	{
		new_delta = array -> float_array[n] - decimal ;
		new_delta = new_delta < 0.0f ? -new_delta : new_delta;	// abs() but double.

		if ( new_delta < delta )
		{
			if (new_delta == 0)
			{
				_num(0);
				return;
			}
			else
			{
				delta = new_delta;
				closest = n;
			}
		}
	}

	_num( -closest );
}

void _match_str( struct kittyData *array,  char *str )
{
	int n;
	int closest = INT_MAX;
	int new_delta;
	int found_chars = 0;
	int delta =INT_MAX;
	int value;
	int i;
	int _l = strlen(str);
	char c;
	char *item;

	for (n =0; n< array -> count; n++)
	{
		item = array -> str_array[n];

		if (item)
		{
			found_chars = 0;

			for ( i=0;i<_l;i++)
			{
				c = item[i];
				if (c == 0) break;
				if (c == str[i]) found_chars++;
			}

			new_delta = abs( max( _l , (int) strlen( item ) )  - found_chars ) ;

			if ( new_delta < delta )
			{
				if (new_delta == 0)
				{
					_num( n );
					return;
				}
				else
				{
					delta = new_delta;
					closest = n;
				}
			}
		}
	}

	_num( -closest );
}

void	sort_int_array(	struct kittyData *var )
{
	bool sorted = FALSE;
	int n,v;
	int *i0,*i1;

	do
	{
		sorted = false;
		i0 = var -> int_array; i1 = i0+1;
		for (n=1; n< var -> count; n++)
		{
			if ( *i0 > *i1  )
			{
				v = *i0; *i0 = *i1; *i1 = v;
				sorted = true;
			}

			i0++; i1++;
		}
	} while (sorted);
}

void	sort_float_array( struct kittyData *var )
{
	bool sorted = FALSE;
	int n;
	double v;
	double *f0;
	double *f1;

	do
	{
		sorted = false;
		f0 = var -> float_array; f1 = f0+1;
		for (n=1; n< var -> count; n++)
		{
			if ( *f0 > *f1  )
			{
				v = *f0; *f0 = *f1; *f1 = v;
				sorted = true;
			}

			f0++; f1++;
		}
	} while (sorted);
}

void	sort_string_array( struct kittyData *var )
{
	bool sorted = FALSE;
	int n;
	char *v;
	char **s0,**s1;

	do
	{
		sorted = false;
		s0 = var -> str_array; s1 = s0+1;
		for (n=1; n< var -> count; n++)
		{
			if ( strcmp( (*s0 != NULL) ? *s0 : "" , (*s1 != NULL) ? *s1 : ""   ) > 0  )
			{
				v = *s0; 
				*s0 = *s1; 
				*s1 = v;

				sorted = true;
			}
			s0++; s1++;
		}
	} while (sorted);
}


// AMOS The Creator User Guide states that array has to be at index 0, 
// so we don't need to read tokens after variable name,
// we can process this direcly no callbacks.|

char *cmdSort(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned short next_token = *((short *) (tokenBuffer));
	struct reference *ref = (struct reference *) (tokenBuffer + 2);

	if (next_token == 0x0006)
	{
		int idx = ref -> ref -1;

		printf("yes we have a var idx %d\n",idx);

		if (globalVars[idx].var.type & type_array)	// is array
		{
			switch (globalVars[idx].var.type & 7)
			{
				case type_int:
					sort_int_array(&globalVars[idx].var);
					break;
				case type_float:
					sort_float_array(&globalVars[idx].var);
					break;
				case type_string:
					sort_string_array(&globalVars[idx].var);	
					break;
			}
		}		

		tokenBuffer += 2 + sizeof( struct reference) + ref -> length;
	}


	stackCmdParm( _str, tokenBuffer );	// we need to store the step counter.

	return tokenBuffer;
}

#define badSyntax() { setError(120); return NULL; }

char *cmdMatch(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned short next_token = *((short *) (tokenBuffer));
	struct reference *ref = NULL;
	int idx1,idx2;
	struct kittyData *array_var = NULL;
	struct kittyData *var = NULL;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (NEXT_TOKEN( tokenBuffer ) != 0x0074) badSyntax();	// (
	tokenBuffer +=2;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (NEXT_TOKEN( tokenBuffer ) != 0x0006) badSyntax();	// array
	
	ref = (struct reference *) (tokenBuffer + 2);
	idx1 = ref -> ref -1;
	array_var = &globalVars[idx1].var;
	tokenBuffer += sizeof( struct reference) + ref -> length + 2;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("%04x\n",NEXT_TOKEN( tokenBuffer ));

	dump_global();

	if (NEXT_TOKEN( tokenBuffer ) != 0x0074) badSyntax();	// (
	tokenBuffer +=2;

	printf("%s:%d\n",__FUNCTION__,__LINE__);


	if (NEXT_TOKEN( tokenBuffer ) != 0x003E) badSyntax();	// 0
	tokenBuffer += 6;	

	printf("%s:%d\n",__FUNCTION__,__LINE__);


	if (NEXT_TOKEN( tokenBuffer ) != 0x007C) badSyntax();	// )
	tokenBuffer +=2;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (NEXT_TOKEN( tokenBuffer ) != 0x005C) badSyntax();	// ,
	tokenBuffer +=2;

	printf("%s:%d\n",__FUNCTION__,__LINE__);


	// --- this part can be rewritten ---, callback on this part can make sense.
	// just store the array for later...

	if (NEXT_TOKEN( tokenBuffer ) != 0x0006) badSyntax();	// var

	printf("%s:%d\n",__FUNCTION__,__LINE__);


	ref = (struct reference *) (tokenBuffer + 2);
	idx2 = ref -> ref -1;
	var = &globalVars[idx2].var;
	tokenBuffer += 2 + sizeof( struct reference) + ref -> length;

	if (NEXT_TOKEN( tokenBuffer ) != 0x007C) badSyntax();	// )
	tokenBuffer +=2;

	if ((array_var -> type & type_array) && ( (array_var -> type & 7) == var -> type ))
	{
		printf("we are here\n");

		switch (var -> type )
		{
			case type_int:

				printf("int\n");

				_match_int( array_var, var -> value );
				break;

			case type_float:

				printf("float\n");

				_match_float( array_var, var -> decimal );
				break;

			case type_string:

				printf("str\n");

				_match_str( array_var, var -> str );
				break;
		}
	}

	return tokenBuffer;
}
