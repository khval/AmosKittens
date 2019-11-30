
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <string.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
extern FILE *engine_fd;
#define Printf printf
#endif

#include "amosKittens.h"
#include "commands.h"
#include "debug.h"
#include <vector>

extern struct globalVar globalVars[1000];
extern std::vector<struct lineAddr> linesAddress;
extern std::vector<struct label> labels;
extern std::vector<struct kittyBank> kittyBankList;
extern std::vector<struct wave *> waves;
extern int global_var_count;

extern struct retroScreen *screens[8] ;
extern struct retroSpriteObject bobs[64];
extern int current_screen ;

struct lineFromPtr lineFromPtr;

// _lessOrEqualData

extern int regs[16];

#ifdef __linux__
extern FILE *engine_fd;
#endif

char *_ifSuccess(struct glueCommands *data, int nextToken) ;
char *_ifNotSuccess(struct glueCommands *data, int nextToken) ;

char *_ifThenSuccess(struct glueCommands *data, int nextToken) ;
char *_ifThenNotSuccess(struct glueCommands *data, int nextToken) ;

char *_cmdNot (struct glueCommands *data, int nextToken);
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
char *_machineDeek( struct glueCommands *data, int nextToken);
char *_machineLeek( struct glueCommands *data, int nextToken);
char *_bankStart(struct glueCommands *data, int nextToken);
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
char *_endProc (struct glueCommands *data, int nextToken);
char *_len (struct glueCommands *data, int nextToken);
char *_lessOrEqualData (struct glueCommands *data, int nextToken);
char *_moreOrEqualData (struct glueCommands *data, int nextToken);
char *_discExist( struct glueCommands *data, int nextToken );
char *_not_equal( struct glueCommands *data, int nextToken );
char *_exit( struct glueCommands *data, int nextToken );
char *_errTrap( struct glueCommands *data, int nextToken );
char *_mathFn( struct glueCommands *data, int nextToken );
char *_mathFnReturn( struct glueCommands *data, int nextToken );
char *_machineAREG( struct glueCommands *data, int nextToken );
char *_machineDREG( struct glueCommands *data, int nextToken );
char *_gfxLogic( struct glueCommands *data, int nextToken );
char *_gfxScreenCopy( struct glueCommands *data, int nextToken );
char *_setVar( struct glueCommands *data, int nextToken );
char *_set_amreg_fn( struct glueCommands *data, int nextToken );
char *_set_amreg_channel_fn( struct glueCommands *data, int nextToken );


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
	{_machineDeek,"Deek" },
	{_machineLeek,"Leek" },
	{_bankStart,"Start" },
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
	{_endProc,"end proc"},
	{_len,"len"},
	{_lessOrEqualData,"<="},
	{_moreOrEqualData,">="},
	{_cmdNot,"Not"},
	{_discExist,"Exist"},
	{_not_equal,"<>"},
	{_exit,"exit loop"},
	{_errTrap,"Trap"},
	{_mathFn,"Fn"},
	{_mathFnReturn, "Fn (Return)"},
	{_machineAREG,"_machineAREG"},
	{_machineDREG,"_machineDREG"},
	{_gfxLogic,"_gfxLogic"},
	{_gfxScreenCopy,"_gfxScreenCopy"},
	{_setVar,"set var"},
	{_set_amreg_fn,"_set_amreg_fn" },
	{_set_amreg_channel_fn,"_set_amreg_channel_fn" },
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
	unsigned int n;

	for (n=0;n<labels.size();n++)
	{
		printf("%d: tokenLocation: %08x, Name: %s - in proc %d\n" ,n, 
				labels[n].tokenLocation, 
				labels[n].name, labels[n].proc);

	}
}

unsigned int str_crc( char *name )
{
	char *s;
	unsigned int crc = 0;
	int cnt;

	cnt = 0;
	for (s=name;*s;s++)
	{
		crc ^= (*s) << ((cnt%4)*8);
		cnt++;
	}
	return crc;
}

unsigned int vars_crc()
{
	int n;
	unsigned int crc = 0;

	for (n=0;n<global_var_count;n++)
	{
		if (globalVars[n].varName == NULL) return 0;
		crc ^= str_crc( globalVars[n].varName );
	}
	return crc;
}

unsigned int mem_crc( char *mem, uint32_t size )
{
	uint32_t n;
	unsigned int crc = 0;
	for (n=0;n<size;n++) crc ^= mem[n] << (n % 24);
	return crc;
}


void dump_var( int n )
{
#ifdef show_array_yes
	int i;
#endif
		switch (globalVars[n].var.type)
		{
			case type_int:
				printf("%d -- %d::%s%s=%d\n",n,
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName, globalVars[n].var.integer.value );
				break;
			case type_float:
				printf("%d -- %d::%s%s=%0.2lf\n",n,
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName, globalVars[n].var.decimal.value );
				break;
			case type_string:
				printf("%d -- %d::%s%s=%c%s%c\n",n,
					globalVars[n].proc, 
					globalVars[n].isGlobal ? "Global " : "",
					globalVars[n].varName, 34, globalVars[n].var.str ? &(globalVars[n].var.str -> ptr) : "NULL", 34 );
				break;
			case type_proc:

				if (globalVars[n].var.procDataPointer == 0)
				{
					getLineFromPointer( globalVars[n].var.tokenBufferPos );

					printf("%d -- %d::%s%s[]=%04X (line %d)\n",n,
						globalVars[n].proc, "Proc ",
						globalVars[n].varName, 
						globalVars[n].var.tokenBufferPos, lineFromPtr.line );
				}
				else
				{
					int tokenBufferLine;
					getLineFromPointer( globalVars[n].var.tokenBufferPos );
					tokenBufferLine = lineFromPtr.line;
					getLineFromPointer( globalVars[n].var.procDataPointer );

					printf("%d -- %d::%s%s[]=%04X (line %d)  --- data read pointer %08x (line %d)\n",n,
						globalVars[n].proc, "Proc ",
						globalVars[n].varName, 
						globalVars[n].var.tokenBufferPos, tokenBufferLine,
						globalVars[n].var.procDataPointer, lineFromPtr.line );
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
					printf("[%d]=%d ,",i, (&(globalVars[n].var.int_array -> ptr) +i) -> value );
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
					printf("[%d]=%0.2f ,",i, (&(globalVars[n].var.float_array -> ptr)+i) -> value );
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
					strptr =(&(globalVars[n].var.str_array -> ptr))[i];

					printf("[%d]=%s ,",i, strptr ? &(strptr -> ptr) : "<NULL>");
				}
#else
				printf("...");
#endif
				printf("\n");


				break;
		}
}

void dump_local( int proc )
{
	int n;

	for (n=0;n<global_var_count;n++)
	{
		if (globalVars[n].varName == NULL) return;

		if (globalVars[n].proc == proc)
		{
			dump_var( n );
		}
	}
}


void dump_global()
{
	int n;

	for (n=0;n<global_var_count;n++)
	{
		if (globalVars[n].varName == NULL) return;
		dump_var( n );
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

		getLineFromPointer(cmdTmp[n].tokenBuffer);

		printf("cmdTmp[%d].cmd = %08x (%s) \n", n, cmdTmp[n].cmd, name ? name : "?????" );
		printf("cmdTmp[%d].tokenBuffer = %08x  - at line: %d \n", n, cmdTmp[n].tokenBuffer, lineFromPtr.line );
		printf("cmdTmp[%d].flag = %08x\n", n, cmdTmp[n].flag);
		printf("cmdTmp[%d].lastVar = %d\n", n, cmdTmp[n].lastVar);
		printf("cmdTmp[%d].token = %04x\n", n, cmdTmp[n].token);
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
			struct kittyData *var = &kittyStack[n];

			switch( var -> type )
			{	
				case type_none:
					printf("<Nothing>\n");
					break;
				case type_int:
					v = var -> integer.value;
					if (  ((v>='a')&&(v<='z'))  ||  ((v>='A')&&(v<='Z'))  )
					{
						printf("%d '%c'\n",v, (char) v );
					}
					else	printf("%d\n",v);
					break;
				case type_float:
					printf("%f\n", var -> decimal.value);
					break;
				case type_string:
					if (var -> str)
					{
						printf("[0x%08x] '%s' (0x%08X) length %d\n", 
							var -> str,
							&var -> str -> ptr, 
							&var -> str -> ptr, 
							var -> str -> size) ;
					}
					else
					{
						printf("<NULL>\n");
					}
					break;
			}
		}
	}
}

void dump_banks()
{
	unsigned int n = 0;
	struct kittyBank *bank;
	printf( "Nr   Type       Start       Length\n\n");
	for (n=0;n<kittyBankList.size();n++)
	{
		bank = &kittyBankList[n];
		if (bank -> start)
		{
			printf("%03d - %.8s S:$%08X L:%d\n", 
				bank -> id,
				(char *) bank->start-8,
				bank -> start, 
				bank -> length);
		}
	}
	printf("\n");
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

	printf("\n-- banks loaded --\n");
	dump_banks();
}

void getLineFromPointer( char *address )
{
	unsigned int n = 0;
	unsigned int offset = 0;

	offset = address - _file_start_;

	for (n=0;n<linesAddress.size();n++)
	{
		if ( (linesAddress[n].start >= offset) && (offset < linesAddress[n].end) )
		{
			lineFromPtr.file =  linesAddress[n].file;
			lineFromPtr.line =  linesAddress[n].lineNumber;
			return;
		}
	}

	if ( linesAddress.size() )
	{
		if ( linesAddress[linesAddress.size()-1].end < offset ) 
		{
			n = linesAddress.size()-1;
			lineFromPtr.file =  linesAddress[n].file;
			lineFromPtr.line =  linesAddress[n].lineNumber;
			return;
		}
	}

	lineFromPtr.file =  -1;
	lineFromPtr.line =  -1;
}

void dump_lines()
{
	unsigned int n = 0;
	unsigned int offset =0;
	unsigned char *ptr;

	for (n=0;n<linesAddress.size();n++)
	{
		printf("File %04d Line %04d, token start %d, end %d: (from src %d and %d)\n", 
				linesAddress[n].file, 
				linesAddress[n].lineNumber, 
				linesAddress[n].start, 
				linesAddress[n].end,
				linesAddress[n].srcStart, 
				linesAddress[n].srcEnd );

		for (offset = linesAddress[n].start; offset < linesAddress[n].end ; offset+=2 )
		{
			ptr = (unsigned char *) _file_start_ + offset;
			printf("%04x ",*((unsigned short *) ptr));
		}
		printf("\n");
	}
}

void dump_680x0_regs()
{
	int n;
	for(n=0;n<8;n++) printf("D%d=%08X ",n,regs[n]);
	printf("\n");
	for(n=0;n<8;n++) printf("A%d=%08X ",n,regs[n+8]);
	printf("\n");
}

void dump_pal(struct retroScreen *s, int colors)
{
	int n;
	Printf("      ");

	for (n =0;n<colors;n++)
	{
		Printf("%02lX%02lX%02lX ",
			s -> orgPalette[n].r,
			s -> orgPalette[n].g,
			s -> orgPalette[n].b);
	}
	Printf("\n");
}

void dump_bobs(int screen_id)
{
	int n;
	Printf("      ");

	for (n =0;n<64;n++)
	{
		if (screen_id == bobs[n].screen_id)
		{
			if (bobs[n].image != -1)
			{
				Printf_iso("[ %ld, %4ld,%4ld,%4ld ] ",
					n,
					bobs[n].x,
					bobs[n].y,
					bobs[n].image);
			}
		}
	}
	Printf("\n");
}

void dump_zones()
{
	int z;
	struct zone *zz;
	struct retroScreen *s;
	for (z=0;z<zones_allocated;z++)
	{
		if ((zones[z].screen>-1) && (zones[z].screen<8))
		{
			if (s = screens[zones[z].screen])
			{
				zz = &zones[z];
				printf ("zone %d at %d,%d to %d,%d - on screen %d\n",z, zz->x0,zz->y0,zz->x1,zz->y1,zz -> screen );
			}
		}
	}
}


void dump_screens()
{
	int n;

	Printf("Screens:\n");
	for (n=0;n<8;n++)
	{
		if (screens[n])
		{
			Printf_iso("screen %3d, dw %3d, dh %3d, rw %3d, rh %3d, display %4d,%4d, offset %4d,%4d, db %s, frame %d, autoback %d, fade_speed %d\n", 
				n,
				screens[n]->displayWidth, screens[n]->displayHeight,
				screens[n]->realWidth,screens[n]->realHeight,
				screens[n]->scanline_x/2+128,screens[n]->scanline_y/2+50,
				screens[n]->offset_x,screens[n]->offset_y,
				screens[n]->Memory[1] ? "Yes" : "No ",
				screens[n]->double_buffer_draw_frame,
				screens[n]->autoback,
				screens[n]->fade_speed);

//				dump_pal( screens[n] , 8 );						
				dump_bobs( n );
		}
	}
};

void dumpWaves( )
{
	unsigned int n,nn;
	printf("-- waves --\n");
	for (n=0;n<waves.size();n++)
	{
		printf("waves[%d] -> id %d\n",n, waves[n] -> id);

		for (nn=0;nn<7;nn++)
		{
			printf("waves[%d] -> envels[%d] = {%d,%d,%d}\n",n, nn,
				waves[n] -> envels[nn].volume, 
				waves[n] -> envels[nn].startDuration,
				waves[n] -> envels[nn].duration );
		}
	}
}

#ifdef __amigaos__
#define IDCMP_COMMON IDCMP_MOUSEBUTTONS | IDCMP_INACTIVEWINDOW | IDCMP_ACTIVEWINDOW  | \
	IDCMP_CHANGEWINDOW | IDCMP_MOUSEMOVE | IDCMP_REFRESHWINDOW | IDCMP_RAWKEY | \
	IDCMP_EXTENDEDMOUSE | IDCMP_CLOSEWINDOW | IDCMP_NEWSIZE | IDCMP_INTUITICKS | IDCMP_MENUPICK | IDCMP_GADGETUP

struct Window *debug_Window = NULL;;
#endif

void open_debug_window()
{
#ifdef __amigaos__
	debug_Window = OpenWindowTags( NULL,
				WA_Left,			820,
				WA_Top,			20,
				WA_InnerWidth,		800,
				WA_InnerHeight,	800,
				WA_SimpleRefresh,	TRUE,
				WA_CloseGadget,	FALSE,
				WA_DepthGadget,	TRUE,
				WA_DragBar,		TRUE,
				WA_Borderless,	FALSE,
				WA_SizeGadget,	FALSE,
				WA_SizeBBottom,	TRUE,
				WA_NewLookMenus,	TRUE,
				WA_Title, "Debug Window",
				WA_Activate,        TRUE,
				WA_Flags, WFLG_RMBTRAP| WFLG_REPORTMOUSE,
				WA_IDCMP,           IDCMP_COMMON,
			TAG_DONE);
#endif
}

void close_debug_window()
{
#ifdef __amigaos__
	if (debug_Window) CloseWindow(debug_Window);
	debug_Window = NULL;
#endif
}

void debug_draw_wave(struct wave *wave)
{
#ifdef __amigaos__
	unsigned int n;
	 char *data;
	data = ( char *) &(wave -> sample.ptr);

	open_debug_window();
	for (n=0;n<wave -> sample.bytes;n++) 	WritePixelColor( debug_Window -> RPort, 50+n, 400 + data[n] , 0xFF0000FF); 
	getchar();
	close_debug_window();
#endif
}


void debug_draw_hline(int x)
{
#ifdef __amigaos__
	int y;
	if (debug_Window)
	{
		for (y=-30;y<=30;y++)
		{
			WritePixelColor( debug_Window -> RPort, 50+x, 400+y, 0xFFFF0000); 
		}
	}
	else
	{
		printf("debug gfx window not open\n");
	}
#endif 
}
