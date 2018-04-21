
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "stack.h"
#include "amosKittens.h"
#include "debug.h"

int stack = 0;
struct kittyData kittyStack[100];

bool dropProgStackToProc( char *(*fn) (struct glueCommands *data) )
{
	while (cmdStack > 0)
	{
		if (cmdTmp[cmdStack-1].cmd == fn) return true;
		cmdStack--;
	}
	return false;
}

bool dropProgStackToType( int type )
{
	while (cmdStack > 0)
	{
		if (cmdTmp[cmdStack-1].flag == type) return true;
		cmdStack--;
	}
	return false;
}

void unLockPara()
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack)
	{
		struct glueCommands *cmd;
		int state;

		cmd = &cmdTmp[cmdStack-1];

		if (cmd -> flag == cmd_para)
		{
			state = kittyStack[cmd -> stack].state;

			if ( state == state_subData ) 
			{
				int i;

				printf("%s:%d -- fix up\n",__FUNCTION__,__LINE__);

				for (i=cmd -> stack+1; i<=stack; i++)
				{
					kittyStack[i-1] = kittyStack[i];
				}

				if (cmd -> stack<stack) kittyStack[stack].str = NULL;

				stack --;
			}
		}
	}
}

void flushCmdParaStack()
{
	// some math operation is blocking... can't flush at this time.
	if (stack)	if (kittyStack[stack-1].state != state_none) return;

	// flush all params.
	if (cmdStack)
	{
		struct glueCommands *cmd;
		int state;

		 while ( (cmdStack>0) && (cmdTmp[cmdStack-1].flag == cmd_para)) 
		{
			cmd = &cmdTmp[cmdStack-1];
			state = kittyStack[cmd -> stack].state;

			if ( state == state_none ) 
			{
				cmd -> cmd(cmd);
				cmdStack--;
			}
			else	break;

			// some math operation is blocking... can't flush at this time.
			if (stack)	if (kittyStack[stack-1].state != state_none) return;
		}
	}
}

void popStack(int n)
{
	while ((n>0)&&(stack>0))
	{
		if (kittyStack[stack].str)
		{
			dprintf("%s::FREE stack(%d)  %08x\n",__FUNCTION__,stack,kittyStack[stack].str);
			free(kittyStack[stack].str);	// we should always set ptr to NULL, if not its not freed.
			kittyStack[stack].str = NULL;
		}
		stack --;
		n--;
	}
}

char *_stackString( int n )
{
	if (kittyStack[n].type == type_string)
	{
		return (kittyStack[n].str);
	}
	return NULL;
}

double _stackDecimal( int n )
{
	switch (kittyStack[n].type)
	{
		case type_int:	return (double) kittyStack[n].value;
		case type_float: return kittyStack[n].decimal;
	}
	return 0.0;
}

int _stackInt( int n )
{
	switch (kittyStack[n].type)
	{
		case type_int:	return kittyStack[n].value;
		case type_float: return (int) kittyStack[n].decimal;
	}
	return 0;
}

void _num( int num )
{
	dprintf("set num stack[%d]\n",stack);

	if (kittyStack[stack].str) 
	{
		free(kittyStack[stack].str);	// we should always set ptr to NULL, if not its not freed.
		kittyStack[stack].str = NULL;
	}

	kittyStack[stack].value = num;
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = type_int;
}

void setStackDecimal( double decimal )
{
	if (kittyStack[stack].str)
	{
		free(kittyStack[stack].str);	// we should always set ptr to NULL, if not its not freed.
		kittyStack[stack].str = NULL;
	}

	kittyStack[stack].decimal = decimal;
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = type_float;
}

void setStackStrDup(const char *str)
{
	if (kittyStack[stack].str)	free(kittyStack[stack].str);
	kittyStack[stack].str = str ? strdup( str ) : NULL;

	kittyStack[stack].len = kittyStack[stack].str ? strlen( kittyStack[stack].str ) : 0;
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = type_string;
}

void setStackStr( char *str)
{
	if ((str != kittyStack[stack].str)&&(kittyStack[stack].str))
	{
		if (kittyStack[stack].str) free(kittyStack[stack].str);	
	}

	kittyStack[stack].str = str ;
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

bool stackStrAddStr(struct kittyData *item0,	struct kittyData *item1)
{
	int new_size = item0 -> len + item1 -> len +1;
	char *str;

	if (item0 -> str == NULL) return false;

	str = (char *) malloc ( new_size );
	if (str)
	{
		sprintf(str,"%s%s", item0 -> str, item1 -> str);
		setStackStr( str );
		return true;
	}
	return false;
}

bool stackMoreStr(struct kittyData *item0,	struct kittyData *item1)
{
	int ret;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((item0 -> str == NULL)||(item1 -> str == NULL))  return false;
	ret = strcmp( item0->str , item1->str );
	popStack(1);	

	_num( ret > 0  );

	return true;
}

bool stackLessStr(struct kittyData *item0,	struct kittyData *item1)
{
	int ret;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((item0 -> str == NULL)||(item1 -> str == NULL))  return false;
	ret = strcmp( item0->str , item1->str );
	popStack(1);	
	_num( ret < 0  );

	return true;
}

bool stackLessOrEqualStr(struct kittyData *item0,	struct kittyData *item1)
{
	int ret;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((item0 -> str == NULL)||(item1 -> str == NULL))  return false;
	ret = strcmp( item0->str , item1->str );
	popStack(1);	
	_num( ret <= 0  );
	return true;
}

bool stackMoreOrEqualStr(struct kittyData *item0,	struct kittyData *item1)
{
	int ret;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((item0 -> str == NULL)||(item1 -> str == NULL))  return false;
	ret = strcmp( item0->str , item1->str );
	popStack(1);	
	_num( ret >= 0  );

	return true;
}

bool stackEqualStr(struct kittyData *item0,	struct kittyData *item1)
{
	int ret;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((item0 -> str == NULL)||(item1 -> str == NULL))  return false;
	ret = strcmp( item0->str , item1->str );
	popStack(1);	

	_num( ret == 0  );
	return true;
}

void correct_for_hidden_sub_data()
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (stack > 0)
	{
		while (kittyStack[stack-1].state == state_hidden_subData)
		{
			kittyStack[stack-1] = kittyStack[stack];
			kittyStack[stack].str = NULL;
			stack --;
		}
	}
}
