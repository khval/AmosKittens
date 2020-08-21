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
#include "amosString.h"
#include "var_helper.h"

int findVar( char *name, int type, int _proc );
extern int findVarPublic( char *name, int type );
extern int findProcByName( char *name );
extern char *dupRef( struct reference *ref );

extern struct globalVar globalVars[1000];
extern struct stackFrame *currentFrame;

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
		struct label *label = findLabel( name, currentFrame -> id );
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
		ret = findProcByName( name );
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

int findProcByName( char *name )
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

struct globalVar *findProcPtrById( int proc )
{
	unsigned int n;
	struct globalVar *var;

	for (n=0;n<procedures.size();n++)
	{
		var = procedures[n];

		if (  var -> proc == proc ) 
		{
			return var;
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

struct kittyData *getVar(uint16_t ref)
{
	if (ref & 0x8000)
	{
		return currentFrame -> localVarData + ((ref & 0x7FFF) -1);
	}

	return &globalVars[ref-1].var;
}

void get_procedures()
{
	int n;
	for (n=0;n<var_count[0];n++)
	{
		if (globalVars[n].var.type == type_proc)	procedures.push_back( globalVars +n );
	}
}

void add_str_var(const char *_name,const char *_value)
{
	char *name;
	struct globalVar *var;
	struct reference ref;

	name = strdup(_name);
	if (name)
	{
		var = add_var_from_ref( &ref, &name, type_string );
		if (var)
		{
			if (var -> var.str) sys_free(var -> var.str);
			var -> var.str = toAmosString(_value,strlen(_value));
		}
	}
}

struct globalVar *add_var_from_ref( struct reference *ref, char **tmp, int type )
{
	struct globalVar *_new = NULL;

	if ( var_count[0] < VAR_BUFFERS )
	{
		var_count[0] ++;

		ref -> ref = var_count[procStackCount ? 1: 0];

		if (type == type_proc)
		{
			ref -> flags = (ref->flags&3) | type;
		}
		else
		{
			ref -> flags = type;
		}

		_new = &globalVars[var_count[0]-1];
		_new -> varName = *tmp;	// tmp is alloced and used here.
		_new -> var.type = type;
		_new -> localIndex = var_count[procStackCount];

		if (type != type_proc) if (procStackCount) var_count[1]++;

		if (_new -> var.type == type_string) _new -> var.str = toAmosString("",0);

		*tmp = NULL;
	}

	return _new;
}

int findVarPublic( char *name, int type )
{
	int n;

	for (n=0;n<var_count[0];n++)
	{
		if (globalVars[n].varName == NULL) return 0;

		if ((strcasecmp( globalVars[n].varName, name)==0)
			&& (globalVars[n].var.type == type)
			&& (globalVars[n].proc == 0)
			&& (globalVars[n].isGlobal == FALSE))
		{
			return n+1;
		}
	}
	return 0;
}

struct kittyData *findPublicVarByName( char *name, int type )
{
	int n;

	for (n=0;n<var_count[0];n++)
	{
		if (globalVars[n].varName == NULL) return 0;

		if ((strcasecmp( globalVars[n].varName, name)==0)
			&& (globalVars[n].var.type == type)
			&& (globalVars[n].proc == 0))
		{
			return &globalVars[n].var;
		}
	}
	return NULL;
}

bool str_var_is( struct kittyData *var, const char *value )
{
	if (var == NULL) return false;
	if (var->type != type_string) return false;

	if (strcasecmp( &(var->str->ptr), value ) == 0)	return true;
	return false;
}
