
#ifdef __amoskittens__

extern APTR audio_mx;

//some define for this nice AHI driver !
#define AHI_CHUNKSIZE         (65536/2)
#define AHI_CHUNKMAX          (131072/2)
#define AHI_DEFAULTUNIT       0

#else

#define AHI_CHUNKSIZE         100

#endif

struct audioChunk
{
	char ptr[AHI_CHUNKSIZE];
	int size;
	int frequency;
	int position;
};

extern void audio_lock();
extern void audio_unlock();

extern bool audio_start();
extern void audio_close();
extern bool play(uint8_t * data,int len, int channel, int frequency);

