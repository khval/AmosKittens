

#ifdef __amoskittens__

extern APTR channel_mx[4];

//some define for this nice AHI driver !
#define AHI_CHUNKSIZE         1024
#define AHI_CHUNKMAX          (131072/2)
#define AHI_DEFAULTUNIT       0

#else

#define AHI_CHUNKSIZE         100

#endif

struct audioChunk
{
	int size;
	int bytesPerSecond;
	int position;
	double lowpass_alpha;
	double lowpass_n_alpha;
	char ptr;
};

extern void channel_lock(int n);
extern void channel_unlock(int n);

extern bool audio_start();
extern void audio_close();
extern void audio_lock();
extern void audio_unlock();
extern void audio_device_flush(int voices);

extern bool play(uint8_t * data,int len, int channel, int frequency);
extern bool play_wave(struct wave *wave,int len, int channel);

