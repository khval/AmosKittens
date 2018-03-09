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

	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	switch (last_token)
	{
		case token_semi:
			if ((next_token == token_add)|| (next_token == token_sub)|| (next_token == token_mul)|| (next_token == token_div)||	(next_token == token_power)) return false;

		case token_add:
			if ((next_token == token_mul)|| (next_token == token_div)||(next_token == token_power)) return false;

		case token_sub:
			if ((next_token == token_mul)|| (next_token == token_div)||(next_token == token_power)) return false;

		case token_mul:
			if (next_token == token_power) return false;

		case token_div	:
			if (next_token == token_power) return false;
	}
	
	return ret;
}

void correct_for_hidden_sub_data()
{
	printf("----------------------------------------\n");

	dump_stack();

	printf("----------------------------------------\n");

	if (stack > 0)
	{
		while (kittyStack[stack-1].state == state_hidden_subData)
		{
			printf("END OF )\n");

			kittyStack[stack-1] = kittyStack[stack];
			stack --;
			if (cmdStack) if (stack) if (kittyStack[stack-1].state == state_none) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
		}
	}
}


char *_addData( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
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
		}
		else if (type1 == type_float)
		{
			setStackDecimal( item0->decimal + item1->decimal );
		}
		return NULL;
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			_num( item0->value + item1->value );
		}
		else if (type1 == type_float)
		{
			setStackDecimal( (double) item0->value + item1->decimal );
		}
		return NULL;
	}
	else if ( type0 == type_string) 
	{
		printf("type0 == type_string\n");

		switch (type1)
		{
			case type_int:		success = stackStrAddValue( item0, item1 ); break;
			case type_float:	success = stackStrAddDecimal( item0, item1 ); break;
			case type_string:	success = stackStrAddStr( item0, item1 ); break;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		printf("%d != %d\n",type0, type1);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}


char *_subStr( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	char *find;
 	int find_len;
	char *d,*s;

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
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
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
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
		}
		else if (type1 == type_float)
		{
			setStackDecimal( item0->decimal - item1->decimal );
		}
		return NULL;
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			printf(" = %d - %d\n", item0->value , item1->value );
			_num( item0->value - item1->value );
		}
		else if (type1 == type_float)
		{
			setStackDecimal( (double) item0->value - item1->decimal );
		}
		return NULL;
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
		printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_mulData( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
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
			setStackDecimal( item0->decimal * (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( item0->decimal * item1->decimal );
			success = TRUE;
		}
		return NULL;
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			printf(" = %d * %d\n", item0->value , item1->value );
			_num( item0->value * item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( (double) item0->value * item1->decimal );
			success = TRUE;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch);
		return NULL;
	}


	return NULL;
}

char *_divData( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
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
			setStackDecimal( item0->decimal / (double) item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( item0->decimal / item1->decimal );
			success = TRUE;
		}
	}
	else if (type0 == type_int) 
	{
		if (type1 == type_int)
		{
			printf(" = %d / %d\n", item0->value , item1->value );
			_num( item0->value / item1->value );
			success = TRUE;
		}
		else if (type1 == type_float)
		{
			setStackDecimal( (double) item0->value / item1->decimal );
			success = TRUE;
		}
	}

	correct_for_hidden_sub_data();

	if (success == FALSE)
	{
		printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch);
		return NULL;
	}

	return NULL;
}

char *_powerData( struct glueCommands *data )
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *item0;
	struct kittyData *item1;
	int type0, type1;
	bool success = FALSE;

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
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
			printf(" = %d ^ %d\n", item0->value , item1->value );
			_num( (int) pow( item0->value , item1->value ) );
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
		printf("%d != %d\n",kittyStack[stack].type,kittyStack[stack+1].type);
		setError(ERROR_Type_mismatch);
		return NULL;
	}
	return NULL;
}

char *addData(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	stackCmdParm( _addData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *subData(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	stackCmdParm(_subData,tokenBuffer);
	stack++;
	return tokenBuffer;
}

char *mulData(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	stackCmdParm( _mulData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *divData(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	stackCmdParm( _divData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *powerData(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);

	stackCmdParm( _powerData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

