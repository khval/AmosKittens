#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#include "stack.h"
#include "amosKittens.h"
#include <vector>
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
extern int global_var_count;
extern char *dir_first_pattern ;
extern struct retroSprite *sprite ;
extern struct retroSprite *icons ;
extern ChannelTableClass *channels;
extern struct retroBlock *cursor_block;

extern std::vector<struct amosMenuItem *> menuitems;
extern std::vector<struct amos_selected> amosSelected;
extern std::vector<struct defFn> defFns;
extern std::vector<struct kittyBank> kittyBankList;
extern std::vector<struct wave *> waves;
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
			free(item);
		}
		
		menuitems.erase( menuitems.begin() );
	}
}


void clear_local_vars( int proc )
{
	int n;
	struct kittyData *var;

	for (n=0;n<global_var_count;n++)
	{
		if (globalVars[n].proc == proc)
		{
			var = &globalVars[n].var;

			switch (var->type)
			{
				case type_int:
					var -> integer.value = 0;
					break;

				case type_float:
					var -> decimal.value = 0;
					break;

				case type_string:
					if (var->str) free(var->str);
					var->str = NULL;
					break;

				case type_int | type_array:
				case type_float | type_array:
				case type_string | type_array:

					if (var -> sizeTab) free( var -> sizeTab);
	 				if (var->str) free (var->str);
					var -> sizeTab = NULL;
					var->str = NULL;
					break;

			}
		}
	}
}

void clean_up_vars()
{
	struct kittyData *var;
	int n;

	for (n=0;n<global_var_count;n++)
	{
		if (globalVars[n].varName) 
		{
			free(globalVars[n].varName);
			globalVars[n].varName = NULL;
		}

		var = &globalVars[n].var;

		switch (var -> type)
		{
			case type_string:
 				if (var->str) free (var->str);
				break;

			case type_int | type_array:
			case type_float | type_array:
			case type_string | type_array:

				// free
				if (var -> sizeTab) free( var -> sizeTab);
 				if (var->str) free (var->str);
				break;
		}

		// reset
		var -> sizeTab = NULL;
		var->str = NULL;
	}

	global_var_count = 0;
}

void clean_up_stack()
{
	int n;

	for (n=0; n<=stack;n++)
	{
		switch( kittyStack[n].type )
		{		
			case type_string:
				if (kittyStack[n].str) free (kittyStack[n].str);
				kittyStack[n].str = NULL;
				kittyStack[n].type = 0;			
				break;
		}
	}
	stack = 0;
}

void clean_up_files()
{
	int n;
	for (n=0;n<10;n++)
	{
		if (kittyFiles[n].fd) fclose(kittyFiles[n].fd);
		if (kittyFiles[n].fields) free(kittyFiles[n].fields);

		kittyFiles[n].fd = NULL;
		kittyFiles[n].fields = NULL;
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

void clean_up_waves()
{
	while (waves.size())
	{
		if( waves[0]) free( waves[0] );
		waves.erase(waves.begin() );
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
	dprintf("clean up libs\n");

	clean_up_libs();

	dprintf("clean up devices\n");

	clean_up_devices();

	dprintf("clean up defFns\n");

	clean_up_defFns();

	dprintf("clean up menus\n");

	clean_up_menus();

	dprintf("clean up channels!!\n");

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

		dprintf("clean up bobs!!\n");

		engine_lock();

		printf("bobs %d\n", bobs.size());

		while ( bobs.size() )
		{
			printf("bob[0] is %08x\n",bobs[0]);
			if (bobs[0])
			{
				retroFreeSpriteObject( bobs[0],TRUE);		// TRUE = only data
			}
			bobs.erase(bobs.begin());
		}
		engine_unlock();
	}

	dprintf("clean up banks!!\n");

	clean_up_banks();

	dprintf("clean up contextDir\n");

	if (contextDir)
	{
		ReleaseDirContext(contextDir);
		contextDir = NULL;
	}

	dprintf("clean up dir first pattern\n");

	if (dir_first_pattern)
	{
		free(dir_first_pattern);
		dir_first_pattern = NULL;
	}

	dprintf("clean up zones\n");

	if (zones)
	{
		free(zones);
		zones = NULL;
	}

	dprintf("clean up waves\n");

	clean_up_waves();
}

