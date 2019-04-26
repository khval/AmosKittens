#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"

#define bank_type_work_or_data 0
#define bank_type_icons 2
#define bank_type_sprite 3

void init_banks( char *data , int size);

struct resourcebank_header
{
	uint16_t chunks;
	uint32_t img_offset;
	uint32_t var_offset;		 // 001A
	uint32_t script_offset;	 // 002E
//	uint32_t gadget_offset;	 // 0000
//	uint32_t strings_size;	 // 0014
//	uint32_t scripts_size;	 // 008A
} PACKED;

// resource strings starts with size, text.
// and ends when size is 0x00FF, last int16 not counted in block_string_size.

// next block starts with 00004 --> this might be number of scripts.
// 00026	-- this numbers looks like incremted offsets.
// 00046
// 00066
// 00000	--> this changes to 0086, when I have 4 scripts.


//------------------------------------------------------------------------------

extern int current_resource_bank ;

void freeBank( int banknr );
struct kittyBank *findBank( int bankNr );
struct kittyBank *__ReserveAs( int type, int bankNr, int length, const char *name, char *mem );

extern char *cmdReserveAsWork(nativeCommand *cmd, char *ptr);
extern char *cmdReserveAsChipWork(nativeCommand *cmd, char *ptr);
extern char *cmdReserveAsData(nativeCommand *cmd, char *ptr);
extern char *cmdReserveAsChipData(nativeCommand *cmd, char *ptr);
extern char *cmdListBank(nativeCommand *cmd, char *ptr);

extern char *cmdErase(nativeCommand *cmd, char *ptr);
extern char *cmdEraseAll(nativeCommand *cmd, char *ptr);
extern char *cmdStart(nativeCommand *cmd, char *ptr);
extern char *cmdLength(nativeCommand *cmd, char *ptr);
extern char *cmdBload(nativeCommand *cmd, char *ptr);
extern char *cmdBsave(nativeCommand *cmd, char *ptr);
extern char *cmdLoad(nativeCommand *cmd, char *ptr);
extern char *cmdSave(nativeCommand *cmd, char *ptr);
extern char *bankBankSwap(nativeCommand *cmd, char *ptr);
extern char *bankBGrab(nativeCommand *cmd, char *ptr);

extern char *bankResourceBank(nativeCommand *cmd, char *ptr);
extern char *bankResourceStr(nativeCommand *cmd, char *tokenBuffer);

extern void __load_bank__(const char *name, int bankNr );

extern char *getResourceStr(int id);



