
#include <vector>
#include <proto/exec.h>
#include <proto/dos.h>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "engine.h"
#include "debug.h"
#include "errors.h"

extern int last_var;
extern struct globalVar globalVars[];
extern int _last_var_index;		// we need to know what index was to keep it.
extern int _set_var_index;		// we need to resore index 
extern unsigned short token_not_found;

// this code should be able to handel expresion in data statement.
//
// The process:
//
// Read [object] -> [keep] -> [execute code from data] -> [store to keeped var], [object] -> [keep] -> [execute code from data] -> [store to keeped var]

extern struct nativeCommand nativeCommands[];
extern int nativeCommandsSize;
extern char *_setVar( struct glueCommands *data, int nextToken );
extern char *_cmdRead( struct glueCommands *data, int nextToken );
extern char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ );
extern const char *TokenName( unsigned short token );

void _read_arg( struct nativeCommand *cmd, char *tokenBuffer );
void _exit_read_data( struct nativeCommand *cmd, char *tokenBuffer );


char *executeDataToken(char *ptr, unsigned short token)
{
	struct nativeCommand *cmd;
	char *ret;

	printf("** %s:%s **\n",__FILE__,__FUNCTION__);

	// we are at end of line, we need to find the next data command.

	switch (token)
	{
		case 0x0000:
					// printf("** END  OF LINE **\n");

					ptr = FinderTokenInBuffer( ptr, 0x0404 , -1, -1, _file_end_ );

					if (ptr)	// if ptr, then token is 0x0404 (new line)
					{
						ptr+=4;	// skip token.
						data_read_pointers[proc_stack_frame] = ptr;	// set data_read_poiner
						
						// end of line => comma, exit we have read something I hope.
						if (do_input[parenthesis_count] == _exit_read_data) 
						{
							do_input[parenthesis_count] = _read_arg;
							return NULL;
						}
					}
					else
					{
						return NULL;
					}

					break;

		case 0x005C: 
					// printf("** EXIT ON COMMA **\n");

					// comma, exit we have read something I hope.
					if (do_input[parenthesis_count] == _exit_read_data) 
					{
						do_input[parenthesis_count] = _read_arg;
						return NULL;
					}
					break;

		case 0x0404:
					// printf("** SKIP DATA CMD **\n");

					// data is expected
					return ptr+4;	// valid
	}

	// find the token
	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id ) 
		{
#ifdef show_token_numbers_yes
			printf("DATA READ %08d   %08X %20s:%08d stack is %d cmd stack is %d flag %d token %04x -- name %s\n",
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

void collect_data()
{
	unsigned short token;
	char *ptr = data_read_pointers[proc_stack_frame];

	setStackNum(0);
	token = *((short *) ptr );
	ptr+=2;

	while ( ptr = executeDataToken(  ptr,  token ) )
	{
		last_tokens[parenthesis_count] = token;
		token = *((short *) ptr );
		ptr += 2;	// next token.	
		data_read_pointers[proc_stack_frame] = ptr;	// store last valid.
	}
}

void _exit_read_data( struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
}


void _read_arg( struct nativeCommand *cmd, char *tokenBuffer )
{
	int args = 0;
	int num = 0;
	double des = -1.0f;
	struct glueCommands data;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);


	if (cmd == NULL)
	{
		args = stack - cmdTmp[cmdStack].stack + 1;
	}
	else
	{
		if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _cmdRead)
		{
			args = stack - cmdTmp[cmdStack-1].stack + 1;
		}
	}
	
	if (last_var)
	{
		int local_var_index = globalVars[last_var -1].var.index;
		int local_last_var = last_var;

		do_input[parenthesis_count] = _exit_read_data;
		collect_data();	

		_set_var_index = local_var_index;
		last_var = local_last_var;
		data.lastVar = last_var;
	}

	_setVar( &data,0 );
/*
	dump_global();
	getchar();
*/
}

char *_cmdRead( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	int args = stack - data -> stack +1;
	_read_arg( NULL, NULL );
	popStack( stack - data -> stack  );
	do_input[parenthesis_count] = do_std_next_arg;
	do_breakdata = NULL;

	return NULL;
}

char *cmdRead(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (data_read_pointers[proc_stack_frame] == 0x0000) 
	{
		printf("we are here\n");
		setError( 25, tokenBuffer);
	}
	else
	{
		do_input[parenthesis_count] = _read_arg;
		do_breakdata = NULL;
		stackCmdNormal( _cmdRead, tokenBuffer );
	}

	return tokenBuffer;
}

