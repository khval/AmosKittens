#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

#define bank_type_work_or_data 0
#define bank_type_icons 2
#define bank_type_sprite 3

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

