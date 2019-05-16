#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#endif

#include "debug.h"
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "amosKittens.h"


char *bgPasteIcon(struct nativeCommand *cmd, char *tokenBuffer);
char *bgGetIcon(struct nativeCommand *cmd, char *tokenBuffer);
char *bgGetIconPalette(struct nativeCommand *cmd, char *tokenBuffer);
char *bgDelIcon(struct nativeCommand *cmd, char *tokenBuffer);
char *bgMaskIconMask(struct nativeCommand *cmd, char *tokenBuffer);
char *bgGetBlock(struct nativeCommand *cmd, char *tokenBuffer);
char *bgPutBlock(struct nativeCommand *cmd, char *tokenBuffer);
char *bgDelBlock(struct nativeCommand *cmd, char *tokenBuffer);
char *bgGetCBlock(struct nativeCommand *cmd, char *tokenBuffer);
char *bgPutCBlock(struct nativeCommand *cmd, char *tokenBuffer);
char *bgDelCBlock(struct nativeCommand *cmd, char *tokenBuffer);
char *bgIconBase(struct nativeCommand *cmd, char *tokenBuffer);
char *bgMakeIconMask(struct nativeCommand *cmd, char *tokenBuffer);



