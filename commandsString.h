
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

extern char *cmdPrint(nativeCommand *cmd, char *ptr);
extern char *cmdLeft(nativeCommand *cmd, char *ptr);
extern char *cmdMid(nativeCommand *cmd, char *ptr);
extern char *cmdRight(nativeCommand *cmd, char *ptr);
extern char *cmdHex(nativeCommand *cmd, char *ptr);
extern char *cmdBin(nativeCommand *cmd, char *ptr);
extern char *cmdInstr(nativeCommand *cmd, char *ptr);
extern char *cmdFlip(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdSpace(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdUpper(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdLower(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdString(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdChr(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdAsc(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdLen(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdVal(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdStr(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdSort(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdMatch(struct nativeCommand *cmd, char *tokenBuffer );
extern char *cmdStr(struct nativeCommand *cmd, char *tokenBuffer );

