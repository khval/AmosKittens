#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

extern char *textPrint(nativeCommand *cmd, char *ptr);
extern char *textLocate(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textPaper(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textPen(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textCentre(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textPrint(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textCursOn(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textCursOff(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textHome(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textInverseOn(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textInverseOff(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textBorderStr(nativeCommand *cmd, char *ptr);
extern char *textPenStr(nativeCommand *cmd, char *ptr);
extern char *textPaperStr(nativeCommand *cmd, char *ptr);
extern char *textWriting(nativeCommand *cmd, char *ptr);
extern char *textShadeOff(nativeCommand *cmd, char *ptr);
extern char *textShadeOn(nativeCommand *cmd, char *ptr);
extern char *textUnderOff(nativeCommand *cmd, char *ptr);
extern char *textUnderOn(nativeCommand *cmd, char *ptr);

