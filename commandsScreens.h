#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

char *gfxLowres(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxHires(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenOpen(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenClose(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenDisplay(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenOffset(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenClone(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreen(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxGetScreen(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenToFront(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenToBack(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenHide(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenShow(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenHeight(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenWidth(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenColour(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScreenCopy(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScin(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxGetScreen(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxLoadIff(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxSaveIff(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxDoubleBuffer(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxDefault(struct nativeCommand *cmd, char *tokenBuffer);


