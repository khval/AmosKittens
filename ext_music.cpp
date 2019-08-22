
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#endif

#include "debug.h"
#include <string>
#include <iostream>
#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsBanks.h"
#include "commandsBlitterObject.h"
#include "errors.h"
#include "engine.h"
#include "os/amigaos/ahi_dev.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern int current_screen;
extern struct retroSprite *sprite ;
extern struct retroSprite *icons ;

struct sampleHeader
{
	char		name[8];
	uint16_t	hertz;
	uint32_t	bytes;
	uint8_t	ptr;
} __attribute__((packed));


char *_ext_cmd_sam_play( struct glueCommands *data, int nextToken )
{
	int ret = 0,voice,sample;
	int args = stack - data->stack +1;
	struct kittyBank *bank;
	uint32_t	*offset;
	struct sampleHeader *sam;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	dump_stack();

	switch (args)
	{
		case 2:
			voice = getStackNum( stack-1 );
			sample = getStackNum( stack )-1;
	
			bank = findBank( 5 );
			if (bank)
			{
				uint16_t samples = *((uint16_t *) bank -> start);

				if ((sample>=0) && (sample < samples))
				{
					offset = ((uint32_t *) (bank -> start + sizeof( uint16_t )));

					sam = (struct sampleHeader *) ( (uint8_t *) bank -> start + offset[ sample ] );

					printf("%s\n",sam -> name);			
				}
				else setError(22,data->tokenBuffer);

			}

			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );
	setStackNum(ret);

	return  NULL ;
}

char *ext_cmd_sam_play(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_sam_play, tokenBuffer );
	return tokenBuffer;
}

char *_ext_cmd_sam_raw( struct glueCommands *data, int nextToken )
{
	int ret = 0,voice, freq;
	uint8_t *start,*end;
	int args = stack - data->stack +1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	dump_stack();

	switch (args)
	{
		case 1:
			break;

		case 4:
			voice = getStackNum( stack-2 );
			start = (uint8_t *) getStackNum( stack-2 );
			end = (uint8_t *) getStackNum( stack-1 );
			freq = getStackNum( stack );
	
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );
	setStackNum(ret);

	return  NULL ;
}

char *ext_cmd_sam_raw(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_sam_raw, tokenBuffer );
	return tokenBuffer;
}
