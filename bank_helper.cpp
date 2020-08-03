
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

extern std::vector<struct kittyBank> kittyBankList;
extern struct retroSprite *patterns;

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

