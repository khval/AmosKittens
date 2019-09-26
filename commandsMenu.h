
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

char *menuChoice(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuStr(struct nativeCommand *cmd, char *tokenBuffer);
char *menuSetMenu(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuOn(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuOff(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuInactive(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuCalc(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuToBank(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuDel(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuX(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuY(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuActive(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuKey(struct nativeCommand *cmd, char *tokenBuffer);
char *menuOnMenu(struct nativeCommand *cmd, char *tokenBuffer);
char *menuOnMenuOn(struct nativeCommand *cmd, char *tokenBuffer);
char *menuOnMenuOff(struct nativeCommand *cmd, char *tokenBuffer);
char *menuOnMenuDel(struct nativeCommand *cmd, char *tokenBuffer);

