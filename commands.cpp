
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

extern int last_var;
extern struct globalVar globalVars[];

void _print( struct glueCommands *data )
{
	int n;
	printf("PRINT: ");

	for (n=0;n<=stack;n++)
	{
//		printf("stack %d, type: %d value %d\n",n, kittyStack[n].type, kittyStack[n].value);

		switch (kittyStack[n].type)
		{
			case 0:
				printf("%d", kittyStack[n].value);
				break;
			case 1:
				printf("%f", kittyStack[n].decimal);
				break;
			case 2:
				if (kittyStack[n].str) printf("%s", kittyStack[n].str);
				break;

		}

		if (n<=stack) printf("    ");
	}
	printf("\n");
}

void _addStr( struct glueCommands *data )
{
	int len = 0;
	char *tmp;
	char *_new;

	len = kittyStack[stack].len + kittyStack[stack+1].len;

	_new = (char *) malloc(len+1);
	if (_new)
	{
		_new[0] = 0;
		strcpy(_new, kittyStack[stack].str);
		strcpy(_new+kittyStack[stack].len,kittyStack[stack+1].str);
		if (kittyStack[stack].str) free( kittyStack[stack].str );
		kittyStack[stack].str = _new;
		kittyStack[stack].len = len;

	}

	// delete string from above.
	if (kittyStack[stack+1].str) free( kittyStack[stack+1].str );
	kittyStack[stack+1].str = NULL;
}

void _addData( struct glueCommands *data )
{
//	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return;
	}

	stack --;

	if (kittyStack[stack].type != kittyStack[stack+1].type)
	{
		printf("mismatch error\n");
		return;
	}

	switch (kittyStack[stack].type & 3)
	{
		case 0:
				printf("stack[%d].value=%d+%d\n", stack, kittyStack[stack].value, kittyStack[stack+1].value );
				kittyStack[stack].value += kittyStack[stack+1].value;
				break;
		case 1:	kittyStack[stack].decimal += kittyStack[stack+1].decimal;
				break;
		case 2:	_addStr( data );
				break;
	}
}

void _subStr( struct glueCommands *data )
{
	char *find;
 	int find_len;
	char *d,*s;

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return;
	}

	stack--;

	find = kittyStack[stack+1].str;
 	find_len = kittyStack[stack+1].len;

	s=d= kittyStack[stack].str;

//	printf("%s - %s\n",s, find);

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
}

void _subData( struct glueCommands *data )
{
//	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return;
	}

	stack --;

	if (kittyStack[stack].type != kittyStack[stack+1].type)
	{
		printf("mismatch error\n");
		return;
	}

	switch (kittyStack[stack].type & 3)
	{
		case 0:
				printf("stack[%d].value=%d-%d\n", stack, kittyStack[stack].value, kittyStack[stack+1].value );
				kittyStack[stack].value -= kittyStack[stack+1].value;
				break;
		case 1:	kittyStack[stack].decimal -= kittyStack[stack+1].decimal;
				break;
		case 2:	_subStr( data );
				break;
	}
}

void _mulData( struct glueCommands *data )
{
//	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (stack==0) 
	{
		printf("%20s:%d,can't do this :-(\n",__FUNCTION__,__LINE__);
		return;
	}

	stack --;

	if (kittyStack[stack].type != kittyStack[stack+1].type)
	{
		printf("mismatch error\n");
		return;
	}

	switch (kittyStack[stack].type & 3)
	{
		case 0:	
				printf("stack[%d].value=%d*%d\n", stack, kittyStack[stack].value, kittyStack[stack+1].value );
				kittyStack[stack].value *= kittyStack[stack+1].value;
				break;
		case 1:	kittyStack[stack].decimal *= kittyStack[stack+1].decimal;
				break;
		case 2:	printf("mismatch error\n");
				break;
	}
}


void _divData( struct glueCommands *data )
{
//	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (stack==0) 
	{
		printf("can't do this :-(\n");
		return;
	}

	stack --;

	if (kittyStack[stack].type != kittyStack[stack+1].type)
	{
		printf("mismatch error\n");
		return;
	}

	switch (kittyStack[stack].type & 3)
	{
		case 0:	
				printf("stack[%d].value=%d/%d\n", stack, kittyStack[stack].value, kittyStack[stack+1].value );
				kittyStack[stack].value /= kittyStack[stack+1].value;
				break;
		case 1:	kittyStack[stack].decimal /= kittyStack[stack+1].decimal;
				break;
		case 2:	printf("mismatch error\n");
				break;
	}
}


void _addNum( struct glueCommands *data )
{
	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	numStack[stack] += numStack[stack+1];
}

void _setVar( struct glueCommands *data )
{
	printf("data: lastVar %d\n", data -> lastVar);

	if (data -> lastVar)
	{
		if (globalVars[data -> lastVar].var.str) 
		{
			free(globalVars[data -> lastVar].var.str);
			globalVars[data -> lastVar].var.str = NULL;
		}

		switch (kittyStack[stack].type & 3)
		{
			case 0:
					globalVars[data -> lastVar].var.value = kittyStack[stack].value;
					break;
			case 1:
					globalVars[data -> lastVar].var.decimal = kittyStack[stack].decimal;
					break;
			case 2:
					globalVars[data -> lastVar].var.str = strdup(kittyStack[stack].str);
					globalVars[data -> lastVar].var.len = kittyStack[stack].len;
					break;
		}
	}
}

//--------------------------------------------------------

char *nextArg(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	stack++;
	return tokenBuffer;
}

char *addStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	cmdParm( _addStr, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *subCalc(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	kittyStack[stack].str = NULL;
	kittyStack[stack].state = state_subData;

	stack++;
	return tokenBuffer;
}

char *subCalcEnd(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (stack > 0)
	{
		if (kittyStack[stack-1].state == state_subData)
		{
			kittyStack[stack-1] = kittyStack[stack];
			stack --;
			if (cmdStack) if (stack) if (kittyStack[stack-1].state == state_none) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
		}
	}
	return tokenBuffer;
}

char *addData(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);


	cmdParm( _addData, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *subData(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("'%20s:%08d stack is %d cmd stack is %d state %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);


	cmdParm(_subData,tokenBuffer);
	stack++;
	return tokenBuffer;
}

char *setVar(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%d\n",__FUNCTION__,__LINE__);

	cmdNormal(_setVar, tokenBuffer);
	return tokenBuffer;
}

char *mulData(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%d\n",__FUNCTION__,__LINE__);

	cmdParm( _mulData, tokenBuffer );
	stack++;
	cmdStack++;
	return tokenBuffer;
}

char *divData(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%d\n",__FUNCTION__,__LINE__);

	cmdParm( _divData, tokenBuffer );
	stack++;
	cmdStack++;
	return tokenBuffer;
}
