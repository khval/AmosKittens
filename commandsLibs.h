#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

extern char *libLibOpen(nativeCommand *cmd, char *ptr);
extern char *libLibClose(nativeCommand *cmd, char *ptr);
extern char *libLibCall(nativeCommand *cmd, char *ptr);
extern char *libLibBase(nativeCommand *cmd, char *ptr);

struct kittyLib *kFindLib( int id );
int kFindLibIndex( int id );
void kFreeLib( int id );




