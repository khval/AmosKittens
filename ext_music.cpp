
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
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

std::vector<struct wave *> waves;

struct wave *allocWave( int id, int size );
struct sampleHeader *allocSample( int size );

void make_wave_noice()
{
	int n;
	struct wave *newWave = allocWave( 0, 256 );

	if (newWave)
	{
		for (n=0;n<256;n++)
		{
			(&newWave -> sample.ptr)[n] = rand() % 256;
		}

		newWave -> sample.bytes = 256;
		newWave -> sample.frequency = 44800/8;

		waves.push_back(newWave);
	}
}


void make_wave_bell()
{
	int n;
	double r;
	double s;
	struct wave *newWave = allocWave( 1, 256 );

	r = 0.0f;
	s = 2* M_PI / 256.0f * 8.0f;
	if (newWave)
	{
		for (n=0;n<256;n++)
		{
			(&newWave -> sample.ptr)[n] = sin( r ) * 126;
			r+=s;
		}

		newWave -> sample.bytes = 256;
		newWave -> sample.frequency = 44800;

		waves.push_back(newWave);
	}
}


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

struct sampleHeader *allocSample( int size )
{
	size =  sizeof(struct sampleHeader) + size -1;
	return (struct sampleHeader *) malloc( size );
}

struct wave *allocWave( int id, int size )
{
	struct wave *data;

	size =  sizeof(struct wave) + size -1;
	data = (struct wave *) malloc( size );

	if (data)
	{
		data -> id = id;
	}

	return data;
}

struct wave *getWave( int id )
{
	unsigned int n;
	if (index<0) return NULL;

	for (n=0;n<waves.size();n++)
	{
		if (waves[n] -> id == id )
		{
			return waves[n] ;
		}
	}
	return NULL;
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
			voicePlay[n]=allocSample( sam -> bytes );

			if (voicePlay[n])
			{
				size =  sizeof(struct sampleHeader) + sam -> bytes -1;
				memcpy( voicePlay[n] , sam, size );
			}
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

char *ext_cmd_boom(nativeCommand *cmd, char *tokenBuffer)
{
	struct wave *wave;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	wave = getWave(0);
	if (wave)
	{
		play( &wave -> sample.ptr, wave -> sample.bytes, 0xF, wave -> sample.frequency );
	}
	else setError(22,tokenBuffer);

	return tokenBuffer;
}

char *ext_cmd_bell(nativeCommand *cmd, char *tokenBuffer)
{
	struct wave *wave;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	wave = getWave(1);
	if (wave)
	{
		play( &wave -> sample.ptr, wave -> sample.bytes, 0xF, wave -> sample.frequency );
	}
	else setError(22,tokenBuffer);

	return tokenBuffer;
}

char *ext_cmd_shoot(nativeCommand *cmd, char *tokenBuffer)
{
	struct wave *wave;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	wave = getWave(0);
	if (wave)
	{
		play( &wave -> sample.ptr, wave -> sample.bytes, 0xF, wave -> sample.frequency );
	}
	else setError(22,tokenBuffer);

	return tokenBuffer;
}

char *_ext_cmd_wave( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1;
	int voices;
	int waveId;
	struct wave *wave = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("args %d\n",args);

	switch (args)
	{
		case 2:
			waveId = getStackNum( stack-1  );
			voices = getStackNum( stack );

			wave = getWave(waveId);
			if (wave)
			{
				copy_sample_to_playback_voices( &(wave -> sample),voices);
			}

			break;
		default:
			setError(22,data->tokenBuffer);
			popStack( stack - cmdTmp[cmdStack-1].stack  );
			return  NULL ;
			break;
	}

	if (wave == NULL)
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

char *_ext_cmd_set_wave( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1;
	unsigned int waveId;
	struct stringData *waveStr;
	struct wave *newWave;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("args %d\n",args);

	switch (args)
	{
		case 2:
			waveId = getStackNum( stack-1  );
			waveStr = getStackString( stack );

			if (waveStr)
			{
				printf("string length %d\n",waveStr->size);

				newWave = allocWave( waveId, waveStr->size );
				if (newWave)
				{
					printf("yes we have a new wave\n");

					memcpy( &newWave -> sample.ptr, &waveStr -> ptr, waveStr->size );
					newWave -> sample.bytes = waveStr->size;
					newWave -> sample.frequency = 44800;

					waves.push_back(newWave);
				}
			}
			getchar();

			break;
		default:
			setError(22,data->tokenBuffer);
			popStack( stack - cmdTmp[cmdStack-1].stack  );
			return  NULL ;
			break;
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );

	return  NULL ;
}


char *ext_cmd_set_wave(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _ext_cmd_set_wave, tokenBuffer );
	return tokenBuffer;
}

char *_ext_cmd_set_envel( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1;
	unsigned int waveId, phase, duration, volume;
	struct wave *wave;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("args %d\n",args);

	switch (args)
	{
		case 4:
			waveId = getStackNum( stack -3  );
			phase = getStackNum( stack -2 );
			duration = getStackNum( stack -1 );
			volume = getStackNum( stack );

			wave = getWave(waveId);
			if ((wave)&&(phase>-1)&&(phase<7))
			{
				wave -> envels[phase].volume = volume;
				wave -> envels[phase].duration = duration;
			}
			break;
		default:
			setError(22,data->tokenBuffer);
			popStack( stack - cmdTmp[cmdStack-1].stack  );
			return  NULL ;
			break;
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );

	return  NULL ;
}


char *ext_cmd_set_envel(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _ext_cmd_set_envel, tokenBuffer );
	return tokenBuffer;
}



