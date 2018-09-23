

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
#include "AmalCompiler.h"
#include "channel.h"

extern char *(*_do_set) ( struct glueCommands *data, int nextToken );
extern char *_setVar( struct glueCommands *data, int nextToken );
int amreg[26];

extern int last_var;
extern ChannelTableClass *channels;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroSpriteObject bobs[64];

extern void remove_lower_case(char *txt);

void channel_do_object( struct kittyChannel *self );

extern char *(*_do_set) ( struct glueCommands *data, int nextToken ) ;

unsigned int _set_amreg_num = 0;
int _set_amreg_channel = 0;

char *_set_amreg_fn( struct glueCommands *data, int nextToken )
{
	// don't need to check num, was checked before.
	amreg[ _set_amreg_num] = getStackNum( stack );
	_do_set = _setVar;
	return NULL;
}

char *_set_amreg_channel_fn( struct glueCommands *data, int nextToken )
{
	struct kittyChannel *item;

	// don't need to check num, was checked before.

	engine_lock();				// most be thread safe!!!
	if (item = channels -> getChannel(_set_amreg_channel))
	{
		 item -> reg[_set_amreg_num] = getStackNum( stack );
	}
	else
	{
		if (item = channels -> newChannel( _set_amreg_channel ) )
		{
			item -> reg[_set_amreg_num] = getStackNum( stack );
		}
	}
	engine_unlock();

	_do_set = _setVar;
	return NULL;
}

char *_amalSetAmReg( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	int ret = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			_set_amreg_num = getStackNum( stack );

			if (_set_amreg_num<26)		// unsigned don't need to check, more then.
			{
				_do_set = _set_amreg_fn;
			}
			else setError(11, data->tokenBuffer);
			break;

		case 2:
			{
				_set_amreg_num = getStackNum( stack-1 );
				_set_amreg_channel = getStackNum( stack );
				if (_set_amreg_num<10)		// unsigned don't need to check, more then.
				{
					_do_set = _set_amreg_channel_fn;
				}
				else setError(11, data->tokenBuffer);
			}
			break;

		defaut:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( ret );

	return NULL;
}


char *_amalGetAmReg( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	unsigned int num = 0;
	int channel = 0;
	int ret = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			num = getStackNum( stack );

			if (num<26)		// unsigned don't need to check, more then.
			{
				ret = amreg[num];
			}
			else setError(11, data->tokenBuffer);

			break;

		case 2:
			{
				struct kittyChannel *item;

				num = getStackNum( stack-1 );
				channel = getStackNum( stack );

				if (num<10)		// unsigned don't need to check, more then.
				{
					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						ret = item -> reg[num];
					}
					else
					{
						if (item = channels -> newChannel( channel ) )
						{
							ret = item -> reg[num];
						}
					}
					engine_unlock();
				}
				else setError(11, data->tokenBuffer);
			}
			break;

		defaut:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackNum( ret );

	return NULL;
}

char *amalAmReg(struct nativeCommand *cmd, char *tokenBuffer)
{
	if ((last_tokens[parenthesis_count] == 0x0000) || (last_tokens[parenthesis_count] == 0x0054))
	{
		stackCmdParm( _amalSetAmReg, tokenBuffer );
	}
	else
	{
		stackCmdParm( _amalGetAmReg, tokenBuffer );
	}

	return tokenBuffer;
}

char *_amalChannel( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	do_to = do_to_default;
	switch (args)
	{
		case 3:
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack - 2 );
				int token = getStackNum( stack - 1 );
				int number = getStackNum( stack );

				printf("channel %d, token %04x, number %d\n", channel, token, number);

				engine_lock();	
				if (item = channels -> getChannel(channel))
				{
					printf("change the one we have\n");

					item -> token = token;
					item -> number = number;
				}
				else
				{
					printf("not found... add new\n");

					if (item = channels -> newChannel( channel ))
					{
						setChannel( item, NULL, NULL );
						item -> token = token;
						item -> number = number;

						printf("we have a new item\n");
					}
				}
				engine_unlock();	
			}
			break;
		defaut:	setError(22,data->tokenBuffer);
	}

	getchar();

	popStack( stack - data->stack );
	return NULL;
}

char *do_to_channel( struct nativeCommand *cmd, char *tokenbuffer )
{
	unsigned short token = *((unsigned short *) tokenbuffer);
	char *ret = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (token)
	{
		case 0x1A94:	// Channel x To Sprite y
		case 0x1B9E: 	// Channel x To Bob y
		case 0x0A18:	// Channel x To Display y

					stack ++;
					setStackNum( token );
					stack ++;

					ret = tokenbuffer+2;
					break;
		default:
			printf("bad token for TO command %04x\n",token);

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

void channel_amal( struct kittyChannel *self )
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	Printf("script: %s\n", self -> at);
	Printf("frame %ld\n", self -> frame);
	channel_do_object( self );
}


char *_amalAmal( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int channel = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack -1 );
				char *script = getStackString( stack );
				char *nscript;

				engine_lock();				// most be thread safe!!!
				if (item = channels -> getChannel(channel))
				{
					nscript = strdup(script);
					if (nscript)
					{
						remove_lower_case( nscript );
						setChannel( item, channel_amal, nscript  );
					}
				}
				else
				{
					if (item = channels -> newChannel( channel ))
					{
						item -> token = 0x1A94;	// default to sprite token
						item -> number = channel;	// default to srpite 

						nscript = strdup(script);
						if (nscript)
						{
							remove_lower_case( nscript );
							setChannel( item, channel_amal, nscript );
						}
					}
				}
				engine_unlock();	
			}
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
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack  );

				if (item = channels -> getChannel(channel))
				{
					item -> status = channel_status::active;
				}
			}
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
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack );

				if (item = channels -> getChannel(channel))
				{
					item -> status = channel_status::done;
				}
			}
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

void channel_anim( struct kittyChannel *self )
{
	if (self -> sleep >= self -> sleep_to )
	{
		int sign = 1;
		int num = 0;
		int arg = 0;
		int para = 0;
		char *c;

		for (c=self->at;*c;c++)
		{
			if (*c=='L') c = self -> script;
			if (*c=='(') para++;
			if ((*c>='0')&&(*c<='9')) num = (num*10) + (*c-'0');
			if (*c=='-') sign = -1;
			if ((*c==',')||(*c==')'))
			{
				switch (arg)
				{
					case 0: self -> frame = num;  break;
					case 1: self -> sleep_to = num; self -> sleep = 0; break;
				}

				arg ++;num = 0;sign = 1;
			}

			if (*c==')') 
			{
				c++;para --;break;
			}
		}

		self -> at = c;
	}
	else
	{
		self -> sleep ++;
		if (self -> sleep == self -> sleep_to)
		{
			Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
			Printf("script: %s\n", self -> at);
			Printf("frame %ld\n", self -> frame);
			channel_do_object( self );
		}
	}
}


char *_amalAnim( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int channel = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack -1 );
				char *script = getStackString( stack );

				if (item = channels -> getChannel(channel))
				{
					setChannel( item, channel_anim, strdup( script ) );
				}
			}
			break;
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



// we are in a engine lock in channel_do_object, do not try to lock again!
void channel_do_object( struct kittyChannel *self )
{
	switch (self -> token)
	{
		case 0x1A94:	// Channel x To Sprite y

			Printf("Channel %ld to sprite %ld\n",self -> id, self -> number);
			break; 

		case 0x1B9E: 	// Channel x To Bob y

			Printf("Channel %ld to Bob %ld\n",self -> id, self -> number);

			if (self -> number<64)
			{
				struct retroSpriteObject *bob = &bobs[self -> number];

				bob -> y += self -> deltay;
				bob -> x += self -> deltax;
				if (self -> frame>0) bob -> image = self->frame;
			}
			break;

		case 0x0A18:	// Channel x To Display y

			Printf("Channel %ld to Display %ld\n",self -> id, self -> number);
			{
				struct retroScreen *screen =	screens[self -> number] ;

				if (screen)
				{
					screen -> scanline_y += self -> deltay;
					screen -> scanline_x += self -> deltax;
					video -> refreshAllScanlines = TRUE;
				}
			}

			break;

		default: 
			Printf("wrong object %04lx, object not set\n", self -> token);
	}
}

void channel_movex( struct kittyChannel *self )
{
	if (self -> count >= self -> count_to )
	{
		int sign = 1;
		int num = 0;
		int arg = 0;
		int para = 0;
		char *c;

		for (c=self->at;*c;c++)
		{
			if (*c=='L') c = self -> script;
			if (*c=='(') para++;
			if ((*c>='0')&&(*c<='9')) num = (num*10) + (*c-'0');
			if (*c=='-') sign = -1;
			if ((*c==',')||(*c==')'))
			{
				switch (arg)
				{
					case 0: self -> sleep_to = num; self -> sleep = 0; break;
					case 1: self -> deltax = sign * num; break;
					case 2: self -> count_to = num; self -> count = 0; break;
				}

				arg ++;num = 0;sign = 1;
			}

			if (*c==')') 
			{
				c++;para --;break;
			}
		}

		if (self -> count != self -> count_to) self -> at = c;
	}
	else
	{
		self -> sleep ++;
		if (self -> sleep >= self -> sleep_to)
		{
			self -> sleep = 0;
			self -> count++;
			channel_do_object( self );
		}
	}
}


void channel_movey( struct kittyChannel *self )
{
	if (self -> count >= self -> count_to )
	{
		int sign = 1;
		int num = 0;
		int arg = 0;
		int para = 0;
		char *c;

		for (c=self->at;*c;c++)
		{
			if (*c=='L') c = self -> script;
			if (*c=='(') para++;
			if ((*c>='0')&&(*c<='9')) num = (num*10) + (*c-'0');
			if (*c=='-') sign = -1;
			if ((*c==',')||(*c==')'))
			{
				switch (arg)
				{
					case 0: self -> sleep_to = num; self -> sleep = 0; break;
					case 1: self -> deltay = sign * num; break;
					case 2: self -> count_to = num; self -> count = 0; break;
				}

				arg ++;num = 0;sign = 1;
			}

			if (*c==')') 
			{
				c++;para --;break;
			}
		}

		if (self -> count != self -> count_to) self -> at = c;
	}
	else
	{
		self -> sleep ++;
		if (self -> sleep >= self -> sleep_to)
		{
			self -> sleep = 0;
			self -> count++;
			channel_do_object( self );
		}
	}
}

char *_amalMoveX( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2: 
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack -1 );
				char *txt = getStackString( stack );

				if (txt)
				{
					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						setChannel( item, channel_movex, strdup(txt) );
					}
					else
					{
						if (item = channels -> newChannel( channel ) )
						{
							setChannel( item, channel_movex, strdup(txt) );
						}
					}
					engine_unlock();
				}
			}
			break;
		default:
			setError(22,data->tokenBuffer);;
			break;
	}

	popStack( stack - data->stack );
	return NULL;
}


char *amalMoveX(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _amalMoveX, tokenBuffer );
	return tokenBuffer;
}

char *_amalMoveY( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2: 
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack -1 );
				char *txt = getStackString( stack );

				if (txt)
				{
					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						setChannel( item, channel_movey, strdup(txt) );
					}
					else
					{
						if (item = channels -> newChannel( channel ) )
						{
							setChannel( item, channel_movey, strdup(txt) );
						}
					}
					engine_unlock();
				}
			}
			break;
		default:
			setError(22,data->tokenBuffer);;
			break;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *amalMoveY(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _amalMoveY, tokenBuffer );
	return tokenBuffer;
}

char *_amalMoveOn( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	// this code don't need to be tread safe, I'm not changing the script, or adding new channels,

	switch (args)
	{
		case 0:
			{
				int n;
				for (n=0;n<channels -> _size();n++) channels -> item(n) -> status = channel_status::active;
			}
			break;
		case 1:
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack  );

				if (item = channels -> getChannel(channel))
				{
					item -> status = channel_status::active;
				}
			}
			break;
		default:
			setError(22,data->tokenBuffer);;
			break;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *amalMoveOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _amalMoveOn, tokenBuffer );
	return tokenBuffer;
}

