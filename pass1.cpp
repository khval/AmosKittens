
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "amosKittens.h"
#include "commands.h"
#include "debug.h"
#include <vector>
#include "AmosKittens.h"
#include "errors.h"

const char *types[]={"","#","$",""};

extern struct globalVar globalVars[1000];	// 0 is not used.
extern int global_var_count;
extern int globalVarsSize;
extern int nativeCommandsSize;
extern struct nativeCommand nativeCommands[];

extern std::vector<struct label> labels;

#define LAST_TOKEN_(name) ((nested_count>0) && (nested_command[ nested_count -1 ].cmd == nested_ ## name ))

int currentLine = 0;

int procCount = 0;

enum
{
	nested_if,
	nested_then,
	nested_then_else,
	nested_else,
	nested_while,
	nested_proc
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

// find Public variables not defined as global

int findVarPublic( char *name )
{
	int n;

	for (n=0;n<global_var_count;n++)
	{
		printf("globalVars[%d].varName=%s\n",n,globalVars[n].varName);

		if (globalVars[n].varName == NULL) return 0;

		if ((strcasecmp( globalVars[n].varName, name)==0)
			&& (globalVars[n].proc == 0)
			&& (globalVars[n].isGlobal == FALSE))
		{
			return n+1;
		}
	}
	return 0;
}

int findVar( char *name, int _proc )
{
	int n;

	for (n=0;n<global_var_count;n++)
	{
		printf("globalVars[%d].varName=%s\n",n,globalVars[n].varName);

		if (globalVars[n].varName == NULL) return 0;

		if ((strcasecmp( globalVars[n].varName, name)==0) && 
			(
				(globalVars[n].proc == _proc) || 
				(globalVars[n].pass1_shared_to == _proc) ||
				(globalVars[n].isGlobal)
			))
		{
			return n+1;
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

void add_var_from_ref( struct reference *ref, char *tmp, bool is_proc )
{
	struct kittyData *var;

	global_var_count ++;
	ref -> ref = global_var_count;

	globalVars[global_var_count-1].varName = tmp;	// tmp is alloced and used here.

	var = &globalVars[global_var_count-1].var;
	var->type = is_proc ? type_proc : ref -> flags & 7;
	var->len = 0;
	if (var -> type == type_string) var->str = strdup("");
}

void pass1var(char *ptr, bool is_proc )
{
	char *tmp;
	int found = 0;
	struct reference *ref = (struct reference *) ptr;
	struct kittyData *var;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	tmp = dupRef( ref );
	if (tmp)
	{
		printf("looking for %s %d\n", tmp, procCount);

		found = findVar(tmp, is_proc ? 0 : procCount);
		if (found)
		{
			free(tmp);		//  don't need tmp
			ref -> ref = found;
		
			if (is_proc)
			{
				printf("proc change %\n",found);

				var = &globalVars[found-1].var;
				var -> type = type_proc;
				var -> tokenBufferPos = ptr + sizeof(struct reference) + ref -> length ;

				globalVars[found-1].proc = is_proc ? 0 : procCount;
			}
			else printf("not changed\n");
		}
		else
		{
			if (is_proc)
			{
				add_var_from_ref( ref, tmp, is_proc );
				globalVars[global_var_count-1].proc = procCount;

				var = &globalVars[global_var_count-1].var;
				var -> type = type_proc;
				var -> tokenBufferPos = ptr + sizeof(struct reference) + ref -> length ;
			}
			else
			{
				add_var_from_ref( ref, tmp, is_proc );
				globalVars[global_var_count-1].proc = procCount;
			}
		}

		// we should not free tmp, see code above.
	}
}

void next_var_should_be_proc_type( char *ptr )
{
	short token = *((short *) ptr);
//	struct reference *ref = (struct reference *) (ptr+2);

	if (token == 0x0006) pass1var(  ptr+2, true );
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

char *pass1_shared( char *ptr )
{
	unsigned short token = *((unsigned short *) ptr);
	struct reference *ref;
	char *tmp;
	int var;

	// we only support two tokens as arguments for shared.

	token = *((unsigned short *) ptr);

	while ( (token != 0x0000) && (token != 0x0054) && ( kittyError.code == 0) )
	{
		switch (token)
		{
			case 0x0006: 	ref = (struct reference *) (ptr+2);
						tmp = dupRef( ref );

						if (tmp)
						{
							var = findVarPublic(tmp);
							if (var) 
							{
								globalVars[var-1].pass1_shared_to = procCount;
								ref->ref = var;
							}
							free(tmp);
						}

						ptr += sizeof(struct reference *) + ref -> length;

						break;
			case 0x005C: break;

			default: 
						setError(1);
		}

		ptr+=2;	// next token
		token = *((unsigned short *) ptr);
		printf("token %04x\n", token);
		getchar();
	}
	return ptr;
}

char *pass1_global( char *ptr )
{
	unsigned short token = *((unsigned short *) ptr);
	struct reference *ref;
	char *tmp;
	int var;

	// we only support two tokens as arguments for shared.

	token = *((unsigned short *) ptr);

	while ( (token != 0x0000) && (token != 0x0054) && ( kittyError.code == 0) )
	{
		switch (token)
		{
			case 0x0006: 	ref = (struct reference *) (ptr+2);
						tmp = dupRef( ref );

						if (tmp)
						{
							var = findVarPublic(tmp);
							if (var) 
							{
								free(tmp);
								globalVars[var-1].isGlobal = TRUE;
								ref->ref = var;
							}
							else
							{
								add_var_from_ref( ref, tmp, FALSE );
								globalVars[global_var_count-1].isGlobal = TRUE;
							}
							// don't need to free tmp, we use or trow it away, see above
						}

						ptr += sizeof(struct reference *) + ref -> length;

						break;
			case 0x005C: break;

			default: 
						setError(1);
		}

		ptr+=2;	// next token
		token = *((unsigned short *) ptr);
		printf("token %04x\n", token);
		getchar();
	}
	return ptr;
}


char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2 )
{
	struct nativeCommand *cmd;
	unsigned short current_token = *((unsigned short *) ptr);
	int token_size;
	
	// loop until we find token, then exit, or find term token then exit.

	while (  (current_token  != token) && (current_token != token_eof1 ) && (current_token != token_eof2 ) )
	{
		printf("current_token %04x\n",current_token);
		ptr += 2; // skip token id.

		token_size = 0;
		switch (current_token)
		{
			// skip varibales, labels and function, rems, where is text string as name or data.

			case 0x0006:	token_size = ReferenceByteLength(ptr)+sizeof(struct reference); break;
			case 0x000C:	token_size = ReferenceByteLength(ptr)+sizeof(struct reference); break;
			case 0x0012:	token_size = ReferenceByteLength(ptr)+sizeof(struct reference); break;
			case 0x0018:	token_size = ReferenceByteLength(ptr)+sizeof(struct reference); break;
			case 0x0386:   token_size = ReferenceByteLength(ptr)+sizeof(struct reference); break;
			case 0x0026:	token_size = QuoteByteLength(ptr)+2; break;
			case 0x002E:	token_size = QuoteByteLength(ptr)+2; break;
			case 0x064A:	token_size = QuoteByteLength(ptr)+2; break;

			// skip other commands data

			default:

				for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
				{
					if (current_token == cmd->id ) { token_size = cmd -> size; break; }
				}
				break;
		}

		ptr += token_size ;	// skip token data, we have skiped token before
		current_token = *((unsigned short *) ptr);
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
			// IF can end at EOL if then is there. (command THEN should replace nested_if )

			case nested_then:
			case nested_then_else:
				printf("%04x\n",*((short *) (nested_command[ nested_count -1 ].ptr - 2)));
				*((short *) (nested_command[ nested_count -1 ].ptr)) =(short) ((int) (ptr - nested_command[ nested_count -1 ].ptr)) / 2 ;
				nested_count --;
				break;
		}
	}
}

void fix_token_short( int cmd, char *ptr )
{
	printf("%d:%d\n",__FUNCTION__,__LINE__);

	if (nested_count>0)
	{
		if ( nested_command[ nested_count -1 ].cmd  == cmd )
		{
			*((short *) (nested_command[ nested_count -1 ].ptr)) =(short) ((int) (ptr - nested_command[ nested_count -1 ].ptr)) / 2 ;
			nested_count --;
		}
	}
}

void pass1_proc_end( char *ptr )
{
	char *ret;

	getchar();

	if ( *((short *) ptr) == 0x0084 )
	{
		ret = FinderTokenInBuffer( ptr, 0x008C , 0x0000, 0x0000 );
		if (ret)
		{
			((struct procedure *) (nested_command[ nested_count -1 ].ptr)) -> EndOfProc = ret;
		}
	}
	else
	{
		((struct procedure *) (nested_command[ nested_count -1 ].ptr)) -> EndOfProc = ptr-2;
	}

	getchar();

	nested_count --;
}

void pass1_if_or_else( char *ptr )
{
	if (nested_count>0)
	{
		switch (nested_command[ nested_count -1 ].cmd )
		{
			case nested_if:
			case nested_then:
			case nested_else:

				printf("write to %08x-------%08x\n",(short *) (nested_command[ nested_count -1 ].ptr),
					(short) ((int) (ptr - nested_command[ nested_count -1 ].ptr)) / 2);

				*((short *) (nested_command[ nested_count -1 ].ptr)) =(short) ((int) (ptr - nested_command[ nested_count -1 ].ptr)) / 2 ;
				nested_count --;
				break;

			default:

//				printf("Error: End If, with out Else or Then\n");
				setError( 25 );	
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
			printf("%08x %20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
						ptr, __FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token);

			ret = ptr;

			switch (token)
			{
				case 0x0000:	eol( ptr );
							currentLine++;
							break;

				case 0x0006:	pass1var( ptr, false );
							ret += ReferenceByteLength(ptr); 
							break;

				case 0x000c:	pass1label( ptr );
							ret += ReferenceByteLength(ptr); 
							break;

				case 0x0012:	pass1var( ptr, false );	
							ret += ReferenceByteLength(ptr); 
							break;

				case 0x0026:	ret += QuoteByteLength(ptr); break;	// skip strings.
				case 0x064A:	ret += QuoteByteLength(ptr); break;	// skip strings.

				case 0x0268:	addNest( nested_while );
							break;
				// Wend
				case 0x0274:	if LAST_TOKEN_(while)
								fix_token_short( nested_while, ptr+2 );
							else
								setError( 30 );	
							break;
				// if
				case 0x02BE:	addNest( nested_if );
							break;

				case 0x02C6:	// THEN 
							if LAST_TOKEN_(if)
								nested_command[ nested_count -1 ].cmd = nested_then;
							else
								setError( 23 );
							break;

				case 0x02D0:	// ELSE
							if LAST_TOKEN_(if)
							{
								pass1_if_or_else(ptr+2);
								addNest( nested_else );
							}
							else if LAST_TOKEN_(then)
							{
								pass1_if_or_else(ptr+2);
								addNest( nested_then_else );
							}
							else
								setError( 25 );	
							break;

				case 0x02DA:	// END IF
							if ( LAST_TOKEN_(if) || LAST_TOKEN_(else) )
							{
								pass1_if_or_else( ptr+2 );
							}
							else
								setError( 23 );
 							break;

				case 0x0376: // Procedure
							procCount ++;
							procStackCount++;
							addNest( nested_proc );
							next_var_should_be_proc_type( ptr + sizeof(struct procedure) );
							break;

				case 0x0390: // End Proc
							procStackCount--;
							if LAST_TOKEN_(proc) 
							{
								pass1_proc_end( ptr );
							}
							else
								setError(11);
							break;

				case 0x039E:	// Shared

							if (procStackCount)
							{
								ret = pass1_shared( ptr );
							}
							else
								setError(11);
							break;

				case 0x03AA:	// Global

							if (procStackCount == 0)
							{
								ret = pass1_global( ptr );
							}
							else
								setError(11);
							break;

				case 0x0404:	if (data_read_pointer == 0) data_read_pointer = ptr + 2;

			}

			ret += cmd -> size;
			return ret;
		}
	}

	printf("'%20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
					__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token);	

	return NULL;
}

char *token_reader_pass1( char *start, char *ptr, unsigned short lastToken, unsigned short token, int tokenlength )
{
	ptr = nextToken_pass1( ptr, token );

	if ( ( (long long int) ptr - (long long int) start)  >= tokenlength ) return NULL;

	return ptr;
}

void pass1_reader( char *start, int tokenlength )
{
	char *ptr;
	int token = 0;
	last_token = 0;
	
	ptr = start;
	while (( ptr = token_reader_pass1(  start, ptr,  last_token, token, tokenlength ) ) && ( kittyError.code == 0))
	{
		if (ptr == NULL) break;

		last_token = token;
		token = *((short *) ptr);
		ptr += 2;	// next token.		
	}

	// if we did not count back to 0, then there most be some thing wrong.

	if (nested_count)
	{
		switch (nested_command[ nested_count - 1 ].cmd )
		{
			case nested_while: setError(29); break;
			case nested_if: setError(22); break;
			case nested_then: setError(22); break;
			case nested_then_else: 	setError(22); 	printf("pass1 test error, should have been deleted by EOL");break;
			case nested_else: setError(22); break;
			case nested_proc: setError(17); break;
			default: setError(35); break;
		}
	}

	nested_count = 0;
}
