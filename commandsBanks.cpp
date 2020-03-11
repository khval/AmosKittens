#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "config.h"

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/retroMode.h>
#include <amosKittens.h>
#endif

#ifdef __linux__
#include <retromode.h>
#include <retromode_lib.h>
#include <unistd.h>
#endif

#include "debug.h"
#include <string>
#include <vector>
#include <iostream>
#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsBanks.h"
#include "kittyErrors.h"
#include "engine.h"
#include "bitmap_font.h"
#include "amosString.h"
#include "cleanup.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern struct retroSprite *patterns;

extern std::vector<struct kittyBank> kittyBankList;

#define bank_header 8

const char *amos_file_ids[] =
	{
		"AmBk",		// work
		"",			// chip work 
		"AmIc",
		"AmSp",
		NULL
	};


const char *bankTypes[] = {
	"ChipWork",		// 0
	"FastWork",		// 1
	"Icons",			// 2
	"Sprites",			// 3
	"Music",			// 4
	"Amal",			// 5
	"Samples",		// 6
	"Menu",			// 7
	"ChipData",		// 8
	"FastData",		// 9
	"Code"
};

void *getBankObject(int id)
{
	unsigned int n;
	for (n=0;n<kittyBankList.size();n++)
	{
		if (id == kittyBankList[n].id) return kittyBankList[n].object_ptr;
	}

	return NULL;
}

extern void makeMaskForAll();

void update_objects()
{
	patterns = (struct retroSprite *) getBankObject( - 3 );

	instance.sprites = (struct retroSprite *) getBankObject( 1 );
	instance.icons = (struct retroSprite *) getBankObject( 2 );

	makeMaskForAll();
}

int hook_mread( char *dest, int size, int e, struct retroMemFd *fd )
{
	void *ret = NULL;

	if (fd->off + (size*e) <= fd->size)
	{
		ret = memcpy( dest, (fd->mem+fd->off), (size*e) );
		if (ret)
		{
			fd->off += (size*e);
		} else printf("memcpy failed\n");
	} else printf("%d <= %d\n",fd->off + (size*e), fd->size);
	return ret ? e : 0;
}

#define mread( dest, size, e, fd ) hook_mread( (char *) dest, size, e, &fd )

int mseek( struct retroMemFd &fd, int off, unsigned mode )
{
	switch (mode)
	{
		case SEEK_SET:
			fd.off = off;
			return 0;
	}

	return 1;
}

struct kittyBank *findBank( int banknr )
{
	unsigned int n;

	for (n=0;n<kittyBankList.size();n++)
	{
		if (kittyBankList[n].id == banknr)	return &kittyBankList[n];
	}

	return NULL;
}

int findBankIndex( int banknr )
{
	unsigned int n;
	for (n=0;n<kittyBankList.size();n++)
	{
		if (kittyBankList[n].id == banknr) return n;
	}

	return -1;
}

struct kittyBank * allocBank( int banknr ) 
{
	struct kittyBank _bank;
	_bank.id = banknr;
	_bank.start = NULL;
	_bank.length = 0;
	kittyBankList.push_back(_bank );
	return findBank( banknr );
}

int bankCount(int opt) 
{
	int ret = 0;

	switch (opt)
	{
		case -1:
			{
				unsigned int n = 0;
				for (n=0;n<kittyBankList.size();n++)
				{
					if (kittyBankList[n].id<0) ret++;
				}
			}
			break;
		case 0:
			ret = kittyBankList.size();
			break;
		case 1:
			{
				unsigned int n = 0;
				for (n=0;n<kittyBankList.size();n++)
				{
					if (kittyBankList[n].id>0) ret++;
				}
			}
			break;
	}
	return ret;	
}

bool bank_is_object( struct kittyBank *bank, void *ptr);


void freeBank( int banknr )
{
	int index;
	struct kittyBank *bank = NULL;

	index = findBankIndex( banknr );

	if (index>-1)
	{
		bank = &kittyBankList[index];
		if (bank)
		{
			switch (bank -> type)
			{
				case bank_type_sprite:

					if (bank_is_object(bank,instance.sprites)) instance.sprites = NULL;
					retroFreeSprite( (struct retroSprite *) bank -> object_ptr );
					break;

				case bank_type_icons:

					if (bank_is_object(bank,instance.icons)) instance.icons = NULL;
					retroFreeSprite( (struct retroSprite *) bank -> object_ptr );
					break;
			}

			bank -> object_ptr = NULL;
			bank->start = NULL;
			bank->length = 0;
			bank->type = 0;
		}

		kittyBankList.erase(kittyBankList.begin()+index);
	}
}


char *_bankErase( struct glueCommands *data, int nextToken )
{
	int bankNr;
	int args = __stack - data->stack +1 ;

	if (args==1)
	{
		bankNr = getStackNum(data->stack);

		engine_lock();
		freeBank( bankNr );
		engine_unlock();
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *bankErase(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _bankErase, tokenBuffer );
	return tokenBuffer;
}

char *_bankEraseAll( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack +1 ;

	if (args==1)
	{
		clean_up_user_banks();
	}
	else setError(22,data->tokenBuffer);

	popStack(__stack - data->stack );
	return NULL;
}

char *bankEraseAll(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _bankEraseAll, tokenBuffer );
	return tokenBuffer;
}


char *bankEraseTemp(nativeCommand *cmd, char *tokenBuffer)
{
	unsigned int n;
	bool erased;
	struct kittyBank *bank = NULL;

	do
	{
		erased = false;
		for (n=0;n<kittyBankList.size();n++)
		{
			bank = &kittyBankList[n];

			switch (bank -> type)
			{
				case 0:
				case 1:
					engine_lock();
					freeBank( n );
					engine_unlock();
					erased = true;
					break;
			}
			if (erased) break;
		}
	} while (erased);

	return tokenBuffer;
}

char *_bankStart( struct glueCommands *data, int nextToken )
{
	int n;
	int args = __stack - data->stack +1 ;
	int ret = 0;
	struct kittyBank *bank = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		n = getStackNum(__stack);
		if ( bank = findBank(n))	ret = (int) bank -> start;
	}

	if (bank == NULL) ret = 0;

	popStack(__stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_bankLength( struct glueCommands *data, int nextToken )
{
	int n;
	int args = __stack - data->stack +1 ;
	struct kittyBank *bank;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		n = getStackNum(__stack);
		if ( bank = findBank(n))	ret = (int) bank -> length;
	}

	popStack(__stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_bankBload( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = __stack - data->stack +1 ;
	FILE *fd;
	int size;

	if (args==2)
	{
		struct stringData *name = getStackString(__stack - 1 );

		fd = fopen( &name -> ptr , "r");
		if (fd)
		{
			uint32_t bankid_or_address;
			struct kittyBank *bank;

			fseek(fd , 0, SEEK_END );			
			size = ftell(fd);
			fseek(fd, 0, SEEK_SET );

			if (size)
			{
				bankid_or_address = getStackNum(__stack);
				bank = findBank( bankid_or_address );	// bank must be previously reserved 

				if (bank)
				{
					fread( bank -> start  ,size,1, fd);
				}
				else if (bankid_or_address != 0)
				{
					fread( (void *) bankid_or_address  ,size,1, fd);
				}
				else setError(22, data -> tokenBuffer );
			}

			fclose(fd);
		}
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *_bankBsave( struct glueCommands *data, int nextToken )
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = __stack - data->stack +1 ;
	FILE *fd;
	char *start, *to;

	if (args==3)
	{
		struct stringData *name = getStackString(__stack - 2 );	

		fd = fopen( &name -> ptr , "w");

		start = (char *) getStackNum(__stack -1 );
		to = (char *) getStackNum(__stack );

		if (fd)
		{
			if ((to-start)>0)
			{
				fwrite( start, to-start,1, fd );
			}
			fclose(fd);
		}
	}

	popStack(__stack - data->stack );
	return NULL;
}

struct kittyBank *reserveAs( int type, int bankNr, int length, const char *name, char *mem )
{
	struct kittyBank *bank;

	freeBank( bankNr );
	bank = allocBank( bankNr );
	if (bank)
	{
		bank -> length = length;

		if (mem)
		{
			bank -> start = mem+bank_header;
		}
		else
		{
			mem =  (char *) sys_priv_alloc( bank-> length + bank_header );
			bank->start = mem ? mem+bank_header : NULL;
			if (mem) memset( mem , 0, bank->length + bank_header );
		}

		if (bank->start) 
		{
			int n = 0;
			const char *ptr;
			char *dest = bank->start-bank_header;

			if (name)
			{
				int tl = strlen( name );
				memset( dest, ' ', 8 );	// fill mem with space.
				memcpy( dest, name, tl > 8 ? 8 : tl );
			}
			else
			{
				for (ptr = bankTypes[type]; *ptr ; ptr++ )
				{
					dest[n]=*ptr;	n++;
				}

				while (n<8)
				{
					dest[n] = ' '; n++;
				}
			}
		}

		bank->type = type;

		return bank;
	}

	return NULL;
}


char *__bankReserveAsWork( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = __stack - data->stack +1 ;

	if (args==2)
	{
		reserveAs( 1, getStackNum(__stack-1) , getStackNum(__stack), NULL, NULL );
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *__bankReserveAsChipWork( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = __stack - data->stack +1 ;

	if (args==2)
	{
		reserveAs( 0, getStackNum(__stack-1) , getStackNum(__stack), NULL, NULL );
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *__bankReserveAsData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = __stack - data->stack +1 ;

	if (args==2)
	{
		reserveAs( 8 | 1, getStackNum(__stack-1) , getStackNum(__stack), NULL, NULL );
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *__bankReserveAsChipData( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = __stack - data->stack +1 ;

	if (args==2)
	{
		reserveAs( 8 | 0, getStackNum(__stack-1) , getStackNum(__stack), NULL, NULL );
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *bankReserveAsWork(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( __bankReserveAsWork, tokenBuffer );
	return tokenBuffer;
}

char *bankReserveAsChipWork(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( __bankReserveAsChipWork, tokenBuffer );
	return tokenBuffer;
}

char *bankReserveAsData(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( __bankReserveAsData, tokenBuffer );
	return tokenBuffer;
}

char *bankReserveAsChipData(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( __bankReserveAsChipData, tokenBuffer );
	return tokenBuffer;
}

extern bool next_print_line_feed;

char *bankListBank(nativeCommand *cmd, char *tokenBuffer)
{
	unsigned int n = 0;
	char txt[1000];
	struct retroScreen *screen;
	struct kittyBank *bank = NULL;
	screen = instance.screens[instance.current_screen];
	bool has_banks = false;

	if (screen)
	{
		clear_cursor( screen );

		for (n=0;n<kittyBankList.size();n++)
		{
			if (kittyBankList[n].id>=0)
			{
				has_banks = true;
				break;
			}
		}

		if (has_banks)
		{
			if (next_print_line_feed) _my_print_text( screen, (char *) "\n", 0, false, false, false,0,0);

			_my_print_text( screen, (char *) "Nr   Type     Start       Length\n\n", 0, false, false, false,0,0);

			for (n=0;n<kittyBankList.size();n++)
			{
				bank = &kittyBankList[n];

				if ((bank->id>-1)&&(bank -> start))
				{
					sprintf(txt,"%2d - %.8s S:$%08X L:%d\n", 
						bank -> id,
						(char *) bank -> start-8,
						bank -> start, 
						bank -> length);	

					_my_print_text( screen, txt, 0, false, false, false,0,0); 
				}
			}
		}
		next_print_line_feed = true;
	}

	return tokenBuffer;
}


char *bankStart(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _bankStart, tokenBuffer );
	return tokenBuffer;
}

char *bankLength(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _bankLength, tokenBuffer );
	return tokenBuffer;
}

char *bankBload(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _bankBload, tokenBuffer );
	return tokenBuffer;
}

char *bankBsave(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _bankBsave, tokenBuffer );
	return tokenBuffer;
}

struct bankItemDisk
{
	uint16_t bank;
	uint16_t type;
	uint32_t length;
	char name[8];
} __attribute__((packed)) ;

void __save_work_data__(FILE *fd,struct kittyBank *bank)
{
	struct bankItemDisk item;
	int type = bank -> type;
	uint32_t flags = 0;

	switch (type)
	{
		case 8:	type-=8;	flags = 0x80000000; break;
		case 9:	type-=8;	flags = 0x80000000; break;
	}

	item.bank = bank->id;
	item.type = type;
	item.length = (bank -> length + 8) | flags;
	memcpy( item.name, bank->start-8, 8 );

#ifdef __LITTLE_ENDIAN__
		item.bank = __bswap_16(item.bank);
		item.type = __bswap_16(item.type);
		item.length = __bswap_32(item.length);
#endif

	fwrite( &item, sizeof(struct bankItemDisk), 1, fd );
	fwrite( bank -> start, bank -> length, 1, fd );
}


void __load_work_data__(FILE *fd,int bank)
{
	struct bankItemDisk item;
	char *mem;

	if (fread( &item, sizeof(struct bankItemDisk), 1, fd )==1)
	{
#ifdef __LITTLE_ENDIAN__
		item.bank = __bswap_16(item.bank);
		item.type = __bswap_16(item.type);
		item.length = __bswap_32(item.length);
#endif

		if (item.length & 0x80000000) item.type+=bank_header;
		item.length = (item.length & 0x7FFFFFF)-bank_header;

		if (item.length >0 )
		{
			mem = (char *) sys_priv_alloc( item.length+bank_header);

			if (mem)
			{
				memset( mem, 0, item.length+bank_header );
				fread( mem+bank_header , item.length, 1, fd );

				if (bank != -1)
				{
					if (reserveAs( item.type, bank, item.length,item.name, mem ) == false) free(mem);
				}
				else
				{
					if ( strncasecmp( item.name , "Samples",7) == 0) item.bank=5;
					if (reserveAs( item.type, item.bank, item.length,item.name, mem ) == false) free(mem);
				}
			}
		}
	}

	update_objects();
}

void __load_work_data_mem__(struct retroMemFd &fd)
{
	struct bankItemDisk item;
	char *mem;

	if (mread( &item, sizeof(struct bankItemDisk), 1, fd )==1)
	{

#ifdef __LITTLE_ENDIAN__
		item.length = __bswap_32(item.length);
#endif

		if (item.length & 0x80000000) item.type+= 8;
		item.length = (item.length & 0x7FFFFFF)-bank_header;

		if (item.length >0 )
		{
			mem = (char *) sys_priv_alloc( item.length+bank_header);

			if (mem)
			{
				memset( mem, 0, item.length+bank_header );
				mread( mem+bank_header , item.length, 1, fd );

				if (reserveAs( item.type, item.bank, item.length,NULL, mem ) == false) free(mem);
			}
		}
	}
}

// callback.

int cust_fread (void *ptr, int size,int elements, FILE *fd)
{
	if (ptr)
	{
		return fread(ptr,size,elements,fd);
	}
	else return 0;
}

int cust_fwrite (void *ptr, int size,int elements, FILE *fd)
{
	if (ptr)
	{
		return fwrite(ptr,size,elements,fd);
	}
	else return 0;
}

extern void clean_up_banks();

bool bank_is_object( struct kittyBank *bank, void *ptr)
{
	if (ptr) 
	{
		if (bank -> object_ptr == ptr ) 
		{
			return true;
		}
	}
	return false;
}

void __write_ambs__( FILE *fd, uint16_t banks)
{
	char id[4]={'A','m','B','s'};

	fwrite( id, 4,1, fd );
	fwrite( &banks, 2,1, fd );
}

void init_banks( char *data , int size)
{
	struct retroMemFd fd;
	char id[6];
	unsigned short banks = -1;
	int n;
	int type = -1;
	struct kittyBank *bank = NULL;
	id[4]=0;	// null terminate id string.

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (data)
	{
		fd.mem = data;
		fd.off = 0;
		fd.size = size;

		if (mread( &id, 4, 1, fd )==1)
		{	
			printf("ID: %c%c%c%c\n",id[0],id[1],id[2],id[3]);
			if (strncmp(id,"AmBs",4)==0)
			{
				printf("file offset = %d\n", fd.off);

				mread( &banks, 2, 1, fd);
#ifdef __LITTLE_ENDIAN__
				banks = __bswap_16(banks);
#endif
			}
		}

		switch (banks)
		{
			case -1:		// No number of banks header.
						mseek( fd, 0, SEEK_SET );	// set set, to start no header found.
						banks = 1;
						break;

			case 0:		// No banks.
						printf("no banks\n");
						return;
		}

		printf ("ready to read %d banks\n",banks);

		for (n=0;n<banks;n++)
		{
			type = -1;
			printf("bank %d of %d\n",n+1,banks);

			if (mread( &id, 4, 1, fd )==1)
			{	
				int cnt = 0;
				const char **idp;

				for (idp = amos_file_ids; *idp ; idp++)
				{
					if (strncmp(id,*idp,4)==0) { type = cnt; break; }
					cnt++;
				}

				if (type == -1) 
				{
					printf("oh no!!... unexpected id: '%c%c%c%c'\n",id[0],id[1],id[2],id[3]);
					printf("ID: %c%c%c%c\n",id[0],id[1],id[2],id[3]);
					getchar();
				}
				else printf("ID: %c%c%c%c\n",id[0],id[1],id[2],id[3]);
			}
			else 
			{
				// clear id.
				memset(id,' ',4);
			}

			switch (type)
			{
				case bank_type_sprite:
					{
						engine_lock();
						freeBank( 1 );
						if (bank = reserveAs( bank_type_sprite, 1, 0,NULL, NULL))							
						{
							if (bank -> object_ptr = (char *) retroLoadSprite( &fd, (cust_fread_t) hook_mread ))
							{
								struct retroSprite *sprite = (struct retroSprite *) bank -> object_ptr;	// local
								bank -> length = sprite -> number_of_frames;
							}
						} 
						engine_unlock();
					}
					break;

				case bank_type_icons:
					{
						freeBank( 2 );
						if (bank = reserveAs( bank_type_icons, 2, 0,NULL, NULL ))
						{
							if (bank -> object_ptr = (char *) retroLoadSprite( &fd, (cust_fread_t) hook_mread ))
							{
								struct retroSprite *sprite = (struct retroSprite *) bank -> object_ptr;	// local
								bank -> length = sprite -> number_of_frames;
							}
						}
					}
					break;

				case bank_type_work_or_data:
					__load_work_data_mem__(fd);
					break;

				default:
					n = banks; // exit for loop.
			}
		}
	}

	update_objects();
}


void __load_bank__(struct stringData *name, int bankNr )
{
	FILE *fd;
	char id[5];
	unsigned short banks = 0;
	id[4]=0;
	int type = -1;
	int n;
	struct kittyBank *bank = NULL;

	fd = fopen( &name -> ptr , "r");
	if (fd)
	{
		if (fread( &id, 4, 1, fd )==1)
		{	
			if (strncmp(id,"AmBs",4)==0)
			{
				fread( &banks, 2, 1, fd);
				clean_up_banks();		
			}
		}

		if (banks == 0) 
		{
			fseek( fd, 0, SEEK_SET );	// set set, to start no header found.
			banks = 1;
		}

		for (n=0;n<banks;n++)
		{
			type = -1;

			if (fread( &id, 4, 1, fd )==1)
			{	
				int cnt = 0;
				const char **idp;

				for (idp = amos_file_ids; *idp ; idp++)
				{
					if (strcmp(id,*idp)==0) { type = cnt; break; }
					cnt++;
				}
			}
			else
			{
				memset( id, ' ',4 );
			}
			
			switch (type)
			{
				case bank_type_sprite:
					{
						int _bank = (bankNr != -1) ? bankNr : 1;
						engine_lock();
						freeBank( _bank );

						if (bank = reserveAs( bank_type_sprite, _bank, sizeof(void *),NULL, NULL  ))	
						{
							if (bank -> object_ptr = (char *) retroLoadSprite(fd, (cust_fread_t) cust_fread ))
							{
								struct retroSprite *sprite = (struct retroSprite *) bank -> object_ptr;	// local
								bank -> length = sprite -> number_of_frames;
							}
						} 
						engine_unlock();
					}
					break;
	
				case bank_type_icons:
					{
						int _bank = bankNr != -1 ? bankNr : 2;
						freeBank( _bank );
						instance.icons = retroLoadSprite(fd, (cust_fread_t) cust_fread );

						// 99 Bottles of beer. 
						if (bank = reserveAs( bank_type_icons, _bank, sizeof(void *),NULL, NULL ))
						{
							bank -> object_ptr = (char *) instance.icons;
						}
						else
						{
							if (instance.icons) retroFreeSprite(instance.icons);
							instance.icons = NULL;
						}
					}
					break;

				case bank_type_work_or_data:
					__load_work_data__(fd,bankNr);
					break;

				default:
					printf("oh no!!... unexpected id: %s\n", id);
					Delay(120);
			}
		}

		fclose(fd);
	}

	update_objects();
}

void __load_bank__(char *_name, int bankNr )
{
	struct stringData *name = toAmosString(_name,strlen(_name));
	
	if (name)
	{
		__load_bank__(name, bankNr);
		freeString(name);
	}
}


char *_bankLoad( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = __stack - data->stack +1 ;


	switch (args)
	{
		case 1:
			__load_bank__( getStackString(__stack  ) , -1 );
			break;

		case 2:
			__load_bank__( getStackString(__stack-1), getStackNum(__stack) );
			break;
	}

	popStack(__stack - data->stack );
	return NULL;
}


char *bankLoad(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _bankLoad, tokenBuffer );
	return tokenBuffer;
}


void __save_sprite_data__(FILE *fd, cust_fread_t cust_fwrite, struct kittyBank *bank)
{
	if (bank == NULL) return;
	instance.sprites = (struct retroSprite *) bank -> object_ptr;
	if (instance.sprites == NULL)  return;

	retroSaveSprite( fd, instance.sprites, cust_fwrite );
}


void __write_bank__( FILE *fd, int bankid )
{
	struct kittyBank *bank = findBank( bankid );

	if (bank)
	{
		if (bank->start)
		{
			switch (bank->type)
			{
				case type_ChipWork:
				case type_FastWork:
				case type_ChipData:
				case type_FastData:

						fwrite("AmBk",4,1,fd);
						__save_work_data__(fd,bank);
						break;

				case type_Sprites:

						fwrite("AmSp",4,1,fd);
						__save_sprite_data__(fd, (cust_fread_t) cust_fwrite, bank);
						break;

				case type_Icons:

						fwrite("AmIc",4,1,fd);
						__save_sprite_data__(fd, (cust_fread_t) cust_fwrite, bank);
						break;

/*
				case type_Music:
				case type_Amal:
				case type_Samples:
				case type_Menu:
				case type_Code:
						break;
*/

				default: printf("can't save bank, not supported yet\n");
			}
		}
	}
}

void __write_banks__( FILE *fd )
{
	unsigned int n=0;
	struct kittyBank *bank = NULL;

	for (n=0;n<kittyBankList.size();n++)
	{
		bank = &kittyBankList[n];
		if (bank) if (bank -> id>0) __write_bank__( fd, bank -> id );
	}
}

char *_bankSave( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = __stack - data->stack +1 ;
	FILE *fd;
	struct stringData *filename = NULL;
	int banknr = 0;

	switch (args)
	{
		case 1:

			filename = getStackString(__stack );

			fd = fopen( &filename -> ptr , "w");
			if (fd)
			{
				__write_ambs__( fd, bankCount(1) );
				__write_banks__(fd);
				fclose(fd);
			}
			break;

		case 2:

			filename = getStackString(__stack - 1 );
			banknr = getStackNum(__stack );

			fd = fopen( &filename -> ptr , "w");
			if (fd)
			{
				__write_bank__(fd,banknr);
				fclose(fd);
			}
			break;

		default:

			setError(22, data -> tokenBuffer );
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *bankSave(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _bankSave, tokenBuffer );
	return tokenBuffer;
}

char *_bankBGrab( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	NYI(__FUNCTION__);

	popStack(__stack - data->stack );
	return NULL;
}


char *bankBGrab(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _bankBGrab, tokenBuffer );
	return tokenBuffer;
}

char *_bankBankSwap( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = __stack - data->stack +1 ;
	int b1,b2;

	struct kittyBank *bank1;
	struct kittyBank *bank2;
	
	switch (args)
	{
		case 2:	b1 = getStackNum(__stack-1);
				b2 = getStackNum(__stack);

				bank1 = findBank(b1);
				bank2 = findBank(b2);

				if (bank1)
				{
					bank1 -> id = b2;
				}

				if (bank2)	bank2 -> id = b1;
				
				// update active data objects.

				engine_lock();

				instance.icons = NULL;
				instance.sprites = NULL;

				bank1 = findBank(1);
				if (bank1) if (bank1 -> type == type_Sprites ) instance.sprites = (struct retroSprite *) bank1 -> object_ptr;

				bank1 = findBank(2);
				if (bank1) if (bank1 -> type == type_Icons ) instance.sprites = (struct retroSprite *) bank1 -> object_ptr;

				engine_unlock();

				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *bankBankSwap(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _bankBankSwap, tokenBuffer );
	return tokenBuffer;
}


char *_bankResourceBank( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = __stack - data->stack +1 ;

	switch (args)
	{
		case 1:	current_resource_bank = getStackNum(__stack);
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *bankResourceBank(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _bankResourceBank, tokenBuffer );
	return tokenBuffer;
}

const char *AmosKittensSystem = "AmosKittens:System/";

const char *DefaultFileNames[] =
{
	"DefaultFileNames0",	// 0		(-1 to -9)
	"DefaultFileNames1",	// 1
	"DefaultFileNames2",	// 2
	"DefaultFileNames3",	// 3
	"DefaultFileNames4",	// 4
	"DefaultFileNames5",	// 5
	"DefaultFileNames6",	// 6
	"DefaultFileNames7",	// 7
	"DefaultFileNames8",	// 8
	NULL
};


struct stringData *getResourceStr(int id)
{
	struct stringData *ret = NULL;
	int retry = 0;
	int cbank = current_resource_bank;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (id>0)
	{
		struct kittyBank *bank1;

		do
		{
			bank1 = findBank(cbank);

			if (bank1)
			{
				struct resourcebank_header *header = (resourcebank_header*) bank1->start;

				if (header -> var_offset)
				{
					unsigned char *pos = (unsigned char *) bank1->start;
					unsigned short len;

					pos += header -> var_offset; 

					for(;;)
					{
						len = *( (unsigned short *) pos );
						pos+=2;

						if (len == 255) break;
	
						if (id == 1) 
						{
							if (len>0) ret = toAmosString( (const char *) pos,len);
							break;
						}

						pos+=len;
						id--;
					}			
				}
			}

			if (ret) break;

			cbank = -2;		// try default resource if not found
			retry++;

		} while ( retry < 2 );
	}
	else if (id == 0)
	{
		ret = toAmosString(AmosKittensSystem, strlen(AmosKittensSystem));
	}
	if ((id >=-1 )&&(id <=-9))	// Default file names
	{
		ret = toAmosString( DefaultFileNames[ (-id)-1], strlen(DefaultFileNames[ (-id)-1]) );
	}
	else if ((id >=-10 )&&(id <=-36))	// name of extentions
	{

	}

	if (ret==NULL)  ret = toAmosString( "",0 );

	return ret;
}

char *_bankResourceStr( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack +1 ;
	int id;
	struct stringData *ret = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	id = getStackNum(__stack);
				ret = getResourceStr( id );
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );

	if (ret)
	{
		setStackStr( ret );
	}
	else 
	{
		struct stringData tmp;
		setStackStrDup( &tmp );
	}

	return NULL;
}

char *bankResourceStr(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _bankResourceStr, tokenBuffer );
	return tokenBuffer;
}

char *_bankBankShrink( struct glueCommands *data, int nextToken )
{
	int args = __stack - data->stack +1 ;
	int banknr,size;
	struct stringData *ret = NULL;
	struct kittyBank *bank = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:	banknr = getStackNum(__stack-1);
				size = getStackNum(__stack);

				bank = findBank( banknr );
				if (bank)
				{
					char *_new = (char *) sys_priv_alloc( bank_header+size );

					if (_new)
					{
						char *_old = bank -> start - bank_header;
						int min_size = (bank -> length<size) ? bank->length : size;

						memcpy(_new,_old,bank_header+min_size);
						bank -> start = _new - bank_header;
						free(_old);
						bank -> length = size;
					}
				}
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );

	if (ret)
	{
		setStackStr( ret );
	}
	else toAmosString( "",0 );


	return NULL;
}

char *bankBankShrink(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdNormal( _bankBankShrink, tokenBuffer );
	return tokenBuffer;
}

char *_bankBlength( struct glueCommands *data, int nextToken )
{
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	NYI(__FUNCTION__);

	popStack(__stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *bankBlength(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdParm( _bankBlength, tokenBuffer );
	return tokenBuffer;
}

char *_bankBstart( struct glueCommands *data, int nextToken )
{
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	NYI(__FUNCTION__);

	popStack(__stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *bankBstart(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdParm( _bankBstart, tokenBuffer );
	return tokenBuffer;
}

char *_bankBsend( struct glueCommands *data, int nextToken )
{
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	NYI(__FUNCTION__);

	popStack(__stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *bankBsend(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	stackCmdParm( _bankBsend, tokenBuffer );
	return tokenBuffer;
}

