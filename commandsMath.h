#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif 

#include <string>
#include <iostream>
#include <math.h>

#include "amosKittens.h"

char *mathInc (struct nativeCommand *cmd, char *tokenBuffer);
char *mathDec (struct nativeCommand *cmd, char *tokenBuffer);
char *mathAdd (struct nativeCommand *cmd, char *tokenBuffer);
char *mathAddRange (struct nativeCommand *cmd, char *tokenBuffer);
char *mathDegree (struct nativeCommand *cmd, char *tokenBuffer);
char *mathRadian (struct nativeCommand *cmd, char *tokenBuffer);
char *mathPi (struct nativeCommand *cmd, char *tokenBuffer);
char *mathSin (struct nativeCommand *cmd, char *tokenBuffer);
char *mathCos (struct nativeCommand *cmd, char *tokenBuffer);
char *mathTan (struct nativeCommand *cmd, char *tokenBuffer);
char *mathAcos (struct nativeCommand *cmd, char *tokenBuffer);
char *mathAsin (struct nativeCommand *cmd, char *tokenBuffer);
char *mathAtan (struct nativeCommand *cmd, char *tokenBuffer);
char *mathHsin (struct nativeCommand *cmd, char *tokenBuffer);
char *mathHcos (struct nativeCommand *cmd, char *tokenBuffer);
char *mathHtan (struct nativeCommand *cmd, char *tokenBuffer);
char *mathLog (struct nativeCommand *cmd, char *tokenBuffer);
char *mathExp (struct nativeCommand *cmd, char *tokenBuffer);
char *mathLn (struct nativeCommand *cmd, char *tokenBuffer);
char *mathSqr (struct nativeCommand *cmd, char *tokenBuffer);
char *mathAbs (struct nativeCommand *cmd, char *tokenBuffer);
char *mathInt (struct nativeCommand *cmd, char *tokenBuffer);
char *mathSgn (struct nativeCommand *cmd, char *tokenBuffer);
char *mathRnd (struct nativeCommand *cmd, char *tokenBuffer);
char *mathRandomize (struct nativeCommand *cmd, char *tokenBuffer);
char *mathMax (struct nativeCommand *cmd, char *tokenBuffer);
char *mathMin (struct nativeCommand *cmd, char *tokenBuffer);
char *mathSwap (struct nativeCommand *cmd, char *tokenBuffer);
char *mathFix (struct nativeCommand *cmd, char *tokenBuffer);
char *mathDefFn (struct nativeCommand *cmd, char *tokenBuffer);
char *mathFn (struct nativeCommand *cmd, char *tokenBuffer);
char *mathSwap (struct nativeCommand *cmd, char *tokenBuffer);

