
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

struct kittyDevice *kFindDevice( int id );
int kFindDeviceIndex( int id );
void kCloseDevice( struct kittyDevice *dev );
void kFreeDevice( int id );

char *deviceDevOpen(struct nativeCommand *device, char *tokenBuffer);
char *deviceDevClose(struct nativeCommand *device, char *tokenBuffer);
char *deviceDevBase(struct nativeCommand *device, char *tokenBuffer);
char *deviceDevDo(struct nativeCommand *device, char *tokenBuffer);
char *deviceDevSend(struct nativeCommand *device, char *tokenBuffer);
char *deviceDevAbort(struct nativeCommand *device, char *tokenBuffer);
char *deviceDevCheck(struct nativeCommand *device, char *tokenBuffer);



