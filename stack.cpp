
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "stack.h"
#include "amosKittens.h"

int stack = 0;
struct kittyData kittyStack[100];

void popStack(int n)
{
	while ((n>0)&&(stack>0))
	{
		if (kittyStack[stack].str)
		{
			printf("%s::FREE stack(%d)  %08x\n",__FUNCTION__,stack,kittyStack[stack].str);
			free(kittyStack[stack].str);	// we should always set ptr to NULL, if not its not freed.
			kittyStack[stack].str = NULL;
		}
		stack --;
		n--;
	}
}

void _num( int num )
{
	printf("set num stack[%d]\n",stack);

	if (kittyStack[stack].str) 
	{
		printf("%s::FREE stack(%d)  %08x\n",__FUNCTION__,stack,kittyStack[stack].str);
		free(kittyStack[stack].str);	// we should always set ptr to NULL, if not its not freed.
		kittyStack[stack].str = NULL;
		printf("%s::FREE stack(%d)  %08x\n",__FUNCTION__,stack,kittyStack[stack].str);
	}

	kittyStack[stack].value = num;
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = type_int;
}

void setStackDecimal( double decimal )
{
	if (kittyStack[stack].str)
	{
		printf("%s::FREE stack(%d)  %08x\n",__FUNCTION__,stack,kittyStack[stack].str);
		free(kittyStack[stack].str);	// we should always set ptr to NULL, if not its not freed.
		kittyStack[stack].str = NULL;
	}

	kittyStack[stack].decimal = decimal;
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = type_float;
}

void setStackStrDup(const char *str)
{
	if (kittyStack[stack].str)
	{
		printf("%s::FREE stack(%d)  %08x\n",__FUNCTION__,stack,kittyStack[stack].str);
		free(kittyStack[stack].str);	// we should always set ptr to NULL, if not its not freed.
	}

	kittyStack[stack].str = str ? strdup( str ) : NULL;

	printf("%s::ALLOC stack(%d) %08x\n",__FUNCTION__, stack, kittyStack[stack].str);

	kittyStack[stack].len = strlen( kittyStack[stack].str );
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = type_string;
}

void setStackStr( char *str)
{
	if ((str != kittyStack[stack].str)&&(kittyStack[stack].str))
	{
		printf("%s::FREE stack(%d)  %08x\n",__FUNCTION__,stack,kittyStack[stack].str);
		if (kittyStack[stack].str) free(kittyStack[stack].str);	
	}

	kittyStack[stack].str = str ;

	printf("%s::SET stack(%d) %08x\n",__FUNCTION__, stack, kittyStack[stack].str);

	kittyStack[stack].len = strlen( kittyStack[stack].str );
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = type_string;
}

bool stackStrAddValue(struct kittyData *item0, struct kittyData *item1)
{
	int new_size = item0 -> len + 20;
	char *str;

	if (item0 -> str == NULL) return false;

	str = (char *) malloc ( new_size );
	if (str)
	{
		sprintf(str,"%s %d", item0 -> str, item1 -> value);
		setStackStr( str );
		return true;
	}
	return false;
}

bool stackStrAddDecimal(struct kittyData *item0,	struct kittyData *item1)
{
	int new_size = item0 -> len + 100;
	char *str;

	if (item0 -> str == NULL) return false;

	str = (char *) malloc ( new_size );
	if (str)
	{
		sprintf(str,"%s %f", item0 -> str, item1 -> decimal);
		setStackStr( str );
		return true;
	}
	return false;
}
