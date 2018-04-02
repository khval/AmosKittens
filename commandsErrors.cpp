#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "debug.h"
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsErrors.h"
#include "errors.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

extern char *findLabel( char *name );

char *(*onError)(char *ptr) = NULL;
char *on_error_goto_location = NULL;

char *_cmdError( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;

	if (args == 1)
	{
		setError(_stackInt(stack));
	}

	popStack( stack - data->stack );
	return NULL;
}

extern char *cmdOnError(nativeCommand *cmd, char *tokenBuffer)
{
	printf("Next token %04x\n",NEXT_TOKEN(tokenBuffer));

	switch ( NEXT_TOKEN(tokenBuffer ))
	{
		case 0x02A8:
				tokenBuffer += 2;
				
				if (NEXT_TOKEN(tokenBuffer ) == 0x006)	// label
				{
					char *name;
					struct reference *ref = (struct reference *) (tokenBuffer + 2);

					name = strndup( tokenBuffer + 2 + sizeof(struct reference), ref->length );

					onError = onErrorBreak;	// default.

					if (name)
					{
						on_error_goto_location = findLabel(name);
						onError = onErrorGoto;
						free(name);
					}

					tokenBuffer += (2 + sizeof(struct reference) + ref -> length) ;					
				}

				printf("On error goto 0x%08X\n", on_error_goto_location);
				break;

		case 0x0386:
				tokenBuffer += 2;
				printf("token %04x\n",NEXT_TOKEN(tokenBuffer));

				if (NEXT_TOKEN(tokenBuffer ) == 0x0012)	// proc
				{
					char *name;
					struct reference *ref = (struct reference *) (tokenBuffer + 2);

					name = strndup( tokenBuffer + 2 + sizeof(struct reference), ref->length );

					onError = onErrorBreak;	// default.

					if (name)
					{
						printf("name: %s\n",name);

//						on_error_goto_location = findLabel(name);
//						onError = onErrorGoto;
						free(name);
					}

					tokenBuffer += (2 + sizeof(struct reference) + ref -> length) ;	
				}

				break;
	}

	return tokenBuffer;
}

extern char *cmdResumeLabel(nativeCommand *cmd, char *tokenBuffer)
{
	printf("Next token %04x\n",NEXT_TOKEN(tokenBuffer));

	return tokenBuffer;
}



char *onErrorBreak(char *ptr)
{
	return kittyError.newError ? NULL : ptr;
}

char *onErrorGoto(char *ptr)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ( kittyError.newError )
	{
		kittyError.newError = false;
		return on_error_goto_location;
	}
	else
	{
		return ptr;
	}
}

char *cmdError(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdError, tokenBuffer );
	return tokenBuffer;
}


