
#include "stdafx.h"

#include <vector>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#endif

#ifdef __linux__
#include <stdint.h>
#include "os/linux/stuff.h"
#endif

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "engine.h"
#include "debug.h"
#include "errors.h"
#include "var_helper.h"
#include "label.h"


extern struct globalVar globalVars[];
extern struct nativeCommand nativeCommands[];
extern int nativeCommandsSize;
extern const char *TokenName( unsigned short token );
extern unsigned short token_not_found;
extern char *_gosub_return( struct glueCommands *data, int nextToken );

extern struct globalVar globalVars[1000];
extern char *dupRef( struct reference *ref );

extern void stack_frame_up(int varIndex);
extern int tokenMode;


extern std::vector<struct label> labels;

#define GOTO 0x02A8
#define GOSUB 0x02B2
#define PROC 0x0386

static unsigned int is_token = 0;
extern int last_var;

char *executeOnToken(char *ptr, unsigned short token)
{
	struct nativeCommand *cmd;
	char *ret;

	printf("** %s:%s **\n",__FILE__,__FUNCTION__);

	// we are at end of line, we need to find the next data command.

	switch (token)
	{
		case GOTO:
		case GOSUB:	
		case PROC:
					printf("PROC, GOSUB OR GOTO\n");
					is_token = token;
					return ptr;
		case 0x0000:
					return NULL;
	}

	// find the token
	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id ) 
		{
#ifdef show_token_numbers_yes
			printf("ON READ %08d   %08X %20s:%08d stack is %d cmd stack is %d flag %d token %04x -- name %s\n",
					getLineFromPointer(ptr), ptr +2,__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token , TokenName(token));	
#endif
			ret = cmd -> fn( cmd, ptr ) ;
			if (ret) ret += cmd -> size;
			return ret;
		}
	}

	token_not_found = token;
	setError(33, ptr);	// Out of Data

	return NULL;
}

static char *collect_data(char *ptr)
{
	unsigned short token = *((short *) ptr );
	setStackNum(0);
	ptr+=2;

	printf("First Token %04x\n",token);

	while ( (ptr = executeOnToken(  ptr,  token )) && (is_token == 0) )
	{
		token = *((short *) ptr );
		printf("Next\n");
		ptr += 2;	// next token.	
	}
	flushCmdParaStack( 0x0000 );
	return ptr;
}

char *execute_on( int num, char *tokenBuffer, unsigned short token );

char *cmdOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned int flags;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	is_token = 0;

	tokenBuffer += 4;	// skip ON data, maybe we don't need it.
	tokenBuffer = collect_data(tokenBuffer);

	while (cmdStack)
	{
		flags = cmdTmp[cmdStack-1].flag;

		if  ( flags & cmd_para )
		{
			cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack], 0);
		}
		else break;
	}

	tokenBuffer = execute_on( getStackNum(stack), tokenBuffer, is_token );

	return tokenBuffer-4-2;
}


char *execute_on( int num, char *tokenBuffer, unsigned short token )
{
	unsigned short ref_num = 0;
	unsigned short is_token = 0;
	unsigned short next_token = 0;
	char *ret = NULL;
	struct reference *ref = NULL;

	if ( (token==PROC) || (token==GOTO) || (token==GOSUB) ) 
	{

		for(;;)
		{	
			next_token = NEXT_TOKEN(tokenBuffer);

			printf("next token %04X - num %d\n", next_token, num);

			switch (next_token)
			{
				case 0x0006:

					is_token = 0x0006;
					tokenBuffer +=2;
					ref = (struct reference *) (tokenBuffer);
					num--;

					if (num == 0)
					{
						if (ref -> flags & type_proc )
						{
							ref_num = ref -> ref;
						}
						else
						{
							ref -> flags |= type_proc;
							ref -> ref = var_find_proc_ref( ref );
							ref_num = ref -> ref;
						}

						if (ref_num == 0) setError(22,tokenBuffer);
					}

					tokenBuffer += sizeof(struct reference) + ref -> length;
					break;

				case 0x0012:
					is_token = 0x0012;
					tokenBuffer +=2;
					ref = (struct reference *) (tokenBuffer);
					num--;

					if (num == 0)
					{
						if (ref -> flags & type_proc )
						{
							ref_num = ref -> ref;
						}
						else
						{
							ref -> ref = var_find_proc_ref( ref );
							ref_num = ref -> ref;
						}

						if (ref_num == 0) setError(22,tokenBuffer);
					}

					tokenBuffer += sizeof(struct reference) + ref -> length;
					break;

				case 0x0018:
					is_token = 0x0018;
					tokenBuffer +=2;
					ref = (struct reference *) (tokenBuffer);
					num--;

					printf("ref->ref %d\n",ref->ref);

					if (num == 0)
					{
						if (ref->ref == -1) // Because line start with 0x0018 is labels.
						{
							char *name = dupRef( ref ) ;
							if (name)
							{
								printf("name: %s\n",name);
								ref_num = ref->ref = findLabelRef( name, procStcakFrame[proc_stack_frame].id );
								free(name); 
							}

							if (ref_num == 0) 
							{
								printf("set errror\n");
								setError(22,tokenBuffer);
							}
						}
						else ref_num = ref->ref;
					}

					tokenBuffer += sizeof(struct reference) + ref -> length;
					break;

				case 0x001E:
				case 0x0036:
				case 0x003E:
					is_token = 0x003E;
					tokenBuffer +=2;

					num--;
					if (num == 0)
					{
						char num[50];
						sprintf(num,"%d", *((int *) tokenBuffer));
						ref_num = findLabelRef( num, procStcakFrame[proc_stack_frame].id );
					}

					tokenBuffer += 4;
					break;

				case 0x005C:
					tokenBuffer +=2;
					break;

				default: 

					goto exit_on_for_loop;
			}
		}

exit_on_for_loop:

		printf("ref_num %d\n",ref_num);

		if (ref_num>0)
		{
			struct label *label = NULL;

			switch (token)
			{
				case GOTO:	
						label = findLabel(globalVars[ref_num-1].varName, procStcakFrame[proc_stack_frame].id);
						ret = label ? label -> tokenLocation : NULL;
						break;

				case GOSUB:	

						// +2 next token

						switch (is_token)
						{
							case 0x0006:
									stackCmdLoop( _gosub_return, tokenBuffer+2 );
									label = findLabel(globalVars[ref_num-1].varName, procStcakFrame[proc_stack_frame].id );
									break;
							case 0x0018:
									stackCmdLoop( _gosub_return, tokenBuffer+2 );
									label = &labels[ref_num-1];
									break;
							case 0x003E:
									stackCmdLoop( _gosub_return, tokenBuffer+2 );
									label = &labels[ref_num-1];
									break;
						}

						ret = label ? label -> tokenLocation : NULL;
						break;

				case PROC:

						stackCmdProc( _procedure, tokenBuffer);  
						ret = globalVars[ref_num-1].var.tokenBufferPos;
						break;
			}
		}
	}

	if (ret) tokenBuffer = ret;

	return tokenBuffer;
}
