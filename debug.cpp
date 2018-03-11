#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "amosKittens.h"
#include "commands.h"
#include <vector>

extern struct globalVar globalVars[1000];
extern std::vector<struct label> labels;
extern int global_var_count;

void dumpLabels()
{
	int n;

	for (n=0;n<labels.size();n++)
	{
		printf("%d: tokenLocation: %08x, Name: %s\n" ,n, labels[n].tokenLocation, labels[n].name);

	}
}

void dump_global()
{
	int n;
	int i;

	for (n=0;n<global_var_count;n++)
	{
		if (globalVars[n].varName == NULL) return;

		switch (globalVars[n].var.type)
		{
			case type_int:
				printf("%d::%s%s=%d\n",
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName, globalVars[n].var.value );
				break;
			case type_float:
				printf("%d::%s%s=%d\n",
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName, globalVars[n].var.decimal );
				break;
			case type_string:
				printf("%d::%s%s=%d\n",
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName, globalVars[n].var.str ? globalVars[n].var.str : "NULL" );
				break;
			case type_proc:
				printf("%d::%s%s[]=%04X\n",
					globalVars[n].proc, "Proc ",
					globalVars[n].varName, globalVars[n].var.tokenBufferPos );

				break;
			case type_int | type_array:

				printf("%d::%s%s(%d)=",
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName,
					globalVars[n].var.count);

				for (i=0; i<globalVars[n].var.count; i++)
				{
					printf("[%d]=%d ,",i, globalVars[n].var.int_array[i]);
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

		if (kittyStack[n].state == state_hidden_subData)
		{
			printf("[blocked]\n");
		}
		else
		{

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
						printf("'%s' (0x%x)\n", kittyStack[n].str, kittyStack[n].str) ;
					}
					else
					{
						printf("no string found\n");
					}
					break;
			}
		}
	}
}

