#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

extern char *ext_cmd_sam_bank(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_sam_play(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_sam_raw(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_sample(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_play(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_boom(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_bell(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_shoot(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_wave(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_del_wave(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_set_wave(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_set_envel(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_volume(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_sam_loop_on(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_sam_loop_off(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_sam_stop(nativeCommand *cmd, char *ptr);

extern char *ext_cmd_ssave(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_sload(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_sam_swap(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_sam_swapped(nativeCommand *cmd, char *ptr);

void make_wave_test();
void make_wave_bell();
void make_wave_noice();
struct wave *getWave(int id);
bool delWave(int id);
void setEnval(struct wave *wave, int phase, int duration, int volume);
bool apply_wave(int waveId, int voices);


