
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

char *fontsGetAllFonts(struct nativeCommand *cmd, char *tokenBuffer);
char *fontsGetDiscFonts(struct nativeCommand *cmd, char *tokenBuffer);
char *fontsGetRomFonts(struct nativeCommand *cmd, char *tokenBuffer);
char *fontsSetFont(struct nativeCommand *cmd, char *tokenBuffer);
char *fontsFontsStr(struct nativeCommand *cmd, char *tokenBuffer);

