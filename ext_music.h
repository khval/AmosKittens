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

