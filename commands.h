
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

char *nextArg(struct nativeCommand *cmd, char *tokenBuffer);
char *addStr(struct nativeCommand *cmd, char *tokenBuffer);
char *subCalc(struct nativeCommand *cmd, char *tokenBuffer);
char *subCalcEnd(struct nativeCommand *cmd, char *tokenBuffer);
char *addData(struct nativeCommand *cmd, char *tokenBuffer);
char *subData(struct nativeCommand *cmd, char *tokenBuffer);
char *setVar(struct nativeCommand *cmd, char *tokenBuffer);
char *mulVar(struct nativeCommand *cmd, char *tokenBuffer);
char *divVar(struct nativeCommand *cmd, char *tokenBuffer);

// not used outside of commands.cpp, normally but just for testing.
void _addStr( void );

