#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "amosKittens.h"
#include "commands.h"

extern struct globalVar globalVars[1000];

void dumpGlobal()
{
	int n;
	int i;

	for (n=1;n<sizeof(globalVars)/sizeof(struct globalVar);n++)
	{
		if (globalVars[n].varName == NULL) return;

		printf("%d\n",globalVars[n].var.type);

		switch (globalVars[n].var.type)
		{
			case type_int:
				printf("%s=%d\n", globalVars[n].varName, globalVars[n].var.value );
				break;
			case type_float:
				printf("%s=%f\n", globalVars[n].varName, globalVars[n].var.decimal );
				break;
			case type_string:
				printf("%s=\"%s\"\n", globalVars[n].varName, globalVars[n].var.str ? globalVars[n].var.str : "NULL" );
				break;
			case type_int | type_array:

				printf("%s(%d)=",
						globalVars[n].varName,
						globalVars[n].var.count);

				for (i=0; i<globalVars[n].var.count; i++)
				{
					printf("%d,",globalVars[n].var.int_array[i]);
				}
				printf("\n");

				break;
			case type_float | type_array:
				break;
			case type_string | type_array:
				break;
		}
	}
}

void dump_prog_stack()
{
	int n;

	for (n=0; n<cmdStack;n++)
	{
		printf("cmdTmp[%d].cmd = %08x\n", n, cmdTmp[n].cmd);
		printf("cmdTmp[%d].tokenBuffer = %08x\n", n, cmdTmp[n].tokenBuffer);
		printf("cmdTmp[%d].flag = %08x\n", n, cmdTmp[n].flag);
		printf("cmdTmp[%d].lastVar = %d\n", n, cmdTmp[n].lastVar);
		printf("cmdTmp[%d].stack = %d\n\n", n, cmdTmp[n].stack);
	}
}

void dump_stack()
{
	int n;

	for (n=0; n<=stack;n++)
	{
		printf("stack[%d]=",n);

		switch( kittyStack[n].type )
		{		
			case type_int:
				printf("%d\n",kittyStack[n].value);
				break;
			case type_float:
				printf("%f\n",kittyStack[n].decimal);
				break;
			case type_string:
				if (kittyStack[n].str)
				{
					printf("'%s' stack is %d cmd stack is %d\n", kittyStack[n].str, stack, cmdStack);
				}
				else
				{
					printf("no string found\n");
				}
				break;
		}
	}
}

