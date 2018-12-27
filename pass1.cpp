
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
#include "var_helper.h"
#include "pass1.h"

const char *types[]={"","#","$",""};

extern struct globalVar globalVars[1000];	// 0 is not used.
extern int global_var_count;
extern int globalVarsSize;
extern int nativeCommandsSize;
extern struct nativeCommand nativeCommands[];
extern const char *TokenName( unsigned short token );

extern std::vector<struct label> labels;
extern std::vector<struct lineAddr> linesAddress;
extern std::vector<struct defFn> defFns;
char *lastLineAddr;

void addLineAddress( char *_start, char *_end );

int ifCount = 0;
int endIfCount = 0;
int currentLine = 0;
int pass1_bracket_for;

enum
{
	pass1_bracket_none,
	pass1_bracket_end_proc
};

bool pass1_inside_proc = false;
int procCount = 0;

const char *nest_names[] =
{
	"nested_if",
	"nested_then",
	"nested_then_else",
	"nested_then_else_if",
	"nested_else",
	"nested_else_if",
	"nested_while",
	"nested_repeat",
	"nested_do",
	"nested_for",
	"nested_proc",
	"nested_data"
};

struct nested nested_command[ max_nested_commands ];
int nested_count = 0;

void dump_nest()
{
	int i;
	for (i=0;i<nested_count;i++)
	{
		printf("%04d -- %04X - %s\n",
			i, nested_command[ i ].cmd,
			nest_names[ nested_command[ i ].cmd ] );
	}
}

char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ );

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

int findVarPublic( char *name, int type )
{
	int n;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (n=0;n<global_var_count;n++)
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

int findProc( char *name )
{
	int n;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (n=0;n<global_var_count;n++)
	{
		if (globalVars[n].varName == NULL) return 0;

		printf("%s\n", globalVars[n].varName);

		if ( (strcasecmp( globalVars[n].varName, name)==0) && (globalVars[n].var.type & type_proc) )
		{
			return n+1;
		}
	}
	return 0;
}


int findVar( char *name, int type, int _proc )
{
	int n;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (n=0;n<global_var_count;n++)
	{
		if (globalVars[n].varName == NULL) return 0;

		// look for any  local.

		if ((strcasecmp( globalVars[n].varName, name)==0) 
			&& (globalVars[n].var.type == type)
			&& (globalVars[n].proc == _proc))
		{
			return n+1;
		}

		// look for any shared.

		if ((strcasecmp( globalVars[n].varName, name)==0) 
			&& (globalVars[n].var.type == type)
			&& (globalVars[n].pass1_shared_to > 0)
			&& (globalVars[n].pass1_shared_to == _proc))
		{
			return n+1;
		}

		// look for any global.

		if (
			(strcasecmp( globalVars[n].varName, name)==0) 
			&& (globalVars[n].var.type == type)
			&& (globalVars[n].isGlobal == TRUE) 
			&& (globalVars[n].proc == 0)
			&& (_proc > 0))
		{
			return n+1;
		}

		// look for any proc.

		if (
			(strcasecmp( globalVars[n].varName, name)==0) 
			&& (globalVars[n].var.type == type)
			&& ( type == type_proc ))
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

int findLabelRef( char *name )
{
	int n;

	for (n=0;n<labels.size();n++)
	{
		if (strcasecmp( labels[n].name, name)==0)
		{
			return n+1;
		}
	}
	return 0;
}

int QuoteByteLength(char *ptr)
{
	unsigned short length = *((unsigned short *) ptr);
	length += (length & 1);		// align to 2 bytes
	return length;
}

int ReferenceByteLength(char *ptr)
{
	struct reference *ref = (struct reference *) ptr;
	unsigned short length = ref -> length;

	length += (length & 1);		// align to 2 bytes
	return length;
}

struct globalVar *add_var_from_ref( struct reference *ref, char **tmp, int type )
{
	struct globalVar *_new = NULL;

	if ( global_var_count < VAR_BUFFERS )
	{
		global_var_count ++;
		ref -> ref = global_var_count;

		_new = &globalVars[global_var_count-1];
		_new -> varName = *tmp;	// tmp is alloced and used here.
		_new -> var.type = type;
		_new -> var.len = 0;
		if (_new -> var.type == type_string) _new -> var.str = strdup("");

		*tmp = NULL;
	}

	return _new;
}

char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ );

int findFnByName(char *name)
{
	int n;
	for (n=0;n<defFns.size();n++)
	{
		if (strcasecmp(name, defFns[n].name) == 0)
		{
			printf("return %d\n",n+1);
			return n +1;
		}
	}
	return 0;
}

char *pass1Fn( char *ptr )
{
	char *_;
	// skip all new lines..

	if (*(unsigned short *) ptr  == 0x0006 )
	{
		struct reference *ref;
		char *tmp;

		ptr+=2;
 		ref = (struct reference *) ptr;

		tmp = dupRef( ref );

		if (tmp)
		{
			ref -> ref = findFnByName(tmp);

			if (ref -> ref == 0)
			{
				struct defFn fn;
				fn.name = tmp;
				defFns.push_back(fn);
				ref -> ref = defFns.size();
			}
		}

		ptr += sizeof(struct reference) + ref -> length;		// no point adding this var list.
	}

	return ptr;
}


char *pass1DefFn( char *ptr )
{
	char *_;
	// skip all new lines..

	if (*(unsigned short *) ptr  == 0x0006 )
	{
		struct reference *ref;
		char *tmp;
		ptr+=2;
 		ref = (struct reference *) ptr;

		tmp = dupRef( ref );

		if (tmp)
		{
			ref -> ref = findFnByName(tmp);

			if (ref -> ref)
			{
				struct defFn *fn = &defFns[ref->ref-1];
				fn -> fnAddr = ptr + sizeof(struct reference) + ref -> length;
				fn -> skipAddr = FinderTokenInBuffer( fn -> fnAddr, 0x0000 , -1, -1, _file_end_ );
			}
			else
			{
				struct defFn fn;
				fn.fnAddr = ptr + sizeof(struct reference) + ref -> length;
				fn.skipAddr = FinderTokenInBuffer( fn.fnAddr, 0x0000 , -1, -1, _file_end_ );
				fn.name = tmp;
				defFns.push_back(fn);
				ref -> ref = defFns.size();
			}
		}

		ptr += sizeof(struct reference) + ref -> length;		// no point adding this var list.
	}

	return ptr;
}


struct kittyData * pass1var(char *ptr, bool is_proc_call, bool is_procedure )
{
	char *tmp;
	int found = 0;
	struct reference *ref = (struct reference *) (ptr);

	tmp = dupRef( ref );
	if (tmp)
	{
		int type = ref -> flags & 7;

		if (is_proc_call | is_procedure)
		{
			type = type_proc;
		}
		else
		{
			char *next_ptr = ptr + sizeof(struct reference) + ref -> length;	
			if  (*((unsigned short *) next_ptr)  == 0x0074) type |= type_array;
		}

		found = findVar(tmp, type , ( is_proc_call | is_procedure )  ? 0 : (pass1_inside_proc ? procCount : 0) );
		if (found)
		{
			ref -> ref = found;

			if (is_procedure)
			{
				struct globalVar *_old = &globalVars[found-1];
				_old -> var.type = type_proc;
				_old -> var.tokenBufferPos = ptr + 2 + sizeof(struct reference) + ref -> length ;
				_old -> proc =  procCount;
				return &_old -> var;
			}
		}
		else
		{
			if (is_procedure)
			{
				if (struct globalVar *_new = add_var_from_ref( ref, &tmp, type_proc ))
				{
					_new -> var.tokenBufferPos = ptr + 2 +sizeof(struct reference) + ref -> length ;
					_new -> proc = procCount;
					return &_new -> var;
				}
			}
			else
			{
				if (struct globalVar *_new = add_var_from_ref( ref, &tmp, type ))
				{
					_new -> proc = (pass1_inside_proc ? procCount : 0);
					return &_new -> var;
				}
			}
		}

		if (tmp) free(tmp); 
		tmp= NULL;
	}

	return NULL;
}

static struct kittyData *current_proc;

char *pass1_procedure( char *ptr )
{
	short token = *((short *) ptr);
	if (token == 0x0006)
	{
		current_proc = pass1var( ptr +2, false, true );

		pass1_inside_proc = true;
		// we like to skip the variable, so its not added as a local variable.
		ptr += 2 + sizeof(struct reference) + ReferenceByteLength(ptr + 2) ;
	}
	else 
	{
		printf("bad token %04x\n",token);
		setError(1,ptr);
	}

	token = *((short *) ptr);

	// sizeof(struct procedure) will be added to move to next token, we don't wont that.
	return ptr ;
}

void pass1label(char *ptr)
{
	char *tmpName;
	int found_ref;
	struct reference *ref = (struct reference *) ptr;
	struct kittyData *var;
	char *next;

	tmpName = strndup( ptr + sizeof(struct reference), ref->length  );
	if (tmpName)
	{
		printf("%s\n",tmpName);
		ref -> ref = 0;	

		// only add new labels if last token is 0.

		if (last_tokens[parenthesis_count]  == 0)
		{

			found_ref = findLabelRef(tmpName);

			if (found_ref>0)
			{
				ref -> ref = found_ref;
				free(tmpName);		//  don't need tmp
			}
			else
			{
				label tmp;
				next = ptr + sizeof(struct reference) + ref->length ; // next token after label

				// skip all new lines..
				while ( (*(unsigned short *) next) == 0x0000 ) next += 4;	// token and newline code

				tmp.name = tmpName;
				tmp.tokenLocation = next +2 ;

				labels.push_back(tmp);
				ref -> ref = labels.size();

				tmpName = NULL;			// we store tmpName, we only set tmpName to NULL
			}
		}
		else
		{
			found_ref = findLabelRef(tmpName);

			if (found_ref>0)
			{
				ref -> ref = found_ref;
			}
			else
			{
				ref -> ref = 0xFFFF;
			}
		}

		if (tmpName) free(tmpName);
	}
}

char *pass1_shared( char *ptr )
{
	unsigned short token = *((unsigned short *) ptr);
	struct reference *ref;
	char *tmp;
	int var;
	int count = 0;

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
							char *next_ptr = ptr + 2 + sizeof(struct reference *) + ref -> length + (ref -> length & 1);
							unsigned short next_token = *((unsigned short *) next_ptr ) ;
							int type = ref -> flags & 7;

							if ( next_token == 0x0074 ) type |= type_array;

							var = findVarPublic(tmp, type);
							if (var)
							{
								globalVars[var-1].pass1_shared_to = procCount;
								ref->ref = var;
							}
							else
							{
								if (struct globalVar *_new = add_var_from_ref( ref, &tmp, type ))
								{
									_new -> pass1_shared_to = procCount;
								}
							}

							if (tmp) free(tmp);
							tmp = NULL;
						}

						ptr += sizeof(struct reference *) + ref -> length;
						break;

			case 0x0074: count ++; break;	// (
			case 0x007C: count--;  break;		// )

			case 0x005C: 	if (count != 0) 	setError(1,ptr);
						break;
			default:
						setError(1,ptr);
		}

		ptr+=2;	// next token
		token = *((unsigned short *) ptr);
	}

	return ptr;
}

char *pass1_global( char *ptr )
{
	unsigned short token = *((unsigned short *) ptr);
	struct reference *ref;
	char *tmp;
	int var;
	int count = 0;

	// we only support two tokens as arguments for global.

	token = *((unsigned short *) ptr);

	while ( (token != 0x0000) && (token != 0x0054) && ( kittyError.code == 0) )
	{
		switch (token)
		{
			case 0x0006: 	ref = (struct reference *) (ptr+2);
						tmp = dupRef( ref );

						if (tmp)
						{
							char *next_ptr = ptr + 2 + sizeof(struct reference *) + ref -> length + (ref -> length & 1);
							unsigned short next_token = *((unsigned short *) next_ptr ) ;
							int type = ref -> flags & 7;

							if ( next_token == 0x0074 ) type |= type_array;

							var = findVarPublic(tmp, type );
							if (var)
							{
								globalVars[var-1].isGlobal = TRUE;
								ref->ref = var;
							}
							else
							{
								add_var_from_ref( ref, &tmp, FALSE );
								globalVars[global_var_count-1].isGlobal = TRUE;
							}
							
							if (tmp) free(tmp);
							tmp = NULL;
						}

						ptr += sizeof(struct reference *) + ref -> length;

						break;

			case 0x0074: count ++; break;	// (
			case 0x007C: count--;  break;		// )

			case 0x005C: 	if (count != 0) 	setError(1,ptr);
						break;

			default:
						printf("%d\n",token);
						setError(1,ptr);
		}

		ptr+=2;	// next token
		token = *((unsigned short *) ptr);
	}
	return ptr;
}


char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ )
{
	struct nativeCommand *cmd;
	unsigned short current_token = *((unsigned short *) ptr);
	int token_size;

	// loop until we find token, then exit, or find term token then exit.

	while (  (current_token  != token) && (current_token != token_eof1 ) && (current_token != token_eof2 ) && ( ptr < _eof_ ) )
	{
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

int getLineFromPointer( char *address );

void eol( char *ptr )
{
	unsigned short offset;

	if (nested_count>0)
	{
		proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

		switch (nested_command[ nested_count -1 ].cmd )
		{
			case nested_data:

			// IF can end at EOL if then is there. (command THEN should replace nested_if )

			case nested_then:
			case nested_then_else:
			case nested_then_else_if:

				{
					offset = (short) ((int) (ptr - nested_command[ nested_count -1 ].ptr)) / 2;
					*((short *) (nested_command[ nested_count -1 ].ptr)) = offset;
				}
				nested_count --;
				break;

			case nested_defFn:

				nested_count --;
				break;
		}
	}
}

void fix_token_short( int cmd, char *ptr )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

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
	procStackCount--;

	((struct procedure *) (nested_command[ nested_count -1 ].ptr )) -> EndOfProc = ptr-2;

	nested_count --;
	pass1_inside_proc = false;
}

void pass1_bracket_end( char *ptr )
{
	switch (pass1_bracket_for)
	{
		case pass1_bracket_end_proc:
			pass1_proc_end( ptr );
			break;
	}

	pass1_bracket_for = pass1_bracket_none;	// reset.
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
			case nested_else_if:


				printf ("0x%08x - 0%08x ----- +0x%02x ------\n",
					nested_command[ nested_count -1 ].ptr,
					ptr,
					(short) (int) (ptr - nested_command[ nested_count -1 ].ptr)) ;

				*((short *) (nested_command[ nested_count -1 ].ptr)) =(short) ((int) (ptr - nested_command[ nested_count -1 ].ptr)) / 2 ;
				nested_count --;
				break;

			default:

				setError( 25, ptr );
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
			pass1_printf("%08x %20s:%08d stack is %d cmd stack is %d flag %d token %04x - name %s\n",
						ptr, __FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token, TokenName(token));


			// ptr points to data of the token. (+2)

			ret = ptr;

			switch (token)
			{
				case 0x0000:	eol( ptr );
							currentLine++;
							addLineAddress( lastLineAddr, ptr );
							lastLineAddr = ptr;
							break;

				case 0x0006:	pass1var( ptr, false, false );
							ret += ReferenceByteLength(ptr);
							break;

				case 0x00b0:	procCount ++;
							procStackCount++;
							addNest( nested_defFn );
							ret = pass1DefFn( ptr );
							break;

				case 0x00bc:	ret = pass1Fn( ptr );
							break;

				case 0x000c:	pass1label( ptr );
							ret += ReferenceByteLength(ptr);
							break;

				case 0x0012:	pass1var( ptr, true, false );
							ret += ReferenceByteLength(ptr);
							break;

				case 0x0018:	pass1label( ptr );
							ret += ReferenceByteLength(ptr);
							break;

				case 0x0026:	ret += QuoteByteLength(ptr); break;	// skip strings.
				case 0x064A:	ret += QuoteByteLength(ptr); break;	// skip strings.
				case 0x0652:	ret += QuoteByteLength(ptr); break;	// skip strings.

				case 0x027E:	addNest( nested_do );
							break;

				// loop
				case 0x0286:	if IS_LAST_NEST_TOKEN(do)
								fix_token_short( nested_do, ptr+2 );
							else
								setError( 28, ptr );
							break;

				case 0x023C: addNest( nested_for );
							break;

				// next
				case 0x0246:	if IS_LAST_NEST_TOKEN(for)
								fix_token_short( nested_for, ptr+2 );
							else
								setError( 34,ptr );
							break;

				case 0x0250:	addNest( nested_repeat );
							break;

				// until
				case 0x025C:	if IS_LAST_NEST_TOKEN(repeat)
								fix_token_short( nested_repeat, ptr+2 );
							else
								setError( 32,ptr );
							break;

				case 0x0268:	addNest( nested_while );
							break;
				// Wend
				case 0x0274:	if IS_LAST_NEST_TOKEN(while)
								fix_token_short( nested_while, ptr+2 );
							else
								setError( 30,ptr );
							break;
				// if
				case 0x02BE:	addNest( nested_if );
							ifCount ++;
							break;

				case 0x02C6:	// THEN
							if IS_LAST_NEST_TOKEN(if)
								nested_command[ nested_count -1 ].cmd = nested_then;
							else
								setError( 23,ptr );
							break;

				case 0x25A4:	// ELSE IF
							if IS_LAST_NEST_TOKEN(if)
							{
								pass1_if_or_else(ptr);
								addNest( nested_else_if );
							}
							else if IS_LAST_NEST_TOKEN(else_if)
							{
								pass1_if_or_else(ptr);
								addNest( nested_else_if );
							}
							else if IS_LAST_NEST_TOKEN(then)
							{
								pass1_if_or_else(ptr);
								addNest( nested_then_else_if );
							}
							else
							{
								printf("ELSE IF -- GET_LAST_NEST %04x\n", GET_LAST_NEST );
								dump_nest();
								setError( 25,ptr );
							}
							break;

				case 0x02D0:	// ELSE
							if IS_LAST_NEST_TOKEN(if)
							{
								pass1_if_or_else(ptr);
								addNest( nested_else );
							}
							else if IS_LAST_NEST_TOKEN(else_if)
							{
								pass1_if_or_else(ptr);
								addNest( nested_else );
							}
							else if IS_LAST_NEST_TOKEN(then)
							{
								pass1_if_or_else(ptr);
								addNest( nested_then_else );
							}
							else if IS_LAST_NEST_TOKEN(then_else_if)
							{
								pass1_if_or_else(ptr);
								addNest( nested_then_else );
							}

							else
								setError( 25,ptr );
							break;

				case 0x02DA:	// END IF

							endIfCount ++;

							if ( IS_LAST_NEST_TOKEN(if) || IS_LAST_NEST_TOKEN(else_if) || IS_LAST_NEST_TOKEN(else) )
							{
								pass1_if_or_else( ptr );
							}
							else
								setError( 23,ptr );
 							break;

				case 0x0376: // Procedure

							printf("last token: %d\n", last_tokens[parenthesis_count] );

							procCount ++;
							procStackCount++;
							addNest( nested_proc );

							ret = pass1_procedure( ptr + sizeof(struct procedure) ) - sizeof(struct procedure);
							break;

				case 0x0390: // End Proc
							current_proc = NULL;

							if IS_LAST_NEST_TOKEN(proc)
							{
								short next_token = *((short *) ptr);

								if (next_token == 0x0084)
								{
									pass1_bracket_for = pass1_bracket_end_proc;
								}
								else 
								{
									pass1_proc_end( ptr );
								}
							}
							else
								setError(11,ptr);
							break;

				case 0x008C:
							pass1_bracket_end( ptr );
							break;

				case 0x039E:	// Shared

							if (procStackCount > 0)
							{
								ret = pass1_shared( ptr );
							}
							else
								setError(11,ptr);
							break;

				case 0x03AA:	// Global

							if (procStackCount == 0)
							{
								ret = pass1_global( ptr );
							}
							else
								setError(11,ptr);
							break;

				case 0x0404:	// Data

							if (current_proc)
							{
								if (current_proc -> procDataPointer == NULL) current_proc -> procDataPointer = ptr + 2;
							}
							else 	if (data_read_pointers[0] == NULL) data_read_pointers[0] = ptr + 2;

							addNest( nested_data );

							break;

			}

			ret += cmd -> size;
			return ret;
		}
	}

	printf("'%20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
					__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token);

	setError(35,ptr);

	return NULL;
}

char *token_reader_pass1( char *start, char *ptr, unsigned short lastToken, unsigned short token, char *file_end )
{
	ptr = nextToken_pass1( ptr, token );

	if ( ptr  >= file_end ) return NULL;

	return ptr;
}

void pass1_reader( char *start, char *file_end )
{
	char *ptr;
	int token = 0;
	last_tokens[parenthesis_count] = 0;

	lastLineAddr = start;
	ptr = start;
	while (( ptr = token_reader_pass1(  start, ptr,  last_tokens[parenthesis_count], token, file_end ) ) && ( kittyError.code == 0))
	{
		if (ptr == NULL) break;

		last_tokens[parenthesis_count] = token;
		token = *((short *) ptr);
		ptr += 2;	// next token.
	}
	addLineAddress( lastLineAddr, ptr );

	if (kittyError.code == 0)	// did not exit on error.
	{
		while (nested_count)
		{
			switch (nested_command[ nested_count - 1 ].cmd )
			{
				case nested_while:		setError(29,ptr); break;
				case nested_if:		setError(22,ptr); break;
				case nested_then:		setError(22,ptr); break;
				case nested_then_else:	setError(22,ptr); 	printf("pass1 test error, should have been deleted by EOL");break;
				case nested_else:		setError(22,ptr); break;
				case nested_proc:		setError(17,ptr); break;
				default:				setError(35,ptr); break;
			}
			nested_count --;
		}
	}

	validate_and_fix_globals();

	printf("lines: %d -- end of tokens shoud be at 0x%08x\n",linesAddress.size()-2,file_end);

	nested_count = 0;
}

void addLineAddress( char *_start, char *_end )
{
	struct lineAddr line;
	line.start = _start;
	line.end = _end;
	linesAddress.push_back( line );
}


