#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <label.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif

#include "amosKittens.h"
#include "commands.h"

int findVar( char *name, int type, int _proc );
extern int findVarPublic( char *name, int type );
extern int findProc( char *name );
extern char *dupRef( struct reference *ref );

extern struct globalVar globalVars[1000];
extern int global_var_count;

int var_type_is( struct reference *ref, int mask )
{
	return ref -> flags & mask;
}

struct label *var_JumpToName(struct reference *ref)
{
	char *name = dupRef(ref);
	if (name) 
	{
		struct label *label = findLabel( name, procStcakFrame[proc_stack_frame].id );
		free(name);
		return label;
	}

	return NULL;
}

int var_find_proc_ref(struct reference *ref)
{
	int ret = 0;
	char *name = dupRef(ref);
	if (name) 
	{
		ret = findProc( name );
		free(name);
	}
	return ret;
}


int findProcAndFix( struct globalVar *toFind )
{
	int n;
	struct globalVar *var;
	if (toFind -> varName == NULL) return -1;

	for (n=0;n<global_var_count;n++)
	{
		var = &globalVars[n];

		if ( (var->varName != NULL) && (var->var.type == type_proc) )
		{
			if ( strcasecmp( var->varName, toFind -> varName ) == 0 )
			{
				toFind -> var.type = var -> var.type;
				toFind -> var.tokenBufferPos = var -> var.tokenBufferPos;
			}
		}
	}
}

void validate_and_fix_globals()
{
	int n;

	for (n=0;n<global_var_count;n++)
	{
		if (globalVars[n].varName == NULL) return;

		switch (globalVars[n].var.type)
		{
			case type_int:
				findProcAndFix( &globalVars[n] );
				break;

		}
	}
}


