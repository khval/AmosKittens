
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>

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



struct sampleHeader *voicePlay[4] = {NULL,NULL,NULL,NULL};

std::vector<struct sampleHeader *> waves;

char *_ext_cmd_sam_play( struct glueCommands *data, int nextToken )
{
	int ret = 0,voices,sample;
	int args = stack - data->stack +1;
	struct kittyBank *bank;
	uint32_t	*offset;
	struct sampleHeader *sam;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			sample = getStackNum( stack-1 )-1;
			voices = getStackNum( stack );
	
			bank = findBank( 5 );
			if (bank)
			{
				uint16_t samples = *((uint16_t *) bank -> start);

				if ((sample>=0) && (sample < samples))
				{
					offset = ((uint32_t *) (bank -> start + sizeof( uint16_t )));
					sam = (struct sampleHeader *) ( (uint8_t *) bank -> start + offset[ sample ] );
					play( &sam -> ptr, sam -> bytes, voices, sam -> frequency );
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
	int ret = 0,voice,length, frequency;
	uint8_t *start;
	int args = stack - data->stack +1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 4:
			voice = getStackNum( stack-2 );
			start = (uint8_t *) getStackNum( stack-2 );
			length = getStackNum( stack-1 );
			frequency = getStackNum( stack );
	
			printf("play sound form start: %08x, length %d, frequency %d\n",start,length,frequency);

			if (start)	play( start, length, voice, frequency );

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


void copy_sample_to_playback_voices(struct sampleHeader *sam, int voices)
{
	int n;
	int size;

	for (n=0;n<4;n++)
	{
		if (voices & (1<<n))
		{
			if (voicePlay[n]) free(voicePlay[n]);
			size =  sizeof(struct sampleHeader) + sam -> bytes -1;
			voicePlay[n]=(struct sampleHeader *) malloc( size );
			if (voicePlay[n])	memcpy( voicePlay[n] , sam, size );
		}
	}
}

char *_ext_cmd_sample( struct glueCommands *data, int nextToken )
{
	int sample,voices;
	int args = stack - data->stack +1;
	struct kittyBank *bank;
	struct sampleHeader *sam;
	uint32_t	*offset;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			sample = getStackNum( stack-1 );
			voices = getStackNum( stack );

			printf("%d,%d\n",sample, voices);

			bank = findBank( 5 );
			if (bank)
			{
				uint16_t samples = *((uint16_t *) bank -> start);

				if ((sample>=0) && (sample < samples))
				{
					offset = ((uint32_t *) (bank -> start + sizeof( uint16_t )));
					sam = (struct sampleHeader *) ( (uint8_t *) bank -> start + offset[ sample ] );
					copy_sample_to_playback_voices(sam,voices);
				}
				else setError(22,data->tokenBuffer);
			}

			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );

	return  NULL ;
}

char *ext_cmd_sample(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_sample, tokenBuffer );
	return tokenBuffer;
}

char *_ext_cmd_play( struct glueCommands *data, int nextToken )
{
	int pitch,delay;
	int n;

	int args = stack - data->stack +1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			pitch = getStackNum( stack-1 );
			delay = getStackNum( stack );

			for (n=0;n<4;n++)
			{
				if (voicePlay[n])
				{
					play( &voicePlay[n] -> ptr , voicePlay[n] -> bytes, (1L << n), voicePlay[n] -> frequency + (( pitch - 50 ) * 2) );
				}
			}

			Delay( delay / 2 );

			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	return  NULL ;
}


char *ext_cmd_play(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _ext_cmd_play, tokenBuffer );
	return tokenBuffer;
}

char *_ext_cmd_boom( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1;
	unsigned int wave;
	struct sampleHeader *sam;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			wave = 0;
			if ((wave>=0) && (wave < waves.size()))
			{
				sam = waves[wave];
				if (sam)
				{
					play( &sam -> ptr, sam -> bytes, 0xF, sam -> frequency );
				}
			}
			else setError(22,data->tokenBuffer);


			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );

	return  NULL ;
}

char *ext_cmd_boom(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _ext_cmd_boom, tokenBuffer );
	return tokenBuffer;
}

char *_ext_cmd_bell( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1;
	unsigned int wave;
	struct sampleHeader *sam;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			wave = 1;
			if ((wave>=0) && (wave < waves.size()))
			{
				sam = waves[wave];
				if (sam)
				{
					play( &sam -> ptr, sam -> bytes, 0xF, sam -> frequency );
				}
			}
			else setError(22,data->tokenBuffer);


			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );

	return  NULL ;
}

char *ext_cmd_bell(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _ext_cmd_bell, tokenBuffer );
	return tokenBuffer;
}

char *_ext_cmd_wave( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1;
	int voices;
	unsigned int wave;
	struct sampleHeader *sam = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("args %d\n",args);

	switch (args)
	{
		case 2:
			wave = getStackNum( stack-1  )-1;
			voices = getStackNum( stack );

			if ((wave>=0) && (wave < waves.size()))
			{
				sam = waves[wave];
				if (sam)
				{
					copy_sample_to_playback_voices(sam,voices);
				}
			}
			break;
		default:
			setError(22,data->tokenBuffer);
			popStack( stack - cmdTmp[cmdStack-1].stack  );
			return  NULL ;
			break;
	}

	if (sam == NULL)
	{
		setError(178,data->tokenBuffer);
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );

	return  NULL ;
}

char *ext_cmd_wave(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _ext_cmd_wave, tokenBuffer );
	return tokenBuffer;
}

