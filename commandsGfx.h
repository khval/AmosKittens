#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

char *gfxColour(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxWaitVbl(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxBox(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxBar(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxDraw(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxCircle(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxEllipse(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxInk(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxFlash(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxFlashOff(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxPlot(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxPoint(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxGrLocate(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxXGR(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxYGR(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxGetColour(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxPolyline(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxPalette(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxGetPalette(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxDefaultPalette(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxPolygon(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxCls(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxDefScroll(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxScroll(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxShiftUp(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxShiftDown(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxShiftOff(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxSetRainbow(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxRainbow(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxZoom(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxFade(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxRain(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxAppear(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxNtsc(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxAutoback(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxPaint(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxColourBack(struct nativeCommand *cmd, char *tokenBuffer);
char *gfxSetPaint(struct nativeCommand *cmd, char *tokenBuffer);

