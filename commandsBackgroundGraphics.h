#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#endif

#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "amosKittens.h"


extern char *bgPasteIcon(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgGetIcon(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgGetIconPalette(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgDelIcon(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgMaskIconMask(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgGetBlock(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgPutBlock(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgDelBlock(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgGetCBlock(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgPutCBlock(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgDelCBlock(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgIconBase(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgMakeIconMask(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgVrevBlock(struct nativeCommand *cmd, char *tokenBuffer);
extern char *bgHrevBlock(struct nativeCommand *cmd, char *tokenBuffer);

extern struct retroBlock *findBlock_in_blocks(int id);
extern struct retroBlock *findBlock_in_cblocks(int id);

