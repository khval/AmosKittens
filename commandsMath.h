#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <string>
#include <iostream>
#include <math.h>

#include "amosKittens.h"

char *incMath(struct nativeCommand *cmd, char *tokenBuffer);
char *decMath(struct nativeCommand *cmd, char *tokenBuffer);
char *addMath(struct nativeCommand *cmd, char *tokenBuffer);

char *degreeMath(struct nativeCommand *cmd, char *tokenBuffer);
char *radianMath(struct nativeCommand *cmd, char *tokenBuffer);
char *piMath(struct nativeCommand *cmd, char *tokenBuffer);
char *sinMath(struct nativeCommand *cmd, char *tokenBuffer);
char *cosMath(struct nativeCommand *cmd, char *tokenBuffer);
char *tanMath(struct nativeCommand *cmd, char *tokenBuffer);
char *acosMath(struct nativeCommand *cmd, char *tokenBuffer);
char *asinMath(struct nativeCommand *cmd, char *tokenBuffer);
char *atanMath(struct nativeCommand *cmd, char *tokenBuffer);
char *hsinMath(struct nativeCommand *cmd, char *tokenBuffer);
char *hcosMath(struct nativeCommand *cmd, char *tokenBuffer);
char *htanMath(struct nativeCommand *cmd, char *tokenBuffer);
char *logMath(struct nativeCommand *cmd, char *tokenBuffer);
char *expMath(struct nativeCommand *cmd, char *tokenBuffer);
char *lnMath(struct nativeCommand *cmd, char *tokenBuffer);
char *sqrMath(struct nativeCommand *cmd, char *tokenBuffer);
char *absMath(struct nativeCommand *cmd, char *tokenBuffer);
char *intMath(struct nativeCommand *cmd, char *tokenBuffer);
char *sgnMath(struct nativeCommand *cmd, char *tokenBuffer);
char *rndMath(struct nativeCommand *cmd, char *tokenBuffer);
char *randomizeMath(struct nativeCommand *cmd, char *tokenBuffer);
char *maxMath(struct nativeCommand *cmd, char *tokenBuffer);
char *minMath(struct nativeCommand *cmd, char *tokenBuffer);
char *swapMath(struct nativeCommand *cmd, char *tokenBuffer);
char *fixMath(struct nativeCommand *cmd, char *tokenBuffer);
char *defFnMath(struct nativeCommand *cmd, char *tokenBuffer);
char *fnMath(struct nativeCommand *cmd, char *tokenBuffer);

