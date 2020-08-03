
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


