#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

char *ocMouseKey(struct nativeCommand *cmd, char *tokenBuffer);
char *ocMouseClick(struct nativeCommand *cmd, char *tokenBuffer);
char *ocXMouse(struct nativeCommand *cmd, char *tokenBuffer);
char *ocYMouse(struct nativeCommand *cmd, char *tokenBuffer);
char *ocHide(struct nativeCommand *cmd, char *tokenBuffer);
char *ocMouseLimit(struct nativeCommand *cmd, char *tokenBuffer);

