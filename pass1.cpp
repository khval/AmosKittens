
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "amosKittens.h"
#include "commands.h"
#include "debug.h"
#include <vector>

const char *types[]={"","#","$",""};

extern struct globalVar globalVars[1000];	// 0 is not used.
extern int global_var_count;
extern int globalVarsSize;
extern int nativeCommandsSize;
extern struct nativeCommand nativeCommands[];

extern std::vector<struct label> labels;

enum
{
	nested_if,
	nested_then,
	nested_else
};

struct nested
{
	int cmd;
	char *ptr;
};

struct nested nested_command[ 1000 ];
int nested_count = 0;

#define addNest( enum_cmd ) \
	nested_command[ nested_count ].cmd = enum_cmd; \
	nested_command[ nested_count ].ptr = ptr; \
	nested_count++; 


char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2 );

int findVar( char *name )
{
	int n;

	for (n=1;n<global_var_count;n++)
	{
		if (globalVars[n].varName == NULL) return 0;

		if (strcasecmp( globalVars[n].varName, name)==0)
		{
			return n;
		}
	}
	return 0;
}

char *findLabel( char *name )
{
	int n;

	for (n=0;n<labels.size();n++)
	{
		if (strcasecmp( labels[n].name, name)==0)
		{
			return labels[n].tokenLocation;
		}
	}
	return NULL;
}

int QuoteByteLength(char *ptr)
{
	unsigned short length = *((unsigned short *) ptr);
	length += (length & 1);		// align to 2 bytes

	printf("length %d\n",length);

	return length;
}

int ReferenceByteLength(char *ptr)
{
	struct reference *ref = (struct reference *) ptr;
	unsigned short length = ref -> length;
	length += (length & 1);		// align to 2 bytes

	printf("length %d\n",length);

	return length;
}

void pass1var(char *ptr)
{
	char *tmp;
	int found = 0;
	struct reference *ref = (struct reference *) ptr;
	struct kittyData *var;

	tmp = (char *) malloc( ref->length + 2 );
	if (tmp)
	{
		tmp[ ref->length -2 ] =0;
		tmp[ ref->length -1 ] =0;

		memcpy(tmp, ptr + sizeof(struct reference), ref->length );
		sprintf(tmp + strlen(tmp),"%s", types[ ref -> flags & 3 ] );

		found = findVar(tmp);
		if (found)
		{
			free(tmp);		//  don't need tmp
			ref -> ref = found;
		}
		else
		{
			global_var_count ++;
			ref -> ref = global_var_count;

			globalVars[global_var_count].varName = tmp;	// tmp is alloced and used here.

			var = &globalVars[global_var_count].var;
			var->type = ref -> flags & 3;
			var->len = 0;
			if (var -> type == type_string) var->str = strdup("");
		}

		// we should not free tmp, see code above.
	}
}

void pass1label(char *ptr)
{
	char *tmpName;
	char *at;
	struct reference *ref = (struct reference *) ptr;
	struct kittyData *var;
	char *next;

	tmpName = strndup( ptr + sizeof(struct reference), ref->length  );
	if (tmpName)
	{
		printf("%s\n",tmpName);

		at = findLabel(tmpName);
		if (at)
		{
			free(tmpName);		//  don't need tmp
			ref -> ref = 1;		// don't care, we have the address in lookup table
		}
		else
		{
			label tmp;
			next = ptr + sizeof(struct reference) + ref->length;

			// skip all new lines..
			while ( (*(unsigned short *) next) == 0x0000 ) next += 4;	// token and newline code

			tmp.name = tmpName;
			tmp.tokenLocation = next ;

			labels.push_back(tmp);
			ref -> ref = labels.size();
		}

		// we should not free tmp, see code above.
	}
}


char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2 )
{
	struct nativeCommand *cmd;
	unsigned short current_token = *((unsigned short *) ptr);
	int token_size;
	
	while (  (current_token  != token) && (current_token != token_eof1 ) && (current_token != token_eof2 ) )
	{
		token_size = 2;

		for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
		{
			if (token == cmd->id ) { token_size += cmd -> size; break; }
		}

		switch (token)
		{
			case 0x0006:	token_size += ReferenceByteLength(ptr); break;
			case 0x000C:	token_size += ReferenceByteLength(ptr); break;
			case 0x0012:	token_size += ReferenceByteLength(ptr); break;
			case 0x0018:	token_size += ReferenceByteLength(ptr); break;
			case 0x0026:	token_size += QuoteByteLength(ptr); break;
			case 0x002E:	token_size += QuoteByteLength(ptr); break;
		}

		ptr += token_size;
	}	

	if ( current_token == token)
	{
		return ptr;
	}

	return 0;
}

void eol( char *ptr )
{
	if (nested_count>0)
	{
		switch (nested_command[ nested_count -1 ].cmd )
		{
			// IF can end at EOL if then is not there. (command THEN should replace nested_if )

			case nested_if:
				printf("%04x\n",*((short *) (nested_command[ nested_count -1 ].ptr - 2)));
				*((short *) (nested_command[ nested_count -1 ].ptr)) =(short) ((int) (ptr - nested_command[ nested_count -1 ].ptr)) / 2 ;
				nested_count --;
				break;
		}
	}
}

void pass1_end_if( char *ptr )
{
	printf("we are here\n");

	if (nested_count>0)
	{

		printf("%d\n",nested_command[ nested_count -1 ].cmd);

		switch (nested_command[ nested_count -1 ].cmd )
		{
			case nested_then:
			case nested_else:

				printf("%04x\n",*((short *) (nested_command[ nested_count -1 ].ptr - 2)));


				*((short *) (nested_command[ nested_count -1 ].ptr)) =(short) ((int) (ptr - nested_command[ nested_count -1 ].ptr)) / 2 ;
				nested_count --;
				break;

			default:

				printf("Error: End If, with out Else or Then\n");
		}
	}
}

char *nextToken_pass1( char *ptr, unsigned short token )
{
	struct nativeCommand *cmd;
	char *ret;
	unsigned short length;

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id ) 
		{
			printf("'%20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
						__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token);

			ret = ptr;

			switch (token)
			{
				case 0x0000:	eol( ptr );	break;

				case 0x0006:	pass1var(  ptr );
							ret += ReferenceByteLength(ptr); 
							break;

				case 0x000c:	pass1label( ptr );
							ret += ReferenceByteLength(ptr); 
							break;

				case 0x02BE:	addNest( nested_if );
							break;

				case 0x02C6:	if (nested_count>0) 	// command THEN
								if (nested_command[ nested_count -1 ].cmd == nested_if )
									nested_command[ nested_count -1 ].cmd = nested_then;
							break;

				case 0x02D0:	
							printf("we are here\n");
							addNest( nested_else );
							break;

				case 0x02DA:	pass1_end_if( ptr );
 							break;


				case 0x0026:	ret += QuoteByteLength(ptr); break;	// skip strings.
			}

			ret += cmd -> size;
			return ret;
		}
	}

	printf("'%20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
					__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token);	

	return NULL;
}

