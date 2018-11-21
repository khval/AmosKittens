
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

char *menuChoice(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuStr(struct nativeCommand *cmd, char *tokenBuffer);
char *menuSetMenu(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuOn(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuOff(struct nativeCommand *cmd, char *tokenBuffer);
char *menuMenuInactive(struct nativeCommand *cmd, char *tokenBuffer);

