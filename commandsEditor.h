#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

char *cmdAskEditor(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdCallEditor(struct nativeCommand *cmd, char *tokenBuffer);
