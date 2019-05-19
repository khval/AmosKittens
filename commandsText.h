#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __amigaos4__
#include <proto/exec.h>
#endif
#include "amosKittens.h"

extern char *textPrint(nativeCommand *cmd, char *ptr);
extern char *textLocate(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textPaper(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textPen(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textCentre(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textPrint(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textYCurs(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textXCurs(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textCursOn(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textCursOff(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textCursPen(struct nativeCommand *cmd, char *tokenBuffer);
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
extern char *textAt(nativeCommand *cmd, char *ptr);
extern char *textXText(nativeCommand *cmd, char *ptr);
extern char *textYText(nativeCommand *cmd, char *ptr);
extern char *textCMove(nativeCommand *cmd, char *ptr);
extern char *textCUp(nativeCommand *cmd, char *ptr);
extern char *textCDown(nativeCommand *cmd, char *ptr);
extern char *textCLeft(nativeCommand *cmd, char *ptr);
extern char *textCRight(nativeCommand *cmd, char *ptr);
extern char *textSetTab(nativeCommand *cmd, char *ptr);
extern char *textSetCurs(nativeCommand *cmd, char *ptr);
extern char *textMemorizeX(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textMemorizeY(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textRememberX(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textRememberY(struct nativeCommand *cmd, char *tokenBuffer);
extern char *textVscroll(struct nativeCommand *cmd, char *ptr);
extern char *textHscroll(struct nativeCommand *cmd, char *ptr);
extern char *textClw(struct nativeCommand *cmd, char *ptr);
extern char *textCline(struct nativeCommand *cmd, char *ptr);
extern char *textCUpStr(nativeCommand *cmd, char *ptr);
extern char *textCDownStr(nativeCommand *cmd, char *ptr);
extern char *textCLeftStr(nativeCommand *cmd, char *ptr);
extern char *textCRightStr(nativeCommand *cmd, char *ptr);
extern char *textPrintUsing(nativeCommand *cmd, char *ptr);
extern char *textWindow(nativeCommand *cmd, char *ptr);
extern char *textWindClose(nativeCommand *cmd, char *ptr);
extern char *textWindOpen(nativeCommand *cmd, char *ptr);
extern char *textWindMove(nativeCommand *cmd, char *ptr);
extern char *textWindSave(nativeCommand *cmd, char *ptr);
extern char *textWindSize(nativeCommand *cmd, char *ptr);
extern char *textWindon(nativeCommand *cmd, char *ptr);
extern char *textXGraphic(nativeCommand *cmd, char *ptr);
extern char *textYGraphic(nativeCommand *cmd, char *ptr);
extern char *textTitleTop(nativeCommand *cmd, char *ptr);
extern char *textTitleBottom(nativeCommand *cmd, char *ptr);
extern char *textAt(struct nativeCommand *disc, char *tokenBuffer);
extern char *textBorder(struct nativeCommand *disc, char *tokenBuffer);

