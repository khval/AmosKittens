
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
		if (strStack[n].str) printf("%s", strStack[n].str);
		if (n<=stack) printf("    ");
	}
	printf("\n");
}


void _addStr( struct glueCommands *data )
{
	int len = 0;
	char *tmp;
	char *_new;

//	printf("'%20s:%08d stack is %d cmd stack is %d flag %d\n",__FUNCTION__,__LINE__, stack, cmdStack, strStack[stack].flag);

	if (stack==0) 
	{
		printf("can't do this :-(\n");
		return;
	}

	stack --;
	len = strStack[stack].len + strStack[stack+1].len;

	_new = (char *) malloc(len+1);
	if (_new)
	{
		_new[0] = 0;
		strcpy(_new, strStack[stack].str);
		strcpy(_new+strStack[stack].len,strStack[stack+1].str);
		if (strStack[stack].str) free( strStack[stack].str );
		strStack[stack].str = _new;
		strStack[stack].len = len;

	}

	// delete string from above.
	if (strStack[stack+1].str) free( strStack[stack+1].str );
	strStack[stack+1].str = NULL;
}

void _subStr( struct glueCommands *data )
{
	char *find;
 	int find_len;
	char *d,*s;

	if (stack==0) 
	{
		printf("can't do this :-(\n");
		return;
	}

	stack--;

	find = strStack[stack+1].str;
 	find_len = strStack[stack+1].len;

	s=d= strStack[stack].str;

//	printf("%s - %s\n",s, find);

	for(;*s;s++)
	{
		if (strncmp(s,find,find_len)==0) s+=find_len;
		if (*s) *d++=*s;
	}
	*d = 0;

	strStack[stack].len = d - strStack[stack].str;

	// delete string from above.
	if (strStack[stack+1].str) free( strStack[stack+1].str );
	strStack[stack+1].str = NULL;
}

void _addNum( struct glueCommands *data )
{
	printf("'%20s:%08d stack is %d cmd stack is %d flag %d\n",__FUNCTION__,__LINE__, stack, cmdStack, strStack[stack].flag);

	numStack[stack] += numStack[stack+1];
}

void _setVar( struct glueCommands *data )
{
	printf("data: lastVar %d\n", data -> lastVar);

	if (data -> lastVar)
	{
		if (globalVars[data -> lastVar].var.str) free(globalVars[data -> lastVar].var.str);

		globalVars[data -> lastVar].var.str = strdup(strStack[stack].str);
		globalVars[data -> lastVar].var.len = strStack[stack].len;
	}
}

//--------------------------------------------------------

char *nextArg(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("'%20s:%08d stack is %d cmd stack is %d flag %d\n",__FUNCTION__,__LINE__, stack, cmdStack, strStack[stack].flag);
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
	printf("'%20s:%08d stack is %d cmd stack is %d flag %d\n",__FUNCTION__,__LINE__, stack, cmdStack, strStack[stack].flag);

	strStack[stack].str = NULL;
	strStack[stack].flag = state_subData;

	stack++;
	return tokenBuffer;
}

char *subCalcEnd(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("'%20s:%08d stack is %d cmd stack is %d flag %d\n",__FUNCTION__,__LINE__, stack, cmdStack, strStack[stack].flag);

	if (stack > 0)
	{
		if (strStack[stack-1].flag == state_subData)
		{
			strStack[stack-1] = strStack[stack];
			stack --;
			if (cmdStack) if (stack) if (strStack[stack-1].flag == state_none) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
		}
	}
	return tokenBuffer;
}

char *addData(struct nativeCommand *cmd, char *tokenBuffer)
{
	cmdParm( _addStr, tokenBuffer );
	stack++;
	return tokenBuffer;
}

char *subData(struct nativeCommand *cmd, char *tokenBuffer)
{
	cmdParm(_subStr,tokenBuffer);
	stack++;
	return tokenBuffer;
}

char *setVar(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%20s:%d\n",__FUNCTION__,__LINE__);

	cmdNormal(_setVar, tokenBuffer);
	return tokenBuffer;
}

char *mulVar(struct nativeCommand *cmd, char *tokenBuffer)
{
//	cmdTmp[cmdStack].cmd = _mulVar;
//	cmdTmp[cmdStack].tokenBuffer = tokenBuffer;
//	cmdStack++;
	return tokenBuffer;
}

char *divVar(struct nativeCommand *cmd, char *tokenBuffer)
{
//	cmdTmp[cmdStack].cmd = _divVar;
//	cmdTmp[cmdStack].tokenBuffer = tokenBuffer;
//	cmdStack++;
	return tokenBuffer;
}
