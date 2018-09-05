#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

extern char *(*onError)(char *ptr);

extern char *onErrorBreak(char *ptr);
extern char *onErrorGoto(char *ptr);
extern char *onErrorProc(char *ptr);

extern char *cmdError(nativeCommand *cmd, char *ptr);
extern char *errError(nativeCommand *err, char *ptr);
extern char *errOnError(nativeCommand *err, char *ptr);
extern char *errResumeLabel(nativeCommand *err, char *ptr);
extern char *errResumeNext(nativeCommand *err, char *ptr);

