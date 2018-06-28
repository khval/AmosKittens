#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "stack.h"
#include "amosKittens.h"
#include <vector>
#include <proto/retroMode.h>
#include "commandsbanks.h"

extern struct globalVar globalVars[1000];
extern std::vector<struct label> labels;
extern int global_var_count;
extern char *dir_first_pattern ;
extern struct retroSprite *sprite ;

void clean_up_vars()
{
	int n;
	int i;

	for (n=0;n<global_var_count;n++)
	{
		if (globalVars[n].varName) 
		{
			free(globalVars[n].varName);
			globalVars[n].varName = NULL;
		}

		switch (globalVars[n].var.type)
		{
			case type_string:
			case type_int | type_array:
			case type_float | type_array:
			case type_string | type_array:

				// its a union, so any array or string will be freed.

 				if (globalVars[n].var.str) free (globalVars[n].var.str);
				globalVars[n].var.str = NULL;
				break;
		}
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

void clean_up_special()
{
	int n;

	printf("clean up banks!!");

	for (n=0;n<15;n++)
	{
		if (kittyBanks[n].start)
		{
			switch ( kittyBanks[n].type )
			{
//				case bank_type_icons:
//						break;

				case bank_type_sprite:
						retroFreeSprite( (struct retroSprite *) kittyBanks[n].start );
						sprite = NULL;
						break;

				default:
						free( kittyBanks[n].start );
						break;
			}

			kittyBanks[n].start = NULL;
		}
	}

	if (contextDir)
	{
		ReleaseDirContext(contextDir);
		contextDir = NULL;
	}

	if (dir_first_pattern)
	{
		free(dir_first_pattern);
		dir_first_pattern = NULL;
	}
}

