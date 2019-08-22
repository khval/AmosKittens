
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/ahi.h>

#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <proto/ahi.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <dos/dostags.h>
#include <dos/dos.h>

#include "ahi_dev.h"
#include "spawn.h"
#include "debug.h"

//some define for this nice AHI driver !
#define AHI_CHUNKSIZE         (65536/2)
#define AHI_CHUNKMAX          (131072/2)
#define AHI_DEFAULTUNIT       0

int samplerate;

bool audio_stopped = false;

extern bool running;

struct audioChunk
{
	int size;
	char ptr[AHI_CHUNKSIZE];
};

struct audioIO
{
	struct AHIRequest *io;
	struct audioChunk *data;
};

struct audioIO *new_audio( struct AHIRequest *io)
{
	struct audioIO *_new = (struct audioIO *) malloc( sizeof( struct audioIO *) );
	if (_new)
	{
		_new -> io = io;
		_new -> data = NULL;
	}
	return _new;
}

std::vector<struct audioChunk *> audioBuffer;

BOOL AHIDevice;

extern struct AHIIFace		*IAHI ;

 struct MsgPort *     AHIMsgPort  = NULL;     // The msg port we will use for the ahi.device
 struct audioIO *  AHIio       = NULL;     // First AHIRequest structure
 struct audioIO *  AHIio2      = NULL;     // second one. Double buffering !
 struct audioIO *  AHIio_orig       = NULL;     // First AHIRequest structure
 struct audioIO *  AHIio2_orig      = NULL;     // second one. Double buffering !
 struct audioIO *link=NULL;

static ULONG AHIType;
static LONG AHI_Volume=0x10000;

static struct Process *main_task = NULL;
static struct Process *audioTask = NULL;         

#define audioTask_NAME       "Amos Kittens Audio Thread"
#define audioTask_PRIORITY   2

int ahi_dev_init(int rate,int channels,int format,int flags);
void ahi_dev_uninit(int immed);

static void cleanup_task(void)
{

	audio_lock();
	while ( audioBuffer.size() > 0 )
	{
		free( audioBuffer[0] );
		audioBuffer.erase( audioBuffer.begin() );
	}
	audio_unlock();


	if (! AHIDevice )
	{
		if (link) if (link -> io)
		{
			AbortIO( (struct IORequest *) link -> io);
			WaitIO( (struct IORequest *) link -> io);
		}

		if (AHIio_orig) if (AHIio_orig -> io)
		{
			CloseDevice ( (struct IORequest *) AHIio_orig -> io);
		}
		if ( IAHI ) DropInterface((struct Interface*)IAHI); IAHI = 0;
	}

   	if (AHIio_orig)
	{
		if (AHIio_orig -> io) DeleteIORequest( (struct IORequest *) AHIio_orig -> io);
		free(AHIio_orig);
		AHIio_orig = NULL;
	}

	if (AHIio2_orig)
	{
		if (AHIio2_orig -> io) FreeVec(AHIio2_orig -> io);
		free(AHIio2_orig);
		AHIio2_orig = NULL;
	}

	if (AHIMsgPort) 
	{
		FreeSysObject( ASOT_PORT, AHIMsgPort);
		AHIMsgPort = NULL;
	}

	link = NULL;
	AHIio = NULL;
	AHIio2 = NULL;
	AHIDevice = -1;       // -1 -> Not opened !
}


void audio_engine (void) {

	static struct AHIRequest *io;

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

static struct audioIO *tempRequest;

	if(  AHIMsgPort = (struct MsgPort*) AllocSysObjectTags(ASOT_PORT, TAG_DONE) )
	{
		if ( (AHIio = new_audio( (struct AHIRequest *)CreateIORequest(AHIMsgPort,sizeof(struct AHIRequest)))))
		{
			 AHIio->io->ahir_Version = 4;
			 AHIDevice = OpenDevice(AHINAME, AHI_DEFAULTUNIT,(struct IORequest *) AHIio -> io, 0);

#ifdef __amigaos4__
			AHIBase = (struct Device *)AHIio->io->ahir_Std.io_Device;
			IAHI = (struct AHIIFace*) GetInterface( (struct Library *) AHIBase,"main",1L,NULL) ;
#endif
		}
	}

	if(AHIDevice) goto end_audioTask;

	AHIio2 = new_audio((struct AHIRequest*) AllocVecTags(sizeof(struct AHIRequest), AVT_Type, MEMF_SHARED, TAG_END )) ;

	if(! AHIio2)
	{
		goto end_audioTask;
	}

	AHIio_orig = AHIio;
	AHIio2_orig= AHIio2;

	if (AHIio2) 
	{
		memcpy(AHIio2 -> io, AHIio -> io, sizeof(struct AHIRequest));
	}

	Signal( (struct Task *) main_task, SIGF_CHILD );

	for(;;)
	{
		if ( running == false ) break;

		audio_lock();

		if ( audioBuffer.size() == 0 )
		{
			audio_unlock(); 
			Delay(1);
			continue; 
		}    

		if ( audioBuffer[0] -> size > 0)
		{
			io = AHIio -> io;
			AHIio-> data = audioBuffer[0];


			if (io)
			{
				io->ahir_Std.io_Message.mn_Node.ln_Pri = 0;
				io->ahir_Std.io_Command  = CMD_WRITE;
				io->ahir_Std.io_Offset   = 0;
				io->ahir_Frequency       = (ULONG) samplerate;
				io->ahir_Volume          = AHI_Volume; 
				io->ahir_Position        = 0x8000;           // Centered
				io->ahir_Std.io_Data     =  AHIio-> data -> ptr; 
				io->ahir_Std.io_Length   = (ULONG) audioBuffer[0] -> size; 
				io->ahir_Type = AHIType;
				io->ahir_Link = link ? link -> io : NULL;
			}

		}

		audio_unlock();

		SendIO( (struct IORequest *) AHIio -> io );

		if (link)
		{
			WaitIO ( (struct IORequest *) link -> io );

			if (CheckIO( (struct IORequest *) AHIio -> io ))
			{
				// Playback caught up with us, rebuffer... 
				WaitIO( (struct IORequest *) AHIio -> io );
				link = NULL;
				continue;
			}
		}

		link = AHIio;
		// Swap requests
		tempRequest = AHIio;
		AHIio  = AHIio2;
		AHIio2 = tempRequest;
	} 

end_audioTask:

	audio_lock();
	cleanup_task();
	Signal( (struct Task *) main_task, SIGF_CHILD );
	audio_unlock();
}

bool audio_start(int rate,int channels)
{
#ifdef enable_audio_debug_output_yes
	BPTR audio_debug_output = Open("CON:660/50/600/480/Kitty audio output",MODE_NEWFILE);
#else
	BPTR audio_debug_output = NULL;
#endif

	AHIType = channels > 1 ? AHIST_S32S : AHIST_M32S;
	main_task = (struct Process *) FindTask(NULL);
	audioTask = spawn( audio_engine, "Amos audio engine",audio_debug_output);

	if (audioTask)
	{
		Wait(SIGF_CHILD);
		return (audio_stopped == false);
	}

	return false;
}

void play(uint8_t * data,int len)
{
	int blocks = len / AHI_CHUNKSIZE;
	int lastLen = len % AHI_CHUNKSIZE;
	struct audioChunk *chunk;
	int offset = 0;

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	while (blocks--)
	{
		chunk = (struct audioChunk *) malloc(sizeof(struct audioChunk));

		if (chunk)
		{
			chunk -> size = AHI_CHUNKSIZE;
			memcpy( chunk -> ptr, (void *) (data + offset),  chunk -> size );

			audio_lock();
			audioBuffer.push_back( chunk );
			audio_unlock();
			offset += chunk -> size;
		}
	}

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if (lastLen)
	{
		chunk = (struct audioChunk *) malloc(sizeof(struct audioChunk));

		if (chunk)
		{
			chunk -> size = lastLen;
			memcpy( chunk -> ptr, (data + offset),  chunk -> size );

			audio_lock();
			audioBuffer.push_back( chunk );
			audio_unlock();
			offset += chunk -> size;
		}
	}
}

void audio_lock()
{
	MutexObtain(audio_mx);
}

void audio_unlock()
{
	MutexRelease(audio_mx);
}



