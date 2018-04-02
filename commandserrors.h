#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

extern char *(*onError)(char *ptr);

extern char *onErrorBreak(char *ptr);
extern char *onErrorGoto(char *ptr);

extern char *cmdOnError(nativeCommand *cmd, char *ptr);

