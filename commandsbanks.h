#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

extern char *cmdReserveAsWork(nativeCommand *cmd, char *ptr);
extern char *cmdReserveAsChipWork(nativeCommand *cmd, char *ptr);
extern char *cmdReserveAsData(nativeCommand *cmd, char *ptr);
extern char *cmdReserveAsChipData(nativeCommand *cmd, char *ptr);
extern char *cmdListBank(nativeCommand *cmd, char *ptr);

extern char *cmdErase(nativeCommand *cmd, char *ptr);
extern char *cmdStart(nativeCommand *cmd, char *ptr);
extern char *cmdLength(nativeCommand *cmd, char *ptr);
extern char *cmdBload(nativeCommand *cmd, char *ptr);
extern char *cmdBsave(nativeCommand *cmd, char *ptr);

