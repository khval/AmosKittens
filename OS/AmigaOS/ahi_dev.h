
#ifdef __amoskittens__

extern APTR channel_mx[4];

//some define for this nice AHI driver !
#define AHI_CHUNKSIZE         (65536/2)
#define AHI_CHUNKMAX          (131072/2)
#define AHI_DEFAULTUNIT       0

#else

#define AHI_CHUNKSIZE         100

#endif

struct audioChunk
{
	int size;
	int frequency;
	int position;
	char ptr;
};

extern void channel_lock(int n);
extern void channel_unlock(int n);

extern void audio_lock();
extern void audio_unlock();

extern void audio_device_flush();
extern bool audio_start();
extern void audio_close();
extern bool play(uint8_t * data,int len, int channel, int frequency);
extern bool play_wave(struct wave *wave,int len, int channel);

