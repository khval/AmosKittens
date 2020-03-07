
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/retroMode.h>
#include <exec/emulation.h>
#include <proto/dos.h>
extern unsigned int regs[16];
#endif

#include "commandsLibs.h"
#include "kittyErrors.h"
#include "debug.h"
#include "stack.h"
#include "amosKittens.h"

extern std::vector<struct kittyLib> libsList;


struct kittyLib *kFindLib( int id )
{
	unsigned int n;
	for (n=0;n<libsList.size();n++)
	{
		if (libsList[n].id == id ) return &libsList[n];
	}
	return NULL;
}

int kFindLibIndex( int id )
{
	unsigned int n;
	for (n=0;n<libsList.size();n++)
	{
		if (libsList[n].id == id ) return n;
	}
	return -1;
}

void kFreeLib( int id )
{
	int index;
	struct kittyLib *lib = NULL;

	index = kFindLibIndex( id );

	if (index>-1)
	{
		lib = &libsList[index];
		if (lib) CloseLibrary( lib -> base );
		libsList.erase(libsList.begin()+index);
	}
}


extern int last_var;

char *_libLibOpen( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:
				{
					int id = getStackNum(__stack-2 );
					struct stringData *name = getStackString(__stack-1  );
					int ver = getStackNum(__stack );
					struct kittyLib lib;

					kFreeLib( id );

					if (name)
					{
						printf("lib Open %d,%s,%d\n",id,&(name->ptr),ver);
						lib.id = id;
						lib.base = OpenLibrary( &(name -> ptr) , ver);

						if (lib.base)
						{
							libsList.push_back(lib);
							popStack(__stack - data -> stack );
							setStackNone();
							return NULL;
						}
					}

					popStack(__stack - data -> stack );
					setError(170,data->tokenBuffer);
					return NULL;
				}
				break;
		default:
				popStack(__stack - data -> stack );
				setError(22,data-> tokenBuffer);
				return NULL;
	}

	popStack(__stack - data -> stack );
	setStackNum( 0 );	
	return NULL;
}
char *libLibOpen(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _libLibOpen, tokenBuffer );
	return tokenBuffer;
}

char *_libLibClose( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			kFreeLib( getStackNum(__stack) );
			break;
		default:
			popStack(__stack - data -> stack );
			setError(23,data-> tokenBuffer);
			break;
	}


	return NULL;
}

char *libLibClose(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _libLibClose, tokenBuffer );
	return tokenBuffer;
}

char *_libLibCall( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack + 1;
	int id, libVec,ret;
	struct kittyLib *lib;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
				id = getStackNum(__stack-1);
				libVec = getStackNum(__stack);

				lib = kFindLib( id );
				if (lib)
				{
					ret = EmulateTags( 
						lib -> base,
						ET_Offset, libVec,
						ET_RegisterD0,regs[0],
						ET_RegisterD1,regs[1],
						ET_RegisterD2,regs[2],
						ET_RegisterD3,regs[3],
						ET_RegisterD4,regs[4],
						ET_RegisterD5,regs[5],
						ET_RegisterD6,regs[6],
						ET_RegisterD7,regs[7],
						ET_RegisterA0,regs[8+0],
						ET_RegisterA1,regs[8+1],
						ET_RegisterA2,regs[8+2],
						ET_RegisterA6,regs[8+6],
						TAG_END	 );

					popStack(__stack - data -> stack );
					setStackNum(ret);
					return NULL;
				}

		default:
				setError(22,data->tokenBuffer);
	}

	setError(169,data->tokenBuffer);
	popStack(__stack - data -> stack );
	return NULL;
}

char *libLibCall(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _libLibCall, tokenBuffer );
	return tokenBuffer;
}


char *_libLibBase( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack + 1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			{
				int id = getStackNum(__stack );
				struct kittyLib *lib;

				lib = kFindLib( id );

				printf("%08x\n",lib);

				if (lib)
				{
					setStackNum( (int) lib -> base );
					return NULL;
				}
				else
				{
					setError( 169 , data -> tokenBuffer );	// library not opened
					return NULL;
				}
				break;
			}
		default:
				setError(22,data->tokenBuffer);
	}

	return NULL;
}

char *libLibBase(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _libLibBase, tokenBuffer );
	return tokenBuffer;
}




