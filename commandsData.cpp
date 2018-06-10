#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <string>
#include <iostream>
#include <math.h>

#include "debug.h"
#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "errors.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

bool correct_order( int last_token, int next_token )
{
	bool ret = true;

	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	switch (last_token)
	{
		case token_semi:
			if ((next_token == token_add)
				|| (next_token == token_sub)
				|| (next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)
				|| (next_token == token_more_or_equal )
				|| (next_token == token_less_or_equal	)
				|| (next_token == token_less_or_equal2 )
				|| (next_token == token_more_or_equal2 )
				|| (next_token == token_not_equal )
				|| (next_token == token_equal )
				|| (next_token == token_more )
				|| (next_token == token_less ) 
				|| (next_token == token_or)
//				|| (next_token == token_xor)		// don't know the token number yet.
				|| (next_token == token_and)) return false;
			break;

		case token_or:
//		case token_xor:
		case token_and:
			if ((next_token == token_add)
				|| (next_token == token_sub)
				|| (next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)
				|| (next_token == token_more_or_equal )
				|| (next_token == token_less_or_equal	)
				|| (next_token == token_less_or_equal2 )
				|| (next_token == token_more_or_equal2 )
				|| (next_token == token_not_equal )
				|| (next_token == token_equal )
				|| (next_token == token_more )
				|| (next_token == token_less )) return false;
			break;

		case token_more_or_equal:
		case token_less_or_equal:
		case token_less_or_equal2:
		case token_more_or_equal2:
		case token_not_equal:
		case token_equal:
		case token_more:
		case token_less:

			if ((next_token == token_add)
				|| (next_token == token_sub)
				|| (next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)) return false;
			break;

		case token_add:
			if ((next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)) return false;
			break;

		case token_sub:
			if ((next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)) return false;
			break;

		case token_mul:
			if (next_token == token_power) return false;
			break;

		case token_div	:
			if (next_token == token_power) return false;
			break;
	}
	
	return true;
}

char *_equalData( struct glueCommands *data )
{
	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	item0 = kittyStack + stack-1;
	item1 = kittyStack + stack;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		stack --;
		
		if (type1 == type_int)
		{
			setStackNum( item0->decimal == (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( item0->decimal == item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		stack --;

		if (type1 == type_int)
		{
			setStackNum( item0->value == item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( (double) item0->value == item1->decimal );
			success = TRUE;
		}
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		stackEqualStr( item0, item1 ); 
		success = TRUE;
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}


char *_lessData( struct glueCommands *data )
{
	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	item0 = kittyStack + stack-1;
	item1 = kittyStack + stack;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		stack --;
		
		if (type1 == type_int)
		{
			setStackNum( item0->decimal < (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			proc_names_printf ("( %d > %d ) = %d \n", item0->decimal , item1->decimal , item0->decimal < item1->decimal);
			setStackNum( item0->decimal < item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		stack --;

		if (type1 == type_int)
		{
			setStackNum( item0->value < item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( (double) item0->value < item1->decimal );
			success = TRUE;
		}
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		stackLessStr( item0, item1 ); 
		success = TRUE;
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_lessOrEqualData( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	item0 = kittyStack + stack-1;
	item1 = kittyStack + stack;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		stack --;	// don't need to worry about strings being freed.

		if (type1 == type_int)
		{
			setStackNum( item0->decimal <= (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( item0->decimal <= item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		stack --;

		if (type1 == type_int)
		{
			proc_names_printf ("( %d <= %d ) = %d \n", item0->value , item1->value , item0->value >= item1->value);
			setStackNum( item0->value <= item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( (double) item0->value <= item1->decimal );
			success = TRUE;
		}
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		stackLessOrEqualStr( item0, item1 ); 
		success = TRUE;
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}


char *_moreData( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	item0 = kittyStack + stack-1;
	item1 = kittyStack + stack;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		stack --;

		if (type1 == type_int)
		{
			setStackNum( item0->decimal > (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( item0->decimal > item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		stack --;

		if (type1 == type_int)
		{
			dprintf ("( %d > %d ) = %d \n", item0->value , item1->value , item0->value > item1->value);
			setStackNum( item0->value > item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( (double) item0->value > item1->decimal );
			success = TRUE;
		}
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		stackMoreStr( item0, item1 ); 
		success = TRUE;
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_moreOrEqualData( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	item0 = kittyStack + stack - 1;
	item1 = kittyStack + stack;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		stack --;

		if (type1 == type_int)
		{
			setStackNum( item0->decimal >= (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( item0->decimal >= item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		stack --;

		if (type1 == type_int)
		{
			dprintf ("( %d >= %d ) = %d \n", item0->value , item1->value , item0->value >= item1->value);
			setStackNum( item0->value >= item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( (double) item0->value >= item1->decimal );
			success = TRUE;
		}
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		stackMoreOrEqualStr( item0, item1 ); 
		success = TRUE;
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_orData( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackNum( (item0->decimal != 0) || (item1->value != 0) );
			success = true;
		}
		else if (type1 == type_float)
		{
			setStackNum( (item0->decimal != 0) || (item1->decimal != 0) );
			success = true;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			setStackNum( (item0->value != 0) ||  (item1->value != 0) );
			success = true;
		}
		else if (type1 == type_float)
		{
			setStackNum(  (item0->value != 0 ) || (item1->decimal != 0) );
			success = true;
		}
	}

	if (success)
	{
		correct_for_hidden_sub_data();
	}
	else
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_andData( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackNum( (item0->decimal != 0) && (item1->value != 0) );
			success = true;
		}
		else if (type1 == type_float)
		{
			setStackNum( (item0->decimal != 0) && (item1->decimal != 0) );
			success = true;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			setStackNum( (item0->value != 0) &&  (item1->value != 0) );
			success = true;
		}
		else if (type1 == type_float)
		{
			setStackNum(  (item0->value != 0 ) && (item1->decimal != 0) );
			success = true;
		}
	}

	if (success)
	{
		correct_for_hidden_sub_data();
	}
	else
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_xorData( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackDecimal( item0->decimal + (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( item0->decimal + item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			setStackNum( item0->value + item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( (double) item0->value + item1->decimal );
			success = TRUE;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}


char *_addData( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	int args = stack - data -> stack + 1;
	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	// handel int / float casting.

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackDecimal( item0->decimal + (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( item0->decimal + item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			dprintf(" = %d + %d\n", item0->value , item1->value );

			setStackNum( item0->value + item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( (double) item0->value + item1->decimal );
			success = TRUE;
		}
	}
	else if ( type0 == type_string) 
	{
		switch (type1)
		{
			case type_int:		success = stackStrAddValue( item0, item1 ); break;
			case type_float:	success = stackStrAddDecimal( item0, item1 ); break;
			case type_string:	success = stackStrAddStr( item0, item1 ); break;
		}
	}

	if (success )
	{
		correct_for_hidden_sub_data();
	}
	else
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_addDataToText( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	int args;
	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;
	char buffer[100];

	args = stack - data -> stack + 1;

	if (stack==0) 
	{
		proc_names_printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 7;
	type1 = item1 -> type & 7;

	switch( type0 )
	{
		case type_int:
			sprintf(buffer,"%d", item0->value );
			setStackStrDup(buffer);
			break;

		case type_float:
			sprintf(buffer,"%d", item0->decimal );
			setStackStrDup(buffer);
			break;
	}

	switch (type1)
	{
		case type_int:		success = stackStrAddValue( item0, item1 ); break;
		case type_float:	success = stackStrAddDecimal( item0, item1 ); break;
		case type_string:	success = stackStrAddStr( item0, item1 ); break;
		case type_none:	success = true; stack++; break;	// nothing to add, will be added some where else.
	}
	
	if (success )
	{
		correct_for_hidden_sub_data();
	}
	else
	{
		proc_names_printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_subStr( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	char *find;
 	int find_len;
	char *d,*s;

	if (stack==0) 
	{
		proc_names_printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack--;

	find = kittyStack[stack+1].str;
 	find_len = kittyStack[stack+1].len;

	s=d= kittyStack[stack].str;

	for(;*s;s++)
	{
		if (strncmp(s,find,find_len)==0) s+=find_len;
		if (*s) *d++=*s;
	}
	*d = 0;

	kittyStack[stack].len = d - kittyStack[stack].str;

	// delete string from above.
	if (kittyStack[stack+1].str) free( kittyStack[stack+1].str );
	kittyStack[stack+1].str = NULL;

	return NULL;
}

char *_subData( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackDecimal( item0->decimal - (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( item0->decimal - item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			proc_names_printf(" = %d - %d\n", item0->value , item1->value );
			setStackNum( item0->value - item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( (double) item0->value - item1->decimal );
			success = TRUE;
		}
	}
	else if ( type0 == type_string) 
	{
		switch (type1)
		{
			case type_string:	success = _subStr( data ); break;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_modData( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			dprintf("%f %% %d\n",  item0->decimal , (double) item1->value );
			setStackNum( (int) item0->decimal % item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			dprintf("%f %% %f\n",  item0->decimal , item1->decimal );
			setStackNum( (int) item0->decimal % (int) item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			dprintf(" %d %% %d\n", item0->value , item1->value );
			setStackNum( item0->value * item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			printf("%f %% %f\n",  (double) item0->value , item1->decimal );
			setStackNum( item0->value % (int) item1->decimal );
			success = TRUE;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_mulData( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			dprintf("%f * %d\n",  item0->decimal , (double) item1->value );
			setStackDecimal( item0->decimal * (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			dprintf("%f * %f\n",  item0->decimal , item1->decimal );
			setStackDecimal( item0->decimal * item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			dprintf(" %d * %d\n", item0->value , item1->value );
			setStackNum( item0->value * item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			printf("%f * %f\n",  (double) item0->value , item1->decimal );
			setStackDecimal( (double) item0->value * item1->decimal );
			success = TRUE;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_divData( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if (stack==0) 
	{
		proc_names_printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			dprintf(" %f / %d\n", item0->decimal , item1->value );
			setStackDecimal( item0->decimal / (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			dprintf(" %f / %f\n", item0->decimal , item1->decimal );
			setStackDecimal( item0->decimal / item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			dprintf(" %d / %d\n", item0->value , item1->value );
			setStackNum( item0->value / item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			dprintf(" %d / %f\n", item0->value , item1->decimal );
			setStackDecimal( (double) item0->value / item1->decimal );
			success = TRUE;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_powerData( struct glueCommands *data )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if (stack==0) 
	{
		proc_names_printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackDecimal( pow( item0->decimal , (double) item1->value ) );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( pow( item0->decimal , item1->decimal ) );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			proc_names_printf(" = %d ^ %d\n", item0->value , item1->value );
			setStackNum( (int) pow( item0->value , item1->value ) );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( pow( (double) item0->value , item1->decimal ) );
			success = TRUE;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		proc_names_printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch);
		return NULL;
	}
	return NULL;
}

char *addData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	stackCmdParm( _addData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *subData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm(_subData,tokenBuffer);
	stack++;
	return tokenBuffer;
}

char *modData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _modData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *mulData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mulData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *divData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _divData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *powerData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _powerData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *orData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	flushCmdParaStack();

	stackCmdParm( _orData, tokenBuffer );
	incStack;

	return tokenBuffer;
}

char *andData(struct nativeCommand *cmd, char *tokenBuffer)
{

	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	flushCmdParaStack();
	stackCmdParm( _andData, tokenBuffer );

	stack++;
	return tokenBuffer;
}

char *xorData(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	flushCmdParaStack();

	stackCmdParm( _xorData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *lessData(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	flushCmdParaStack();
	stackCmdParm(_lessData, tokenBuffer);
	incStack;

	return tokenBuffer;
}

char *moreData(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	flushCmdParaStack();
	stackCmdParm(_moreData, tokenBuffer);
	stack++;

	return tokenBuffer;
}

char *lessOrEqualData(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	flushCmdParaStack();
	stackCmdParm(_lessOrEqualData, tokenBuffer);


	return tokenBuffer;
}

char *moreOrEqualData(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	flushCmdParaStack();
	stackCmdParm(_moreOrEqualData, tokenBuffer);
	stack++;

	return tokenBuffer;
}

char *_not_equal( struct glueCommands *data )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if ((stack - data -> stack + 1)!=2)
	{
		setError(22);
		return NULL;
	}

	stack --;

	item0 = kittyStack + stack;
	item1 = kittyStack + stack+1;

	type0 = item0 -> type & 3;
	type1 = item1 -> type & 3;

	if (type0 == type_float) 
	{
		if (type1 == type_int)
		{
			setStackNum( item0->decimal != (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( item0->decimal != item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			setStackNum( item0->value != item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackNum( (double) item0->value != item1->decimal );
			success = TRUE;
		}
	}
	else if (( type0 == type_string) && (type1 == type_string))
	{
		stackNotEqualStr( item0, item1 ); 
		success = TRUE;
	}

	correct_for_hidden_sub_data();

	return NULL;
}

char *cmdNotEqual(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	flushCmdParaStack();
	stackCmdParm(_not_equal, tokenBuffer);
	incStack;

	return tokenBuffer;
}

