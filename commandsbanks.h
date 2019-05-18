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

enum
{
	type_ChipWork,	// 0
	type_FastWork,	// 1
	type_Icons,		// 2
	type_Sprites,		// 3
	type_Music,		// 4
	type_Amal,		// 5
	type_Samples,		// 6
	type_Menu,		// 7
	type_ChipData,	// 8
	type_FastData,	// 9
	type_Code
};

struct resourcebank_header
{
	uint16_t chunks;
	uint32_t img_offset;
	uint32_t var_offset;		 // 001A
	uint32_t script_offset;	 // 002E
	uint32_t gadget_offset;	 // 0000
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

extern char *bankReserveAsWork(nativeCommand *cmd, char *ptr);
extern char *bankReserveAsChipWork(nativeCommand *cmd, char *ptr);
extern char *bankReserveAsData(nativeCommand *cmd, char *ptr);
extern char *bankReserveAsChipData(nativeCommand *cmd, char *ptr);
extern char *bankListBank(nativeCommand *cmd, char *ptr);

extern char *bankErase(nativeCommand *cmd, char *ptr);
extern char *bankEraseAll(nativeCommand *cmd, char *ptr);
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

extern void __load_bank__(const char *name, int bankNr );

extern char *getResourceStr(int id);
extern void *getBankObject(int id);



