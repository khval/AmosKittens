
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/retroMode.h>

char *amalAmReg(struct nativeCommand *cmd, char *tokenBuffer);
char *amalChannel(struct nativeCommand *cmd, char *tokenBuffer);
char *amalAmal(struct nativeCommand *cmd, char *tokenBuffer);
char *amalAmalOn(struct nativeCommand *cmd, char *tokenBuffer);
char *amalAmalOff(struct nativeCommand *cmd, char *tokenBuffer);
char *amalAnim(struct nativeCommand *cmd, char *tokenBuffer);
char *amalAnimOn(struct nativeCommand *cmd, char *tokenBuffer);
char *amalAmalFreeze(struct nativeCommand *cmd, char *tokenBuffer);

