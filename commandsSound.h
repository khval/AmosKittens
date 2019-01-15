
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

char *soundBoom(struct nativeCommand *cmd, char *tokenBuffer);
char *soundTempo(struct nativeCommand *cmd, char *tokenBuffer);

