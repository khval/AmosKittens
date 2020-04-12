
extern APTR channel_mx[4];

struct audioChunk
{
	int size;
	int bytesPerSecond;
	int position;
	double lowpass_alpha;
	double lowpass_n_alpha;
	char ptr;	// <------ No extra data must be added after PTR !!!!
};

extern void channel_lock(int n);
extern void channel_unlock(int n);

extern bool audioStart();
extern void audioClose();
extern void audioLock();
extern void audioUnlock();
extern void audioSetSampleLoop( ULONG voices, bool value );
extern void audioDeviceFlush(int voices);
extern bool audioPlay(uint8_t * data,int len, int channel, int frequency);
extern bool audioPlayWave(struct wave *wave,int len, int channel);

