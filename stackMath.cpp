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

#include <amosKittens.h>
#include <amosstring.h>
#include <stack.h>
#include "debug.h"

#ifdef __amoskittens__
extern struct kittyData kittyStack[100];
#endif



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

