
extern APTR audio_mx;
extern void audio_lock();
extern void audio_unlock();

extern bool audio_start(int rate,int channels);
extern void audio_close();
extern void play(uint8_t * data,int len);

