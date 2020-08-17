#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>

#include <amosKittens.h>
#include <stack.h>

#include <proto/retroMode.h>
#include "amoskittens.h"
#include "commandsbanks.h"
#include "commands.h"
#include "engine.h"
#include "amalcompiler.h"
#include "channel.h"
#include "debug.h"
#include "commandsDevice.h"
#include "commandsLibs.h"


extern std::vector<struct retroSpriteObject *> bobs;;
extern std::vector<struct label> labels;

extern struct retroScreen *screens[8] ;
extern struct globalVar globalVars[1000];
extern int var_count[2];
extern char *dir_first_pattern ;
extern struct retroSprite *sprite ;
extern struct retroSprite *icons ;
extern ChannelTableClass *channels;
extern struct retroBlock *cursor_block;

extern std::vector<struct amosMenuItem *> menuitems;
extern std::vector<struct amos_selected> amosSelected;
extern std::vector<struct defFn> defFns;
extern std::vector<struct kittyBank> kittyBankList;
extern std::vector<struct kittyDevice> deviceList;
extern std::vector<struct kittyLib> libsList;


void clean_up_defFns()
{
	struct defFn *item;

	while (defFns.size())
	{
		if (item = &defFns[0])
		{
			if (item -> name) free (item -> name);
			item -> name = NULL;
		}
		
		defFns.erase( defFns.begin() );
	}
}

void clean_up_menus()
{
	struct amosMenuItem *item;

	while (menuitems.size())
	{
		if (item = menuitems[0])
		{
			menuitems[0] = NULL;
			if (item -> str) free (item -> str);
			item -> str = NULL;
			if (item -> key) free (item -> key);
			item -> key = NULL;
			freeStruct(item);
		}
		
		menuitems.erase( menuitems.begin() );
	}
}



void free_local_var(struct kittyData *var)
{
	switch (var->type)
	{
		case type_int:
			var -> integer.value = 0;
			break;

		case type_float:
			var -> decimal.value = 0;
			break;

		case type_string:
			if (var->str) freeString(var->str);
			var->str = NULL;
			break;

		case type_int | type_array:
		case type_float | type_array:
		case type_string | type_array:

			if (var -> sizeTab) freeStruct( var -> sizeTab);
	 		if (var->str) freeString (var->str);
			var -> sizeTab = NULL;
			var->str = NULL;
			break;
	}
}

void setup_local_var(struct kittyData *var)
{
	switch (var->type)
	{
		case type_int:
			var -> integer.value = 0;
			break;

		case type_float:
			var -> decimal.value = 0;
			break;

		case type_string:
			if (var->str) freeString(var->str);
			var->str = NULL;
			break;

		case type_int | type_array:
		case type_float | type_array:
		case type_string | type_array:

			if (var -> sizeTab) freeStruct( var -> sizeTab);
	 		if (var->str) freeString (var->str);
			var -> sizeTab = NULL;
			var->str = NULL;
			break;
	}
}


#warning wont clean up local vars

void clean_up_vars()
{
	struct kittyData *var;
	int n;

	for (n=0;n<var_count[0];n++)
	{
		if (globalVars[n].varName) 
		{
			//printf("globalVars[%d].varName='%s'\n",n,globalVars[n].varName);

			free(globalVars[n].varName);
			globalVars[n].varName = NULL;
		}

		var = &globalVars[n].var;

		switch (var -> type)
		{
			case type_string:

 				if (var->str) freeString (var->str);
				break;

			case type_int | type_array:
			case type_float | type_array:
			case type_string | type_array:

				// free
				if (var -> sizeTab) freeStruct( var -> sizeTab);
 				if (var->str) sys_free(var->str);
				break;
		}

		// reset
		var -> sizeTab = NULL;
		var->str = NULL;
	}

	var_count[0] = 0;
}

void clean_up_stack()
{
	popStack( instance_stack );
	printf("after clean up stack is: %d\n", instance_stack);
	setStackNone();
}

void clean_up_files()
{
	int n;
	for (n=0;n<10;n++)
	{
		if (instance.files[n].fd) fclose(instance.files[n].fd);
		if (instance.files[n].fields) free(instance.files[n].fields);

		instance.files[n].fd = NULL;
		instance.files[n].fields = NULL;
	}
}

extern void freeBank( int banknr );

void clean_up_banks()
{
	while (kittyBankList.size())
	{
		freeBank( kittyBankList[0].id );
	}
}

void clean_up_devices()
{
	while (deviceList.size())
	{
		kFreeDevice( deviceList[0].id );
	}
}

void clean_up_libs()
{
	while (libsList.size())
	{
		kFreeLib( libsList[0].id );
	}
}

struct kittyBank *get_first_user_bank()
{
	unsigned int n;

	for (n=0; n<kittyBankList.size();n++)
	{
		if (kittyBankList[n].id > 0) return &kittyBankList[n];
	}

	return NULL;
}

void clean_up_user_banks()
{
	struct kittyBank *userBank = NULL;

	while ( userBank = get_first_user_bank())
	{
		freeBank( userBank -> id );
	}
}

void clean_up_special()
{
	cleanup_printf("clean up libs\n");

	clean_up_libs();

	dprintf("clean up devices\n");

	clean_up_devices();

	cleanup_printf("clean up defFns\n");

	clean_up_defFns();

	cleanup_printf("clean up menus\n");

	clean_up_menus();

	cleanup_printf("clean up channels!!\n");

	if (channels) 
	{
		delete channels;
		channels = NULL;
	}

	if (IRetroMode)
	{
		if (cursor_block)
		{
			retroFreeBlock(cursor_block);
			cursor_block = NULL;
		}

		cleanup_printf("clean up bobs!!\n");

		engine_lock();

		cleanup_printf("bobs %d\n", bobs.size());

		while ( bobs.size() )
		{
			cleanup_printf("bob[0] is %08x\n",bobs[0]);
			if (bobs[0])
			{
				retroFreeSpriteObject( bobs[0],TRUE);		// TRUE = only data
			}
			bobs.erase(bobs.begin());
		}
		engine_unlock();
	}

	cleanup_printf("clean up banks!!\n");

	clean_up_banks();

	cleanup_printf("clean up contextDir\n");

	if (contextDir)
	{
		ReleaseDirContext(contextDir);
		contextDir = NULL;
	}

	cleanup_printf("clean up dir first pattern\n");

	if (dir_first_pattern)
	{
		free(dir_first_pattern);
		dir_first_pattern = NULL;
	}

	cleanup_printf("clean up zones\n");

	if (instance.zones)
	{
		freeStruct(instance.zones);
		instance.zones = NULL;
	}
}

