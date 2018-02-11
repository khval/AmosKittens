
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
extern int nativeCommandsSize;
extern struct nativeCommand nativeCommands[];


int findVar( char *name )
{
	int n;

	for (n=1;n<sizeof(globalVars)/sizeof(struct globalVar);n++)
	{
		if (globalVars[n].varName == NULL) return 0;

		if (strcasecmp( globalVars[n].varName, name)==0)
		{
			return n;
		}
	}
	return 0;
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
				case 0x0026:	ret += QuoteByteLength(ptr); break;	// skip strings.

				case 0x0006:	pass1var(  ptr );
							ret += ReferenceByteLength(ptr); 
							break;
			}

			ret += cmd -> size;
			return ret;
		}
	}

	printf("'%20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
					__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token);	

	return NULL;
}

