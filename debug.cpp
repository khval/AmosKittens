
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "amosKittens.h"
#include "commands.h"
#include "debug.h"
#include <vector>
#include <proto/retroMode.h>

extern struct globalVar globalVars[1000];
extern std::vector<struct lineAddr> linesAddress;
extern std::vector<struct label> labels;
extern int global_var_count;

extern struct retroScreen *screens[8] ;
extern int current_screen ;

char *_ifSuccess(struct glueCommands *data, int nextToken) ;
char *_ifNotSuccess(struct glueCommands *data, int nextToken) ;

char *_ifThenSuccess(struct glueCommands *data, int nextToken) ;
char *_ifThenNotSuccess(struct glueCommands *data, int nextToken) ;

char *_textCentre (struct glueCommands *data, int nextToken);
char *_mathSin (struct glueCommands *data, int nextToken);
char *_addData (struct glueCommands *data, int nextToken);
char *_subData (struct glueCommands *data, int nextToken);
char *_mulData (struct glueCommands *data, int nextToken);
char *_divData (struct glueCommands *data, int nextToken);
char *_orData (struct glueCommands *data, int nextToken);
char *_andData (struct glueCommands *data, int nextToken);
char *_setVar (struct glueCommands *data, int nextToken);
char *_for (struct glueCommands *data, int nextToken);
char *_do (struct glueCommands *data, int nextToken);
char *_equalData (struct glueCommands *data, int nextToken);
char *_andData (struct glueCommands *data, int nextToken);
char *_ifSuccess (struct glueCommands *data, int nextToken);
char *_ifThenSuccess (struct glueCommands *data, int nextToken);
char *_machinePeek(struct glueCommands *data, int nextToken);
char *_cmdStart(struct glueCommands *data, int nextToken);
char *_chr(struct glueCommands *data, int nextToken);
char *_gfxPoint(struct glueCommands *data, int nextToken);
char *_mid(struct glueCommands *data, int nextToken);
char *_left(struct glueCommands *data, int nextToken);
char *_right(struct glueCommands *data, int nextToken);
char *_cmdStr(struct glueCommands *data, int nextToken);
char *_while(struct glueCommands *data, int nextToken);
char *_repeat(struct glueCommands *data, int nextToken);
char *_gosub_return(struct glueCommands *data, int nextToken);
char *_get_var_index(struct glueCommands *data, int nextToken);
char *_alloc_mode_off(struct glueCommands *data, int nextToken);
char *_procAndArgs (struct glueCommands *data, int nextToken);

struct stackDebugSymbol
{
	char *(*fn) (struct glueCommands *data, int nextToken );
	const char *name;
};

struct stackDebugSymbol stackDebugSymbols[] =
{
	{_mathSin,"_mathSin"},
	{_addData,"_addData"},
	{_subData,"_subData"},
	{_divData,"_divData"},
	{_mulData,"_mulData"},
	{_orData,"_orData"},
	{_andData,"_andData"},
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
	{_gfxPoint,"Point" },
	{_mid,"Mid" },
	{_left,"Left" },
	{_right,"Right" },
	{_cmdStr,"Str$" },
	{_ifSuccess,"if Success" },
	{_ifNotSuccess,"if Not Success"},
	{_ifThenSuccess,"if Then Success" },
	{_ifThenNotSuccess,"if Then Not Success"},
	{_repeat,"repeat"},
	{_while,"while" },
	{_gosub_return,"gosub_return" },
	{_get_var_index,"get var(index,...)" },
	{_textCentre, "Centre" },
	{_alloc_mode_off,"_alloc_mode_off"},
	{_procedure,"procedure"},
	{_procAndArgs,"procedure with args"},
	{NULL, NULL}
};

const char *findDebugSymbolName( char *(*fn) (struct glueCommands *data, int nextToken) )
{
	struct stackDebugSymbol *ptr;

	for (ptr = stackDebugSymbols; ptr -> fn; ptr++)
	{
		if (ptr -> fn == fn) return ptr -> name;
	}

	return NULL;
}

void dump_labels()
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
				printf("%d -- %d::%s%s=%0.2lf\n",n,
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

				if (globalVars[n].var.procDataPointer == 0)
				{
					printf("%d -- %d::%s%s[]=%04X (line %d)\n",n,
						globalVars[n].proc, "Proc ",
						globalVars[n].varName, 
						globalVars[n].var.tokenBufferPos, getLineFromPointer( globalVars[n].var.tokenBufferPos ) );
				}
				else
				{
					printf("%d -- %d::%s%s[]=%04X (line %d)  --- data read pointer %08x (line %d)\n",n,
						globalVars[n].proc, "Proc ",
						globalVars[n].varName, 
						globalVars[n].var.tokenBufferPos, getLineFromPointer( globalVars[n].var.tokenBufferPos ),
						globalVars[n].var.procDataPointer, getLineFromPointer( globalVars[n].var.procDataPointer ) );
				}

				break;
			case type_int | type_array:

				printf("%d -- %d::%s%s(%d)=",n,
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName,
					globalVars[n].var.count);
#ifdef show_array_yes
				for (i=0; i<globalVars[n].var.count; i++)
				{
					printf("[%d]=%d ,",i, globalVars[n].var.int_array[i]);
				}
#else
				printf("...");
#endif
				printf("\n");

				break;
			case type_float | type_array:

				printf("%d -- %d::%s%s(%d)=",n,
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName,
					globalVars[n].var.count);
#ifdef show_array_yes
				for (i=0; i<globalVars[n].var.count; i++)
				{
					printf("[%d]=%0.2f ,",i, globalVars[n].var.float_array[i]);
				}
#else
				printf("...");
#endif
				printf("\n");

				break;
			case type_string | type_array:

				printf("%d -- %d::%s%s(%d)=",n,
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName,
					globalVars[n].var.count);
#ifdef show_array_yes
				for (i=0; i<globalVars[n].var.count; i++)
				{
					printf("[%d]=%s ,",i, globalVars[n].var.str_array[i]);
				}
#else
				printf("...");
#endif
				printf("\n");


				break;
		}
	}
}

void dump_prog_stack()
{
	int n;
	const char *name;

	printf("\nDump prog stack:\n\n");

	for (n=0; n<cmdStack;n++)
	{

		name = findDebugSymbolName( cmdTmp[n].cmd );

		printf("cmdTmp[%d].cmd = %08x (%s) \n", n, cmdTmp[n].cmd, name ? name : "?????" );
		printf("cmdTmp[%d].tokenBuffer = %08x  - at line: %d \n", n, cmdTmp[n].tokenBuffer, getLineFromPointer(cmdTmp[n].tokenBuffer));
		printf("cmdTmp[%d].flag = %08x\n", n, cmdTmp[n].flag);
		printf("cmdTmp[%d].lastVar = %d\n", n, cmdTmp[n].lastVar);
		printf("cmdTmp[%d].stack = %d\n\n", n, cmdTmp[n].stack);
	}
}

void dump_stack()
{
	int n,v;

	printf("\nDump stack:\n\n");

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

void dump_banks()
{
	int n = 0;
	printf( "%s\n", "Nr   Type       Start       Length\n\n");

	for (n=0;n<15;n++)
	{
		if (kittyBanks[n].start)
		{
			printf("%2d - %.8s S:$%08X L:%d\n", 
				n+1,
				(char *) kittyBanks[n].start-8,
				kittyBanks[n].start, 
				kittyBanks[n].length);
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
	dump_labels();
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

	if ( linesAddress.size() )
	{
		if ( linesAddress[linesAddress.size()-1].end < address ) return linesAddress.size()-2;
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

void dumpScreenInfo()
			{
				int n;
				struct retroScreen *s;

				s = screens[current_screen];

				printf("-- dump screen info --\n");
				printf(" current screen %d\n",current_screen );
				printf(" current screen hex %08x\n",s);


				if (s -> fade_speed)
				{
					printf("fade speed: %d\n", s->fade_speed);

					for (n=0;n<256;n++)
					{
						printf("%d, %02X,%02X,%02X --> %02X,%02X,%02X\n",
							n,
							s -> orgPalette[n].r,
							s -> orgPalette[n].g,
							s -> orgPalette[n].b,
							s -> fadePalette[n].r,
							s -> fadePalette[n].g,
							s -> fadePalette[n].b );
					}
				}

				printf("other screens:\n");
				for (n=0;n<8;n++)
				{
					if (screens[n])
					{
						printf("screen %d, dw %d, dh %d, rw %d, rh %d, display x %d display y %d pen %d, paper %d, fade_speed %d\n", 
							n,
							screens[n]->displayWidth, screens[n]->displayHeight,
							screens[n]->realWidth,screens[n]->realHeight,
							screens[n]->scanline_x,screens[n]->scanline_y,
							screens[n]->pen,screens[n]->paper,
							screens[n]->fade_speed);							
					}
				}
			};

