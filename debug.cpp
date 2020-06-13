
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
#include "AmalCompiler.h"
#include "channel.h"
#include "debug.h"
#include <vector>

extern struct globalVar globalVars[1000];
extern std::vector<struct lineAddr> linesAddress;
extern std::vector<struct label> labels;
extern std::vector<struct kittyBank> kittyBankList;
extern std::vector<int> collided;

extern ChannelTableClass *channels;
extern int var_count[2];

extern  std::vector<struct retroSpriteObject *> bobs;

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
char *_cmdChrStr(struct glueCommands *data, int nextToken);
char *_cmdMidStr(struct glueCommands *data, int nextToken);
char *_cmdLeftStr(struct glueCommands *data, int nextToken);
char *_cmdRightStr(struct glueCommands *data, int nextToken);
char *_gfxPoint(struct glueCommands *data, int nextToken);
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
char *_gfxColour( struct glueCommands *data, int nextToken );
char *_textPrint( struct glueCommands *data, int nextToken );
char *_textUsing( struct glueCommands *data, int nextToken );


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
	{_textPrint,"_textPrint"},
	{_textUsing,"_textUsing"},
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
	{_cmdChrStr,"Chr$" },
	{_gfxPoint,"Point" },
	{_cmdMidStr,"Mid" },
	{_cmdLeftStr,"Left" },
	{_cmdRightStr,"Right" },
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
	{_gfxColour, "Colour" },
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

void dump_sprite()
{
	int image;
	struct retroFrameHeader *frame;

	if (instance.sprites)
	{
		for (image=0;image<instance.sprites -> number_of_frames;image++)
		{
			frame = &instance.sprites -> frames[image];

			printf("sprite %-3d, w %-3d, h %-3d, bpr %-3d, hotspot x %-3d, hotspot y %-3d, data %08x, mask %08x - alpha %d\n", 
				image+1,
				frame -> width,
				frame -> height,
				frame -> bytesPerRow,
				frame -> XHotSpot,
				frame -> YHotSpot,
				frame -> data,
				frame -> mask,
				frame -> alpha );
		}
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

	for (n=0;n<var_count[0];n++)
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
	for (n=0;n<size;n++) 
	{
		crc ^= mem[n] << (n % 24);
	}
	return crc;
}

extern struct stackFrame *find_stackframe(int proc);
void dump_var_ptr(int n, struct globalVar *varInfo, struct kittyData *var );
void dump_var_ptr_undefined(int n, struct globalVar *varInfo );

void dump_var( int n)
{
	struct kittyData *var = NULL;

	if (globalVars[n].var.type == type_proc)
	{
		var = &globalVars[n].var;
	}
	else if (globalVars[n].proc == 0)
	{
		var = &globalVars[n].var;
	}
	else
	{
		struct stackFrame *stackFrame = find_stackframe(globalVars[n].proc);
		if (stackFrame)	var = stackFrame -> localVarData + globalVars[n].localIndex;
	}

	if (var)
	{
		dump_var_ptr( n, globalVars + n , var );
	}
#ifdef show_dump_vars_undefined_yes
	else
	{
		dump_var_ptr_undefined( n, globalVars + n );
	}
#endif
}


void dump_var_ptr( int n, struct globalVar *varInfo, struct kittyData *var )
{
#ifdef show_array_yes
	int i;
#endif
		switch (var -> type)
		{
			case type_int:
				printf("%d -- %d:%d:%s%s=%d\n",n,
					varInfo ->  proc, 
					varInfo ->  localIndex,
					varInfo ->  isGlobal ? "Global " : "",
					varInfo ->  varName, var -> integer.value );
				break;
			case type_float:
				printf("%d -- %d:%d:%s%s=%0.2lf\n",n,
					varInfo ->  proc, 
					varInfo ->  localIndex,
					varInfo ->  isGlobal ? "Global " : "",
					varInfo ->  varName, var -> decimal.value );
				break;
			case type_string:
				printf("%d -- %d:%d:%s%s=%c%s%c\n",n,
					varInfo ->  proc, 
					varInfo ->  localIndex,
					varInfo ->  isGlobal ? "Global " : "",
					varInfo ->  varName, 34, var -> str ? &(var -> str -> ptr) : "NULL", 34 );
				break;
			case type_proc:

				if (varInfo ->  procDataPointer == 0)
				{
					getLineFromPointer( var -> tokenBufferPos );

					printf("%d -- %d:%d:%s%s[]=%04X (line %d)\n",n,
						varInfo ->  proc, 
						varInfo ->  localIndexSize, 
						"Proc ",					
						varInfo ->  varName, 
						var -> tokenBufferPos, lineFromPtr.line );
				}
				else
				{
					int tokenBufferLine;
					getLineFromPointer( var -> tokenBufferPos );
					tokenBufferLine = lineFromPtr.line;
					getLineFromPointer( varInfo ->  procDataPointer );

					printf("%d -- %d::%s%s[]=%04X (line %d)  --- data read pointer %08x (line %d)\n",n,
						varInfo ->  proc, "Proc ",
						varInfo ->  varName, 
						var -> tokenBufferPos, tokenBufferLine,
						varInfo ->  procDataPointer, lineFromPtr.line );
				}

				break;
			case type_int | type_array:

				printf("%d -- %d:%d:%s%s(%d)=",n,
					varInfo ->  proc, 
					varInfo ->  localIndex,
					varInfo ->  isGlobal ? "Global " : "",
					varInfo ->  varName,
					var -> count);
#ifdef show_array_yes
				for (i=0; i<var -> count; i++)
				{
					printf("[%d]=%d ,",i, (&(var -> int_array -> ptr) +i) -> value );
				}
#else
				printf("...");
#endif
				printf("\n");

				break;
			case type_float | type_array:

				printf("%d -- %d:%d:%s%s(%d)=",n,
					varInfo ->  proc, 
					varInfo ->  localIndex,
					varInfo ->  isGlobal ? "Global " : "",
					varInfo ->  varName,
					var -> count);
#ifdef show_array_yes
				for (i=0; i<var -> count; i++)
				{
					printf("[%d]=%0.2f ,",i, (&(var -> float_array -> ptr)+i) -> value );
				}
#else
				printf("...");
#endif
				printf("\n");

				break;
			case type_string | type_array:

				printf("%d -- %d:%d:%s%s(%d)=",n,
					varInfo ->  proc, 
					varInfo ->  localIndex,
					varInfo ->  isGlobal ? "Global " : "",
					varInfo ->  varName,
					var -> count);
#ifdef show_array_yes

				{
					struct stringData *strptr;
					for (i=0; i<var -> count; i++)
					{
						strptr =(&(var -> str_array -> ptr))[i];

						printf("[%d]=%s ,",i, strptr ? &(strptr -> ptr) : "<NULL>");
					}
				}
#else
				printf("...");
#endif
				printf("\n");


				break;
		}
}

void dump_var_ptr_undefined(int n,struct globalVar *varInfo )
{
	printf("%d -- %d:%d:%s%s=<undefined>\n",n,
			varInfo ->  proc, 
			varInfo ->  localIndex,
			varInfo ->  isGlobal ? "Global " : "",
			varInfo ->  varName );
}



void dump_local( int proc )
{
	int n;

	for (n=0;n<var_count[0];n++)
	{
		if (globalVars[n].varName == NULL) return;

		if (globalVars[n].var.type != type_proc )
		{
			if (globalVars[n].proc == proc)
			{
				dump_var( n );
			}
		}
	}
}


void dump_global()
{
	int n;

	// dump global vars

	for (n=0;n<var_count[0];n++)
	{
		if (globalVars[n].varName == NULL) return;

		if (globalVars[n].proc == 0)  
		{
			dump_var( n );
		}
	}

	// proc

	for (n=0;n<var_count[0];n++)
	{
		if (globalVars[n].varName == NULL) return;

		if (globalVars[n].var.type == type_proc)  
		{
			dump_var( n );
			dump_local( globalVars[n].proc );
		}
	}

}

void dump_prog_stack()
{
	int n;
	const char *name;

	printf("\nDump prog stack:\n\n");

	for (n=0; n<instance.cmdStack;n++)
	{
		name = findDebugSymbolName( cmdTmp[n].cmd );

		getLineFromPointer(cmdTmp[n].tokenBuffer);

		printf("cmdTmp[%d].cmd = %08x (%s) \n", n, cmdTmp[n].cmd, name ? name : "?????" );
		printf("cmdTmp[%d].tokenBuffer = %08x  - at line: %d \n", n, cmdTmp[n].tokenBuffer, lineFromPtr.line );
		printf("cmdTmp[%d].flag = %08x\n", n, cmdTmp[n].flag);
		printf("cmdTmp[%d].lastVar = %d\n", n, cmdTmp[n].lastVar);
		printf("cmdTmp[%d].token = %04x\n", n, cmdTmp[n].token);
		printf("cmdTmp[%d].parenthesis_count = %d\n", n, cmdTmp[n].parenthesis_count);
		printf("cmdTmp[%d].stack = %d\n\n", n, cmdTmp[n].stack);
	}
}

void dump_stack()
{
	int n,v;

	printf("\nDump stack:\n\n");

	for (n=0; n<=instance.stack;n++)
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

bool var_has_name( struct kittyData *var, const char *name )
{
	int n;

	for (n=0;n<var_count[0];n++)
	{
		if (var == &globalVars[n].var)
		{
			if (globalVars[n].varName)
			{
				if (strcasecmp( globalVars[n].varName, name ) == 0)
				{
					return true;
				}
			}
		}
	}
	return false;
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
			printf("%03d - %.8s S:$%08X L:%d -> object %08x\n", 
				bank -> id,
				(char *) bank->start-8,
				bank -> start, 
				bank -> length,
				bank -> object_ptr);
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

void dump_all_bobs()
{
	unsigned int n;

	Printf_iso("dump_all_bobs\n");

	for (n =0;n<bobs.size();n++)
	{
		if (bobs[n] -> image != -1)
		{
			Printf_iso("bob %ld, %4ld,%4ld,%4ld -> screen %ld\n",
				bobs[n] -> id,
				bobs[n] -> x,
				bobs[n] -> y,
				bobs[n] -> image,
				bobs[n] -> screen_id );
		}
	}
	Printf("\n");
}


void dump_bobs_on_screen(int screen_id)
{
	unsigned int n;

	Printf_iso("dump_bobs on screen %ld",screen_id);

	for (n =0;n<bobs.size();n++)
	{
		if (screen_id == bobs[n] -> screen_id)
		{
			if (bobs[n] -> image != -1)
			{
				Printf_iso("[ %ld, %4ld,%4ld,%4ld ] ",
					bobs[n] -> id,
					bobs[n] -> x,
					bobs[n] -> y,
					bobs[n] -> image);
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
			if (s = instance.screens[zones[z].screen])
			{
				zz = &zones[z];
				printf ("zone %d at %d,%d to %d,%d - on screen %d\n",z, zz->x0,zz->y0,zz->x1,zz->y1,zz -> screen );
			}
		}
	}
}

void dump_anim()
{
	struct kittyChannel *item;

	Printf("\nDump anim channels\n");

	for ( unsigned int n  = 0 ; n < channels -> _size();n++ )
	{
		item = channels -> item(n);

		if (item)
		{	
			Printf("id: %ld, anim_loops: %ld, anim_sleep %ld, anim_sleep_to: %ld, status %ld\n",
				item -> id,
				item -> anim_loops,
				item -> anim_sleep,
				item -> anim_sleep_to,
				item -> animStatus);

			if (item -> anim_script) Printf("anim: '%s'\n",&(item -> anim_script -> ptr));
		}
	}
}


void dump_channels()
{
	struct kittyChannel *item;

	Printf("\nDump channels\n");

	for (unsigned int n  = 0 ; n < channels -> _size();n++ )
	{
		item = channels -> item(n);

		if (item)
		{	
			Printf("id: %ld, amal status: %ld amal script %s, (%08lx) anim status: %ld, anim script: %s\n",
				item -> id,
				item -> amalStatus,
				item -> amal_script ? "Yes" : "No",
				item -> amalProg.prog_crc,
				item -> animStatus,
				item -> anim_script ? "Yes" : "No"
				);
		}
	}
}

void dump_screens()
{
	int n;
	struct retroScreen *screen;

	Printf("Screens:\n");
	for (n=0;n<8;n++)
	{
		if (screen = instance.screens[n])
		{
			Printf_iso("screen %3d, dw %3d, dh %3d, rw %3d, rh %3d, display %4d,%4d, offset %4d,%4d, db %s, frame %d, autoback %d, fade_speed %d\n", 
				n,
				screen->displayWidth, screen->displayHeight,
				screen->realWidth,screen->realHeight,
				screen->scanline_x/2+128,screen->scanline_y/2+50,
				screen->offset_x,screen->offset_y,
				screen->Memory[1] ? "Yes" : "No ",
				screen->double_buffer_draw_frame,
				screen->autoback,
				screen->fade_speed);

//				dump_pal( screens[n] , 8 );						
				dump_bobs_on_screen( n );
		}
	}
};

void dump_collided()
{
	for (unsigned int n=0;n<collided.size();n++)
	{
		printf("collided id: %d\n",collided[n]);
	}
}

char *stackErrorBuffer = NULL;

int32 printStack(struct Hook *hook, struct Task *task, struct StackFrameMsg *frame)
{
	struct DebugSymbol *symbol = NULL;

	switch (frame->State)
	{
		case STACK_FRAME_DECODED:

				if (symbol = ObtainDebugSymbol(frame->MemoryAddress, NULL))
				{
					Printf("%s : %s\n", symbol -> Name, 
						symbol->SourceFunctionName ? symbol->SourceFunctionName : "NULL");
						ReleaseDebugSymbol(symbol);
				}
				else
				{
					Printf("(%p) -> %p\n", frame->StackPointer, frame->MemoryAddress);
				}
				break;

		case STACK_FRAME_INVALID_BACKCHAIN_PTR:

				Printf( "(%p) invalid backchain pointer\n",
					frame->StackPointer);
				break;

		case STACK_FRAME_TRASHED_MEMORY_LOOP:

				Printf( "(%p) trashed memory loop\n",
					frame->StackPointer);
				break;

		case STACK_FRAME_BACKCHAIN_PTR_LOOP:

				Printf( "(%p) backchain pointer loop\n",
					frame->StackPointer);
				break;

		default:
				Printf( "Unknown state=%lu\n", frame->State);
	}

	return 0;  // Continue tracing.
}


static int stack_trace_recored_count = 0;
static struct StackFrameMsg *stack_trace_recored;

int32 stack_trace_recored_fn(struct Hook *hook, struct Task *task, struct StackFrameMsg *frame)
{
	stack_trace_recored[ stack_trace_recored_count ] = *frame;
	stack_trace_recored_count ++;

	return 0;  // Continue tracing.
}



extern struct Task *main_task;

void __real_stack_trace()	// only call this from a new process.
{
	Printf("Error Error ---- stack trace\n");

	stack_trace_recored_count = 0;
	stack_trace_recored = (struct StackFrameMsg *) malloc( sizeof(struct StackFrameMsg) * 1000 );

	if (stack_trace_recored)
	{
		if (main_task != NULL)
		{
			struct Hook *hook = (struct Hook *) AllocSysObjectTags(ASOT_HOOK, ASOHOOK_Entry, printStack, TAG_END);

			if (hook != NULL)
			{
				SuspendTask(main_task, 0);
				uint32 result = StackTrace(main_task, hook);
				RestartTask(main_task, 0);
				
				Printf("-- stack trace count %d\n",stack_trace_recored_count);

				for (int i=0; i<stack_trace_recored_count;i++)
				{
					printStack( hook, main_task , &stack_trace_recored[ i ]);
				}

				Delay(50*20);

				FreeSysObject(ASOT_HOOK, hook);
			}
		}

		free(stack_trace_recored);
		stack_trace_recored = NULL;
	}
}


