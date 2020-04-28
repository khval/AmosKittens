#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <label.h>
#include <vector>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/types.h>
#include <proto/retroMode.h>
#endif


#include "amosKittens.h"
#include "commands.h"

int findVar( char *name, int type, int _proc );
extern int findVarPublic( char *name, int type );
extern int findProc( char *name );
extern char *dupRef( struct reference *ref );

extern struct stackFrame procStcakFrame[PROC_STACK_SIZE];
extern struct globalVar globalVars[1000];
extern int var_count[2];
extern std::vector<struct label> labels;
extern std::vector<struct  globalVar *> procedures;

const char *types[]={"","#","$",""};

const char *type_names[]=
{
	"int",
	"float",
	"string",
	"file",
	"proc int",
	"proc float",
	"proc string",
	"proc file",
	"int[]",
	"float[]",
	"string[]",
	"file[]",
	"invalid",
	"invaild",
	"inavild",
	"invaild",
	"none",
	NULL
};

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
	unsigned int n;
	struct globalVar *var;
	if (toFind -> varName == NULL) return -1;

	for (n=0;n<procedures.size();n++)
	{
		var = procedures[n];

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

int findProc( char *name )
{
	unsigned int n;
	struct globalVar *var;

	for (n=0;n<procedures.size();n++)
	{
		var = procedures[n];

		if (var -> varName == NULL) return 0;

		if ( strcasecmp( var -> varName, name)==0) 
		{
			return (unsigned int) (var - globalVars) +1;
		}
	}
	return 0;
}


void validate_and_fix_globals()
{
	int n;

	for (n=0;n<var_count[0];n++)
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

int ReferenceByteLength(char *ptr)
{
	struct reference *ref = (struct reference *) ptr;
	unsigned short length = ref -> length;

	length += (length & 1);		// align to 2 bytes
	return length;
}

int QuoteByteLength(char *ptr)
{
	unsigned short length = *((unsigned short *) ptr);
	length += (length & 1);		// align to 2 bytes
	return length;
}

char *dupRef( struct reference *ref )
{
	char *tmp = (char *) malloc( ref->length + 2 );
	if (tmp)
	{
		memcpy(tmp, ((char *) ref) + sizeof(struct reference), ref->length );
			tmp[ ref->length ] =0;
			tmp[ ref->length + 1 ] =0;
		sprintf(tmp + strlen(tmp),"%s", types[ ref -> flags & 3 ] );
	}
	return tmp;
}

struct label *findLabel( char *name, int _proc )
{
	unsigned int n;
	struct label *label;

	for (n=0;n<labels.size();n++)
	{
		label = &labels[n];

		if (label -> proc == _proc)
		{
			if (strcasecmp( label -> name, name)==0)
			{
				return label;
			}
		}
	}

	return NULL;
}

int findLabelRef( char *name, int _proc )
{
	unsigned int n;
	struct label *label;

	printf("%s(%s,%d)\n",__FUNCTION__,name,_proc);

	for (n=0;n<labels.size();n++)
	{
		label = &labels[n];

		if (label -> proc == _proc)
		{
			if (strcasecmp( label -> name, name)==0)
			{
				return n+1;
			}
		}
	}
	return 0;
}

