
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#include <amosKittens.h>
#include <amosstring.h>
#include <stack.h>

#ifdef __amoskittens__
struct kittyData kittyStack[100];
#else
#define cmdTmp instance->cmdTmp
#define kittyStack instance->kittyStack
#endif

bool correct_order( this_instance_first int last_token, int next_token );

bool dropProgStackToProc( this_instance_first char *(*fn) (struct glueCommands *data, int nextToken) )
{
	while (instance_cmdStack > 0)
	{
		if (cmdTmp[instance_cmdStack-1].cmd == fn) return true;
		instance_cmdStack--;
	}
	return false;
}

bool dropProgStackToFlag( this_instance_first int flag )
{
	while (instance_cmdStack > 0)
	{
		if (cmdTmp[instance_cmdStack-1].flag & flag) return true;
		instance_cmdStack--;
	}
	return false;
}

bool dropProgStackAllFlag( this_instance_first int flag )
{
	bool deleted = false;
	while (instance_cmdStack > 0)
	{
		if (cmdTmp[instance_cmdStack-1].flag & flag) 
		{
			instance_cmdStack--;
			deleted = true;
		}
		else return deleted;
	}
	return false;
}


void remove_parenthesis( this_instance_first int black_at_stack )
{
	if ( kittyStack[black_at_stack].state == state_subData ) 
	{
		int i;
		for (i=black_at_stack+1; i<=instance_stack; i++)
		{
			kittyStack[i-1] = kittyStack[i];
		}

		if (black_at_stack <instance_stack) kittyStack[instance_stack].str = NULL;

		instance_stack --;
	}
}


void _unLockPara( this_instance_one )
{
	if (instance_cmdStack)
	{
		struct glueCommands *cmd;
		cmd = &cmdTmp[instance_cmdStack-1];

		if (cmd -> flag == cmd_para)
		{
			remove_parenthesis( opt_instance_first cmd -> stack );
		}
	}
}


bool isArgsClean( this_instance_first struct glueCommands *cmd )
{
	int s;
	for (s= cmd -> stack; s<instance_stack;s++)
	{
		if (kittyStack[s].state != state_none) return FALSE;
	}
	return TRUE;
}


char *flushCmdParaStack( this_instance_first int nextToken )
{
	struct glueCommands *cmd;
	char *ret = NULL;

	if (instance_cmdStack)
	{
		 while ( (instance_cmdStack>0) && (cmdTmp[instance_cmdStack-1].flag & cmd_para)) 
		{
			cmd = &cmdTmp[instance_cmdStack-1];

			if ( isArgsClean( opt_instance_first cmd ) ) 
			{
				ret = cmd -> cmd(cmd, nextToken );		// can only return value if foced, or last arg
				instance_cmdStack--;

				// opt_instance this used when function does not always take a intance.

				if ( correct_order( opt_instance_first getLastProgStackToken(), nextToken ) == false )
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

void popStack( this_instance_first int n)
{
	struct kittyData *s,*i,*e;
	int _s;

	_s = instance_stack-n ;
	if (_s<0) _s=0;

	s = &kittyStack[_s];
	e = &kittyStack[instance_stack];

	for (i=e; i>=s ; i-- )
	{
		if (i -> str)
		{
			freeString(i->str);
			i->str=NULL;
		}
	}

	instance_stack = _s;
}

struct stringData *getStackString( this_instance_first int n )
{
	if (kittyStack[n].type == type_string)
	{
		return (kittyStack[n].str);
	}
	return NULL;
}

double getStackDecimal( this_instance_first int n )
{
	switch (kittyStack[n].type)
	{
		case type_int:	return (double) kittyStack[n].integer.value;
		case type_float: return kittyStack[n].decimal.value;
	}
	return 0.0;
}

int getStackNum( this_instance_first int n )
{
	switch (kittyStack[n].type)
	{
		case type_int:	return kittyStack[n].integer.value;
		case type_float: return (int) kittyStack[n].decimal.value;
	}
	return 0;
}

void stack_get_if_int( this_instance_first int n, int *ret )
{
	switch (kittyStack[n].type)
	{
		case type_int:	*ret = kittyStack[n].integer.value; break;
		case type_float: *ret = (int) kittyStack[n].decimal.value; break;
	}
}


bool stack_is_number( this_instance_first int n )
{
	switch (kittyStack[n].type)
	{
		case type_int:	return true;
		case type_float: return true;
	}
	return false;
}


void setStackNone( this_instance_one )
{
	if (kittyStack[instance_stack].str) 
	{
		freeString(kittyStack[instance_stack].str);	// we should always set ptr to NULL, if not its not freed.
		kittyStack[instance_stack].str = NULL;
	}

	kittyStack[instance_stack].integer.value = 0;
	kittyStack[instance_stack].state = state_none;
	kittyStack[instance_stack].type = type_none;
}

void setStackNum( this_instance_first int num )
{
	if (kittyStack[instance_stack].str) 
	{
		freeString(kittyStack[instance_stack].str);	// we should always set ptr to NULL, if not its not freed.
		kittyStack[instance_stack].str = NULL;
	}

	kittyStack[instance_stack].integer.value = num;
	kittyStack[instance_stack].state = state_none;
	kittyStack[instance_stack].type = type_int;
}

void setStackDecimal( this_instance_first double decimal )
{
	if (kittyStack[instance_stack].str)
	{
		freeString(kittyStack[instance_stack].str);	// we should always set ptr to NULL, if not its not freed.
		kittyStack[instance_stack].str = NULL;
	}

	kittyStack[instance_stack].decimal.value = decimal;
	kittyStack[instance_stack].state = state_none;
	kittyStack[instance_stack].type = type_float;
}

void setStackStrDup( this_instance_first struct stringData *str)
{
	if (kittyStack[instance_stack].str)	freeString(kittyStack[instance_stack].str);
	kittyStack[instance_stack].str = str ? amos_strdup( str ) : alloc_amos_string( 0);
	kittyStack[instance_stack].state = state_none;
	kittyStack[instance_stack].type = type_string;
}

void setStackStr( this_instance_first struct stringData *str)
{
	struct kittyData *item = &kittyStack[instance_stack];

	if ((str != item -> str)&&(item -> str))
	{
		freeString(item -> str);	
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

void setStackParenthesis( this_instance_one )
{
	struct kittyData *item = &kittyStack[instance_stack];

	if (item -> str)
	{
		freeString(item -> str);
		item -> str = NULL ;
	}

	item -> state = state_subData;
	item -> type = type_none;
}


void correct_for_hidden_sub_data( this_instance_one )
{
	if (instance_stack > 0)
	{
		while (kittyStack[instance_stack-1].state == state_hidden_subData)
		{
			kittyStack[instance_stack-1] = kittyStack[instance_stack];
			kittyStack[instance_stack].str = NULL;
			instance_stack --;
		}
	}
}
