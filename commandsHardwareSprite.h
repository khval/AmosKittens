#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

char *hsSprite(struct nativeCommand *cmd, char *tokenBuffer);
char *hsSpriteCol(struct nativeCommand *cmd, char *tokenBuffer);
char *hsSpriteOff(struct nativeCommand *cmd, char *tokenBuffer);
char *hsGetSprite(struct nativeCommand *cmd, char *tokenBuffer);
char *hsGetSpritePalette(struct nativeCommand *cmd, char *tokenBuffer);
char *hsSpriteBase(struct nativeCommand *cmd, char *tokenBuffer);
char *hsSetSpriteBuffer(struct nativeCommand *cmd, char *tokenBuffer);

