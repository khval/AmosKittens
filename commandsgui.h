#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

extern char *guiDialogRun(nativeCommand *cmd, char *ptr);
extern char *guiDialog(nativeCommand *cmd, char *ptr);
extern char *guiDialogStr(nativeCommand *cmd, char *ptr);
extern char *guiDialogBox(nativeCommand *cmd, char *ptr);
extern char *guiDialogFreeze(nativeCommand *cmd, char *ptr);
extern char *guiDialogUnfreeze(nativeCommand *cmd, char *tokenBuffer);
extern char *guiDialogOpen(nativeCommand *cmd, char *ptr);
extern char *guiDialogClose(nativeCommand *cmd, char *ptr);
extern char *guiVdialog(nativeCommand *cmd, char *ptr);


