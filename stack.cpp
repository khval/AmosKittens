#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#include "stack.h"
#include "amosKittens.h"
#include "debug.h"
#include "amosstring.h"

struct kittyData kittyStack[100];

bool correct_order( int last_token, int next_token );

bool dropProgStackToProc( char *(*fn) (struct glueCommands *data, int nextToken) )
{
	while (instance.cmdStack > 0)
	{
		if (cmdTmp[instance.cmdStack-1].cmd == fn) return true;
		instance.cmdStack--;
	}
	return false;
}

bool dropProgStackToFlag( int flag )
{
	while (instance.cmdStack > 0)
	{
		if (cmdTmp[instance.cmdStack-1].flag & flag) return true;
		instance.cmdStack--;
	}
	return false;
}

bool dropProgStackAllFlag( int flag )
{
	bool deleted = false;
	while (instance.cmdStack > 0)
	{
		if (cmdTmp[instance.cmdStack-1].flag & flag) 
		{
			instance.cmdStack--;
			deleted = true;
		}
		else return deleted;
	}
	return false;
}


void remove_parenthesis(int black_at_stack )
{
	if ( kittyStack[black_at_stack].state == state_subData ) 
	{
		int i;
		for (i=black_at_stack+1; i<=instance.stack; i++)
		{
			kittyStack[i-1] = kittyStack[i];
		}

		if (black_at_stack <instance.stack) kittyStack[__stack].str = NULL;

		instance.stack --;
	}
}


void _unLockPara()
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (instance.cmdStack)
	{
		struct glueCommands *cmd;
		cmd = &cmdTmp[instance.cmdStack-1];

		if (cmd -> flag == cmd_para)
		{
			remove_parenthesis(cmd -> stack );
		}
	}
}


bool isArgsClean( struct glueCommands *cmd )
{
	int s;
	for (s= cmd -> stack; s<instance.stack;s++)
	{
		if (kittyStack[s].state != state_none) return FALSE;
	}
	return TRUE;
}


char *flushCmdParaStack( int nextToken )
{
	struct glueCommands *cmd;
	char *ret = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (instance.cmdStack)
	{
		 while ( (instance.cmdStack>0) && (cmdTmp[instance.cmdStack-1].flag & cmd_para)) 
		{
			cmd = &cmdTmp[instance.cmdStack-1];

			if ( isArgsClean( cmd ) ) 
			{
				ret = cmd -> cmd(cmd, nextToken );		// can only return value if foced, or last arg
				instance.cmdStack--;

				if ( correct_order( getLastProgStackToken() ,  nextToken ) == false )
				{
//					printf("**** Looks like I need to exit here, not the correct order ***\n");
					return ret;		// exit here.
				}
			}
			else	break;
		}
	}

	return ret;
}

void popStack(int n)
{
	while ((n>0)&&(instance.stack>0))
	{
		if (kittyStack[__stack].str)
		{
			dprintf("%s::FREE stack(%d)  %08x\n",__FUNCTION__,stack,kittyStack[__stack].str);
			free(kittyStack[__stack].str);	// we should always set ptr to NULL, if not its not freed.
			kittyStack[__stack].str = NULL;
		}
		instance.stack --;
		n--;
	}
}

struct stringData *getStackString( int n )
{
	if (kittyStack[n].type == type_string)
	{
		return (kittyStack[n].str);
	}
	return NULL;
}

double getStackDecimal( int n )
{
	switch (kittyStack[n].type)
	{
		case type_int:	return (double) kittyStack[n].integer.value;
		case type_float: return kittyStack[n].decimal.value;
	}
	return 0.0;
}

int getStackNum( int n )
{
	switch (kittyStack[n].type)
	{
		case type_int:	return kittyStack[n].integer.value;
		case type_float: return (int) kittyStack[n].decimal.value;
	}
	return 0;
}

void stack_get_if_int( int n, int *ret )
{
	switch (kittyStack[n].type)
	{
		case type_int:	*ret = kittyStack[n].integer.value; break;
		case type_float: *ret = (int) kittyStack[n].decimal.value; break;
	}
}


bool stack_is_number( int n )
{
	switch (kittyStack[n].type)
	{
		case type_int:	return true;
		case type_float: return true;
	}
	return false;
}


void setStackNone( void )
{
	if (kittyStack[__stack].str) 
	{
		free(kittyStack[__stack].str);	// we should always set ptr to NULL, if not its not freed.
		kittyStack[__stack].str = NULL;
	}

	kittyStack[__stack].integer.value = 0;
	kittyStack[__stack].state = state_none;
	kittyStack[__stack].type = type_none;
}

void setStackNum( int num )
{
	if (kittyStack[__stack].str) 
	{
		free(kittyStack[__stack].str);	// we should always set ptr to NULL, if not its not freed.
		kittyStack[__stack].str = NULL;
	}

	kittyStack[__stack].integer.value = num;
	kittyStack[__stack].state = state_none;
	kittyStack[__stack].type = type_int;
}

void setStackDecimal( double decimal )
{
	if (kittyStack[__stack].str)
	{
		free(kittyStack[__stack].str);	// we should always set ptr to NULL, if not its not freed.
		kittyStack[__stack].str = NULL;
	}

	kittyStack[__stack].decimal.value = decimal;
	kittyStack[__stack].state = state_none;
	kittyStack[__stack].type = type_float;
}

void setStackStrDup( struct stringData *str)
{
	if (kittyStack[__stack].str)	free(kittyStack[__stack].str);
	kittyStack[__stack].str = str ? amos_strdup( str ) : alloc_amos_string( 0);
	kittyStack[__stack].state = state_none;
	kittyStack[__stack].type = type_string;
}

void setStackStr( struct stringData *str)
{
	struct kittyData *item = &kittyStack[__stack];

	if ((str != item -> str)&&(item -> str))
	{
		free(item -> str);	
	}

	if (str)
	{
		item -> str = str ;
	}
	else
	{
		item -> str = alloc_amos_string( 0) ;
	}

	item -> state = state_none;
	item -> type = type_string;
}

void setStackParenthesis()
{
	struct kittyData *item = &kittyStack[__stack];

	if (item -> str)
	{
		free(item -> str);
		item -> str = NULL ;
	}

	item -> state = state_subData;
	item -> type = type_none;
}

bool stackStrAddValue(struct kittyData *item0, struct kittyData *item1)
{
	int new_size = item0 -> str -> size + 20;
	struct stringData *str;

	if (item0 -> str == NULL) return false;

	str = alloc_amos_string( new_size );
	if (str)
	{
		char *dest = &(str -> ptr);

		str -> size = 0;
		memcpy( dest, &(item0 -> str -> ptr), item0 -> str -> size );

		dest += item0 -> str -> size;
		str -> size += item0 -> str -> size;

		if ( item1->integer.value > -1 )
		{
			sprintf( dest,"  %d", item1 -> integer.value);
		}
		else
		{
			sprintf( dest," %d", item1 -> integer.value);
		}

		str -> size += strlen( dest );

		setStackStr( str );
		return true;
	}
	return false;
}

bool stackStrAddDecimal(struct kittyData *item0,	struct kittyData *item1)
{
	int new_size = item0 -> str -> size + 100;
	struct stringData *str;

	if (item0 -> str == NULL) return false;

	str = alloc_amos_string( new_size );
	if (str)
	{
		char *dest = &(str -> ptr);

		memcpy( dest, &(item0 -> str -> ptr), item0 -> str -> size );
		dest += item0 -> str -> size;
		str -> size += item0 -> str -> size;

		if (item1->decimal.value>= 0.0)
		{
			sprintf( dest,"  %f",item1->decimal.value );
			str -> size += strlen(dest);
		}
		else
		{
			sprintf( dest," %f",item1->decimal.value );
			str -> size += strlen(dest);
		}
		str -> size = strlen( &str -> ptr );

		setStackStr( str );
		return true;
	}
	return false;
}

bool stackStrAddStr(struct kittyData *item0,	struct kittyData *item1)
{
	int new_size = item0 -> str -> size + item1 -> str -> size ;
	struct stringData *str;

	if (item0 -> str == NULL) return false;

	str = alloc_amos_string( new_size );
	if (str)
	{
		char *dest = &(str -> ptr);

		memcpy( dest, &item0 -> str -> ptr, item0 -> str -> size );	dest += item0 -> str -> size;
		memcpy( dest, &item1 -> str -> ptr, item1 -> str -> size );	dest += item1 -> str -> size;

		setStackStr( str );
		return true;
	}
	return false;
}

bool stackMoreStr(struct kittyData *item0,	struct kittyData *item1)
{
	int ret, lendiff;
	struct stringData *s0,*s1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	s0 = item0 -> str;
	s1 = item1 -> str;

	if ((s0 == NULL)||(s1 == NULL))  return false;

	lendiff = ( s0->size - s1->size) ;

	if (lendiff == 0)
	{
		ret = memcmp( &s0->ptr , &s1->ptr , s0->size );
		popStack(1);	
		setStackNum( ret > 0 ? ~0 : 0  );
	}
	else
	{
		popStack(1);	
		setStackNum( lendiff > 0 ? ~0 : 0  );
	}

	return true;
}

bool stackLessStr(struct kittyData *item0,	struct kittyData *item1)
{
	int ret, lendiff;
	struct stringData *s0,*s1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	s0 = item0 -> str;
	s1 = item1 -> str;

	if ((s0 == NULL)||(s1 == NULL))  return false;

	lendiff = ( s0->size - s1->size) ;

	if (lendiff == 0)
	{
		ret = memcmp( &s0->ptr , &s1->ptr , s0->size );
		popStack(1);	
		setStackNum( ret < 0 ? ~0 : 0  );
	}
	else
	{
		popStack(1);	
		setStackNum( lendiff < 0 ? ~0 : 0  );
	}

	return true;
}

bool stackLessOrEqualStr(struct kittyData *item0,	struct kittyData *item1)
{
	int ret, lendiff;
	struct stringData *s0,*s1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	s0 = item0 -> str;
	s1 = item1 -> str;

	if ((s0 == NULL)||(s1 == NULL))  return false;

	lendiff = ( s0->size - s1->size) ;

	if (lendiff == 0)
	{
		ret = memcmp( &s0->ptr , &s1->ptr , s0->size );
		popStack(1);	
		setStackNum( ret <= 0 ? ~0 : 0  );
	}
	else
	{
		popStack(1);	
		setStackNum( lendiff < 0 ? ~0 : 0  );
	}

	return true;
}

bool stackMoreOrEqualStr(struct kittyData *item0,	struct kittyData *item1)
{
	int ret, lendiff;
	struct stringData *s0,*s1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	s0 = item0 -> str;
	s1 = item1 -> str;

	if ((s0 == NULL)||(s1 == NULL))  return false;

	lendiff = ( s0->size - s1->size) ;

	if (lendiff == 0)
	{
		ret = memcmp( &s0->ptr , &s1->ptr , s0->size );
		popStack(1);	
		setStackNum( ret >= 0 ? ~0 : 0  );
	}
	else
	{
		popStack(1);	
		setStackNum( lendiff > 0 ? ~0 : 0  );
	}
	return true;
}

bool stackEqualStr(struct kittyData *item0,	struct kittyData *item1)
{
	int ret, lendiff;
	struct stringData *s0,*s1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	s0 = item0 -> str;
	s1 = item1 -> str;

	if ((s0 == NULL)||(s1 == NULL))  return false;

	lendiff = ( s0->size - s1->size) ;

	if (lendiff == 0)
	{
		ret = memcmp( &s0->ptr , &s1->ptr , s0->size );
		popStack(1);	
		setStackNum( ret == 0 ? ~0 : 0  );
	}
	else
	{
		popStack(1);	
		setStackNum( 0 );
	}

	return true;
}

bool stackNotEqualStr(struct kittyData *item0,	struct kittyData *item1)
{
	int ret, lendiff;
	struct stringData *s0,*s1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	s0 = item0 -> str;
	s1 = item1 -> str;

	if ((s0 == NULL)||(s1 == NULL))  return false;

	lendiff = ( s0->size - s1->size) ;

	if (lendiff == 0)
	{
		ret = memcmp( &s0->ptr , &s1->ptr , s0->size );
		popStack(1);	
		setStackNum( ret != 0 ? ~0 : 0  );
	}
	else
	{
		popStack(1);	
		setStackNum( ~0 );
	}

	return true;
}

void correct_for_hidden_sub_data()
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (instance.stack > 0)
	{
		while (kittyStack[__stack-1].state == state_hidden_subData)
		{
			dprintf("removed hidden ')' \n");

			kittyStack[__stack-1] = kittyStack[__stack];
			kittyStack[__stack].str = NULL;
			instance.stack --;
		}
	}
}
