
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

bool audio_stopped = false;

extern bool running;

struct audioChunk
{
	char ptr[AHI_CHUNKSIZE];
	int size;
	int frequency;
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

int AHIDevice = -666;

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
	while ( audioBuffer.size() > 0 )
	{
		Printf("clearing buffer\n");
		free( audioBuffer[0] );
		audioBuffer.erase( audioBuffer.begin() );
	}

	if ( AHIDevice == 0 )
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
		Printf("Free AHIio\n");

		if (AHIio_orig -> io) FreeSysObject ( ASOT_IOREQUEST, (struct IORequest *) AHIio_orig -> io);
		free(AHIio_orig);
		AHIio_orig = NULL;
	}

	if (AHIio2_orig)
	{
		Printf("Free AHIio2\n");

		if (AHIio2_orig -> io) FreeVec(AHIio2_orig -> io);
		free(AHIio2_orig);
		AHIio2_orig = NULL;
	}

	if (AHIMsgPort) 
	{
		Printf("Free AHIMsgPort\n");

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
	static struct audioIO *tempRequest;
	extern struct AHIIFace	*IAHI ;		// instence.

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	Printf("AHIDevice is %ld\n", AHIDevice);

	if( AHIMsgPort = (struct MsgPort*) AllocSysObjectTags(ASOT_PORT, TAG_END) )
	{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

		io = (struct AHIRequest *) AllocSysObjectTags ( 
					ASOT_IOREQUEST, 
					ASOIOR_ReplyPort, AHIMsgPort, 
					ASOIOR_Size, sizeof(struct AHIRequest), 
					TAG_END );

		if (io)
		{
			 if ( AHIio = new_audio( io ) ) 
			{
				Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
				AHIio->io->ahir_Version = 4;
				AHIDevice = OpenDevice(AHINAME, AHI_DEFAULTUNIT,(struct IORequest *) AHIio -> io, 0);

				if (AHIDevice == 0)
				{
					Printf("%s:%s:%ld -- success \n",__FILE__,__FUNCTION__,__LINE__);

					AHIBase = (struct Device *)AHIio->io->ahir_Std.io_Device;
					IAHI = (struct AHIIFace*) GetInterface( (struct Library *) AHIBase,"main",1L,NULL) ;
				}
				else
				{
					Printf("Open device error code %ld\n", AHIDevice);
					goto end_audioTask;
				}
			}
			else 	goto end_audioTask;
		}
		else
		{
			FreeSysObject( ASOT_IOREQUEST, io );
			goto end_audioTask;
		}
	}
	else
	{
		Printf("Failed to create port %ld\n", AHIDevice);
		goto end_audioTask;
	}

	AHIio2 = new_audio((struct AHIRequest*) AllocVecTags(sizeof(struct AHIRequest), AVT_Type, MEMF_SHARED, TAG_END )) ;

	if(! AHIio2)
	{
		goto end_audioTask;
	}
	else
	{
		Printf("Cool we got a copy..\n");
		memcpy(AHIio2 -> io, AHIio -> io, sizeof(struct AHIRequest));
	}

	AHIio_orig = AHIio;
	AHIio2_orig= AHIio2;

	Printf("audio ready, waiting for input ;-)\n");

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
			if (AHIio-> data)	free( AHIio-> data );	// free old data.
			AHIio-> data = audioBuffer[0];
			audioBuffer.erase( audioBuffer.begin());
			audio_unlock();

			Printf("We have data\n");

			Printf("AHIio %08lx, (io: %08lx,  frequency: %ld)\n", AHIio, AHIio -> io, AHIio->data -> frequency);

			if (io = AHIio -> io)
			{
				Printf("Volume: %ld\n",AHI_Volume);

				io->ahir_Std.io_Message.mn_Node.ln_Pri = 0;
				io->ahir_Std.io_Command  = CMD_WRITE;
				io->ahir_Std.io_Offset   = 0;
				io->ahir_Std.io_Data     =  AHIio-> data -> ptr; 
				io->ahir_Std.io_Length   = (ULONG) AHIio-> data -> size; 
				io->ahir_Frequency       = (ULONG) AHIio-> data ->frequency;
				io->ahir_Volume          = AHI_Volume; 
				io->ahir_Position        = 0x8000;           // Centered
				io->ahir_Type = AHIST_M8S;
				io->ahir_Link = link ? link -> io : NULL;
			}

			Printf("io->ahir_Std.io_Data %08lx\n",io->ahir_Std.io_Data);
		}
		else 	audio_unlock();

		Printf("SendIO( AHIio %08lx (io: %08lx) )\n", AHIio, AHIio -> io );

		SendIO( (struct IORequest *) AHIio -> io );

		if (link)
		{
			Printf("wait for link %08lx (io: %08lx, len %ld) \n", link, link -> io, link -> io ? link -> io -> ahir_Std.io_Length : 0 );

			WaitIO ( (struct IORequest *) link -> io );

			Printf("check if main is done... \n");

			if (CheckIO( (struct IORequest *) AHIio -> io ))
			{
				// Playback caught up with us, rebuffer... 
				WaitIO( (struct IORequest *) AHIio -> io );
				link = NULL;
				continue;
			}
		}

		Printf("swap the order of buffers\n");

		Printf("%08lx, %08lx ( link is %08lx)\n", AHIio, AHIio2, link);

		// Swap requests
		tempRequest = AHIio;
		AHIio  = AHIio2;
		AHIio2 = tempRequest;
		link = AHIio2;

		Printf("after swap\n");

		Printf("%08lx, %08lx ( link is %08lx)\n",  AHIio, AHIio2, link);


	} 

end_audioTask:

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	audio_lock();
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	cleanup_task();
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	Signal( (struct Task *) main_task, SIGF_CHILD );
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	audio_unlock();

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
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

bool play(uint8_t * data,int len, int frequency)
{
	int blocks = len / AHI_CHUNKSIZE;
	int lastLen = len % AHI_CHUNKSIZE;
	struct audioChunk *chunk;
	int offset = 0;

	if ((frequency <= 0) || (frequency > (44800*4)))
	{
		printf("bad frequency\n"); getchar();
		return false;	// avoid getting stuck...
	}


	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	while (blocks--)
	{
		printf("offset at %d, max %d\n", offset, len);

		printf("\nallocated\n");
		chunk = (struct audioChunk *) malloc(sizeof(struct audioChunk));

		if (chunk)
		{
			printf("memcpy\n");

			chunk -> size = AHI_CHUNKSIZE;
			chunk -> frequency = frequency;
			memcpy( chunk -> ptr, (void *) (data + offset),  chunk -> size );

			printf("add to buffer, (wait, if table is locked)\n");

			audio_lock();
			audioBuffer.push_back( chunk );
			audio_unlock();
			offset += chunk -> size;

			printf("its added \n");
		}
	}

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if (lastLen)
	{
		printf("offset at %d, max %d\n", offset, len);

		printf("\nallocated\n");
		chunk = (struct audioChunk *) malloc(sizeof(struct audioChunk));

		if (chunk)
		{
			printf("memcpy\n");

			chunk -> size = lastLen;
			chunk -> frequency = frequency;
			memcpy( chunk -> ptr, (data + offset),  chunk -> size );

			printf("add to buffe, (wait, if table is locked)r\n");

			audio_lock();
			audioBuffer.push_back( chunk );
			audio_unlock();

			printf("its added\n");

			offset += chunk -> size;
		}
	}

	return true;
}

#define debug_lock

#ifdef debug_lock
int audio_locked = 0;
#endif

void audio_lock()
{
#ifdef debug_lock
	if (audio_locked != 0)  Printf("***** audio is already locked *****\n");
#endif
	MutexObtain(audio_mx);
	audio_locked ++;
}

void audio_unlock()
{
#ifdef debug_lock
	if (audio_locked == 0)  
	{
		Printf("***** audio is not locked, nothing to unlock *****\n");
	}
	else if (audio_locked > 0)  
	{
		Printf("**** unlocked with success ****\n");
	}
	else
	{
		Printf("**** unlock this is none sense  ****\n");
	}
#endif 
	audio_locked--;
	MutexRelease(audio_mx);
}

