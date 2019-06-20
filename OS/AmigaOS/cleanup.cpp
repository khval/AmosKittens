#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "stack.h"
#include "amosKittens.h"
#include <vector>
#include <proto/retroMode.h>
#include "commandsbanks.h"
#include "amalcompiler.h"
#include "channel.h"

extern struct retroScreen *screens[8] ;
extern struct retroSpriteObject bobs[64];
extern struct globalVar globalVars[1000];
extern std::vector<struct label> labels;
extern int global_var_count;
extern char *dir_first_pattern ;
extern struct retroSprite *sprite ;
extern struct retroSprite *icons ;
extern ChannelTableClass *channels;
extern struct retroBlock *cursor_block;

extern std::vector<struct kittyBank> kittyBankList;


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
	unsigned int n;
	for (n=0;n<kittyBankList.size();n++)
	{
		freeBank(n);
	}
}

void clean_up_special()
{
	int n;

	printf("should clean up menus here, don't forget me\n");

	if (cursor_block)
	{
		retroFreeBlock(cursor_block);
		cursor_block = NULL;
	}

	printf("clean up channels!!\n");

	if (channels) 
	{
		delete channels;
		channels = NULL;
	}

	printf("clean up bobs!!\n");

	for (n=0;n<64;n++)
	{
		retroFreeSpriteObject( &bobs[n],TRUE);		// TRUE = only data
	}

	printf("clean up banks!!\n");

	clean_up_banks();

	printf("clean up contextDir\n");

	if (contextDir)
	{
		ReleaseDirContext(contextDir);
		contextDir = NULL;
	}

	printf("clean up dir first pattern");

	if (dir_first_pattern)
	{
		free(dir_first_pattern);
		dir_first_pattern = NULL;
	}

	printf("clean up zones\n");

	if (zones)
	{
		free(zones);
		zones = NULL;
	}
}

