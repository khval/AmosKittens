
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

char *discDir(struct nativeCommand *disc, char *tokenBuffer);
char *discDirStr(struct nativeCommand *disc, char *tokenBuffer);
char *discParent(struct nativeCommand *disc, char *tokenBuffer);
char *discSetDir(struct nativeCommand *disc, char *tokenBuffer);
char *discDfree(struct nativeCommand *disc, char *tokenBuffer);
char *discKill(struct nativeCommand *disc, char *tokenBuffer);
char *discRename(struct nativeCommand *disc, char *tokenBuffer);
char *discFselStr(struct nativeCommand *disc, char *tokenBuffer);
char *discExist(struct nativeCommand *disc, char *tokenBuffer);
char *discDirFirstStr(struct nativeCommand *disc, char *tokenBuffer);
char *discDirNextStr(struct nativeCommand *disc, char *tokenBuffer);
char *discDevFirstStr(struct nativeCommand *disc, char *tokenBuffer);
char *discDevNextStr(struct nativeCommand *disc, char *tokenBuffer);

char *discPrintOut(struct nativeCommand *disc, char *tokenBuffer);
char *discInputIn(struct nativeCommand *disc, char *tokenBuffer);
char *discLineInputFile(struct nativeCommand *disc, char *tokenBuffer);
char *discOpenIn(struct nativeCommand *disc, char *tokenBuffer);
char *discOpenOut(struct nativeCommand *disc, char *tokenBuffer);
char *discAppend(struct nativeCommand *disc, char *tokenBuffer);
char *discClose(struct nativeCommand *disc, char *tokenBuffer);

char *discInputStrFile(struct nativeCommand *disc, char *tokenBuffer);
char *discSetInput(struct nativeCommand *disc, char *tokenBuffer);
char *discLof(struct nativeCommand *disc, char *tokenBuffer);
char *discPof(struct nativeCommand *disc, char *tokenBuffer);
char *discEof(struct nativeCommand *disc, char *tokenBuffer);

char *discOpenRandom(struct nativeCommand *disc, char *tokenBuffer);
char *discField(struct nativeCommand *disc, char *tokenBuffer);
char *discGet(struct nativeCommand *disc, char *tokenBuffer);
char *discPut(struct nativeCommand *disc, char *tokenBuffer);

char *discMakedir(struct nativeCommand *disc, char *tokenBuffer);

