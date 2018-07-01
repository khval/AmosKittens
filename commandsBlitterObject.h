#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

char *boBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boBobOff(struct nativeCommand *cmd, char *tokenBuffer);
char *boSetBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boNoMask(struct nativeCommand *cmd, char *tokenBuffer);
char *boXBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boYBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boPasteBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boPutBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boGetBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boLimitBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boHotSpot(struct nativeCommand *cmd, char *tokenBuffer);

