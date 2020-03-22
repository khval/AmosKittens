
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

extern char *cmdLeftStr(nativeCommand *cmd, char *ptr);
extern char *cmdMidStr(nativeCommand *cmd, char *ptr);
extern char *cmdRightStr(nativeCommand *cmd, char *ptr);
extern char *cmdHexStr(nativeCommand *cmd, char *ptr);
extern char *cmdBinStr(nativeCommand *cmd, char *ptr);
extern char *cmdInstr(nativeCommand *cmd, char *ptr);
extern char *cmdFlipStr(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdSpaceStr(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdUpperStr(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdLowerStr(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdStringStr(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdChrStr(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdAsc(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdLen(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdVal(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdStr(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdSort(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdMatch(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdStr(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdRepeatStr(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdTabStr(struct nativeCommand *cmd, char *tokenBuffer );

