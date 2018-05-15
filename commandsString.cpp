
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "debug.h"
#include "errors.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int current_screen;

extern struct retroScreen *screens[8] ;

extern void setStackStr( char *str );
extern void setStackStrDup( const char *str );
extern bool engine_started ;

using namespace std;


/*********

string names like _xxx is read only (const char), and need to copied.
string names like xxx is new and can saved on stack, with out being copied.

*********/


char *_left( struct glueCommands *data )
{
	int args = stack - data->stack + 1;
	char *str;
	char *tmp = NULL;
	int _len;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	if (args == 2)
	{
		str = _stackString( stack - 1 );
		_len = _stackInt( stack );
		tmp = strndup(str, _len );
	}	

	popStack(args);
	if (tmp) setStackStr(tmp);

	return NULL;
}


char *_mid( struct glueCommands *data )
{
	int args = stack - data->stack +1;
	char *str;
	char *tmp = NULL;
	int _start, _len;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 2:
			str = _stackString( stack - 1 );
			_start = _stackInt( stack ) -1;

			if (_start<0) _start = 0;
			if (_start>strlen(str)-1) _start = strlen(str)-1;
			tmp = strdup(str + _start );
			break;

		case 3:
			str = _stackString( stack - 2 );
			_start = _stackInt( stack -1 ) -1;
			_len = _stackInt( stack );

			if (_start<0) _start = 0;
			if (_start>strlen(str)-1) _start = strlen(str)-1;
			tmp = strndup(str + _start, _len );
			break;
		default:
			setError(22);
	}	

	popStack(stack - data->stack);

	if (tmp) setStackStr(tmp);
	return NULL;
}

char *_right( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	char *str;
	char *tmp = NULL;
	int _start, _len;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	if (args == 2)
	{
		str = _stackString( stack - 1 );
		_len = _stackInt( stack  );
		if (_len>strlen(str)) _start = strlen(str);

		tmp = strdup(str + strlen(str) - _len );
	}	

	popStack(args);

	if (tmp) setStackStr(tmp);

	return NULL;
}

char *_instr( struct glueCommands *data )
{
	int args = stack - data->stack + 1 ;
	char *str,*find, *ret;
	char *tmp = NULL;
	int  _pos = 0;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	if (args == 2)
	{
		str = _stackString( stack - 1 );
		find = _stackString( stack );
		ret = strstr( str, find );
		_pos = ret ? (unsigned int) (ret - str) +1 : 0;
	}	

	popStack(stack - data->stack);

	_num( _pos );

	return NULL;
}

char *_cmdStr( struct glueCommands *data )
{
	int args = stack - data->stack + 1;
	int num;
	char _str[30];

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 1:
				num = _stackInt( stack );
				_str[0]=0;

				if (num>0)
					sprintf(_str," %d",num);
				else
					sprintf(_str,"%d",num);
				break;
		default:
				setError(22);
	}

	popStack(stack - data->stack);
	setStackStrDup(_str);

	return NULL;
}

char *_hex( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int num,chars;
	char _str[12];
	char fmt[10];

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 1:
				num = _stackInt( stack );
				sprintf(_str,"$%X",num);
				break;
		case 2:
				num = _stackInt( stack-1 );
				chars = _stackInt( stack );	
				sprintf(fmt,"$%%0%dX",chars);
				sprintf(_str,fmt,num);
				break;
	}

	popStack(stack - data->stack);

	setStackStrDup(_str);

	return NULL;
}

char *_bin( struct glueCommands *data )
{
	int args = stack - data->stack + 1;
	unsigned int num= 0,len =0,n;
	char *str,*p;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 1:	num = _stackInt( stack  );
				break;
		case 2:	num = _stackInt( stack -1 );
				len = _stackInt( stack );
				break;
		default: 
				setError(22);
	}
	popStack(stack - data->stack);

	if (args == 1)
	{
		len = 0;
		for (n= num ; n!=0 ; n >>= 1 ) len++;
		len = len ? len : len + 1;	// always one number in bin number.
	}

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

	setStackStr(str);

	return NULL;
}

char *_flip( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int l,i;
	char *str,*p,t;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	str = _stackString( stack  );

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

	popStack(stack - data->stack);

	return NULL;
}

char *_string( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	int i,_len;
	char *str;
	char *_str;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	_str = _stackString( stack - 1 );
	_len = _stackInt( stack  );

	str = (char *) malloc(_len+1);

	for (i=0;i<_len;i++) str[i]= (_str ? *_str : 0) ;
	str[i]= 0;

	popStack(stack - data->stack);

	setStackStr(str);

	return NULL;
}

char *_asc( struct glueCommands *data )
{
	int args = stack - data->stack + 1 ;
	char *_str;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	_str = _stackString( stack  );

	popStack(stack - data->stack);

	_num( _str ? *_str : 0 );

	return NULL;
}

char *_val( struct glueCommands *data )
{
	int args = stack - data->stack  + 1;
	int num;
	char *_str;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	_str = _stackString( stack  );
	if (_str) sscanf(_str,"%d",&num);

	popStack(stack - data->stack);

	_num( num );

	return NULL;
}

char *_chr( struct glueCommands *data )
{
	int args = stack - data->stack + 1;
	char _str[2];

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	_str[0] = args == 1 ? (char) _stackInt( stack ) : 0;
	_str[1] =0;

	popStack(stack - data->stack);

	setStackStrDup(_str);

	return NULL;
}

char *_len( struct glueCommands *data )
{
	int args = stack - data->stack + 1 ;
	int len = 0;
	char *_str;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	if (kittyStack[data->stack + 1].type == type_string)
	{
		len  = (kittyStack[stack].len);
	}

	popStack(stack - data->stack);

	_num( len );

	return NULL;
}

char *_space( struct glueCommands *data )
{
	int args = stack - data->stack + 1 ;
	int i,_len;
	char *str;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	_len = _stackInt( stack );

	str = (char *) malloc(_len+1);

	for (i=0;i<_len;i++) str[i]=' ';
	str[i]= 0;

	popStack(stack - data->stack);

	setStackStr(str);

	return NULL;
}

char *_upper( struct glueCommands *data )
{
	int args = stack - data->stack + 1  ;
	char *str,*s;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	str = _stackString( stack );

	if (str)
	{
		for (s=str;*s;s++) if ((*s>='a')&&(*s<='z')) *s+=('A'-'a');
	}

	popStack(stack - data->stack);

	return NULL;
}

char *_lower( struct glueCommands *data )
{
	int args = stack - data->stack + 1 ;
	char *str,*s;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	dump_stack();

	str = _stackString( stack );

	if (str)
	{
		for (s=str;*s;s++) if ((*s>='A')&&(*s<='Z')) *s-=('A'-'a');
	}

	popStack(stack - data->stack);

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
	stackCmdParm( _cmdStr, tokenBuffer );	// we need to store the step counter.
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


	stackCmdParm( _cmdStr, tokenBuffer );	// we need to store the step counter.

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

	if (NEXT_TOKEN( tokenBuffer ) != 0x0006) badSyntax();	// array
	
	ref = (struct reference *) (tokenBuffer + 2);
	idx1 = ref -> ref -1;
	array_var = &globalVars[idx1].var;
	tokenBuffer += sizeof( struct reference) + ref -> length + 2;

	if (NEXT_TOKEN( tokenBuffer ) != 0x0074) badSyntax();	// (
	tokenBuffer +=2;

	if (NEXT_TOKEN( tokenBuffer ) != 0x003E) badSyntax();	// 0
	tokenBuffer += 6;	

	if (NEXT_TOKEN( tokenBuffer ) != 0x007C) badSyntax();	// )
	tokenBuffer +=2;

	if (NEXT_TOKEN( tokenBuffer ) != 0x005C) badSyntax();	// ,
	tokenBuffer +=2;

	// --- this part can be rewritten ---, callback on this part can make sense.
	// just store the array for later...

	if (NEXT_TOKEN( tokenBuffer ) != 0x0006) badSyntax();	// var

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

