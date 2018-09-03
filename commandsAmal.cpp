

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "debug.h"
#include <string>
#include <proto/dos.h>
#include <vector>
#include <proto/retroMode.h>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsAmal.h"
#include "var_helper.h"
#include "errors.h"
#include "engine.h"
#include "channel.h"

extern int last_var;
extern ChannelTableClass *channels;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;

extern struct retroSpriteObject bobs[64];

void setChannel( struct kittyChannel *item, void (*cmd) (struct kittyChannel *) ,char *str);
void channel_do_object( struct kittyChannel *self );

extern char *(*_do_set) ( struct glueCommands *data, int nextToken ) ;

char *_amalAmReg( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int num = 0;
	int channel = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			num = getStackNum( stack );
			break;

		case 2:
			num = getStackNum( stack-1 );
			channel = getStackNum( stack );
			break;

		defaut:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );

	return NULL;
}

char *amalAmReg(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _amalAmReg, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_amalChannel( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int channel = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	do_to = do_to_default;

	switch (args)
	{
		case 2:	printf("Sorry Amal not yet working...\nBut it looks ok\n");
				break;
		defaut:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );

	return NULL;
}

char *do_to_channel( struct nativeCommand *cmd, char *tokenbuffer )
{
	unsigned short token = *((unsigned short *) tokenbuffer);
	char *ret = NULL;

	switch (token)
	{
		case 0x1B9E: 	// Channel x To Bob y

					stack ++;
					setStackNum( token );
					ret = tokenbuffer+2;
					break;

		default:
			setError(22,tokenbuffer);
	}

	return ret;
}

char *amalChannel(struct nativeCommand *cmd, char *tokenBuffer)
{
//       Channel(A+3) To Bob A+3 

	do_to = do_to_channel;

	stackCmdNormal( _amalChannel, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_amalAmal( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int channel = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:	printf("Sorry Amal not yet working...\nBut it looks ok\n");
				break;
		defaut:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );

	return NULL;
}

char *amalAmal(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _amalAmal, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_amalAmalOn( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int channel = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			channel = getStackNum( stack );
			break;

		defaut:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );

	return NULL;
}

char *amalAmalOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _amalAmalOn, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_amalAmalOff( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int channel = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			channel = getStackNum( stack );
			break;

		defaut:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );

	return NULL;
}

char *amalAmalOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _amalAmalOff, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_amalAnim( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int channel = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		defaut:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );

	return NULL;
}

char *amalAnim(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _amalAnim, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *amalAnimOn(struct nativeCommand *cmd, char *tokenBuffer)		// this dummy don't do anything.
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

// call back after reading args this called..

char *_amalAmalFreeze( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int channel = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	channel = getStackNum( stack );
				// we should freeze amal channel, we don't yet excute amal code!.
				break;

		defaut:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );

	return NULL;
}

char *amalAmalFreeze(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _amalAmalFreeze, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}
