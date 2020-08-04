
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <vector>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/retroMode.h>
#include <amosKittens.h>
#endif

#include "debug.h"
#include "bank_helper.h"
#include "amosString.h"
#include "load_config.h"

extern std::vector<struct kittyBank> kittyBankList;
extern struct retroSprite *patterns;

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

struct kittyBank *firstBank()
{
	int n=0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	while (n<kittyBankList.size())
	{
		if (kittyBankList[n].id > 0) 	return &kittyBankList[n];
		n++;
	}
	return NULL;
}

struct kittyBank * allocBank( int banknr ) 
{
	struct kittyBank _bank;
	_bank.id = banknr;
	_bank.start = NULL;
	_bank.length = 0;
	kittyBankList.push_back(_bank );
	return findBankById( banknr );
}

struct kittyBank *findBankById( int banknr )
{
	unsigned int n;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (n=0;n<kittyBankList.size();n++)
	{
		if (kittyBankList[n].id == banknr)	return &kittyBankList[n];
	}

	return NULL;
}

struct kittyBank *findBankByIndex( int index )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (index<-1) return NULL;
	if ( (unsigned int) index>=kittyBankList.size()) return NULL;
	return &kittyBankList[index];
}

int findBankIndex( int banknr )
{
	unsigned int n;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (n=0;n<kittyBankList.size();n++)
	{
		if (kittyBankList[n].id == banknr) return n;
	}

	return -1;
}

int getBankListSize()
{
	return kittyBankList.size();
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

void update_objects()
{
	patterns = (struct retroSprite *) getBankObject( - 3 );

	instance.sprites = (struct retroSprite *) getBankObject( 1 );
	instance.icons = (struct retroSprite *) getBankObject( 2 );

	makeMaskForAll();
}

void freeBank( int banknr )
{
	int index;
	struct kittyBank *bank = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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
		bank->object_ptr=NULL;

		return bank;
	}

	return NULL;
}

struct stringData *get_default_resource_str( const char *group, int id )
{
	char tmp[30];
	std::string *value;
	
	sprintf( tmp, "%s_%d", group, id );
	value = getConfigValue( tmp );

	if (value)
	{
		const char *cs = value -> c_str();
		return toAmosString(cs, strlen(cs));
	}

	return NULL;
}


struct stringData *getResourceStr(int id)
{

	int retry = 0;
	int cbank = instance.current_resource_bank;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (id>0)
	{
		struct stringData *ret = NULL;
		struct kittyBank *bank1;

		do
		{
			bank1 = findBankById(cbank);

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

		return ret;
	}

	// --------------------------------------------------------------------------------------------------
	//   Doc's in AmosPro is are wrong: says -10 to -36 is extensions names, 
	//   But extention names is from -14 to -40
	//   Using the same command for different things kind of stupid :-(
	// -------------------------------------------------------------------------------------------------

	if ((id >=-13)&&(id <=0 ))	// Default file names
	{
		return get_default_resource_str( "resource", -id );
	}
	else if ((id >=-36 )&&(id <=-14))	// name of extentions
	{
		return get_default_resource_str( "extension", (-id) -13 );
	}

	return NULL;
}
