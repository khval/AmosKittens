#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

void init_banks( char *data , int size);


//------------------------------------------------------------------------------

extern char *bankReserveAsWork(nativeCommand *cmd, char *ptr);
extern char *bankReserveAsChipWork(nativeCommand *cmd, char *ptr);
extern char *bankReserveAsData(nativeCommand *cmd, char *ptr);
extern char *bankReserveAsChipData(nativeCommand *cmd, char *ptr);
extern char *bankListBank(nativeCommand *cmd, char *ptr);
extern char *bankErase(nativeCommand *cmd, char *ptr);
extern char *bankEraseAll(nativeCommand *cmd, char *ptr);
extern char *bankEraseTemp(nativeCommand *cmd, char *ptr);
extern char *bankStart(nativeCommand *cmd, char *ptr);
extern char *bankLength(nativeCommand *cmd, char *ptr);
extern char *bankBload(nativeCommand *cmd, char *ptr);
extern char *bankBsave(nativeCommand *cmd, char *ptr);
extern char *bankLoad(nativeCommand *cmd, char *ptr);
extern char *bankSave(nativeCommand *cmd, char *ptr);
extern char *bankBankSwap(nativeCommand *cmd, char *ptr);
extern char *bankBGrab(nativeCommand *cmd, char *ptr);
extern char *bankResourceBank(nativeCommand *cmd, char *ptr);
extern char *bankResourceStr(nativeCommand *cmd, char *tokenBuffer);
extern char *bankBankShrink(nativeCommand *cmd, char *tokenBuffer);
extern char *bankBlength(nativeCommand *cmd, char *ptr);
extern char *bankBstart(nativeCommand *cmd, char *ptr);
extern char *bankBsend(nativeCommand *cmd, char *ptr);


extern void __load_bank__(struct stringData *name, int bankNr );
extern void __load_bank__(char *name, int bankNr );

extern struct stringData *getResourceStr(int id);
extern void *getBankObject(int id);



