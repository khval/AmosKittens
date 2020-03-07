
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

char *_setVar( struct glueCommands *data, int nextToken );
char *_cmdRead( struct glueCommands *data, int nextToken );
char *FinderTokenInBuffer( char *ptr, unsigned short token , unsigned short token_eof1, unsigned short token_eof2, char *_eof_ );


void read_from_data()
{
	unsigned short token;
	int _len;
	bool neg = false;

		if (data_read_pointers[proc_stack_frame])
		{
			bool try_next_token;
	
			do
			{
				try_next_token = false;
				token = *((short *) data_read_pointers[proc_stack_frame]);

				switch (token)
				{
					case 0x0404:	// data
							data_read_pointers[proc_stack_frame]+=4;	// token + data size 2
							neg = false;
							try_next_token = true;
							break;

					case 0xFFCA:	// negative number.
							data_read_pointers[proc_stack_frame] +=2;
							neg = true;
							try_next_token = true;	
							break;

					case 0x003E: 	// num
							{
								int num = *((int *) (data_read_pointers[proc_stack_frame] + 2));
								setStackNum ( neg ? -num :  num );
							}
							data_read_pointers[proc_stack_frame] +=6;
							break;
	
					case 0x0026:	// string
							_len = *((unsigned short *) (data_read_pointers[proc_stack_frame] + 2));
							_len = _len + (_len & 1);
							if (kittyStack[__stack].str) free(kittyStack[__stack].str);
							kittyStack[__stack].str = strndup( data_read_pointers[proc_stack_frame] + 4, _len );
							kittyStack[__stack].len = strlen( kittyStack[__stack].str );
							data_read_pointers[proc_stack_frame] +=4 + _len;
							break;

					case 0x000c:	// label
							{
								struct reference *ref = (struct reference *) (data_read_pointers[proc_stack_frame] + 2);
								data_read_pointers[proc_stack_frame] += 2 + sizeof(struct reference) + ref -> length;
							}
							try_next_token = true;
							break;

					case 0x005C:	// comma
							data_read_pointers[proc_stack_frame] +=2;
							neg = false;
							try_next_token = true;
							break;

					case 0x0000:	// new line
							data_read_pointers[proc_stack_frame] +=4;
							try_next_token = true;	
							break;
					default:
							printf("SHIT unkown token\n");
							getchar();

							data_read_pointers[proc_stack_frame] = FinderTokenInBuffer( data_read_pointers[proc_stack_frame], 0x0404 , -1, -1, _file_end_ );
							try_next_token = true;	
				}

				if (data_read_pointers[proc_stack_frame] == 0x0000) break;
			} while ( try_next_token );
		}
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
		args =__stack - cmdTmp[cmdStack].stack + 1;
	}
	else
	{
		if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _cmdRead)
		{
			args =__stack - cmdTmp[cmdStack-1].stack + 1;
		}
	}
	
	if (last_var)
	{
		_set_var_index = globalVars[last_var -1].var.index;
		read_from_data();
	}

	data.lastVar = last_var;
	_setVar( &data,0 );
}

char *_cmdRead( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	int args =__stack - data -> stack +1;
	_read_arg( NULL, NULL );
	popStack(__stack - data -> stack  );
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