#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"


char *gfxScreenOpen(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxLowres(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxHires(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenDisplay(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxColour(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxMouseKey(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxWaitVbl(struct nativeCommand *cmd, char *tokenBuffer);

