
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#ifdef __linux__
#include <stdint.h>
#include <limits.h>
#endif

#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "debug.h"
#include "errors.h"
#include "amosString.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int current_screen;

extern struct retroScreen *screens[8] ;

extern void setStackStr( struct stringData *str );
extern void setStackStrDup( struct stringData *str );

using namespace std;


/*********

string names like _xxx is read only (const char), and need to copied.
string names like xxx is new and can saved on stack, with out being copied.

*********/


char *_left( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack + 1;
	struct stringData *str;
	struct stringData *tmp = NULL;
	int _len;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	if (args == 2)
	{
		str = getStackString( stack - 1 );
		_len = getStackNum( stack );
		tmp = amos_strndup(str, _len );
	}	

	popStack(stack - data->stack);
	if (tmp) setStackStr(tmp);

	return NULL;
}


char *_mid( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1;
	struct stringData *str;
	struct stringData *tmp = NULL;
	int _slen=0;
	int _start=0, _len = 0;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 2:
			str = getStackString( stack - 1 );
			_start = getStackNum( stack ) ;	

			if (_start == 0 ) _start = 1;	// 0 is allowed, even if string starts at 1.
			if (_start>0)
			{
				_start--;
				_slen = str -> size;
				if (_start>_slen-1) 
				{
					tmp = toAmosString("",0);
				}
				else
				{
					if (_start<0) _start=0;
					tmp = amos_right(str , str -> size - _start );
				}
			}
			break;

		case 3:
			str = getStackString( stack - 2 );
			_start = getStackNum( stack -1 ) ;
			_len = getStackNum( stack );

			if (_start == 0 ) _start = 1;
			if (_start>0)
			{
				_start--;
				_slen = str -> size;
				if (_start>_slen-1) 
				{
					tmp = toAmosString("",0);
				}
				else
				{
					if (_start<0) _start=0;
					tmp = amos_mid(str, _start, _len );
				}
			}	
			break;

		default:
			setError(22,data->tokenBuffer);
	}

	if ((_start<0)||(_len<0)) 
	{
		setError(23,data->tokenBuffer);
	}
	popStack(stack - data->stack);

	if (tmp) setStackStr(tmp);
	return NULL;
}

char *_right( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	struct stringData *str;
	struct stringData *tmp = NULL;
	int _len;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	if (args == 2)
	{
		str = getStackString( stack - 1 );
		_len = getStackNum( stack  );
		if (_len>str->size) _len = str ->size;

		tmp = amos_right(str , _len );
	}	

	popStack(stack - data->stack);

	if (tmp) setStackStr(tmp);

	return NULL;
}

char *_instr( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack + 1 ;
	struct stringData *_str,*_find, *ret;
	int  _pos = 0;
	int _start = 0;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 2:
				_str = getStackString( stack - 1 );
				_find = getStackString( stack );

				if ((_str)&&(_find))
				{
					if ((_str -> size) &&(_find -> size))		// not empty
					{
						_pos = amos_instr( _str, 0, _find );
					}
				}
				break;
		case 3:
				_str = getStackString( stack - 2 );
				_find = getStackString( stack -1 );
				_start = getStackNum( stack ) -1;

				if ((_str)&&(_find)&&(_start>-1) )
				{
					if ((_str -> size) &&(_find -> size))		// not empty
					{
						int str_len = kittyStack[stack-2].str -> size;

						if (_start >= str_len) _start = str_len-1;

						_pos = amos_instr( _str , _start, _find );
					}
				}
				break;
		default:
				setError(22,data->tokenBuffer);
	}	

	popStack(stack - data->stack);

	setStackNum( _pos );

	return NULL;
}

char *_cmdStr( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack + 1;
	int num;
	struct stringData *_str = alloc_amos_string( 50 );

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 1:
				num = getStackNum( stack );

				if (num>-1)
					sprintf(&_str->ptr," %d",num);
				else
					sprintf(&_str->ptr,"%d",num);

				_str->size = strlen(&_str->ptr);				
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack(stack - data->stack);

	_str -> size = strlen( &_str->ptr );
	setStackStr(_str);

	return NULL;
}

char *_hex( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num,chars;
	char fmt[10];
	struct stringData *_str = alloc_amos_string( 50 );

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 1:
				num = getStackNum( stack );
				sprintf(&_str->ptr,"$%X",num);
				break;
		case 2:
				num = getStackNum( stack-1 );
				chars = getStackNum( stack );	
				sprintf(fmt,"$%%0%dX",chars);
				sprintf(&_str->ptr,fmt,num);
				break;
	}

	popStack(stack - data->stack);

	_str -> size = strlen( &_str->ptr );
	setStackStr(_str);

	return NULL;
}

char *_bin( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack + 1;
	unsigned int num= 0,len =0,n;
	struct stringData *str;
	char *p;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	switch (args)
	{
		case 1:	num = getStackNum( stack  );
				break;
		case 2:	num = getStackNum( stack -1 );
				len = getStackNum( stack );
				break;
		default: 
				setError(22,data->tokenBuffer);
	}
	popStack(stack - data->stack);

	if (args == 1)
	{
		len = 0;
		for (n= num ; n!=0 ; n >>= 1 ) len++;
		len = len ? len : len + 1;	// always one number in bin number.
	}

	str = alloc_amos_string(len+1);	 //  '%' 

	if (str)
	{
		p = &(str -> ptr);

		*p++='%';

		for (n=len;n>0;n--)
		{
			*p++= (num & (1<<(n-1))) ? '1' : '0';
		}
		*p = 0;
	}

	str -> size = strlen( &str->ptr );
	setStackStr(str);

	return NULL;
}

char *_flip( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int l,i;
	struct stringData *_str;
	char *str,t;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	if (args == 1)
	{
		_str = getStackString( stack  );

		if (str)
		{
			l = _str -> size;
			str = &_str -> ptr;

			for (i=0;i<l/2;i++)
			{
			 	t = str[i] ;
				str[i] = str[l-1-i];
				str[l-1-i] = t;
			}
		}
	}
	else
	{
		popStack(stack - data->stack);
		setError(22, data -> tokenBuffer);
	}

	return NULL;
}

char *_string( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int i,_len;
	struct stringData *str = NULL;
	struct stringData *_str;
	char *dest;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			_str = getStackString( stack - 1 );
			_len = getStackNum( stack  );

			str = alloc_amos_string(_len);
			
			dest = &str -> ptr;

			for (i=0;i<_len;i++)
			{
				dest[i]= (_str ? _str -> ptr : 0) ;
			}
			dest[i]= 0;

			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack(stack - data->stack);
	if (str) setStackStr(str);

	return NULL;
}

char *_asc( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack + 1 ;
	struct stringData *_str;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		_str = getStackString( stack  );
		if (_str) ret = _str -> ptr;
	}
	else setError(22,data->tokenBuffer);

	popStack(stack - data->stack);

	setStackNum( ret );

	return NULL;
}


static bool get_bin(char *str, int &num)
{
	char *c = str;
	num = 0;

	while (*c == ' ') c++;
	if (*c!='%') return false;
	c++;
	while ((*c=='0')||(*c=='1'))
	{
		num = num << 1;
		num += *c - '0';
		c++;
	}

	return true;
}

static bool get_hex(char *str, int &num)
{
	char *c = str;
	num = 0;

	while (*c == ' ') c++;
	if (*c!='$') return false;
	c++;
	while ( (*c) &&  (*c != ' ')  )
	{
		num = num * 16;

		if ((*c>='0')&&(*c<='9'))
		{
			num += *c -'0';
		}
		else if ((*c>='a')&&(*c<='f'))
		{
			num += *c -'a' +10;
		}
		else if ((*c>='A')&&(*c<='F'))
		{
			num += *c -'A' +10;
		}
		else break;

		c++;
	}

	return true;
}


char *_val( struct glueCommands *data, int nextToken )
{
	int num = 0;
	double numf = 0.0f;
	char *c;
	struct stringData *_str;
	int type_count = 0;
	int type = 0;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	_str = getStackString( stack  );
	if (_str)
	{
		c = &_str -> ptr;
		// skip spaces in the start of the string.
		while (*c == ' ') c++;

		printf("%s\n",c);

		// check for symbol until first space or end of string.
		for (; (*c)  && (*c != ' ') ;c++)
		{
			switch (*c)
			{
				case '.':	type= 1; type_count++; break;
				case '$':	type= 2; type_count++; break;
				case '%':	type= 3; type_count++; break;
			}
		}	

		if (type_count<2)
		{
			success = true;

			printf("type %d\n",type);

			switch (type)
			{
				case 0:	if (sscanf(&_str -> ptr,"%d",&num)==0) num=0.0f;
						break;
				case 1:	if (sscanf(&_str -> ptr,"%lf",&numf)==0) numf=0.0f;
						break;
				case 2:	success = get_hex(&_str -> ptr,num);
						break;
				case 3:	success = get_bin(&_str -> ptr,num) ;
						break;

				default: success = false;
			}
		}
		else printf("type_count %d\n",type_count);
	}

	popStack(stack - data->stack);

	if (success == true)
	{
		if (type == 1)
		{
			setStackDecimal( numf );
		}
		else
		{
			setStackNum( num );
		}
	}
	else
	{
		setError(22, data -> tokenBuffer);
	}

	return NULL;
}

char *_chr( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack + 1;
	struct stringData *_str = alloc_amos_string( 1 );
	char *p;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		if (_str)
		{
			p = &(_str -> ptr);
			*p++ = args == 1 ? (char) getStackNum( stack ) : 0;
			*p = 0;
		}
		setStackStr(_str);
		return NULL;
	}

	setError(22,data -> tokenBuffer);
	popStack(stack - data->stack);
	setStackStr(_str);

	return NULL;
}

char *_len( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack + 1 ;
	int len = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			if (kittyStack[stack].type == type_string)
			{
				len  = kittyStack[stack].str -> size;
			}
			break;

		default:		
			setError(22,data->tokenBuffer);

	}

	popStack(stack - data->stack);

	setStackNum( len );

	return NULL;
}

char *_space( struct glueCommands *data, int nextToken )
{
	int i,_len;
	struct stringData *str;
	char *p;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	_len = getStackNum( stack );

	str = alloc_amos_string(_len);
	p = &str -> ptr;
	for (i=0;i<_len;i++) *p++=' ';
	*p= 0;

	popStack(stack - data->stack);

	setStackStr(str);

	return NULL;
}

char *_upper( struct glueCommands *data, int nextToken )
{
	struct stringData *str;
	char *s;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	str = getStackString( stack );

	if (str)
	{
		for (s=&str -> ptr;*s;s++) if ((*s>='a')&&(*s<='z')) *s+=('A'-'a');
	}

	popStack(stack - data->stack);

	return NULL;
}

char *_lower( struct glueCommands *data, int nextToken )
{
//	int args = stack - data->stack + 1 ;
	struct stringData *str;
	char *s;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	str = getStackString( stack );

	if (str)
	{
		for (s=&str ->ptr;*s;s++) if ((*s>='A')&&(*s<='Z')) *s-=('A'-'a');
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
		new_delta = abs(array -> int_array[n].value - value) ;

		if ( new_delta < delta )
		{
			if (new_delta == 0)
			{
				setStackNum(0);
				return;
			}
			else
			{
				delta = new_delta;
				closest = n;
			}
		}
	}

	setStackNum( -closest );
}

void _match_float( struct kittyData *array, double decimal )
{
	int n;
	int closest = INT_MAX;
	int new_delta;
	double delta =INT_MAX;	// yes I know double supports larger numbers, but this should work here.

	for (n =0; n< array -> count; n++)
	{
		new_delta = array -> float_array[n].value - decimal ;
		new_delta = new_delta < 0.0f ? -new_delta : new_delta;	// abs() but double.

		if ( new_delta < delta )
		{
			if (new_delta == 0)
			{
				setStackNum(0);
				return;
			}
			else
			{
				delta = new_delta;
				closest = n;
			}
		}
	}

	setStackNum( -closest );
}

void _match_str( struct kittyData *array,  struct stringData *strArg )
{
	int n;
	int closest = INT_MAX;
	int new_delta;
	int found_chars = 0;
	int delta =INT_MAX;
	char *str = &(strArg -> ptr);
	int i;

	int _l = strArg->size;

	char c;
	char *item;

	for (n =0; n< array -> count; n++)
	{
		item = &(array -> str_array[n] -> ptr);

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
					setStackNum( n );
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

	setStackNum( -closest );
}

void	sort_int_array(	struct kittyData *var )
{
	bool sorted = FALSE;
	int n,v;
	struct valueData *i0,*i1;

	do
	{
		sorted = false;
		i0 = var -> int_array; i1 = i0+1;
		for (n=1; n< var -> count; n++)
		{
			if ( i0 -> value > i1 -> value  )
			{
				v = i0 -> value; i0 -> value = i1 -> value; i1 -> value = v;
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
	struct desimalData v;
	struct desimalData *f0;
	struct desimalData *f1;

	do
	{
		sorted = false;
		f0 = var -> float_array; f1 = f0+1;
		for (n=1; n< var -> count; n++)
		{
			if ( f0 -> value > f1 -> value  )
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
	struct stringData *v;
	struct stringData **s0,**s1;

	do
	{
		sorted = false;
		s0 = var -> str_array; s1 = s0+1;
		for (n=1; n< var -> count; n++)
		{
			if ( strcmp( &(*s0)->ptr, &(*s1)->ptr ) > 0  )
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

#define badSyntax() { setError(120,tokenBuffer); return NULL; }

char *cmdMatch(struct nativeCommand *cmd, char *tokenBuffer )
{
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

				_match_int( array_var, var -> integer.value );
				break;

			case type_float:

				printf("float\n");

				_match_float( array_var, var -> decimal.value );
				break;

			case type_string:

				printf("str\n");

				_match_str( array_var, var -> str );
				break;
		}
	}

	return tokenBuffer;
}

char *_cmdRepeatStr( struct glueCommands *data, int nextToken )
{
	string txt;
	int args = stack - data->stack + 1;
	struct stringData *str;
	struct stringData *dest = NULL;
	int _num,n;
	int _new_size;
	char *d;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);

	if (args == 2)
	{
		str = getStackString( stack - 1 );
		_num = getStackNum( stack );

		_new_size = _num * str -> size;

		dest = alloc_amos_string( _new_size );
		d =&dest -> ptr;

		for (n=0;n<_new_size;n++)
		{
			d[n] = (&str -> ptr) [ n % str->size ] ;
		}
	}	

	popStack(stack - data->stack);

	setStackStr(dest);

	return NULL;
}

char *cmdRepeatStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _cmdRepeatStr, tokenBuffer );	// we need to store the step counter.
	return tokenBuffer;
}

char *cmdTabStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	struct stringData *txt = alloc_amos_string(1);

	if (txt)
	{
		char *p = &txt -> ptr ;
		*p++=9; 
		*p=0;
	}

	setStackStr(txt);
	return tokenBuffer;
}

