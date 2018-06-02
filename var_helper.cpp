
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"
#include "commands.h"

extern char *findLabel( char *name );
int findVar( char *name, int type, int _proc );

extern int findVarPublic( char *name, int type );
extern int findProc( char *name );

extern char *dupRef( struct reference *ref );

int var_type_is( struct reference *ref, int mask )
{
	return ref -> flags & mask;
}

char *var_JumpToName(struct reference *ref)
{
	char *ptr = NULL;
	char *name = dupRef(ref);
	if (name) 
	{
		ptr = findLabel( name );
		free(name);
	}
	return ptr;
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

