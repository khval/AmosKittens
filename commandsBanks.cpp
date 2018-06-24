#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "debug.h"
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsBanks.h"
#include "errors.h"
#include "engine.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern int current_screen;

void _my_print_text(struct retroScreen *screen, char *text, int maxchars);

char *_cmdErase( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;

	if (args==1)
	{
		n = getStackNum(data->stack);

		if ((n>0)&&(n<16))
		{
			if (kittyBanks[n-1].start)
			{
				free( kittyBanks[n-1].start );
				kittyBanks[n-1].start = NULL;
				kittyBanks[n-1].length = 0;
				kittyBanks[n-1].type = 0;
			}
		} 
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdStart( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		n = getStackNum(stack);

		if ((n>0)&&(n<16))
		{
			ret = (int) kittyBanks[n-1].start;
			success = true;
		} 
	}

	if (success == false ) ret = 0;

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_cmdLength( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		n = getStackNum(stack);
		if ((n>0)&&(n<16))
		{
			ret = (int)  kittyBanks[n-1].length;
			success = true;
		} 
	}

	if (success == false ) ret = 0;

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}


char *_cmdBload( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	int n;
	int args = stack - data->stack +1 ;
	FILE *fd;
	int size;
	char *adr = NULL;

	dump_stack();

	if (args==2)
	{
		fd = fopen( getStackString( stack - 1 ) , "r");
		if (fd)
		{
			n = getStackNum(stack);

			fseek(fd , 0, SEEK_END );			
			size = ftell(fd);
			fseek(fd, 0, SEEK_SET );

			if (size)
			{
				if ((n>0)&&(n<16))
				{
					kittyBanks[n-1].length = size;
					if (kittyBanks[n-1].start) free( kittyBanks[n-1].start );
					kittyBanks[n-1].start = malloc( size );
					kittyBanks[n-1].type = 9;	
					adr = (char *)  kittyBanks[n-1].start;
				}
				else
				{
					char *adr = (char *) n;
				}
				
				if (adr) fread( adr ,size,1, fd);
			}

			fclose(fd);
		}

	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdBsave( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int n;
	int args = stack - data->stack +1 ;
	FILE *fd;
	char *start, *to;

	dump_stack();

	if (args==3)
	{
		fd = fopen( getStackString( stack - 2 ) , "w");

		start = (char *) getStackNum(stack -1 );
		to = (char *) getStackNum( stack );

		if (fd)
		{
			if ((to-start)>0)
			{
				fwrite( start, to-start,1, fd );
			}
			fclose(fd);
		}
	}

	getchar();

	popStack( stack - data->stack );
	return NULL;
}

bool __ReserveAs( int type, int bank, int length, char *name, char *mem )
{
	if ((bank>0)&&(bank<16))
	{
		kittyBanks[bank-1].length = length;
		if (kittyBanks[bank-1].start) free( kittyBanks[bank-1].start );

		printf("ok\n");

		if (mem)
		{
			kittyBanks[bank-1].start = mem;
		}
		else
		{
			kittyBanks[bank-1].start = malloc( kittyBanks[bank-1].length );
		}

		kittyBanks[bank-1].type = type;
		return true;
	}
	return false;
}


char *_cmdReserveAsWork( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	bool success = false;

	if (args==2)
	{
		success = __ReserveAs( 9, getStackNum(stack-1) , getStackNum(stack), NULL, NULL );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsChipWork( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	bool success = false;

	if (args==2)
	{
		success = __ReserveAs( 7, getStackNum(stack-1) , getStackNum(stack), NULL, NULL );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsData( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	bool success = false;

	if (args==2)
	{
		success = __ReserveAs( 10, getStackNum(stack-1) , getStackNum(stack), NULL, NULL );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsChipData( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	bool success = false;

	if (args==2)
	{
		success = __ReserveAs( 8, getStackNum(stack-1) , getStackNum(stack), NULL, NULL );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *cmdReserveAsWork(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdReserveAsWork, tokenBuffer );
	return tokenBuffer;
}

char *cmdReserveAsChipWork(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdReserveAsChipWork, tokenBuffer );
	return tokenBuffer;
}

char *cmdReserveAsData(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdReserveAsData, tokenBuffer );
	return tokenBuffer;
}

char *cmdReserveAsChipData(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdReserveAsChipData, tokenBuffer );
	return tokenBuffer;
}


const char *bankTypes[] = {
	"Chip work",		// 0
	"Fast work",		// 1
	"Sprites",			// 2
	"Icons",			// 3
	"Music",			// 4
	"Amal",			// 5
	"Samples",		// 6
	"Menu",			// 7
	"Chip Data",		// 8
	"Fast Data",		// 9
	"Code"
};


char *cmdListBank(nativeCommand *cmd, char *tokenBuffer)
{
	int n = 0;
	char txt[1000];
	struct retroScreen *screen;

	screen = screens[current_screen];

	if (screen)
	{
		clear_cursor( screen );
		_my_print_text( screen, (char *) "Nr   Type       Start       Length\n\n", 0);

		for (n=0;n<15;n++)
		{
			if (kittyBanks[n].start)
			{
				sprintf(txt,"%2d - %-10s S:$%08X L:%d\n", 
					n+1,
					bankTypes[kittyBanks[n].type],
					kittyBanks[n].start, 
					kittyBanks[n].length);

				_my_print_text( screen, txt, 0 );
			}
		}
		_my_print_text( screen, (char *) "\n", 0 );
	}

	return tokenBuffer;
}

char *cmdErase(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdErase, tokenBuffer );
	return tokenBuffer;
}

char *cmdStart(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _cmdStart, tokenBuffer );
	return tokenBuffer;
}

char *cmdLength(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _cmdLength, tokenBuffer );
	return tokenBuffer;
}

char *cmdBload(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdBload, tokenBuffer );
	return tokenBuffer;
}

char *cmdBsave(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdBsave, tokenBuffer );
	return tokenBuffer;
}

char *_cmdSave( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int n;
	int args = stack - data->stack +1 ;
	FILE *fd;
	char *start, *to;

	dump_stack();

	if (args==3)
	{
		fd = fopen( getStackString( stack - 2 ) , "w");

		if (fd)
		{
			fclose(fd);
		}
	}

	getchar();

	popStack( stack - data->stack );
	return NULL;
}

char *cmdSave(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdSave, tokenBuffer );
	return tokenBuffer;
}

