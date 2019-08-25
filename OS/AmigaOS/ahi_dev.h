
extern APTR audio_mx;
extern void audio_lock();
extern void audio_unlock();

extern bool audio_start();
extern void audio_close();
extern bool play(uint8_t * data,int len, int channel, int frequency);

