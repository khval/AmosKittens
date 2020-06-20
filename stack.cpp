
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#ifdef __no_stdlib__
#define memcpy(d,s,n) __memcpy((char *)d, (char *)s,n)
#endif

#include <amosKittens.h>
#include <amosstring.h>
#include <stack.h>

#ifdef __amoskittens__
struct kittyData kittyStack[100];
bool correct_order( this_instance_first int last_token, int next_token );
#else
#define cmdTmp instance->cmdTmp
#define kittyStack instance->kittyStack
#endif



void memcpy4(int32_t *d,int32_t *s,int l)
{
	int32_t *e;
	l/=4;
	e=s+l;
	while (s<e) *d++=*s++;
}

#define memcpy4(d,s,l) memcpy4((int32_t *)d,(int32_t *)s,l)

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


#ifdef __amoskittens__
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
#endif

// this function pops x number of args from stack

void popStack( this_instance_first int n)
{
	struct kittyData *s,*i,*e;
	int _s;

	_s = instance_stack-n ;
	if (_s<0) _s=0;

	s = &kittyStack[_s];
	e = &kittyStack[instance_stack];

	for (i=e; i>s ; i-- )	// should not pop start, do not change ;-)
	{
		if (i -> str)
		{
			freeString(i->str);
			i->str=NULL;
		}
	}

	instance_stack = _s;
}

void remove_parenthesis( this_instance_first int black_at_stack )
{
	if ( kittyStack[black_at_stack].state == state_subData ) 
	{
		struct kittyData *d = &kittyStack[black_at_stack];
		struct kittyData *s = &kittyStack[black_at_stack+1];
		struct kittyData *e = &kittyStack[instance_stack];

		for (; s<=e; s++)
		{
			memcpy4(d,s,sizeof(struct kittyData));
			d++;
		}

		if (black_at_stack <instance_stack) kittyStack[instance_stack].str = NULL;

		instance_stack --;
	}
}

void correct_for_hidden_sub_data( this_instance_one )
{
	if (instance_stack > 0)
	{
		struct kittyData *d = &kittyStack[instance_stack-1];
		struct kittyData *s = &kittyStack[instance_stack];

		while (d -> state == state_hidden_subData)
		{
			memcpy4(d,s,sizeof(struct kittyData));
			s -> str = NULL;
			d--; s--;
			instance_stack --;
		}
	}
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



