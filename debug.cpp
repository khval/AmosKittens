#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "amosKittens.h"
#include "commands.h"
#include <vector>

extern struct globalVar globalVars[1000];
extern std::vector<struct lineAddr> linesAddress;
extern std::vector<struct label> labels;
extern int global_var_count;

char *_mathSin (struct glueCommands *data);
char *_addData (struct glueCommands *data);
char *_subData (struct glueCommands *data);
char *_mulData (struct glueCommands *data);
char *_divData (struct glueCommands *data);
char *_setVar (struct glueCommands *data);
char *_for (struct glueCommands *data);
char *_do (struct glueCommands *data);
char *_equalData (struct glueCommands *data);
char *_andData (struct glueCommands *data);
char *_ifSuccess (struct glueCommands *data);
char *_ifThenSuccess (struct glueCommands *data);
char *_machinePeek( struct glueCommands *data );
char *_cmdStart( struct glueCommands *data );
char *_chr( struct glueCommands *data );

struct stackDebugSymbol
{
	char *(*fn) (struct glueCommands *data);
	const char *name;
};

struct stackDebugSymbol stackDebugSymbols[] =
{
	{_mathSin,"_mathSin"},
	{_addData,"_addData"},
	{_subData,"_subData"},
	{_divData,"_divData"},
	{_mulData,"_mulData"},
	{_print,"_print"},
	{_setVar, "_setVar"},
	{_for,"_for" },
	{_do,"_do" },
	{_equalData, "=" },
	{_andData, "AND" },
	{_ifSuccess,"If Success" },
	{_ifThenSuccess,"If Then Success" },
	{_machinePeek,"Peek" },
	{_cmdStart,"Start" },
	{_chr,"Chr$" },
	{NULL, NULL}
};

const char *findDebugSymbolName( char *(*fn) (struct glueCommands *data) )
{
	struct stackDebugSymbol *ptr;

	for (ptr = stackDebugSymbols; ptr -> fn; ptr++)
	{
		if (ptr -> fn == fn) return ptr -> name;
	}

	return NULL;
}

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
				printf("%d -- %d::%s%s=%d\n",n,
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName, globalVars[n].var.value );
				break;
			case type_float:
				printf("%d -- %d::%s%s=%d\n",n,
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName, globalVars[n].var.decimal );
				break;
			case type_string:
				printf("%d -- %d::%s%s=%s\n",n,
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName, globalVars[n].var.str ? globalVars[n].var.str : "NULL" );
				break;
			case type_proc:
				printf("%d -- %d::%s%s[]=%04X\n",n,
					globalVars[n].proc, "Proc ",
					globalVars[n].varName, globalVars[n].var.tokenBufferPos );

				break;
			case type_int | type_array:

				printf("%d -- %d::%s%s(%d)=",n,
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

				printf("%d -- %d::%s%s(%d)=",n,
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName,
					globalVars[n].var.count);

				for (i=0; i<globalVars[n].var.count; i++)
				{
					printf("[%d]=%0.2f ,",i, globalVars[n].var.float_array[i]);
				}
				printf("\n");

				break;
			case type_string | type_array:

				printf("%d -- %d::%s%s(%d)=",n,
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName,
					globalVars[n].var.count);

				for (i=0; i<globalVars[n].var.count; i++)
				{
					printf("[%d]=%s ,",i, globalVars[n].var.str_array[i]);
				}
				printf("\n");


				break;
		}
	}
}

void dump_prog_stack()
{
	int n;
	const char *name;


	for (n=0; n<cmdStack;n++)
	{

		name = findDebugSymbolName( cmdTmp[n].cmd );

		printf("cmdTmp[%d].cmd = %08x (%s) \n", n, cmdTmp[n].cmd, name ? name : "?????" );
		printf("cmdTmp[%d].tokenBuffer = %08x\n", n, cmdTmp[n].tokenBuffer);
		printf("cmdTmp[%d].flag = %08x\n", n, cmdTmp[n].flag);
		printf("cmdTmp[%d].lastVar = %d\n", n, cmdTmp[n].lastVar);
		printf("cmdTmp[%d].stack = %d\n\n", n, cmdTmp[n].stack);
	}
}

void dump_stack()
{
	int n,v;

	for (n=0; n<=stack;n++)
	{
		printf("stack[%d]=",n);

		if (kittyStack[n].state == state_hidden_subData)
		{
			printf("[blocked hidden] ---- data: %08x\n", kittyStack[n].str);
		}
		else if (kittyStack[n].state == state_subData)
		{
			printf("[blocked]\n");
		}
		else
		{
			switch( kittyStack[n].type )
			{	
				case type_none:
					printf("<Nothing>\n");
					break;
				case type_int:
					v = kittyStack[n].value;
					if (  ((v>='a')&&(v<='z'))  ||  ((v>='A')&&(v<='Z'))  )
					{
						printf("%d '%c'\n",v, (char) v );
					}
					else	printf("%d\n",v);
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

void dump_end_of_program()
{
	printf("--- End of program status ---\n");

	printf("\n--- var dump ---\n");
	dump_global();

	printf("\n--- value stack dump ---\n");
	dump_stack();

	printf("\n--- program stack dump ---\n");
	dump_prog_stack();

	printf("\n--- label dump ---\n");
	dumpLabels();
}

int getLineFromPointer( char *address )
{
	int n = 0;

	for (n=0;n<linesAddress.size();n++)
	{
		if ( (linesAddress[n].start >= address) && (address < linesAddress[n].end) )
		{
			return n-1;
		}
	}
	return -1;
}

void dumpLineAddress()
{
	int n = 0;

	for (n=0;n<linesAddress.size();n++)
	{
		printf("Line %08d, start %08x end %08x\n", n-1, linesAddress[n].start , linesAddress[n].end );
	}
}

