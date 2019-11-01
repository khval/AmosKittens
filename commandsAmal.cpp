
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
#include <retromode.h>
#include <retromode_lib.h>
#endif

#include "debug.h"
#include <string>
#include <vector>
#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsAmal.h"
#include "var_helper.h"
#include "kittyErrors.h"
#include "engine.h"
#include "AmalCompiler.h"
#include "channel.h"
#include "amal_object.h"
#include "amosstring.h"

extern char *(*_do_set) ( struct glueCommands *data, int nextToken );
extern char *_setVar( struct glueCommands *data, int nextToken );
int amreg[26];

extern int last_var;
extern ChannelTableClass *channels;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroSpriteObject sprites[64];
extern struct retroSpriteObject bobs[64];

extern void remove_lower_case( struct stringData *txt);

void setChannelToken(struct kittyChannel *item,int token, int number);
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
			setChannelToken(item,0x1A94,_set_amreg_channel); // default to sprite token
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
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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
	unsigned int num = 0;
	int channel = 0;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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
							setChannelToken(item,0x1A94,channel); // default to sprite token
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
	if (token_is_fresh)	// token_is_fresh, if do not start with variable
	{
		stackCmdParm( _amalSetAmReg, tokenBuffer );
	}
	else
	{
		stackCmdParm( _amalGetAmReg, tokenBuffer );
	}

	return tokenBuffer;
}

void setChannelToken(struct kittyChannel *item,int token, int number)
{
	item -> objectAPI = NULL;
	item -> token = 0;
	item -> number = 0;

	switch (token)
	{
		case 0x1A94:	item -> objectAPI = &sprite_api; break;
		case 0x1B9E: 	item -> objectAPI = &bob_api; break;
		case 0x0A18:	item -> objectAPI = &screen_display_api; break;
		case 0x0A36:	item -> objectAPI = &screen_offset_api; break;
		case 0x0A4E:	item -> objectAPI = &screen_size_api; break;
		case 0x0DDC:	item -> objectAPI = &rainbow_api; break;
	}

	if (item -> objectAPI) 
	{
		item -> token = token;
		item -> number = number;
	}
}

char *_amalChannel( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	do_to[parenthesis_count] = do_to_default;
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
					setChannelToken(item,token,number);
				}
				else
				{
					printf("not found... add new\n");

					if (item = channels -> newChannel( channel ))
					{
						setChannelToken(item,token,number);
						setChannelAmal( item,  NULL );	// no script yet.
					}
				}
				engine_unlock();	
			}
			break;
		defaut:	setError(22,data->tokenBuffer);
	}

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
		case 0x0A36:	// Channel x to Screen Offset Y
		case 0x0A4E:	// Channel x to Screen Size Y
		case 0x0DDC:  // Channel x to Rainbow y

					stack ++;
					setStackNum( token );
					stack ++;

					ret = tokenbuffer+2;
					break;
		default:
			printf("bad token for CHANNEL ... TO command %04x\n",token);
			getchar();
			setError(22,tokenbuffer);
	}

	return ret;
}

char *amalChannel(struct nativeCommand *cmd, char *tokenBuffer)
{
//       Channel(A+3) To Bob A+3 

	do_to[parenthesis_count] = do_to_channel;

	stackCmdNormal( _amalChannel, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

void channel_amal( struct kittyChannel *channel )
{
	AmalPrintf("%s:%s:%d - channel -> status: %d, channel -> amalProg,amalProgCounter %08x \n",__FILE__,__FUNCTION__,__LINE__, channel -> status, channel -> amalProg.amalProgCounter);

	if (channel -> amalProg.amalAutotest != NULL)
	{
		amal_run_one_cycle(channel,channel -> amalProg.amalAutotest,false);
	}

	if (channel -> amalStatus == channel_status::wait) return;		// if amal program is set to wait..., only autotest can activate it.

	if (channel -> amalStatus == channel_status::direct) 	// if amal program gets paused, we reset program to direct.
	{
		channel -> amalProg.amalProgCounter = channel -> amalProg.directProgCounter;
		channel -> amalStatus = channel_status::active;
	}

	// check if program is ready to run, and it has program.
	if ( ( channel -> amalStatus == channel_status::active ) && ( channel -> amalProg.amalProgCounter ) )
	{
		AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		// Check that program has not ended.
		if ( *channel -> amalProg.amalProgCounter )	
		{
			amal_run_one_cycle(channel, channel -> amalProg.amalProgCounter, true );
		}
		else 	AmalPrintf("%s:%s:%d - channel -> amalProgCounter %d\n",__FILE__,__FUNCTION__,__LINE__, channel -> amalProg.amalProgCounter);
	}
}


char *_amalAmal( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack -1 );
				struct stringData *script = getStackString( stack );
				struct stringData *nscript;

				engine_lock();				// most be thread safe!!!
				if (item = channels -> getChannel(channel))
				{
					nscript = amos_strdup(script);
					if (nscript)
					{
						remove_lower_case( nscript );
						setChannelAmal( item, nscript  );
						asc_to_amal_tokens( item );
						amal_fix_labels( (void **) item -> amalProg.call_array );
						amal_clean_up_labels( );
					}
				}
				else
				{
					if (item = channels -> newChannel( channel ))
					{
						setChannelToken(item,0x1A94,channel); // default to sprite token

						nscript = amos_strdup(script);
						if (nscript)
						{
							remove_lower_case( nscript );
							setChannelAmal( item, nscript );
							asc_to_amal_tokens( item );
							amal_fix_labels( (void **) item -> amalProg.call_array );
							amal_clean_up_labels( );
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

	getchar();

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

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	if (kittyStack[stack].type == type_none )	// arg 1 not set.
				{
					int index = 0;

					engine_lock();				// most be thread safe!!!
					for (index = 0; index < channels -> _size(); index++)
					{
						(channels -> item(index)) -> amalStatus = channel_status::active;
						Printf("is active\n");
						engine_unlock();
						return NULL;
					}
					engine_unlock();
				}
				else
				{
					struct kittyChannel *item;
					int channel = getStackNum( stack  );

					if (item = channels -> getChannel(channel))
					{
						item -> amalStatus = channel_status::active;
						Printf("is active\n");
						engine_unlock();
						return NULL;
					}
				}
				break;

		defaut:	 
				popStack( stack - data->stack );
				setError(22,data->tokenBuffer);
				return NULL;
	}

	setError(22, data-> tokenBuffer);
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

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	if (kittyStack[stack].type == type_none )	// arg 1 not set.
				{
					int index = 0;
					struct kittyChannel *item;

					engine_lock();				// most be thread safe!!!
					for (index = 0; index < channels -> _size(); index++)
					{
						item = channels -> item(index);
						item -> amalStatus = channel_status::uninitialized;
						if (item -> amal_script) free(item -> amal_script); 
						item -> amal_script = NULL;
						item -> amal_at = NULL;
						freeAmalBuf( &item -> amalProg );
					}
					engine_unlock();
				}
				else
				{
					struct kittyChannel *item;
					int channel = getStackNum( stack );

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						item -> amalStatus = channel_status::uninitialized;
						if (item -> amal_script) free(item -> amal_script); 
						item -> amal_script = NULL;
						item -> amal_at = NULL;
						freeAmalBuf( &item -> amalProg );
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

char *amalAmalOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _amalAmalOff, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

void channel_anim( struct kittyChannel *self )
{
	struct channelAPI *api = self -> objectAPI;

	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	Printf("API: %08x, STAUS: %ld\n",api,self->animStatus);

	if (api == NULL) return;
	if (self->animStatus != channel_status::active) return;

	// we are at end of anim, what to do...
	if (*self->anim_at == 0)
	{
		if (self->anim_loops>0)
		{
			self->anim_loops--;
			if (self->anim_loops==0) self->anim_loops==-1;	// done.
		}

		if (self->anim_loops==-1)	// infinity.
		{
			self->anim_at = &self -> anim_script -> ptr;
		}
	}

	if (self -> anim_sleep >= self -> anim_sleep_to )
	{
		int sign = 1;
		int num = 0;
		int arg = 0;
		int para = 0;
		char *c;

		for (c=self->anim_at;*c;c++)
		{
			if (*c=='L') c = &(self -> anim_script -> ptr);	//  when using normal anim command, "L" loops back to start.
			if (*c=='(') para++;
			if ((*c>='0')&&(*c<='9')) num = (num*10) + (*c-'0');
			if (*c=='-') sign = -1;

			switch (para)
			{
				case 0:	// outside
		
						if (*c==',')
						{
							// 0 is normaly infinity loops, but -1 is better for infinity, 0 now becomes, no more loops.
							self -> anim_loops = num==0 ? -1 :  num ;
						}
						break;

				case 1:	// insinde 

						if ((*c==',')||(*c==')'))
						{
							switch (arg)
							{
								case 0: api -> setImage( self -> number,  num );  break;
								case 1: self -> anim_sleep_to = num; self -> anim_sleep = 0; break;
							}

							arg ++;num = 0;sign = 1;
						}
						break;
			}

			if (*c==')') 
			{
				c++;para --;break;
			}
		}

		self -> anim_at = c;
	}
	else
	{
		self -> anim_sleep ++;
		if (self -> anim_sleep == self -> anim_sleep_to)
		{
//			engine_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
//			engine_printf("script: %s\n", self -> anim_at);
			channel_do_object( self );
		}
	}
}


char *_amalAnim( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack -1 );
				struct stringData *script = getStackString( stack );

				if (item = channels -> getChannel(channel))
				{
					setChannelAnim( item, amos_strdup( script ) );
					success = true;
				}
			}
			break;
		defaut:
			setError(22,data->tokenBuffer);
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );

	return NULL;
}

char *amalAnim(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _amalAnim, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}


// call back after reading args this called..

char *_amalAmalFreeze( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	 if (kittyStack[stack].type == type_none )	// arg 1 not set.
				{
					int index = 0;

					engine_lock();				// most be thread safe!!!
					for (index = 0; index < channels -> _size(); index++)
					{
						(channels -> item(index)) -> amalStatus = channel_status::frozen;
						success = true;
					}
					engine_unlock();
				}
				else
				{
					struct kittyChannel *item;
					int channel = getStackNum( stack );

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						item -> amalStatus = channel_status::frozen;
						success = true;
					}
					engine_unlock();
				}
				break;

		defaut:
				setError(22,data->tokenBuffer);
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack( stack - data->stack );



	return NULL;
}

char *amalAmalFreeze(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _amalAmalFreeze, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

// we are in a engine lock in channel_do_object, do not try to lock again!
void channel_do_object( struct kittyChannel *self )
{
	int objectNumber = self -> number;
	struct channelAPI *api = self -> objectAPI;
	if (api == NULL) return;

	if (self -> number< api -> getMax())
	{
		api -> setX ( objectNumber, api -> getX( objectNumber ) + self -> deltax );
		api -> setY ( objectNumber, api -> getY( objectNumber ) + self -> deltay );
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

		for (c=self->movex_at;*c;c++)
		{
			if (*c=='L') c = &self -> movex_script -> ptr;
			if (*c=='(') para++;
			if ((*c>='0')&&(*c<='9')) num = (num*10) + (*c-'0');
			if (*c=='-') sign = -1;
			if ((*c==',')||(*c==')'))
			{
				switch (arg)
				{
					case 0: self -> move_sleep_to = num; self -> move_sleep = 0; break;
					case 1: self -> deltax = sign * num; break;
					case 2: self -> move_count_to = num; self -> move_count = 0; break;
				}

				arg ++;num = 0;sign = 1;
			}

			if (*c==')') 
			{
				c++;para --;break;
			}
		}

		if (self -> count != self -> count_to) self -> movex_at = c;
	}
	else
	{
		self -> move_sleep ++;
		if (self -> move_sleep >= self -> move_sleep_to)
		{
			self -> move_sleep = 0;
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

		for (c=self->movey_at;*c;c++)
		{
			if (*c=='L') c = &self -> movey_script -> ptr;
			if (*c=='(') para++;
			if ((*c>='0')&&(*c<='9')) num = (num*10) + (*c-'0');
			if (*c=='-') sign = -1;
			if ((*c==',')||(*c==')'))
			{
				switch (arg)
				{
					case 0: self -> move_sleep_to = num; self -> move_sleep = 0; break;
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

		if (self -> count != self -> count_to) self -> movey_at = c;
	}
	else
	{
		self -> move_sleep ++;
		if (self -> move_sleep >= self -> move_sleep_to)
		{
			self ->move_sleep = 0;
			self -> count++;
			channel_do_object( self );
		}
	}
}

char *_amalMoveX( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2: 
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack -1 );
				struct stringData *txt = getStackString( stack );

				if (txt)
				{
					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						setChannelMoveX( item, amos_strdup(txt) );
					}
					else
					{
						if (item = channels -> newChannel( channel ) )
						{
							setChannelMoveX( item, amos_strdup(txt) );
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _amalMoveX, tokenBuffer );
	return tokenBuffer;
}

char *_amalMoveY( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2: 
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack -1 );
				struct stringData *txt = getStackString( stack );

				if (txt)
				{
					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						setChannelMoveY( item, amos_strdup(txt) );
					}
					else
					{
						if (item = channels -> newChannel( channel ) )
						{
							setChannelMoveY( item, amos_strdup(txt) );
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _amalMoveY, tokenBuffer );
	return tokenBuffer;
}

char *_amalMoveOn( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	// this code don't need to be tread safe, I'm not changing the script, or adding new channels,

	switch (args)
	{
		case 0:
			{
				int n;
				for (n=0;n<channels -> _size();n++) channels -> item(n) -> amalStatus = channel_status::active;
			}
			break;
		case 1:
			{
				struct kittyChannel *item;
				int channel = getStackNum( stack  );

				if (item = channels -> getChannel(channel))
				{
					item -> amalStatus = channel_status::active;
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
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _amalMoveOn, tokenBuffer );
	return tokenBuffer;
}

char *amalAmalErr(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackNum(0);	// should return error pos in string.
	return tokenBuffer;
}

char *_amalChanmv( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					struct kittyChannel *item;
					int channel = getStackNum( stack  );
					bool moved = false;

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						if ((item -> amalStatus == channel_status::active) && (item -> count < item -> count_to ))
						{
							engine_unlock();
							setStackNum(~0);
							return NULL;
						}
					}
					engine_unlock();
					setStackNum(0);
					return NULL;
				}
				break;
		defaut:
				popStack( stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *amalChanmv(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _amalChanmv, tokenBuffer );
	return tokenBuffer;
}

char *_amalChanan( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					struct kittyChannel *item;
					int channel = getStackNum( stack  );
					bool moved = false;

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						if (item -> anim_script) 
						{
							engine_unlock();
							setStackNum(~0);
							return NULL;
						}
					}
					engine_unlock();
					setStackNum(0);
					return NULL;
				}
				break;
		defaut:
				popStack( stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *amalChanan(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _amalChanan, tokenBuffer );
	return tokenBuffer;
}

char *_amalAnimOff( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					struct kittyChannel *item;
					int channel = getStackNum( stack  );
					bool moved = false;

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						item -> animStatus = channel_status::done;
						item -> anim_at = NULL;

						if (item -> anim_script)
						{
							free( item -> anim_script );
							item -> anim_script = NULL;
						}
					}
					engine_unlock();
					setStackNum(0);
					return NULL;
				}
				break;
		defaut:
				popStack( stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *amalAnimOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);
	int idx;

	if ((next_token == token_nextCmd) || (next_token == token_newLine ))
	{
		struct kittyChannel *item;
		engine_lock();
		for (idx = 0; idx < channels -> _size(); idx++)
		{
			if (item = channels -> item( idx ))
			{
				item -> animStatus = channel_status::done;
				item -> anim_at = NULL;

				if (item -> anim_script)
				{
					free( item -> anim_script );
					item -> anim_script = NULL;
				}
			}
		}
		engine_unlock();
		return tokenBuffer;
	}

	stackCmdNormal( _amalAnimOff, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_amalAnimOn( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					struct kittyChannel *item;
					int channel = getStackNum( stack  );
					bool moved = false;

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						item -> animStatus = channel_status::active;
					}
					engine_unlock();
					setStackNum(0);
					return NULL;
				}
				break;
		defaut:
				popStack( stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *amalAnimOn(struct nativeCommand *cmd, char *tokenBuffer)		// this dummy don't do anything.
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);
	int idx;

	if ((next_token == token_nextCmd) || (next_token == token_newLine ))
	{
		struct kittyChannel *item;
		engine_lock();
		for (idx = 0; idx < channels -> _size(); idx++)
		{
			if (item = channels -> item( idx ))
				item -> animStatus = channel_status::active;
		}
		engine_unlock();
		return tokenBuffer;
	}

	stackCmdNormal( _amalAnimOn, tokenBuffer );
	return tokenBuffer;
}

char *_amalAnimFreeze( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					struct kittyChannel *item;
					int channel = getStackNum( stack  );

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						item -> animStatus = channel_status::frozen;
					}
					engine_unlock();
					return NULL;
				}
				break;
		defaut:
				popStack( stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *amalAnimFreeze(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);
	int idx;

	if ((next_token == token_nextCmd) || (next_token == token_newLine ))
	{
		struct kittyChannel *item;
		engine_lock();
		for (idx = 0; idx < channels -> _size(); idx++)
		{
			if (item = channels -> item( idx ))
				item -> animStatus = channel_status::frozen;
		}
		engine_unlock();
		return tokenBuffer;
	}

	stackCmdNormal( _amalAnimFreeze, tokenBuffer );
	return tokenBuffer;
}

