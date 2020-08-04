
#define bank_type_work_or_data 0
#define bank_type_icons 2
#define bank_type_sprite 3

extern void makeMaskForAll();

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
	type_Code,		// 10
	type_num_of_banks
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

#define bank_header 8

void freeBank( int banknr );
void update_objects();
int bankCount(int opt); 
int getBankListSize();
int findBankIndex( int banknr );
struct kittyBank *findBankByIndex( int index );
struct kittyBank *findBankById( int banknr );
struct kittyBank * allocBank( int banknr ) ;
struct kittyBank *firstBank();
void *getBankObject(int id);
bool bank_is_object( struct kittyBank *bank, void *ptr);
struct kittyBank *reserveAs( int type, int bankNr, int length, const char *name, char *mem );
struct stringData *get_default_resource_str( const char *group, int id );
struct stringData *getResourceStr(int id);

