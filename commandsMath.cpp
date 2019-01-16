#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <string>
#include <iostream>
#include <math.h>
#include <vector>

#include "debug.h"
#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "errors.h"

extern std::vector<struct defFn> defFns;

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

double to_rad_factor=1.0f;
double to_degree_factor=1.0f ;
int decimals = 2;		// FIX sets text formating 
extern char *_file_pos_ ;

extern char *read_kitty_args(char *tokenBuffer, struct glueCommands *sdata);

char *_mathInc( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *var = NULL;
	char *ptr = data -> tokenBuffer ;

	if (NEXT_TOKEN( ptr ) == 0x0006)
	{
		struct reference *ref = (struct reference *) (ptr + 2);
		var = &globalVars[ref->ref-1].var;
	}

	if (var)
	{
		switch (var->type)
		{
			case type_int:
				var->value++;
				break;
	
			case type_int | type_array:
				var->int_array[var -> index]++;
				break;
	
			default:
				setError(ERROR_Type_mismatch,data->tokenBuffer);
		}
	}

	popStack(stack - data->stack);
	return NULL;
}

char *_mathDec( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *var = NULL;
	char *ptr = data -> tokenBuffer ;

	if (NEXT_TOKEN( ptr ) == 0x0006)
	{
		struct reference *ref = (struct reference *) (ptr + 2);
		var = &globalVars[ref->ref-1].var;
	}

	if (var)
	{
		switch (var->type)
		{
			case type_int:
				var->value--;
				break;
	
			case type_int | type_array:
				var->int_array[var -> index]--;
				break;
	
			default:
				setError(ERROR_Type_mismatch,data->tokenBuffer);
		}
	}

	popStack(stack - data->stack);
	return NULL;
}


char *_mathAdd( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	struct kittyData *var = NULL;
	int args = stack - data->stack +1;
	char *ptr = data -> tokenBuffer ;

	if (NEXT_TOKEN( ptr ) == 0x0006)
	{
		struct reference *ref = (struct reference *) (ptr + 2);
		var = &globalVars[ref->ref-1].var;
	}

	if (var)
	{
		int _value = 0;

		switch (args)
		{
			case 2:
				{
					int _inc = getStackNum( stack );
					_value = getStackNum( stack - 1 ) + _inc;
				}
				break;

			case 4:
				{
					int _inc = getStackNum( stack - 2 );
					int _from = getStackNum( stack - 1 );
					int _to = getStackNum( stack );

					 _value = getStackNum( stack - 3 );

					if (_inc>0)
					{
						_value += _inc;	
						if (_value > _to ) _value = _from;	// if more then max reset to min
						if (_value < _from) _value =_from;	// limit to min
					}
					else
					{
						_value += _inc;	
						if (_value < _from) _value = _to;	// if less then min reset to max
						if (_value > _to) _value = _to;		// limit to max
					}
				}
				break;
		}

		switch (var->type)
		{
			case type_int:
				var->value= _value;
				break;
	
			case type_int | type_array:
				var->int_array[var -> index]= _value;
				break;
	
			default:
				setError(ERROR_Type_mismatch,data->tokenBuffer);
		}
	}
	else setError(22,data->tokenBuffer);


	popStack(stack - data->stack);
	return NULL;
}

char *mathInc(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);

	stackCmdNormal( _mathInc, tokenBuffer );
	return tokenBuffer;
}

char *mathDec(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);

	stackCmdNormal( _mathDec, tokenBuffer );
	return tokenBuffer;
}

char *mathAdd(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (cmdStack) if (stack) if (cmdTmp[cmdStack-1].flag == cmd_index ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack],0);

	stackCmdNormal( _mathAdd, tokenBuffer );
	return tokenBuffer;
}


//------------------------------------------------------------


char *_mathSin( struct glueCommands *data, int nextToken )
{
	double r =0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	int args = stack - data->stack + 1;
	if (args == 1)	r = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackDecimal( sin( r * to_rad_factor ) );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathCos( struct glueCommands *data, int nextToken )
{
	double r =0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	int args = stack - data->stack + 1;
	if (args == 1)	r = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackDecimal( cos(  r * to_rad_factor ) );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathTan( struct glueCommands *data, int nextToken )
{
	double r =0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	int args = stack - data->stack + 1;
	if (args == 1)	r = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackDecimal( tan(  r * to_rad_factor ) );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathAcos( struct glueCommands *data, int nextToken )
{
	double r =0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	int args = stack - data->stack + 1;
	if (args == 1)	r = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackDecimal( acos( r ) * to_degree_factor );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathAsin( struct glueCommands *data, int nextToken )
{
	double r =0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	int args = stack - data->stack + 1;
	if (args == 1)	r = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackDecimal( asin( r ) * to_degree_factor );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathAtan( struct glueCommands *data, int nextToken )
{
	double r =0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);
	setStackDecimal( atan( r ) * to_degree_factor );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathHsin( struct glueCommands *data, int nextToken )
{
	double r =0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	r = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackDecimal( sinh( r * to_rad_factor ) );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathHcos( struct glueCommands *data, int nextToken )
{
	double r =0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	r = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackDecimal( cosh( r * to_rad_factor ) );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathHtan( struct glueCommands *data, int nextToken )
{
	double r =0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	r = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackDecimal( tanh( r * to_rad_factor ) );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathLog( struct glueCommands *data, int nextToken )
{
	double d = 0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	d = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackDecimal( log10( d ) );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathExp( struct glueCommands *data, int nextToken )
{
	double d = 0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	d = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackDecimal( exp( d ) );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathLn( struct glueCommands *data, int nextToken )
{
	double d = 0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	d = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackDecimal( log( d ) );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathSqr( struct glueCommands *data, int nextToken )
{
	double d = 0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	d = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackDecimal( sqrt( d ) );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathAbs( struct glueCommands *data, int nextToken )
{
	int n = 0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	n = getStackNum( stack );
	popStack(stack - data->stack);
	setStackNum( abs(n) );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathInt( struct glueCommands *data, int nextToken )
{
	double d = 0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	d = floor(getStackDecimal( stack ));
	popStack(stack - data->stack);
	setStackNum( (int) d ) ;
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathSgn( struct glueCommands *data, int nextToken )
{
	double d = 0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	d = getStackDecimal( stack );
	popStack(stack - data->stack);
	setStackNum( (d<0) ? -1 : ((d>0) ? 1 : 0) ) ;
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathRnd( struct glueCommands *data, int nextToken )
{
	int n = 0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	n = getStackNum( stack );
	popStack(stack - data->stack);
	setStackNum( rand() % (n+1) );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathRandomize( struct glueCommands *data, int nextToken )
{
	int n = 0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	n = getStackNum( stack );
	popStack(stack - data->stack);
	srand( n );
	return NULL;
}

char *_mathMax( struct glueCommands *data, int nextToken )
{
	double a = 0.0, b=0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 2)
	{
		a = getStackDecimal( stack );
		b = getStackDecimal( stack );
	}
	popStack(stack - data->stack);
	setStackDecimal( a>b ? a: b );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathMin( struct glueCommands *data, int nextToken )
{
	double a = 0.0, b=0.0;
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 2)
	{
		a = getStackDecimal( stack );
		b = getStackDecimal( stack );
	}
	popStack(stack - data->stack);
	setStackDecimal( a<b ? a: b );
	kittyStack[stack].state = state_none;
	return NULL;
}

char *_mathSwap( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1;

	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (args==2)
	{
		struct kittyData tmp;

		// swap stack

		tmp = kittyStack[stack -1];
		kittyStack[stack-1] =kittyStack[stack];
		kittyStack[stack]=tmp;

		// read the stack back in.
		// read kitty_args pop's stack.
		read_kitty_args(data -> tokenBuffer, data);
	}
	else
	{
		popStack(stack - data->stack);
	}

	return NULL;
}

char *_mathFix( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	if (args == 1)	decimals = getStackNum( stack );
	popStack(stack - data->stack);
	return NULL;
}

char *_mathDefFn( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	int args = stack - data->stack +1;
	popStack(stack - data->stack);

	kittyStack[stack].state = state_none;
	return NULL;
}

//------------------------------------------------------------

char *mathDegree(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	to_rad_factor=M_PI/180.0f;
	to_degree_factor=180.0f/M_PI ;

	return tokenBuffer;
}

char *mathRadian(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	to_rad_factor=1.0f;
	to_degree_factor=1.0f ;

	return tokenBuffer;
}

// is const number so,

char *mathPi(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	setStackDecimal(M_PI);
	flushCmdParaStack( NEXT_TOKEN(tokenBuffer) );		// PI is on stack, we are ready.

	return tokenBuffer;
}

char *mathSin(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathSin, tokenBuffer );
	return tokenBuffer;
}

char *mathCos(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathCos, tokenBuffer );
	return tokenBuffer;
}

char *mathTan(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathTan, tokenBuffer );
	return tokenBuffer;
}

char *mathAcos(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathAcos, tokenBuffer );
	return tokenBuffer;
}

char *mathAsin(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathAsin, tokenBuffer );
	return tokenBuffer;
}

char *mathAtan(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathAtan, tokenBuffer );
	return tokenBuffer;
}

char *mathHsin(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathHsin, tokenBuffer );
	return tokenBuffer;
}

char *mathHcos(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathHcos, tokenBuffer );
	return tokenBuffer;
}

char *mathHtan(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathHtan, tokenBuffer );
	return tokenBuffer;
}

char *mathLog(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathLog, tokenBuffer );
	return tokenBuffer;
}

char *mathExp(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _mathExp, tokenBuffer );
	return tokenBuffer;
}

char *mathLn(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdNormal( _mathLn, tokenBuffer );
	return tokenBuffer;
}

char *mathSqr(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathSqr, tokenBuffer );
	return tokenBuffer;
}

char *mathAbs(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathAbs, tokenBuffer );
	return tokenBuffer;
}

char *mathInt(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathInt, tokenBuffer );
	return tokenBuffer;
}

char *mathSgn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathSgn, tokenBuffer );
	return tokenBuffer;
}

char *mathRnd(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stackCmdParm( _mathRnd, tokenBuffer );
	return tokenBuffer;
}

char *mathRandomize(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _mathRandomize, tokenBuffer );
	return tokenBuffer;
}

char *mathMax(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _mathMax, tokenBuffer );
	return tokenBuffer;
}

char *mathMin(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _mathMin, tokenBuffer );
	return tokenBuffer;
}

char *mathSwap(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _mathSwap, tokenBuffer );
	return tokenBuffer;
}

char *mathFix(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _mathFix, tokenBuffer );
	return tokenBuffer;
}

char *mathDefFn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (NEXT_TOKEN(tokenBuffer) == 0x0006 )
	{
		struct reference *ref = (struct reference *) (tokenBuffer + 2);

		if (ref -> ref)
		{
			tokenBuffer = defFns[ ref -> ref -1 ].skipAddr;
		}
	}

	return tokenBuffer;
}

char *read_kitty_args(char *tokenBuffer, struct glueCommands *sdata);

char *_mathFnReturn( struct glueCommands *data, int nextToken )
{
	printf("End of Line\n");
	getchar();
	return data -> tokenBuffer;
}

char *_mathFn( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack + 1 ;
	int ref = data -> lastVar;
	char *ptr;

	proc_names_printf("%s: args %d\n",__FUNCTION__,args);
	

	if (ref)
	{
		printf("ref->ref %d, %08x\n", ref, defFns[  ref -1 ].fnAddr);
		ptr = defFns[  ref -1 ].fnAddr;

		if (NEXT_TOKEN(ptr)==0x0074)
		{
			ptr+=2;
			ptr = read_kitty_args(ptr, data);

			data -> tokenBuffer = _file_pos_;
			data -> cmd = _mathFnReturn;
			data -> cmd_type = cmd_onEol;	// force flush to stop.
			cmdStack++;		// stop stack from being deleted

			printf("args read, next addr: %08x\n",ptr);

			return ptr;
		}
	}
	else
	{
		printf("pass1 failed\n");
	}

	popStack(stack - data->stack);

	return NULL;
}


char *mathFn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (NEXT_TOKEN(tokenBuffer) == 0x0006 )
	{
		struct reference *ref = (struct reference *) (tokenBuffer + 2);

		stackCmdParm( _mathFn, tokenBuffer );
		cmdTmp[cmdStack-1].lastVar = ref -> ref;

		tokenBuffer += 2 + sizeof(struct reference) + ref -> length;
	}
	return tokenBuffer;
}

