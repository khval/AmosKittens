
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"
#include "commands.h"

extern char *findLabel( char *name );
extern char *dupRef( struct reference *ref );

int var_type_is( struct reference *ref, int mask )
{
	return ref -> flags & mask;
}

char *var_JumpToName(struct reference *ref)
{
	char *ptr = NULL;
	char *name = dupRef(ref);
	if (name) ptr = findLabel( name );
	return ptr;
}

