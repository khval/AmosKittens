#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

char *ocMouseKey(struct nativeCommand *cmd, char *tokenBuffer);
char *ocMouseClick(struct nativeCommand *cmd, char *tokenBuffer);
char *ocXMouse(struct nativeCommand *cmd, char *tokenBuffer);
char *ocYMouse(struct nativeCommand *cmd, char *tokenBuffer);
char *ocHide(struct nativeCommand *cmd, char *tokenBuffer);
char *ocMouseLimit(struct nativeCommand *cmd, char *tokenBuffer);
char *ocReserveZone(struct nativeCommand *cmd, char *tokenBuffer);
char *ocZoneStr(struct nativeCommand *cmd, char *tokenBuffer);
char *ocMouseZone(struct nativeCommand *cmd, char *tokenBuffer);
char *ocZone(struct nativeCommand *cmd, char *tokenBuffer);
char *ocHZone(struct nativeCommand *cmd, char *tokenBuffer);
char *ocChangeMouse(struct nativeCommand *cmd, char *tokenBuffer);
char *ocSetZone(struct nativeCommand *cmd, char *tokenBuffer);
char *ocResetZone(struct nativeCommand *cmd, char *tokenBuffer);
char *ocShow(struct nativeCommand *cmd, char *tokenBuffer);
char *ocShowOn(struct nativeCommand *cmd, char *tokenBuffer);
char *ocHideOn(struct nativeCommand *cmd, char *tokenBuffer);
char *ocPriorityOn(struct nativeCommand *cmd, char *tokenBuffer);
char *ocPriorityOff(struct nativeCommand *cmd, char *tokenBuffer);
char *ocPriorityReverseOn(struct nativeCommand *cmd, char *tokenBuffer);
char *ocPriorityReverseOff(struct nativeCommand *cmd, char *tokenBuffer);
char *ocAutoViewOff(struct nativeCommand *cmd, char *tokenBuffer);
char *ocAutoViewOn(struct nativeCommand *cmd, char *tokenBuffer);
char *ocView(struct nativeCommand *cmd, char *tokenBuffer);
char *ocUpdateOff(struct nativeCommand *cmd, char *tokenBuffer);
char *ocUpdate(struct nativeCommand *cmd, char *tokenBuffer);
char *ocUpdateEvery(struct nativeCommand *cmd, char *tokenBuffer);
char *ocSynchroOn(struct nativeCommand *cmd, char *tokenBuffer);
char *ocSynchroOff(struct nativeCommand *cmd, char *tokenBuffer);
char *ocSynchro(struct nativeCommand *cmd, char *tokenBuffer);
char *ocJUp(struct nativeCommand *cmd, char *tokenBuffer);
char *ocJDown(struct nativeCommand *cmd, char *tokenBuffer);
char *ocJLeft(struct nativeCommand *cmd, char *tokenBuffer);
char *ocJRight(struct nativeCommand *cmd, char *tokenBuffer);
char *ocFire(struct nativeCommand *cmd, char *tokenBuffer);
char *ocUpdateOn(struct nativeCommand *cmd, char *tokenBuffer);
char *ocMakeMask(struct nativeCommand *cmd, char *tokenBuffer);
char *ocJoy(struct nativeCommand *cmd, char *tokenBuffer);
char *ocIconMakeMask(struct nativeCommand *cmd, char *tokenBuffer);

