#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "config.h"

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/retroMode.h>
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
#include "errors.h"
#include "engine.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern int current_screen;
extern struct retroSprite *sprite ;
extern struct retroSprite *icons ;
extern std::vector<struct kittyBank> kittyBankList;

void _my_print_text(struct retroScreen *screen, char *text, int maxchars);

const char *amos_file_ids[] =
	{
		"AmBk",		// work
		"",			// chip work 
		"AmIc",
		"AmSp",
		NULL
	};

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
	struct kittyBank *bank;

	for (n=0;n<kittyBankList.size();n++)
	{
		bank = &kittyBankList[n];
		if (bank)
		{
			if (bank->id == banknr)
			{
				return bank;
			}
		}
	}

	return NULL;
}

int findBankIndex( int banknr )
{
	unsigned int n;
	struct kittyBank *bank;

	for (n=0;n<kittyBankList.size();n++)
	{
		bank = &kittyBankList[n];
		if (bank)
		{
			if (bank->id == banknr)
			{
				return n;
			}
		}
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

#define bankCount()  kittyBankList.size()

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

					if (bank_is_object(bank,sprite)) sprite = NULL;
					retroFreeSprite( (struct retroSprite *) bank -> object_ptr );
					break;

				case bank_type_icons:

					if (bank_is_object(bank,icons)) icons = NULL;
					retroFreeSprite( (struct retroSprite *) bank -> object_ptr );
					break;
			}

			bank->start = NULL;
			bank->length = 0;
			bank->type = 0;
		}
		kittyBankList.erase(kittyBankList.begin()+index);
	}
}


char *_cmdErase( struct glueCommands *data, int nextToken )
{
	int bankNr;
	int args = stack - data->stack +1 ;

	if (args==1)
	{
		bankNr = getStackNum(data->stack);

		engine_lock();
		freeBank( bankNr );
		engine_unlock();
	}

	popStack( stack - data->stack );
	return NULL;
}

char *cmdErase(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdErase, tokenBuffer );
	return tokenBuffer;
}

extern void clean_up_banks();

char *_cmdEraseAll( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	if (args==1)
	{
		clean_up_banks();
	}
	else setError(22,data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *cmdEraseAll(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdEraseAll, tokenBuffer );
	return tokenBuffer;
}

char *_cmdStart( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	int ret = 0;
	struct kittyBank *bank = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		n = getStackNum(stack);
		if ( bank = findBank(n))	ret = (int) bank -> start;
	}

	if (bank == NULL) ret = 0;

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_cmdLength( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	struct kittyBank *bank;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		n = getStackNum(stack);
		if ( bank = findBank(n))	ret = (int) bank -> length;
	}

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *_cmdBload( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	struct kittyBank *bank;
	int args = stack - data->stack +1 ;
	FILE *fd;
	int size;
	int n;
	char *adr = NULL;

	dump_stack();

	if (args==2)
	{
		fd = fopen( getStackString( stack - 1 ) , "r");
		if (fd)
		{
			n = getStackNum(stack);

			fseek(fd , 0, SEEK_END );			
			size = ftell(fd);
			fseek(fd, 0, SEEK_SET );

			if (size)
			{
				freeBank(n);
				bank = allocBank(n);

				if (bank)
				{
					char *mem = (char *) malloc( size + 8 );

					bank -> length = size;
					if (bank -> start) free( (char *) bank -> start - 8 );

					bank -> start = mem ? mem+8 : NULL;
					bank -> type = 9;	
					adr = (char *)  bank -> start;
				}
				
				if (adr) fread( adr ,size,1, fd);
			}

			fclose(fd);
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdBsave( struct glueCommands *data, int nextToken )
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	FILE *fd;
	char *start, *to;

	if (args==3)
	{
		fd = fopen( getStackString( stack - 2 ) , "w");

		start = (char *) getStackNum(stack -1 );
		to = (char *) getStackNum( stack );

		if (fd)
		{
			if ((to-start)>0)
			{
				fwrite( start, to-start,1, fd );
			}
			fclose(fd);
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

struct kittyBank *__ReserveAs( int type, int bankNr, int length, const char *name, char *mem )
{
	struct kittyBank *bank;

	printf("%s:%s:%d - bank %d\n",__FILE__,__FUNCTION__,__LINE__, bankNr);

	freeBank( bankNr );
	bank = allocBank( bankNr );
	if (bank)
	{
		bank -> length = length;

		if (mem)
		{
			bank -> start = mem+8;
		}
		else
		{
			mem =  (char *) malloc( bank-> length + 8 );
			bank->start = mem ? mem+8 : NULL;
			if (mem) memset( mem , 0, bank->length + 8 );
		}

		if (bank->start) 
		{
			int n = 0;
			const char *ptr;
			char *dest = bank->start-8;

			for (ptr = bankTypes[type]; *ptr ; ptr++ )
			{
				dest[n]=*ptr;	n++;
			}

			while (n<8)
			{
				dest[n] = ' '; n++;
			}
		}

		bank->type = type;

		return bank;
	}

	return NULL;
}


char *_cmdReserveAsWork( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;

	if (args==2)
	{
		__ReserveAs( 1, getStackNum(stack-1) , getStackNum(stack), NULL, NULL );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsChipWork( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;

	if (args==2)
	{
		__ReserveAs( 0, getStackNum(stack-1) , getStackNum(stack), NULL, NULL );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsData( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;

	if (args==2)
	{
		__ReserveAs( 8 | 1, getStackNum(stack-1) , getStackNum(stack), NULL, NULL );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *_cmdReserveAsChipData( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;

	if (args==2)
	{
		__ReserveAs( 8 | 0, getStackNum(stack-1) , getStackNum(stack), NULL, NULL );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *cmdReserveAsWork(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdReserveAsWork, tokenBuffer );
	return tokenBuffer;
}

char *cmdReserveAsChipWork(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdReserveAsChipWork, tokenBuffer );
	return tokenBuffer;
}

char *cmdReserveAsData(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdReserveAsData, tokenBuffer );
	return tokenBuffer;
}

char *cmdReserveAsChipData(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdReserveAsChipData, tokenBuffer );
	return tokenBuffer;
}


extern bool next_print_line_feed;


char *cmdListBank(nativeCommand *cmd, char *tokenBuffer)
{
	unsigned int n = 0;
	char txt[1000];
	struct retroScreen *screen;
	struct kittyBank *bank = NULL;

	screen = screens[current_screen];

	if (screen)
	{
		clear_cursor( screen );
	
		if (next_print_line_feed) _my_print_text( screen, (char *) "\n", 0);

		_my_print_text( screen, (char *) "Nr   Type     Start       Length\n\n", 0);

		for (n=0;n<kittyBankList.size();n++)
		{
			bank = &kittyBankList[n];

			if (bank -> start)
			{
				sprintf(txt,"%2d - %.8s S:$%08X L:%d\n", 
					bank -> id,
					(char *) bank -> start-8,
					bank -> start, 
					bank -> length);

				_my_print_text( screen, txt, 0 );
			}
		}
		next_print_line_feed = true;
	}


	return tokenBuffer;
}


char *cmdStart(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _cmdStart, tokenBuffer );
	return tokenBuffer;
}

char *cmdLength(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _cmdLength, tokenBuffer );
	return tokenBuffer;
}

char *cmdBload(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdBload, tokenBuffer );
	return tokenBuffer;
}

char *cmdBsave(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdBsave, tokenBuffer );
	return tokenBuffer;
}

struct bankItemDisk
{
	unsigned short bank;
	unsigned short type;
	unsigned int length;
	char name[8];
} __attribute__((packed)) ;

void __save_work_data__(FILE *fd,int bankno,struct kittyBank *bank)
{
	struct bankItemDisk item;
	int type = bank -> type;
	uint32_t flags = 0;

	switch (type)
	{
		case 8:	type-=8;	flags = 0x80000000; break;
		case 9:	type-=8;	flags = 0x80000000; break;
	}

	item.bank = bankno;
	item.type = type;
	item.length = (bank -> length + 8) | flags;
	memcpy( item.name, bank->start-8, 8 );

	fwrite( &item, sizeof(struct bankItemDisk), 1, fd );
	fwrite( bank -> start, bank -> length, 1, fd );
}


void __load_work_data__(FILE *fd,int bank)
{
	struct bankItemDisk item;
	char *mem;

	if (fread( &item, sizeof(struct bankItemDisk), 1, fd )==1)
	{
		if (bank>0) item.bank = bank;

		if (item.length & 0x80000000) item.type += 8;
		item.length = (item.length & 0x7FFFFFF) -8;

		if (item.length >0 )
		{
			mem = (char *) malloc( item.length + 8);

			if (mem)
			{
				memset( mem, 0, item.length + 8 );
				fread( mem +8 , item.length, 1, fd );

				if (__ReserveAs( item.type, item.bank, item.length,NULL, mem ) == false) free(mem);
			}
		}
	}
}

void __load_work_data_mem__(struct retroMemFd &fd)
{
	struct bankItemDisk item;
	char *mem;

	if (mread( &item, sizeof(struct bankItemDisk), 1, fd )==1)
	{
		if (item.length & 0x80000000) item.type += 8;
		item.length = (item.length & 0x7FFFFFF) -8;

		if (item.length >0 )
		{
			mem = (char *) malloc( item.length + 8);

			if (mem)
			{
				memset( mem, 0, item.length + 8 );
				mread( mem +8 , item.length, 1, fd );

				if (__ReserveAs( item.type, item.bank, item.length,NULL, mem ) == false) free(mem);
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
	char id[5];
	unsigned short banks = 0;
	int n;
	int type = -1;
	struct kittyBank *bank = NULL;

	if (data)
	{
		fd.mem = data;
		fd.off = 0;
		fd.size = size;

				if (mread( &id, 4, 1, fd )==1)
				{	
					printf("ID: %c%c%c%c\n",id[0],id[1],id[2],id[3]);
					if (strcmp(id,"AmBs")==0)
					{
						mread( &banks, 2, 1, fd);
#ifdef __LITTLE_ENDIAN__
						banks = __bswap_16(banks);
#endif
					}
				}

				if (banks == 0) 
				{
					mseek( fd, 0, SEEK_SET );	// set set, to start no header found.
					banks = 1;
				}

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
							if (strcmp(id,*idp)==0) { type = cnt; break; }
							cnt++;
						}

						if (type == -1) 
						{
							printf("oh no!!... unexpected id: '%c%c%c%c'\n",id[0],id[1],id[2],id[3]);
							printf("ID: %c%c%c%c\n",id[0],id[1],id[2],id[3]);
							getchar();
						}
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
								sprite = retroLoadSprite( (void *) &fd, (cust_fread_t) hook_mread );
								engine_unlock();

								// 4 Bottles of beer. 
								if (bank = __ReserveAs( bank_type_sprite, 1, sizeof(void *),NULL, NULL))							
								{
									bank -> object_ptr = (char *) sprite;
								} 
								else
								{
									if (sprite) retroFreeSprite(sprite);
									sprite = NULL;
								}
							}
							break;
	
						case bank_type_icons:
							{
								freeBank( 2 );
								icons = retroLoadSprite( &fd, (cust_fread_t) hook_mread );

								// 99 Bottles of beer. 
								if (bank = __ReserveAs( bank_type_icons, 2, sizeof(void *),NULL, NULL ))
								{
									bank -> object_ptr = (char *) icons;
								}
								else
								{
									if (icons) retroFreeSprite(icons);
									icons = NULL;
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
	printf("--press enter--\n");
	getchar();
}


void __load_bank__(const char *name, int bankNr )
{
	FILE *fd;
	char id[5];
	unsigned short banks = 0;
	id[4]=0;
	int type = -1;
	int n;
	struct kittyBank *bank = NULL;

			fd = fopen( name , "r");
			if (fd)
			{
				if (fread( &id, 4, 1, fd )==1)
				{	
					if (strcmp(id,"AmBs")==0)
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
								int _bank = bankNr>-1 ? bankNr : 1;

								engine_lock();
								freeBank( _bank );
								sprite = retroLoadSprite(fd, (cust_fread_t) cust_fread );
								engine_unlock();

								if (bank = __ReserveAs( bank_type_sprite, _bank, sizeof(void *),NULL, NULL  ))	
								{
									bank -> object_ptr = (char *) sprite;
								} 
								else
								{
									if (sprite) retroFreeSprite(sprite);
									sprite = NULL;
								}
							}
							break;
	
						case bank_type_icons:
							{
								int _bank = bankNr>-1 ? bankNr : 2;

								freeBank( _bank );
								icons = retroLoadSprite(fd, (cust_fread_t) cust_fread );

								// 99 Bottles of beer. 
								if (bank = __ReserveAs( bank_type_icons, _bank, sizeof(void *),NULL, NULL ))
								{
									bank -> object_ptr = (char *) icons;
								}
								else
								{
									if (icons) retroFreeSprite(icons);
									icons = NULL;
								}
							}
							break;

						case bank_type_work_or_data:

							printf("we are here\n");

							__load_work_data__(fd,bankNr);
							break;

						default:
							printf("oh no!!... unexpected id: %s\n", id);
							Delay(120);

					}
				}

				fclose(fd);
			}
}

char *_cmdLoad( struct glueCommands *data, int nextToken )
{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;


	switch (args)
	{
		case 1:
			__load_bank__(getStackString( stack  ), -1 );
			break;

		case 2:
			__load_bank__( getStackString(stack-1), getStackNum(stack) );
			break;
	}

	popStack( stack - data->stack );
	return NULL;
}


char *cmdLoad(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdLoad, tokenBuffer );
	return tokenBuffer;
}

void __write_banks__( FILE *fd )
{
	int n=0;
	struct kittyBank *bank = NULL;

	for (n=0;n<kittyBankList.size();n++)
	{
		bank = &kittyBankList[n];

		if (bank->start)
		{
			switch (bank->type)
			{
				case type_ChipWork:
				case type_FastWork:
				case type_ChipData:
				case type_FastData:

						fwrite("AmBk",4,1,fd);
						__save_work_data__(fd,n+1,bank);
						break;
/*
				case type_Music:
				case type_Amal:
				case type_Samples:
				case type_Menu:
				case type_Code:
				case type_Icons:
				case type_Sprites:
						break;
*/

				default: printf("can't save bank, not supported yet\n");
			}
		}
	}
}

char *_cmdSave( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	FILE *fd;
	char *filename = NULL;
	int banknr = 0;

	dump_stack();

	switch (args)
	{
		case 1:

			filename = getStackString( stack );

			fd = fopen( filename , "w");
			if (fd)
			{
				__write_ambs__( fd, bankCount() );
				__write_banks__(fd);
				fclose(fd);
			}
			break;

		case 2:

			filename = getStackString( stack - 1 );
			banknr = getStackNum( stack );

			fd = fopen( filename , "w");

			if (fd)
			{
				fclose(fd);
			}

			break;

		default:

			setError(22, data -> tokenBuffer );
	}

	popStack( stack - data->stack );
	return NULL;
}

char *cmdSave(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdSave, tokenBuffer );
	return tokenBuffer;
}

char *_bankBGrab( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;

	printf("Not yet working, sorry only a dummy command.\n");

	popStack( stack - data->stack );
	return NULL;
}


char *bankBGrab(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _bankBGrab, tokenBuffer );
	return tokenBuffer;
}

char *_bankBankSwap( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	int args = stack - data->stack +1 ;
	int b1,b2;

	struct kittyBank *bank1;
	struct kittyBank *bank2;
	
	switch (args)
	{
		case 2:	b1 = getStackNum(stack-1);
				b2 = getStackNum(stack);

				bank1 = findBank(b1);
				bank2 = findBank(b2);

				if (bank1)
				{
					bank1 -> id = b2;
				}

				if (bank2)	bank2 -> id = b1;
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *bankBankSwap(nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _bankBankSwap, tokenBuffer );
	return tokenBuffer;
}


