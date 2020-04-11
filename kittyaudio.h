

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

extern bool audioStart();
extern void audioClose();
extern void audioLock();
extern void audioUnlock();
extern void audioDeviceFlush(int voices);
extern bool audioPlay(uint8_t * data,int len, int channel, int frequency);
extern bool audioPlayWave(struct wave *wave,int len, int channel);

