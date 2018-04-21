
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

char *machinePoke(struct nativeCommand *cmd, char *tokenBuffer);
char *machinePeek(struct nativeCommand *cmd, char *tokenBuffer);
char *machineDoke(struct nativeCommand *cmd, char *tokenBuffer);
char *machineDeek(struct nativeCommand *cmd, char *tokenBuffer);
char *machineLoke(struct nativeCommand *cmd, char *tokenBuffer);
char *machineLeek(struct nativeCommand *cmd, char *tokenBuffer);
char *machineCopy(struct nativeCommand *cmd, char *tokenBuffer);
char *machineVarPtr(struct nativeCommand *cmd, char *tokenBuffer);
char *machineFill(struct nativeCommand *cmd, char *tokenBuffer);

char *machineAREG(struct nativeCommand *cmd, char *tokenBuffer);
char *machineDREG(struct nativeCommand *cmd, char *tokenBuffer);
char *machineDOSCALL(struct nativeCommand *cmd, char *tokenBuffer);
char *machineEXECALL(struct nativeCommand *cmd, char *tokenBuffer);
char *machineGFXCALL(struct nativeCommand *cmd, char *tokenBuffer);
char *machineINTCALL(struct nativeCommand *cmd, char *tokenBuffer);

/*
char *machineHunt(struct nativeCommand *cmd, char *tokenBuffer);
char *machineRolB(struct nativeCommand *cmd, char *tokenBuffer);
char *machineRolW(struct nativeCommand *cmd, char *tokenBuffer);
char *machineRolL(struct nativeCommand *cmd, char *tokenBuffer);
char *machineRorB(struct nativeCommand *cmd, char *tokenBuffer);
char *machineRorW(struct nativeCommand *cmd, char *tokenBuffer);
char *machineRorL(struct nativeCommand *cmd, char *tokenBuffer);
char *machineBtst(struct nativeCommand *cmd, char *tokenBuffer);
char *machineBset(struct nativeCommand *cmd, char *tokenBuffer);
char *machineBchg(struct nativeCommand *cmd, char *tokenBuffer);
char *machineBclr(struct nativeCommand *cmd, char *tokenBuffer);
*/

char *machineAREG(struct nativeCommand *cmd, char *tokenBuffer);
char *machineDREG(struct nativeCommand *cmd, char *tokenBuffer);
char *machineDOSCALL(struct nativeCommand *cmd, char *tokenBuffer);
char *machineEXECALL(struct nativeCommand *cmd, char *tokenBuffer);
char *machineGFXCALL(struct nativeCommand *cmd, char *tokenBuffer);
char *machineINTCALL(struct nativeCommand *cmd, char *tokenBuffer);
char *machineAREG(struct nativeCommand *cmd, char *tokenBuffer);
char *machineDREG(struct nativeCommand *cmd, char *tokenBuffer);
char *machineDOSCALL(struct nativeCommand *cmd, char *tokenBuffer);
char *machineEXECALL(struct nativeCommand *cmd, char *tokenBuffer);
char *machineGFXCALL(struct nativeCommand *cmd, char *tokenBuffer);
char *machineINTCALL(struct nativeCommand *cmd, char *tokenBuffer);

