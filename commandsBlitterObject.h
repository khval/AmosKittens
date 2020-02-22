#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

char *boBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boBobOff(struct nativeCommand *cmd, char *tokenBuffer);
char *boSetBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boNoMask(struct nativeCommand *cmd, char *tokenBuffer);
char *boXBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boYBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boIBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boPasteBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boPutBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boGetBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boLimitBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boHotSpot(struct nativeCommand *cmd, char *tokenBuffer);
char *boHrev(struct nativeCommand *cmd, char *tokenBuffer);
char *boVrev(struct nativeCommand *cmd, char *tokenBuffer);
char *boRev(struct nativeCommand *cmd, char *tokenBuffer);
char *boBobCol(struct nativeCommand *cmd, char *tokenBuffer);
char *boBobSpriteCol(struct nativeCommand *cmd, char *tokenBuffer);
char *boBobUpdateOff(struct nativeCommand *cmd, char *tokenBuffer);
char *boBobUpdateOn(struct nativeCommand *cmd, char *tokenBuffer);
char *boCol(struct nativeCommand *cmd, char *tokenBuffer);
char *boDelBob(struct nativeCommand *cmd, char *tokenBuffer);
char *boBobUpdate(struct nativeCommand *cmd, char *tokenBuffer);
char *boBobClear(struct nativeCommand *cmd, char *tokenBuffer);
char *boBobDraw(struct nativeCommand *cmd, char *tokenBuffer);
char *boMakeMask(struct nativeCommand *cmd, char *tokenBuffer);
void freeScreenBobs( int screen_id );
void freeBobClear( struct retroSpriteObject *bob );

