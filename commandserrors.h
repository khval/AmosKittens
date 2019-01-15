#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

extern char *(*onError)(char *ptr);

extern char *onErrorBreak(char *ptr);
extern char *onErrorGoto(char *ptr);
extern char *onErrorProc(char *ptr);

extern char *errTrap(nativeCommand *err, char *ptr);
extern char *errErrTrap(nativeCommand *err, char *ptr);
extern char *errError(nativeCommand *err, char *ptr);
extern char *errErrn(nativeCommand *err, char *ptr);
extern char *errOnError(nativeCommand *err, char *ptr);
extern char *errResumeLabel(nativeCommand *err, char *ptr);
extern char *errResumeNext(nativeCommand *err, char *ptr);
extern char *errResume(nativeCommand *err, char *ptr);

