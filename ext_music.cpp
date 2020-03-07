
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
#include "amosKittens.h"
#include "stack.h"
#include "commands.h"
#include "commandsBanks.h"
#include "commandsBlitterObject.h"
#include "KittyErrors.h"
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

bool write_file_start_end( int channel, char *start, char *end );

extern bool audio_3k3_lowpass;

extern LONG volume;

int sample_bank = 5;

extern bool sample_loop;

void make_wave_test()
{
	int bytes = 30;
	unsigned int n;
	struct wave *newWave = allocWave(999, bytes);

	newWave->sample.bytes = bytes;

	if (newWave)
	{
		for (n = 0; n<newWave->sample.bytes; n++)
		{
			(&newWave->sample.ptr)[n] = (uint8_t)(sin((float)n * M_PI * 2 / (float)newWave->sample.bytes) * 126 + 127);
		}

		newWave->sample.frequency = newWave->sample.bytes;	// sample rate

		waves.push_back(newWave);
	}
}



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

#define harmonic(h,n,m) h * ((double) n) * 2.0f * M_PI / ((double) m) ;


void make_wave_bell()
{
	int n;
	double r1,r3,r5;
	int bytes = 256;
	struct wave *newWave = allocWave( 1, bytes );
	char *data;

	r1 = 0x0f;
	r3 = M_PI ;
	r5 = 0.0f;

	if (newWave)
	{
		data = ( char *) &(newWave -> sample.ptr);

		for (n=0;n<bytes;n++)
		{

//			data[n] =  (signed char) ( (sin( r1 ) + sin( r3 ) ) /2.0 * 127.0)  ;
//			data[n] =  (signed char) ( (sin( r1 ) + sin( r3 ) + sin( r5)) /3.0 * 127.0)  ;
			r1=harmonic(1,n,bytes);
//			r3=harmonic(3,n,bytes) + M_PI;
//			r5=harmonic(5,n,bytes);

			data[n] =  (signed char) ( sin( r1 ) * 127.0)  +127;
		}

		newWave -> sample.bytes = bytes;
		newWave -> sample.frequency = bytes;

		waves.push_back(newWave);
	}
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

struct wave *getWave(int id)
{
	unsigned int n;

	for (n=0;n<waves.size();n++)
	{
		if (waves[n] -> id == id )
		{
			return waves[n] ;
		}
	}
	return NULL;
}


bool delWave( int id )
{
	unsigned int n;

	for (n=0;n<waves.size();n++)
	{
		if (waves[n] -> id == id )
		{
			free(waves[n]);
			waves.erase(waves.begin() + n );
			return true ;
		}
	}
	return false;
}

void setEnval(struct wave *wave, int phase, int duration, int volume)
{
	int n;
	int startDuration = 0;
	if ((wave)&&(phase>-1)&&(phase<7))
	{
		for (n = 0; n < phase; n++) startDuration += wave->envels[n].duration;

		wave -> envels[phase].volume = volume;
		wave -> envels[phase].startDuration = startDuration;
		wave -> envels[phase].duration = duration;
	}
}

#ifdef __amoskittens__


char *_ext_cmd_sam_play( struct glueCommands *data, int nextToken )
{
	int voices,sample;
	int args =__stack - data->stack +1;
	struct kittyBank *bank;
	uint32_t	*offset;
	struct sampleHeader *sam;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			sample = getStackNum(__stack )-1;
			voices = 0xF;
			break;
		case 2:	// Sam Play 	voices, sample
			voices = getStackNum(__stack-1 );		
			sample = getStackNum(__stack )-1;
			break;
		default:
			setError(22,data->tokenBuffer);
			popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
			return  NULL ;
	}

	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );

	bank = findBank( sample_bank );
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
	int args =__stack - data->stack +1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 4:
			voice = getStackNum(__stack-2 );
			start = (uint8_t *) getStackNum(__stack-2 );
			length = getStackNum(__stack-1 );
			frequency = getStackNum(__stack );
	
			printf("play sound form start: %08x, length %d, frequency %d\n",start,length,frequency);

			if (start)	play( start, length, voice, frequency );

			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
	setStackNum(ret);

	return  NULL ;
}

char *ext_cmd_sam_raw(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_sam_raw, tokenBuffer );
	return tokenBuffer;
}

char *_ext_cmd_sam_bank( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			sample_bank = getStackNum(__stack );
			break;

		default:
			setError(22,data->tokenBuffer);
	}

	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
	return  NULL ;
}

char *ext_cmd_sam_bank(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_sam_bank, tokenBuffer );
	return tokenBuffer;
}


void copy_sample_to_playback_voices(struct sampleHeader *sam, int voices)
{
	int n;
	int size;

	printf("sam -> bytes: %d\n", sam -> bytes);

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
	int args =__stack - data->stack +1;
	struct kittyBank *bank;
	struct sampleHeader *sam;
	uint32_t	*offset;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			sample = getStackNum(__stack-1 );
			voices = getStackNum(__stack );

			printf("%d,%d\n",sample, voices);

			bank = findBank( sample_bank );
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

	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );

	return  NULL ;
}

char *ext_cmd_sample(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_sample, tokenBuffer );
	return tokenBuffer;
}

double noteFreq[12] = {
		261.63,
		277.18,
		293.66,
		311.13,
		329.63,
		349.23,
		369.99,
		392.0,
		415.3,
		440.0,
		466.16,
		493.88
	};

char *_ext_cmd_play( struct glueCommands *data, int nextToken )
{
	int pitch,delay;
	int note,octav;

#ifdef show_wave
	int s;
	int x;
	int pixelWidth;
#endif

	int y;
	double freq;
	double r;
	int byteOffset;
	int secOfData;
	int totData;

	double totNumberOfFreq;
	uint8_t	*wavedata;

	struct wave *localwave;

	int args =__stack - data->stack +1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);


#if show_wave
	open_debug_window();
#endif

	switch (args)
	{
		case 2:
			pitch = getStackNum(__stack-1 );
			delay = getStackNum(__stack );

			octav = (pitch-1) / 12;
			note = (pitch-1) % 12;
			freq = noteFreq[ note ] * (double) (1L << octav) / 4;

//			printf( "pitch: %d note: %d octav: %d freq %f\n", pitch, note, octav , freq );

			secOfData = 44800;
			totData = delay * secOfData;
			totNumberOfFreq = freq * delay ;

#if show_wave

			pixelWidth = 700;
			for (s=0;s<delay;s++)
			{
				byteOffset = secOfData * s;
				x = pixelWidth * byteOffset / totData;
				draw_hline( x );
			}
#endif
			localwave = allocWave( 0, totData );
			if (localwave)
			{
				localwave -> bytesPerSecond = secOfData;

				wavedata = &(localwave -> sample.ptr);

				for (byteOffset=0;byteOffset<totData;byteOffset++)
				{
					r =  (double) byteOffset * (2.0 * M_PI * freq) / (double) secOfData;
					y= sin(r)*64.0f;

					wavedata[ byteOffset ] = y;
#ifdef show_wave
					x = pixelWidth * byteOffset / totData;
					WritePixelColor( debug_Window -> RPort, 50+x, 400+y, 0xFFFFFFFF); 
#endif
				}

				setEnval( localwave, 0, 1, 63 );
				setEnval( localwave, 1, 1, 63 );
				setEnval( localwave, 2, 1, 63 );
				setEnval( localwave, 3, 1, 63 );
				setEnval( localwave, 4, 1, 63 );
				setEnval( localwave, 5, 1, 63 );
				setEnval( localwave, 6, 1, 63 );

				localwave->sample.frequency = localwave -> bytesPerSecond;
				localwave -> sample.bytes = totData;

				audio_device_flush(0x1);
				play_wave( localwave, localwave -> sample.bytes, 0x1 );

				free( localwave );
			}


			Delay( delay  );

			break;
		default:
			setError(22,data->tokenBuffer);
	}

#if show_wave
	close_debug_window();
#endif

	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );

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
	struct wave *localwave;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	audio_device_flush(0xF);
	getchar();

	wave = getWave(0);
	if (wave)
	{
		localwave = allocWave( wave -> id,  wave -> sample.bytes );

		if (localwave)
		{
			int len;

			*localwave = *wave;
			localwave -> bytesPerSecond = 50;

			memcpy( &(localwave->sample.ptr), &(wave -> sample.ptr), wave -> sample.bytes);

			setEnval( localwave, 0, 1, 63 );
			setEnval( localwave, 1, 1, 63 );
			setEnval( localwave, 2, 1, 63 );
			setEnval( localwave, 3, 1, 63 );
			setEnval( localwave, 4, 1, 63 );
			setEnval( localwave, 5, 1, 63 );
			setEnval( localwave, 6, 1, 63 );

			localwave->sample = wave->sample;
			localwave->sample.frequency = localwave -> bytesPerSecond;
			len = localwave -> bytesPerSecond * localwave -> envels[6].startDuration;

			play_wave( localwave, localwave -> sample.bytes, 0xF );

			free( localwave );
		}
	}
	else setError(22,tokenBuffer);

	return tokenBuffer;
}

char *ext_cmd_bell(nativeCommand *cmd, char *tokenBuffer)
{
	struct wave *wave;
	struct wave *localwave;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	audio_device_flush(0xF);
	getchar();

	wave = getWave(1);
	if (wave)
	{
		localwave = allocWave( wave -> id,  wave -> sample.bytes );

		if (localwave)
		{
			int len;

			*localwave = *wave;
			localwave -> bytesPerSecond = wave -> sample.bytes ;

			memcpy( &(localwave->sample.ptr), &(wave -> sample.ptr), wave -> sample.bytes);

			setEnval( localwave, 0, 1, 0 );
			setEnval( localwave, 1, 1, 63 );
			setEnval( localwave, 2, 1, 50);
			setEnval( localwave, 3, 1, 63 );
			setEnval( localwave, 4, 1, 50 );
			setEnval( localwave, 5, 1, 25 );
			setEnval( localwave, 6, 1, 0 );

			localwave->sample = wave->sample;
			localwave->sample.frequency = localwave -> bytesPerSecond;
			len = localwave -> bytesPerSecond * localwave -> envels[6].startDuration;

			play_wave( localwave, len, 0xF );

			debug_draw_wave( localwave );

			free( localwave );
		}
	}
	else setError(22,tokenBuffer);

	return tokenBuffer;
}

char *ext_cmd_shoot(nativeCommand *cmd, char *tokenBuffer)
{
	struct wave *wave;
	struct wave *localwave;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	audio_device_flush(0xF);
	getchar();

	wave = getWave(0);
	if (wave)
	{
		localwave = allocWave( wave -> id,  wave -> sample.bytes );

		if (localwave)
		{
			int len;

			*localwave = *wave;
			localwave -> bytesPerSecond = wave -> sample.bytes *4;

			memcpy( &(localwave->sample.ptr), &(wave -> sample.ptr), wave -> sample.bytes);

			setEnval( localwave, 0, 1, 15 );
			setEnval( localwave, 1, 1, 63 );
			setEnval( localwave, 2, 1, 40);
			setEnval( localwave, 3, 1, 40 );
			setEnval( localwave, 4, 1, 30 );
			setEnval( localwave, 5, 1, 15 );
			setEnval( localwave, 6, 1, 0 );

			localwave->sample = wave->sample;
			localwave->sample.frequency = localwave -> bytesPerSecond;
			len = localwave -> bytesPerSecond * localwave -> envels[6].startDuration;

			play_wave( localwave, len, 1 );

			free( localwave );
		}
	}
	else setError(22,tokenBuffer);

	return tokenBuffer;
}

bool apply_wave(int waveId, int voices)
{
	struct wave *wave = getWave(waveId);
	if (wave)
	{
		copy_sample_to_playback_voices( &(wave -> sample),voices);
		return true;
	}
	return false;
}

char *_ext_cmd_wave( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1;
	int waveId,voices;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("args %d\n",args);

	switch (args)
	{
		case 2:
			waveId = getStackNum(__stack-1  );
			voices = getStackNum(__stack );

			if ( apply_wave( waveId,  voices)  == false)
			{
				setError(178,data->tokenBuffer);
			}
			break;
		default:
			setError(22,data->tokenBuffer);
			popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
			return  NULL ;
			break;
	}

	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );

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
	int args =__stack - data->stack +1;
	unsigned int waveId;
	struct stringData *waveStr;
	struct wave *newWave;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			waveId = getStackNum(__stack-1  );
			waveStr = getStackString(__stack );

			if (waveStr)
			{
				printf("string length %d\n",waveStr->size);

				delWave(waveId);

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
			popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
			return  NULL ;
			break;
	}

	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
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
	int args =__stack - data->stack +1;
	int waveId,phase, duration, volume;
	struct wave *wave;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 4:
			waveId = getStackNum(__stack -3  );
			phase = getStackNum(__stack -2 );
			duration = getStackNum(__stack -1 );
			volume = getStackNum(__stack );

			wave = getWave(waveId);
			setEnval( wave, phase, duration, volume );
			break;
		default:
			setError(22,data->tokenBuffer);
			popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
			return  NULL ;
			break;
	}

	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
	return  NULL ;
}

char *ext_cmd_set_envel(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _ext_cmd_set_envel, tokenBuffer );
	return tokenBuffer;
}

char *_ext_cmd_del_wave( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1;
	int waveId;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			waveId = getStackNum(__stack  );
			if (delWave(waveId)==false)
			{
				setError(22,data->tokenBuffer);
			}
			return  NULL ;
			break;

	}

	setError(22,data->tokenBuffer);
	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
	return  NULL ;
}

char *ext_cmd_del_wave(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_del_wave, tokenBuffer );
	return tokenBuffer;
}

char *_ext_cmd_volume( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			volume = (LONG) (( uint32_t) (0x10000 * getStackNum(__stack )) / 63);
			return  NULL ;
			break;
	}

	setError(22,data->tokenBuffer);
	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
	return  NULL ;
}


char *ext_cmd_volume(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_volume, tokenBuffer );
	return tokenBuffer;
}


char *ext_cmd_sam_loop_on(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	sample_loop = true;

	return tokenBuffer;
}

char *ext_cmd_sam_loop_off(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	sample_loop = false;

	return tokenBuffer;
}

char *_ext_cmd_sam_stop( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1;
	int voices;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			voices = getStackNum(__stack );
			audio_device_flush(voices);
			return NULL;
			break;
	}

	setError(22,data->tokenBuffer);
	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
	return  NULL ;
}

char *ext_cmd_sam_stop(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_sam_stop, tokenBuffer );
	setStackNum(15);		// set default value.
	return tokenBuffer;
}

char *_ext_cmd_sam_ssave( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1;
	int channel,start,end;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:
			channel = getStackNum(__stack-2 );
			start = getStackNum(__stack-1 );
			end = getStackNum(__stack );

			write_file_start_end( channel, (char *) start, (char *) end );

			getchar();
			break;

		default:
			setError(22,data->tokenBuffer);
	}

	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
	return  NULL ;
}

char *ext_cmd_ssave(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_sam_ssave, tokenBuffer );
	return tokenBuffer;
}

char *_ext_cmd_sam_sload( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1;
	int voices;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			voices = getStackNum(__stack );
			audio_device_flush(voices);
			return NULL;
			break;
	}

	setError(22,data->tokenBuffer);
	popStack(__stack - cmdTmp[instance.cmdStack-1].stack  );
	return  NULL ;
}

char *ext_cmd_sload(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _ext_cmd_sam_sload, tokenBuffer );
	return tokenBuffer;
}

char *ext_cmd_sam_swap(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *ext_cmd_sam_swapped(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackNum(0);
	return tokenBuffer;
}

char *ext_cmd_led_on(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	audio_3k3_lowpass = true;
	return tokenBuffer;
}

char *ext_cmd_led_off(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	audio_3k3_lowpass = false;
	return tokenBuffer;
}

#endif

