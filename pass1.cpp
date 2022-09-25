
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include "os/linux/stuff.h"
#endif

#include "amosKittens.h"
#include "commands.h"
#include "debug.h"
#include <vector>
#include "AmosKittens.h"
#include "kittyErrors.h"
#include "var_helper.h"
#include "pass1.h"
#include "label.h"
#include "amosstring.h"
#include "req.h"

extern struct globalVar globalVars[1000];	// 0 is not used.
unsigned int var_count[2] = {0,0};		// global and local count
extern int globalVarsSize;
extern int nativeCommandsSize;
extern struct nativeCommand nativeCommands[];
extern const char *TokenName( unsigned short token );

std::vector<struct reference *> pass1CallProcedures;
extern std::vector<struct label> labels;
extern std::vector<struct lineAddr> linesAddress;
extern std::vector<struct defFn> defFns;
char *lastLineAddr;


// size of prev pointer, + size of token length

#define next_token_off (sizeof(void *)+sizeof(void *))

// we need to keep count, to make sure code is valid.

int ifCount = 0;
int endIfCount = 0;
int currentLine = 0;
int pass1_bracket_for;
int pass1_token_count = 0;
int nest_loop_count = 0;

extern uint32_t _file_bank_size;

#ifdef enable_bank_crc_yes
extern uint32_t bank_crc;
#endif

unsigned short last_tokens[MAX_PARENTHESIS_COUNT];		// used for nested loops, etc.
unsigned short pass1_prev_token =0;	// not for nested loops. (can't be used sub passes)

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

struct nested *find_nest_loop()
{
	int n;

	for (n = nested_count -1 ; n>=0;n--)
	{
		switch (nested_command[ n ].cmd )
		{
			case nested_while:
			case nested_repeat:
			case nested_do:
			case nested_for:
					return &nested_command[ n ];
		} 
	}
	return NULL;
}

char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ );


int findVar( char *name, bool is_first_token, int type, int _proc )
{
	unsigned int n;
	pass1_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (n=0;n<var_count[0];n++)
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
			(is_first_token == true)
			&&(strcasecmp( globalVars[n].varName, name)==0) 
			&& (globalVars[n].var.type == type)
			&& ( type == type_proc ))
		{
			return n+1;
		}
	}

	return 0;
}

// this function does allocate memory, it uses static list.



char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ );

int findFnByName(char *name)
{
	unsigned int n;
	for (n=0;n<defFns.size();n++)
	{
		if (strcasecmp(name, defFns[n].name) == 0)
		{
			return n +1;
		}
	}
	return 0;
}

char *pass1Fn( char *ptr )
{
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

uint32_t getTrueVarType( char *varname, uint32_t type )
{
	if ( ( type & 7) == type_undefined )		// type_int is also not defined.
	{
		int tl = strlen(varname);

		if (tl)
		{
			switch (varname[tl-1])
			{
				case '#' : type |= type_float; break;
				case '$': type |= type_string; break;
			}
		}
	}
	return type;
}

 // this function does allocate memory, it uses a static list as return value.

struct globalVar * pass1var(char *ptr, bool first_token, bool is_proc_call, bool is_procedure )
{
	char *tmp;
	int found = 0;
	struct reference *ref = (struct reference *) (ptr);

	pass1_printf("%s:%s:%d\n",__FUNCTION__,__FILE__,__LINE__);

	tmp = dupRef( ref );
	if (tmp)
	{
		int type = ref -> flags & 7;
		short next_token = *((short *) (ptr + sizeof(struct reference) + ReferenceByteLength( ptr )));

		if (first_token)
		{

			// <EOL> <VAR> <EOL>
			// <EOL> <VAR> <NEXT CMD>
			// <EOL> <VAR> <BRACKET START>

			if ((next_token == 0x0000) || (next_token == 0x0054) || (next_token == 0x0084)||(is_proc_call))
			{
#ifdef show_pass1_procedure_fixes_yes
				printf("this looks alot like a procedure call\n");
#endif
				pass1CallProcedures.push_back(ref);		// this token will get ref number set correct,
				is_proc_call = true;
			}
		}

		if (is_proc_call | is_procedure)
		{
			type = type_proc;
		}
		else
		{
			char *next_ptr = ptr + sizeof(struct reference) + ref -> length;	
			if  (*((unsigned short *) next_ptr)  == 0x0074) type |= type_array;
			type = getTrueVarType( tmp, type );
		}

		found = findVar(tmp, first_token, type, ( is_proc_call | is_procedure )  ? 0 : (pass1_inside_proc ? procCount : 0) );

		if (found)
		{
			if (is_procedure)
			{
				struct globalVar *_old = &globalVars[found-1];
				_old -> var.type = type_proc;
				_old -> var.tokenBufferPos = ptr + 2 + sizeof(struct reference) + ref -> length ;
				_old -> proc =  procCount;
				ref -> ref = found;

				return _old;
			}
			else if (procStackCount)
			{
				struct globalVar *_old = &globalVars[found-1];

				if (_old -> proc)	// is local?
				{
					ref -> ref = (_old -> localIndex+1) | 0x8000;
				}
				else 	ref -> ref = found;
			}
			else	// not inside procedure.
			{
				ref -> ref = found;
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
					return _new;
				}
			}
			else 	if (is_proc_call == false)
			{
				if (struct globalVar *_new = add_var_from_ref( ref, &tmp, type ))
				{
					_new -> proc = (pass1_inside_proc ? procCount : 0);
					ref -> ref = var_count[ procStackCount ? 1:0] | (procStackCount ? 0x8000 : 0x0000);
					return _new;
				}
			}
		}

		if (tmp) free(tmp); 
		tmp= NULL;
	}

	return NULL;
}

extern struct globalVar proc_main_data;
static struct globalVar *current_proc = &proc_main_data;

void pass1_restore( char *ptr )
{
	unsigned short next_token = *((unsigned short *) (ptr));

	switch ( next_token )
	{
		case 0x0000:	// new line.
		case 0x0054:	// next command 
				printf("moded to restore command with no args\n");
				*((unsigned short *) (ptr - 2)) = 0x0418+next_token_off;
	}
}

void pass1_sign( char * ptr )
{
//	printf("prev %04x this %04x",pass1_prev_token ,*((unsigned short *) (ptr -2) ));

	switch ( pass1_prev_token )	// should be signes
	{
		case 0xFF4C:	// or
		case 0xFF3E:	// xor
		case 0xFF58:	// and
		case 0xFF66:	// not equal
		case 0xFF7A:	// less or equal
		case 0xFF84:
		case 0xFF8E:	// more or euqal
		case 0xFF98:
		case 0xFFA2:	// equal / or set.
		case 0xFFAC:	// less 
		case 0xFFB6:	// more
		case 0xFFC0:	// add
		case 0xFFCA:	// subtract
		case 0xFFD4:	// mode
		case 0xFFE2:	// *
		case 0xFFEC:	// div
		case 0xFFF6:	// power
		case 0x0074:	// "("
		case 0x005C:	// ","
		case 0x0094:	// To

#ifdef show_pass1_modified_code_yes
				printf("Modified to negative signed\n");
#endif
				*((unsigned short *) (ptr - 2)) = 0xFF4C-next_token_off;		// mod the token, so signes token.
				return;	// nothing more to do....

		default:				// not sure if its signes or subtract

				switch ( pass1_prev_token )		// exclude list	(for signes)
				{
					case 0x0006:	//	"vars"
					case 0x001E:	//	"bin"
					case 0x0036:	//	"hex"
					case 0x003E:	//	"numbers"
					case 0x0046:	//	"float"
					case 0x123E:	//	"True"
					case 0x1248:	//	"False"
					case 0x0026:	//	"Strings"
							return;	// nothing more to do....
					default:
							if (pass1_token_count <= 2)
							{
#ifdef show_pass1_modified_code_yes
								printf("Modified to negative signed\n");
#endif
								*((unsigned short *) (ptr -2)) = 0xFF4C-next_token_off;		// mod the token, so signes token.
							}
							return;
				}
				break;
	}
}

char *pass1_procedure( char *ptr )
{
	short token = *((short *) ptr);
	if (token == 0x0006)
	{
		current_proc = pass1var( ptr +2, true, false, true );

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
	char *next;

	pass1_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	tmpName = strndup( ptr + sizeof(struct reference), ref->length  );
	if (tmpName)
	{
		ref -> ref = 0;	

		// only add new labels if last token is 0.

		if (last_tokens[instance.parenthesis_count]  == 0)
		{
			found_ref = findLabelRef(tmpName, (pass1_inside_proc ? procCount : 0));

			if (found_ref>0)
			{
				ref -> ref = found_ref;
				free(tmpName);		//  don't need tmp
				tmpName = NULL;
			}
			else
			{
				label tmp;
				struct nested *thisNest;
				next = ptr + sizeof(struct reference) + ref->length ; // next token after label

				// skip all new lines..
				while ( (*(unsigned short *) next) == 0x0000 ) next += 4;	// token and newline code

				tmp.proc = (pass1_inside_proc ? procCount : 0);
				tmp.name = tmpName;
				tmp.tokenLocation = next +2 ;

				tmp.loopLocation = NULL;

				if (thisNest = find_nest_loop())
				{
					tmp.loopLocation = thisNest -> ptr;
				}

				labels.push_back(tmp);
				ref -> ref = labels.size();
				tmpName = NULL;			// we store tmpName, we only set tmpName to NULL
			}
		}
		else
		{
			found_ref = findLabelRef(tmpName,(pass1_inside_proc ? procCount : 0));
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
	else
	{
		printf("bad data\n");
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

	while ( (token != 0x0000) && (token != 0x0054) && ( instance.kittyError.code == 0) )
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

	while ( (token != 0x0000) && (token != 0x0054) && ( instance.kittyError.code == 0) )
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

							type = getTrueVarType( tmp, type );
							
							var = findVarPublic(tmp, type );
							if (var)
							{
								globalVars[var-1].isGlobal = TRUE;
								ref->ref = var;
							}
							else
							{
								add_var_from_ref( ref, &tmp, type );
								globalVars[var_count[0]-1].isGlobal = TRUE;
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
						printf("%04x\n",token);
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

void set_nested_if_condition( char *ptr )
{
	unsigned short offset;
	offset = (short) ((int) (ptr - nested_command[ nested_count -1 ].ptr)) / 2;
	*((short *) (nested_command[ nested_count -1 ].ptr)) = offset;
	nested_count --;
}

void eol( char *ptr )
{
	pass1_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (nested_count>0)
	{
		switch (nested_command[ nested_count -1 ].cmd )
		{
			case nested_data:
			case nested_then_else:
			case nested_then_else_if:
				set_nested_if_condition( ptr );
				break;

			case nested_defFn:

				nested_count --;
				break;
		}
	}

	// in a while in case of.
	// IF ... TEHN IF ... TEHN IF ... THEN 

	while (nested_count>0)
	{
		if (nested_command[ nested_count -1 ].cmd  == nested_then)
		{
			set_nested_if_condition( ptr );
		} else break;
	}

#if 0
	printf("nested_count %d\n",nested_count);

	if (nested_count>0)
	{
		printf("nested_command[ %d ].cmd  = %s\n", nested_count -1, nest_names[ nested_command[ nested_count -1 ].cmd ] );
	}
#endif
}

void fix_token_short( int cmd, char *ptr )
{
	pass1_printf("%s:%d\n",__FUNCTION__,__LINE__);

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
	current_proc -> localIndexSize = var_count[1];
	var_count[1] = 0;

	procStackCount--;

	((struct procedure *) (nested_command[ nested_count -1 ].ptr )) -> EndOfProc = ptr-2;

	nested_count --;
	pass1_inside_proc = false;

	current_proc = &proc_main_data;
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

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id )
		{
#ifdef show_pass1_tokens_yes

			getLineFromPointer(ptr);
			printf("Line %08d offset %08x token %04x - name %s\n",
						lineFromPtr.line,
						ptr-_file_start_,
						token, 
						TokenName(token));

#endif

			// ptr points to data of the token. (+2)

			ret = ptr;

			switch (token)
			{
				case 0x0000:	eol( ptr );
							currentLine++;
							lastLineAddr = ptr;
							pass1_token_count = 0;
							break;

				case 0x0386:	pass1_token_count = 0;
							break;

				case 0x0006:	pass1var( ptr, (pass1_token_count == 1), false, false );	// is not proc call, is not procedure
							ret += ReferenceByteLength(ptr);
							break;

				case 0x0054:	pass1_token_count = 0;		// next command 
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

				case 0x0012:	pass1var( ptr, true, true, false );		// is first token, is proc call, is not procedure.
							ret += ReferenceByteLength(ptr);
							break;

				case 0x0018:	pass1label( ptr );
							ret += ReferenceByteLength(ptr);
							break;

				case 0x0026:	ret += QuoteByteLength(ptr); break;	// skip strings.
				case 0x002E:	ret += QuoteByteLength(ptr); break;	// skip strings.
				case 0x064A:	ret += QuoteByteLength(ptr); break;	// skip strings.
				case 0x0652:	ret += QuoteByteLength(ptr); break;	// skip strings.

				case 0x027E:	addNestLoop( nested_do );
							break;

				// loop
				case 0x0286:	if IS_LAST_NEST_TOKEN(do)
							{
								fix_token_short( nested_do, ptr+2 );
								nest_loop_count--;
							}
							else
							{
								printf("expected DO found nested command: %s\n",(nested_count>0) ? nest_names[nested_command[ nested_count -1 ].cmd] : "none" );
								setError( 28, ptr );
							}
							break;

				case 0x023C:  addNestLoop( nested_for );
							break;

				// next
				case 0x0246:	if IS_LAST_NEST_TOKEN(for) {
								fix_token_short( nested_for, ptr+2 );
								nest_loop_count--;
							} else
								setError( 34,ptr );
							break;

				case 0x0250:	addNestLoop( nested_repeat );
							break;

				// until
				case 0x025C:	if IS_LAST_NEST_TOKEN(repeat) {
								fix_token_short( nested_repeat, ptr+2 );
								nest_loop_count--;
							} else
								setError( 32,ptr );
							break;

				case 0x0268:	addNestLoop( nested_while );
							break;
				// Wend
				case 0x0274:	if IS_LAST_NEST_TOKEN(while) {
								fix_token_short( nested_while, ptr+2 );
								nest_loop_count--;
							} else
								setError( 30,ptr );
							break;
				// if
				case 0x02BE:	addNest( nested_if );
							ifCount ++;
							break;

				case 0x02C6:	// THEN
							pass1_token_count = 0;
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
							pass1_token_count = 0;
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

							procCount ++;
							procStackCount++;
							addNest( nested_proc );

							ret = pass1_procedure( ptr + sizeof(struct procedure) ) - sizeof(struct procedure);
							break;

				case 0x0390: // End Proc

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
							// inside procedure this command should copies from global to local..

							break;

				case 0x0418:	// Restore
							pass1_restore( ptr );
							break;

				case 0x0404:	// Data

							if (current_proc -> procDataPointer == NULL) current_proc -> procDataPointer = ptr + 2;
							addNest( nested_data );

							break;
				
				case 0xFFCA:	// negative sign... check if is used for signes.
							pass1_sign( ptr );
							break;

			}

			ret += cmd -> size;
			pass1_token_count ++;
			return ret;
		}

	}

	{
		printf("ERROR    %20s:%08d stack is %d cmd stack is %d token %04x\n",
					__FUNCTION__,__LINE__, instance.stack, instance.cmdStack, token);
	}

	setError(35,ptr);

	return NULL;
}

char *token_reader_pass1( char *start, char *ptr, unsigned short lastToken, unsigned short token, char *file_end )
{
	ptr = nextToken_pass1( ptr, token );

#ifdef enable_bank_crc
	if (bank_crc != mem_crc( _file_end_ , amos_filesize - tokenlength - _file_code_start_  ))
	{
		printf("memory bank corrupted in %s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
		getchar();
	}
#endif 

	if ( ptr  >= file_end ) 	return NULL;

	return ptr;
}

bool findRefAndFixProcCall( struct reference *toFind )
{
	unsigned int n;
	struct globalVar *varInfo;
	char *toFindName = dupRef( toFind );
	if (toFindName == NULL) return false;

	for (n=0;n<var_count[0];n++)
	{
		varInfo = &globalVars[n];

		if ( (varInfo->varName != NULL) && (varInfo->var.type == type_proc) )
		{
			if ( strcasecmp( varInfo->varName, toFindName ) == 0 )
			{
				*((unsigned short*) ((char *) toFind-2)) = 0x0012;
				toFind -> ref = n + 1;
				free(toFindName);
				return true;
			}
		}
	}

	free(toFindName);
	return false;
}

void pass1_reader( char *start, char *file_end )
{
	char *ptr;
	int token = 0;
	last_tokens[instance.parenthesis_count] = 0;
	unsigned int n;

	lastLineAddr = start;
	
	pass1_prev_token = 0x0;
	token = *((short *) start);
	ptr = start +2;

	while (( ptr = token_reader_pass1(  start, ptr,  last_tokens[instance.parenthesis_count], token, file_end ) ) && ( instance.kittyError.code == 0))
	{
		if (ptr == NULL) break;

		last_tokens[instance.parenthesis_count] = token;

		pass1_prev_token = token;
		token = *((short *) ptr);

		// remap tokens.

		switch (token)
		{
			case 0x0064:  *((short *) ptr) = token_semi;
		}

		ptr += 2;	// next token.
	}

	if (instance.kittyError.code == 0)	// did not exit on error.
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

	dprintf("number of procedure calls %d\n", pass1CallProcedures.size() );

#ifdef enable_bank_crc_yes
	if (bank_crc != mem_crc( _file_end_ , _file_bank_size ))
	{
		printf("memory bank corrupted in %s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
		getchar();
	}
#endif

	for (n = 0 ; n < pass1CallProcedures.size(); n++)
	{
		if ( findRefAndFixProcCall(pass1CallProcedures[n])  )
		{
#ifdef show_pass1_procedure_fixes_yes
			printf("fixed at: %08x ref is %d\n", pass1CallProcedures[n], pass1CallProcedures[n] -> ref - 1 );
#endif
		}
		else
		{
			printf("ref -> ref %04x\n",pass1CallProcedures[n] -> ref);
			setError( 20, (char *) pass1CallProcedures[n] );
			break;
		}
	}

#ifdef enable_bank_crc_yes
	if (bank_crc != mem_crc( _file_end_ , _file_bank_size ))
	{
		printf("memory bank corrupted in %s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
		getchar();
	}
#endif

	for (n = 0; n< var_count[0] ; n++)
	{
		if (globalVars[n].var.type == type_proc)
		{
			if (globalVars[n].var.tokenBufferPos == NULL)
			{
				printf("Procedure %s not declared\n", globalVars[n].varName );
				setError(20, 0);	// errorsTestTime 20, 
			}
		}
	}

#ifdef enable_bank_crc_yes
	if (bank_crc != mem_crc( _file_end_ , _file_bank_size ))
	{
		printf("memory bank corrupted in %s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
		getchar();
	}
#endif

#ifdef show_pass1_end_of_file_yes
	printf("lines: %d -- end of tokens shoud be at 0x%08x\n",linesAddress.size()-2,file_end);
#endif
	nested_count = 0;
}



