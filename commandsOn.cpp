
#include "stdafx.h"

#include <vector>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <stdint.h>
#include "os/linux/stuff.h"
#endif

#include <amosKittens.h>
#include <stack.h>
#include "commands.h"
#include "engine.h"
#include "debug.h"
#include "kittyErrors.h"
#include "var_helper.h"
#include "label.h"


extern struct globalVar globalVars[];
extern struct nativeCommand nativeCommands[];
extern struct globalVar globalVars[1000];
extern std::vector<struct label> labels;
extern struct stackFrame *currentFrame;

extern int nativeCommandsSize;
extern unsigned short token_not_found;
extern int tokenMode;
static unsigned int is_token_cmd = 0;

extern const char *TokenName( unsigned short token );
extern char *_gosub_return( struct glueCommands *data, int nextToken );
extern char *dupRef( struct reference *ref );
extern void stack_frame_up(int varIndex);

char *executeOnToken(char *ptr, unsigned short token)
{
	struct nativeCommand *cmd;
	char *ret;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	// we are at end of line, we need to find the next data command.

	switch (token)
	{
		case token_goto:
		case token_gosub:	
		case token_proc:
					printf("PROC, GOSUB OR GOTO\n");
					is_token_cmd = token;
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
			getLineFromPointer(ptr);
			printf("ON READ %08d   %08X %20s:%08d stack is %d cmd stack is %d flag %d token %04x -- name %s\n",
					lineFromPtr.line, ptr +2,__FUNCTION__,__LINE__, instance.stack, instance.cmdStack, kittyStack[__stack].state, token , TokenName(token));	
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

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	while ( (ptr = executeOnToken(  ptr,  token )) && (is_token_cmd == 0) )
	{
		token = *((short *) ptr );
		ptr += 2;	// next token.	
	}
	flushCmdParaStack( 0x0000 );

	return ptr;
}


char *execute_on( int num, char *tokenBuffer, char *returnTokenBuffer, unsigned short token );

char *cmdOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	unsigned int flags;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	is_token_cmd = 0;

	tokenBuffer += 4;	// skip ON data, maybe we don't need it.
	tokenBuffer = collect_data(tokenBuffer);

	while (instance.cmdStack)
	{
		flags = cmdTmp[instance.cmdStack-1].flag;

		if  ( flags & cmd_para )
		{
			cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack], 0);
		}
		else break;
	}

	tokenBuffer = execute_on( getStackNum(__stack), tokenBuffer, NULL, is_token_cmd );

	return tokenBuffer-4;	// on function exit +4 is added.
}


struct label *GetLabel( unsigned int is_token, int ref_num )
{
	struct label *label = NULL;

	switch (is_token)
	{
		case 0x0006:		label = findLabel(globalVars[ref_num-1].varName, currentFrame -> id );
						break;

		case 0x0018:		label = &labels[ref_num-1];
						break;

		case 0x003E:		label = &labels[ref_num-1];
						break;
	}
	return label;
}

char *do_ON_command(uint16_t token_cmd,  uint16_t token_item, int ref_num, char *tokenBuffer )
{
	char *ret = NULL;

	if (ref_num>0)
	{
		struct label *label = NULL;

		switch (token_cmd)
		{
			case token_goto:	

					dprintf("--GOTO--\n");

					label = GetLabel( token_item, ref_num );
					if (label)
					{
						dropProgStackAllFlag( cmd_true | cmd_false );	// just kill the if condition, if any.
						ret = label -> tokenLocation -2;
					}
					break;

			case token_gosub:	

					dprintf("--GOSUB--\n");
					label = GetLabel( token_item, ref_num );
					if (label)
					{
						stackCmdLoop( _gosub_return, tokenBuffer+2 );
						ret = label -> tokenLocation -2;
					}
					break;

			case token_proc:

					dprintf("--PROC--\n");

					{
						int idx = ref_num - 1;
						int oldStack = __stack;
						stackCmdProc( _procedure, tokenBuffer);  
						cmdTmp[instance.cmdStack-1].stack = oldStack;	// carry stack.

						tokenMode = mode_store;
						stack_frame_up(idx);
						ret = globalVars[idx].var.tokenBufferPos-2;
					}
					break;
		}
	}
	return ret;
}

char *execute_on( int num, char *tokenBuffer, char *returnTokenBuffer, unsigned short token_cmd )
{
	unsigned short ref_num = 0;
	unsigned short is_token = 0;
	unsigned short next_token = 0;
	char *ret = NULL;
	struct reference *ref = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( (token_cmd==token_proc) || (token_cmd==token_goto) || (token_cmd==token_gosub) ) 
	{

		for(;;)
		{	
			next_token = NEXT_TOKEN(tokenBuffer);

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

					if (num == 0)
					{
						if (ref->ref == -1) // Because line start with 0x0018 is labels.
						{
							char *name = dupRef( ref ) ;
							if (name)
							{
								dprintf("name: %s\n",name);
								ref_num = ref->ref = findLabelRef( name, currentFrame -> id );
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
						ref_num = findLabelRef( num, currentFrame -> id );
					}

					tokenBuffer += 4;
					break;

				case 0x005C:
					tokenBuffer +=2;
					break;

				case 0x0000:	// exit at end of list..
				case 0x0054:
					ret = tokenBuffer +2;

				default: 
					goto exit_on_for_loop;
			}
		}

exit_on_for_loop:
		ret = do_ON_command( token_cmd, is_token, ref_num, returnTokenBuffer ? returnTokenBuffer :  tokenBuffer );
	}

	// if this function is used some where else then in normal ON ... PROC. 
	if (returnTokenBuffer) tokenBuffer = NULL;

	if (ret) tokenBuffer = ret;

	return tokenBuffer;
}
