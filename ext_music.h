#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

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

