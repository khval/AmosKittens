#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <iostream>
#include <vector>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <retromode.h>
#include <retromode_lib.h>
#include <unistd.h>
#endif

#include <amosKittens.h>
#include <stack.h>

#include "debug.h"
#include "commands.h"
#include "commandsErrors.h"
#include "kittyErrors.h"
#include "label.h"
#include "amosString.h"
#include "var_helper.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;
extern char *tokenbufferResume;

extern int findVarPublic( char *name, int type );
extern std::vector<struct label> labels;

char *(*onErrorTemp)(char *ptr) = NULL;
char *(*onError)(char *ptr) = NULL;
char *on_error_goto_location = NULL;
char *on_error_proc_location = NULL;
char *resume_location = NULL;

void name_from_ref( char **tokenBuffer, char **name_out)
{
	unsigned short next_token = NEXT_TOKEN(*tokenBuffer );

	if ((next_token == 0x006) || (next_token == 0x0018))
	{
		struct reference *ref = (struct reference *) (*tokenBuffer + 2);
		*name_out = strndup( *tokenBuffer + 2 + sizeof(struct reference), ref->length );
		*tokenBuffer += (2 + sizeof(struct reference) + ref -> length) ;	
	}	
}

char *errOnError(nativeCommand *cmd, char *tokenBuffer)
{
	char *name = NULL;

	onError = onErrorBreak;	// default.

	switch ( NEXT_TOKEN(tokenBuffer ))
	{
		case 0x02A8:	// Goto

				printf("On Error ... Goto ...\n");

				tokenBuffer += 2;				
				name_from_ref(&tokenBuffer, &name);
				if (name)
				{
					struct label *label =  findLabel(name, procStcakFrame[proc_stack_frame].id);
					free(name);

					if (label)
					{
						on_error_goto_location = label -> tokenLocation;
					}
					else
					{
						printf("label not found, code broken\n");
						setError(40, tokenBuffer);
						return NULL;
					}

					onError = onErrorGoto;
				}
				break;

		case 0x0386:	// Proc

				printf("On Error ... Proc ...\n");

				tokenBuffer += 2;

				if (NEXT_TOKEN(tokenBuffer ) == 0x0012)	// proc
				{
					struct reference *ref = (struct reference *) (tokenBuffer + 2);
					int found = var_find_proc_ref( ref );

					if (found)
					{
						on_error_proc_location = globalVars[found -1].var.tokenBufferPos;
						onError = onErrorProc;
					}

					tokenBuffer += (2 + sizeof(struct reference) + ref -> length) ;	
				}
				else
				{
					setError(22,tokenBuffer);
				}

				break;
	}
	return tokenBuffer;
}

char *errEndProc(struct nativeCommand *cmd, char *tokenBuffer );

char *errResumeLabel(nativeCommand *cmd, char *tokenBuffer)
{
	printf("Next token %04x\n",NEXT_TOKEN(tokenBuffer));

	switch (NEXT_TOKEN(tokenBuffer))
	{
		case 0x0018:

			{
				struct reference *ref = (struct reference *) (tokenBuffer + 2);
				struct label *label = var_JumpToName( ref );		// after function, amos kittens try access next token and adds +2 (+0 data)

				if (label)
				{
					resume_location = label -> tokenLocation;
				}

				tokenBuffer += 2;
				tokenBuffer += sizeof(struct reference) + ref -> length;
			}
			break;

		case 0x0000:
		case 0x0054:

			if (dropProgStackToProc( _procedure ))
			{
				if (cmdTmp[instance.cmdStack-1].cmd == _procedure ) 
				{
					printf(" maybe need flush some stack here? %d - %d --\n", cmdTmp[instance.cmdStack-1].stack,__stack );
					tokenBuffer=cmdTmp[--instance.cmdStack].cmd(&cmdTmp[instance.cmdStack],0) - 2;		// +2 will be added on exit.
				}
			}
			if ( resume_location ) tokenBuffer = resume_location -2;		// +2 will be added on exit.
			break;
	}

	return tokenBuffer;
}

char *errResumeNext(nativeCommand *cmd, char *tokenBuffer)
{
//	struct reference *ref;
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	NYI(__FUNCTION__);
	return tokenBuffer;
}


char *onErrorBreak(char *ptr)
{
	return NULL;
}

char *onErrorIgnore(char *ptr)
{
	return ptr;
}

char *onErrorGoto(char *ptr)
{
	instance.kittyError.newError = false;
	return on_error_goto_location -2;
}

char *onErrorProc(char *ptr)
{
	instance.kittyError.newError = false;
	stackCmdLoop( _procedure, ptr);
	return on_error_proc_location -2;
}

char *_errError( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args =__stack - data->stack +1 ;

	if (args == 1)
	{
		setError( getStackNum(__stack), data -> tokenBuffer );
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *errError(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _errError, tokenBuffer );
	return tokenBuffer;
}


char *errResume(struct nativeCommand *cmd, char *tokenBuffer)
{
	char *name = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	name_from_ref(&tokenBuffer, &name);

	if (name)	// has args
	{
		char *ret;
		struct label *label =  findLabel(name, procStcakFrame[proc_stack_frame].id);
		ret = label -> tokenLocation;
		free(name);

		proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		if (ret) 
		{
			proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

			instance.kittyError.code = 0;
			instance.kittyError.pos = 0;  
			instance.kittyError.newError = false;
			return ret -2;
		}

		proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	}
	else	// has no args, return to error.
	{
		proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		if (instance.kittyError.posResume)
		{
			tokenBuffer = instance.kittyError.posResume - 2;		// -2 for location of the error, -2 for resume command.

			proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

			instance.kittyError.code = 0;
			instance.kittyError.pos = 0;  
			instance.kittyError.posResume = 0;
			instance.kittyError.newError = false;
			return tokenBuffer;
		}
		else 
		{
			printf("do not have resume location\n");
			return NULL;	// exit here.
		}
	}

	return tokenBuffer;
}


char *_errTrap( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (onErrorTemp)
	{
		onError = onErrorTemp;
		onErrorTemp = NULL;

		if (instance.kittyError.code)
		{
			instance.kittyError.trapCode = instance.kittyError.code;
			instance.kittyError.code = 0;
			instance.kittyError.newError = false;
		}
	}

	return NULL;
}

char *errTrap(nativeCommand *err, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	onErrorTemp = onError;
	onError = onErrorIgnore;
	stackCmdFlags( _errTrap, tokenBuffer, cmd_onNextCmd | cmd_onEol );
	instance.kittyError.trapCode = 0;
	return tokenBuffer;
}


char *errErrn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackNum( instance.kittyError.code );
	return tokenBuffer;
}

char *errErrTrap(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackNum( instance.kittyError.trapCode );
	return tokenBuffer;
}

char *_errErrStr( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	int args =__stack - data->stack +1 ;
	struct stringData *err_str = NULL;
	int err = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1: 	err = getStackNum(__stack);
				break;
		default:
				popStack(__stack - data->stack );
				setError(22,data->tokenBuffer);
				return NULL;
	}


	for (struct error *e = errorsRunTime; e->errorText;e++ )
	{
		if (e -> errorCode == err )
		{
			err_str = toAmosString( e -> errorText, strlen(e -> errorText) );
			break;
		}
	}

	if (err_str == NULL) err_str = toAmosString( "", 0 );
	setStackStr( err_str );

	return NULL;
}

char *errErrStr(nativeCommand *err, char *tokenBuffer)
{
	stackCmdParm( _errErrStr, tokenBuffer );
	return tokenBuffer;
}



