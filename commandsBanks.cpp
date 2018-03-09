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

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;


char *_cmdErase( struct glueCommands *data )
{
	int n;
	int args = stack - data->stack +1 ;

	if (args==2)
	{
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdStart( struct glueCommands *data )
{
	int n;
	int args = stack - data->stack +1 ;

	if (args==2)
	{
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdLength( struct glueCommands *data )
{
	int n;
	int args = stack - data->stack +1 ;

	if (args==2)
	{
	}

	popStack( stack - data->stack );
	return NULL;
}


char *_cmdBload( struct glueCommands *data )
{
	int n;
	int args = stack - data->stack +1 ;

	if (args==2)
	{
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdBsave( struct glueCommands *data )
{
	int n;
	int args = stack - data->stack +1 ;

	if (args==2)
	{
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsWork( struct glueCommands *data )
{
	int n;
	int args = stack - data->stack +1 ;

	if (args==2)
	{
		n = _stackInt(stack-1);

		if ((n>0)&&(n<16))
		{
			kittyBanks[n-1].length = _stackInt(stack);
			if (kittyBanks[n-1].start) free( kittyBanks[n-1].start );
			kittyBanks[n-1].start = malloc( kittyBanks[n-1].length );
			kittyBanks[n-1].type = 9;
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsChipWork( struct glueCommands *data )
{
	int n;
	int args = stack - data->stack +1 ;

	if (args==2)
	{
		n = _stackInt(stack-1);

		if ((n>0)&&(n<16))
		{
			kittyBanks[n-1].length = _stackInt(stack);
			if (kittyBanks[n-1].start) free( kittyBanks[n-1].start );
			kittyBanks[n-1].start = malloc( kittyBanks[n-1].length );
			kittyBanks[n-1].type = 7;
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsData( struct glueCommands *data )
{
	int n;
	int args = stack - data->stack +1 ;

	if (args==2)
	{
		n = _stackInt(stack-1);

		if ((n>0)&&(n<16))
		{
			kittyBanks[n-1].length = _stackInt(stack);
			if (kittyBanks[n-1].start) free( kittyBanks[n-1].start );
			kittyBanks[n-1].start = malloc( kittyBanks[n-1].length );
			kittyBanks[n-1].type = 10;
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsChipData( struct glueCommands *data )
{
	int n;
	int args = stack - data->stack +1 ;

	if (args==2)
	{
		n = _stackInt(stack-1);

		if ((n>0)&&(n<16))
		{
			kittyBanks[n-1].length = _stackInt(stack);
			if (kittyBanks[n-1].start) free( kittyBanks[n-1].start );
			kittyBanks[n-1].start = malloc( kittyBanks[n-1].length );
			kittyBanks[n-1].type = 8;
		}
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
	"NULL",
	"Sprites",
	"Icons",
	"Music",
	"Amal",
	"Samples",
	"Menu",
	"Chip Work",
	"Chip Data",
	"Fast work",
	"Fast Data"
};

char *cmdListBank(nativeCommand *cmd, char *tokenBuffer)
{
	int n = 0;

	printf("\nNumber  Type        Start          Length\n\n");

	for (n=0;n<15;n++)
	{
		if (kittyBanks[n].type)
		{
			printf("%2d    - %-10s  S:$%04X    L:$%04X\n", n+1,
				bankTypes[kittyBanks[n].type],
				kittyBanks[n].start, 
				kittyBanks[n].length);
		}
	}

	printf("\n");

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

