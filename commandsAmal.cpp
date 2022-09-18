
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <vector>

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

#include <amosKittens.h>
#include <stack.h>

#include "debug.h"
#include "commands.h"
#include "commandsAmal.h"
#include "var_helper.h"
#include "kittyErrors.h"
#include "engine.h"
#include "AmalCompiler.h"
#include "channel.h"
#include "amal_object.h"
#include "amosstring.h"
#include "bank_helper.h"
#include "AmalBank.h"

extern char *(*_do_set) ( struct glueCommands *data, int nextToken );
extern char *_setVar( struct glueCommands *data, int nextToken );
int amreg[26];

extern int amal_error_pos;

extern ChannelTableClass *channels;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroSpriteObject sprites[64];
extern std::vector<struct retroSpriteObject *> bobs;

extern void remove_lower_case( struct stringData *txt);

void setChannelToken(struct kittyChannel *item,int token, int number);
void channel_do_object( struct kittyChannel *self );

extern char *(*_do_set) ( struct glueCommands *data, int nextToken ) ;

unsigned int _set_amreg_num = 0;
int _set_amreg_channel = 0;

char *_set_amreg_fn( struct glueCommands *data, int nextToken )
{
	// don't need to check num, was checked before.
	amreg[ _set_amreg_num] = getStackNum(__stack );
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
		 item -> reg[_set_amreg_num] = getStackNum(__stack );
	}
	else
	{
		if (item = channels -> newChannel( _set_amreg_channel ) )
		{
			setChannelToken(item,0x1A94,_set_amreg_channel); // default to sprite token
			item -> reg[_set_amreg_num] = getStackNum(__stack );
		}
	}
	engine_unlock();

	_do_set = _setVar;
	return NULL;
}

char *_amalSetAmReg( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			_set_amreg_num = getStackNum(__stack );

			if (_set_amreg_num<26)		// unsigned don't need to check, more then.
			{
				_do_set = _set_amreg_fn;
			}
			else setError(11, data->tokenBuffer);
			break;

		case 2:
			{
				// Amos Pro manual: Amreg(channel,reg)
				// Amos The Creator manual:  Amreg(reg,channel)

				_set_amreg_channel = getStackNum(__stack-1 );
				_set_amreg_num = getStackNum(__stack );
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

	popStack(__stack - data->stack );
	setStackNum( ret );

	return NULL;
}


char *_amalGetAmReg( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	unsigned int num = 0;
	int channel = 0;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			num = getStackNum(__stack );

			if (num<26)		// unsigned don't need to check, more then.
			{
				ret = amreg[num];
			}
			else setError(11, data->tokenBuffer);

			break;

		case 2:
			{
				struct kittyChannel *item;

				channel = getStackNum(__stack-1 );
				num = getStackNum(__stack );

				// Amos Pro manual: Amreg(channel,reg)
				// Amos The Creator manual:  Amreg(reg,channel)

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

	popStack(__stack - data->stack );
	setStackNum( ret );

	return NULL;
}

char *amalAmReg(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (instance.token_is_fresh)	// token_is_fresh, if do not start with variable
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
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	do_to[instance.parenthesis_count] = do_to_default;
	switch (args)
	{
		case 3:
			{
				struct kittyChannel *item;
				int channel = getStackNum(__stack - 2 );
				int token = getStackNum(__stack - 1 );
				int number = getStackNum(__stack );

				engine_lock();	
				if (item = channels -> getChannel(channel))
				{
					setChannelToken(item,token,number);
				}
				else
				{
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

	popStack(__stack - data->stack );
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

					__stack ++;
					setStackNum( token );
					__stack ++;

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

	do_to[instance.parenthesis_count] = do_to_channel;

	stackCmdNormal( _amalChannel, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

void channel_amal( struct kittyChannel *channel )
{
	AmalPrintf("%s:%s:%d - channel -> status: %d, channel -> amalProg,amalProgCounter %08x \n",__FILE__,__FUNCTION__,__LINE__, channel -> amalStatus, channel -> amalProg.amalProgCounter);

	channel -> amalStatus &=  ~channel_status::paused;

	if (channel -> amalProg.amalAutotest != NULL)
	{
		channel -> autotest_loopCount = 0;		// unstuck counter.
		amal_run_one_cycle(channel,channel -> amalProg.amalAutotest,false);
	}

	if (channel -> amalStatus & channel_status::wait) return;		// if amal program is set to wait..., only autotest can activate it.

	// check if program is ready to run, and it has program.
	if ( ( channel -> amalStatus & channel_status::active ) && ( channel -> amalProg.amalProgCounter ) )
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



void SetChannelScript( int channel, struct stringData *script, int *_err )
{
	struct kittyChannel *item;
	struct stringData *nscript;	// create new script, so its not freed from stack, or bank by accident.

	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	engine_lock();				// most be thread safe!!!
	if (item = channels -> getChannel(channel))
	{
		nscript = amos_strdup(script);
		if (nscript)
		{
			remove_lower_case( nscript );
			setChannelAmal( item, nscript  );
			*_err = asc_to_amal_tokens( item );
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
				*_err = asc_to_amal_tokens( item );
				amal_fix_labels( (void **) item -> amalProg.call_array );
				amal_clean_up_labels( );
			}
		}
	}

	engine_unlock();
}

char *_amalAmal( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int _err = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			{
				int channel = getStackNum(__stack -1 );

				switch ( kittyStack[__stack].type )
				{
					case type_string:

						SetChannelScript( channel, kittyStack[__stack].str , &_err );
						break;

					case type_int:

						{
							kittyBank *bank = findBankById( 4 );
						
							if (bank)
							{
								int i = kittyStack[ __stack].integer.value;
								amalBankScript obj( bank -> start );
								char *script_ptr;
								struct stringData *script;

								if ((i < 0) || (i >= obj.progs) )
								{
									popStack(__stack - data->stack );
									setError(33,data->tokenBuffer);
									return NULL;
								}

								// get Amos script string.
								script_ptr = (((char *) obj.offsets) + (obj.offsets[ i ] * 2))  ;

								// convert to kitty string	(has integer size, not short size)
								script = toAmosString( script_ptr +2 , *((unsigned short *) script_ptr ) );

								if (script)
								{
									for (char *c = &script -> ptr ; *c ; c++  )
									{
										if (*c=='~') *c = ' ';	// banks use ~ as newline..
									}

									printf("%s\n",&script -> ptr);

									SetChannelScript( channel, script , &_err );
									// SetChannelScript will copy the string, so we must free the script now.
									sys_free( script );
								}						
							}
						}
						break;

					default:
						popStack(__stack - data->stack );
						setError(40,data->tokenBuffer);
						return NULL;
				}
			}
			break;
		defaut:
			setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );

	if (_err) setError(_err,data->tokenBuffer);

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
	int args =__stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	if (kittyStack[__stack].type == type_none )	// arg 1 not set.
				{
					struct kittyChannel *item;
					unsigned int i = 0;

					engine_lock();				// most be thread safe!!!
					for (i = 0; i < channels -> _size(); i++)
					{
						if (item = channels -> item(i))
						{
							item  -> amalStatus |= channel_status::active;
						}
					}
					engine_unlock();
					return NULL;
				}
				else
				{
					struct kittyChannel *item;
					int channel = getStackNum(__stack  );

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						item -> amalStatus |= channel_status::active;
					}
					engine_unlock();
					return NULL;
				}
				break;

		defaut:	 
				popStack(__stack - data->stack );
				setError(22,data->tokenBuffer);
				return NULL;
	}

	setError(22, data-> tokenBuffer);
	return NULL;
}

char *amalAmalOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _amalAmalOn, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_amalAmalOff( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	if (kittyStack[__stack].type == type_none )	// arg 1 not set.
				{
					unsigned int index = 0;
					struct kittyChannel *item;

					engine_lock();				// most be thread safe!!!
					for (index = 0; index < channels -> _size(); index++)
					{
						printf("Free item[%d]\n",index);

						item = channels -> item(index);
						if (item)
						{
							printf("Amal channel id: %d\n", item -> id);

							item -> amalStatus = channel_status::uninitialized;
							if (item -> amal_script) freeString(item -> amal_script); 
							item -> amal_script = NULL;
							item -> amal_at = NULL;
							freeAmalBuf( &item -> amalProg );
						}
						else
						{
							printf("item not found\n");
						}
					}
					engine_unlock();
				}
				else
				{
					struct kittyChannel *item;
					int channel = getStackNum(__stack );

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						item -> amalStatus = channel_status::uninitialized;
						if (item -> amal_script) freeString(item -> amal_script); 
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

	popStack(__stack - data->stack );
	return NULL;
}

char *amalAmalOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _amalAmalOff, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

void channel_anim( struct kittyChannel *self )
{
	struct channelAPI *api = self -> objectAPI;

	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (api == NULL) return;
	if (self->animStatus != channel_status::active) return;

	// we are at end of anim, what to do...
	if (*self->anim_at == 0)
	{
		if (self->anim_loops>0)
		{
			self->anim_loops--;
			if (self->anim_loops==0)
			{
				self->anim_loops==-1;	// done.
				self->animStatus = channel_status::done;
			}
			else 
			{
				self->anim_at = &self -> anim_script -> ptr;
				while (( *(self->anim_at) != 0 ) && ( *(self->anim_at) != '(' )) self->anim_at++;
			}
		}

		if (self->anim_loops==-1)	// infinity.
		{
			self->anim_at = &self -> anim_script -> ptr;
			while (( *(self->anim_at) != 0 ) && ( *(self->anim_at) != '(' )) self->anim_at++;
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
			if (*c=='(') 
			{
				self -> anim_sleep = 0;
				para++;
			}
			if ((*c>='0')&&(*c<='9')) num = (num*10) + (*c-'0');
			if (*c=='-') sign = -1;

			switch (para)
			{
				case 0:	// outside
		
						if (*c==',')
						{
							// 0 is normaly infinity loops, but -1 is better for infinity, 0 now becomes, no more loops.
							self -> anim_loops = num==0 ? -1 :  num ;
							num =0;
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
	int args =__stack - data->stack +1 ;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2:
			{
				struct kittyChannel *item;
				int channel = getStackNum(__stack -1 );
				struct stringData *script = getStackString(__stack );

				if (item = channels -> getChannel(channel))
				{
					setChannelAnim( item, amos_strdup( script ) , false );
					success = true;
				}
			}
			break;
		defaut:
			setError(22,data->tokenBuffer);
	}

	if (success == false) setError(22,data->tokenBuffer);

	popStack(__stack - data->stack );

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
	int args = instance.stack - data -> stack + 1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	 if (kittyStack[__stack].type == type_none )	// arg 1 not set.
				{
					unsigned int index = 0;

					engine_lock();				// most be thread safe!!!
					for (index = 0; index < channels -> _size(); index++)
					{
						(channels -> item(index)) -> amalStatus |= channel_status::frozen;
					}
					engine_unlock();
				}
				else
				{
					struct kittyChannel *item;
					int channel = getStackNum(__stack );

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						item -> amalStatus |= channel_status::frozen;
					}
					engine_unlock();
				}
				break;

		defaut:
				setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );

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
	if (self -> moveStatus != channel_status::active) return;

	if (self -> count >= self -> move_count_to )
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
				self -> count = 0;
				c++;para --;
				break;
			}
		}

		if (self -> count != self -> move_count_to) self -> movex_at = c;
	}
	else
	{
		self -> move_sleep ++;
		if (self -> move_sleep >= self -> move_sleep_to)
		{
			self -> move_sleep = 0;
			self -> count++;
			channel_do_object( self );

			if (self -> count >= self -> move_count_to )
			{
				if (*self -> movex_at == 0) self -> moveStatus = channel_status::done;
			}
		}
	}
}


void channel_movey( struct kittyChannel *self )
{
	if (self -> moveStatus != channel_status::active) return;

	if (self -> count >= self -> move_count_to )
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
			if ((*c>='0')&&(*c<='9'))
			{
			 	num = (num*10) + (*c-'0');
			}

			if (*c=='-') sign = -1;
			if ((*c==',')||(*c==')'))
			{
				switch (arg)
				{
					case 0: self -> move_sleep_to = num; self -> move_sleep = 0; break;
					case 1: self -> deltay = sign * num; break;
					case 2: self -> move_count_to = num; self -> count = 0; break;
				}

				arg ++;num = 0;sign = 1;
			}

			if (*c==')') 
			{
				self -> count = 0;
				c++;para --;
				break;
			}
		}

		if (self -> count != self -> move_count_to) self -> movey_at = c;
	}
	else
	{
		self -> move_sleep ++;
		if (self -> move_sleep >= self -> move_sleep_to)
		{
			self ->move_sleep = 0;
			self -> count++;
			channel_do_object( self );

			if (self -> count >= self -> move_count_to )
			{
				if (*self -> movex_at == 0) self -> moveStatus = channel_status::done;
			}
		}
	}
}

char *_amalMoveX( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2: 
			{
				struct kittyChannel *item;
				int channel = getStackNum(__stack -1 );
				struct stringData *txt = getStackString(__stack );

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
							setChannelToken(item,0x1A94,channel); // default to sprite token
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

	popStack(__stack - data->stack );
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
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 2: 
			{
				struct kittyChannel *item;
				int channel = getStackNum(__stack -1 );
				struct stringData *txt = getStackString(__stack );

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
							setChannelToken(item,0x1A94,channel); // default to sprite token
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

	popStack(__stack - data->stack );
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
	int args =__stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	// this code don't need to be tread safe, I'm not changing the script, or adding new channels,

	switch (args)
	{
		case 1:

			if (kittyStack[__stack].type == type_none)
			{
				unsigned int n;
				for (n=0;n<channels -> _size();n++)
				{
					channels -> item(n) -> moveStatus = channel_status::active;
				}
			}
			else
			{
				struct kittyChannel *item;
				int channel = getStackNum(__stack  );

				if (item = channels -> getChannel(channel))
				{
					item -> moveStatus = channel_status::active;
				}
			}
			return NULL;

		default:
			setError(22,data->tokenBuffer);;
			break;
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *amalMoveOn(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _amalMoveOn, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_amalMoveOff( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	if (kittyStack[__stack].type == type_none )	// arg 1 not set.
				{
					unsigned int index = 0;
					struct kittyChannel *item;

					engine_lock();				// most be thread safe!!!
					for (index = 0; index < channels -> _size(); index++)
					{
						item = channels -> item(index);
						item -> moveStatus = channel_status::uninitialized;

						if (item -> movex_script) free(item -> movex_script); 
						item -> movex_script = NULL;
						item -> movex_at = NULL;

						if (item -> movey_script) free(item -> movey_script); 
						item -> movey_script = NULL;
						item -> movey_at = NULL;
					}
					engine_unlock();
				}
				else
				{
					struct kittyChannel *item;
					int channel = getStackNum(__stack );

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						item -> moveStatus = channel_status::uninitialized;

						if (item -> movex_script) free(item -> movex_script); 
						item -> movex_script = NULL;
						item -> movex_at = NULL;

						if (item -> movey_script) free(item -> movey_script); 
						item -> movey_script = NULL;
						item -> movey_at = NULL;
					}
					engine_unlock();
				}
				break;

		default:
			setError(22,data->tokenBuffer);;
			break;
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *amalMoveOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _amalMoveOff, tokenBuffer );
	return tokenBuffer;
}

char *amalAmalErr(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackNum(amal_error_pos);	
	return tokenBuffer;
}

//---

char *_amalMovon( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					struct kittyChannel *item;
					int channel = getStackNum(__stack  );

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						if (item -> moveStatus == channel_status::active)
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
				popStack(__stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *amalMovon(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _amalMovon, tokenBuffer );
	return tokenBuffer;
}

//---

char *_amalChanmv( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					struct kittyChannel *item;
					int channel = getStackNum(__stack  );

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						if ((item -> amalStatus & channel_status::active) || (item -> moveStatus & channel_status::active))
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
				popStack(__stack - data->stack );
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
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					struct kittyChannel *item;
					int channel = getStackNum(__stack  );

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						if (item -> animStatus == channel_status::active ) 
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
				popStack(__stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *amalChanan(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _amalChanan, tokenBuffer );
	return tokenBuffer;
}

char *_amalAmplay( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{

		case 2:	{
					unsigned int n;
					struct kittyChannel *item;

					engine_lock();				// most be thread safe!!!
					for ( n=0;n < channels -> _size();n++)
					{
						if (item = channels -> item(n))		// find item at index.
						{
							if (stack_is_number(__stack-1)) item -> reg[0]=getStackNum(__stack-1);
							if (stack_is_number(__stack)) item -> reg[1]=getStackNum(__stack);
						}
					}
					engine_unlock();

					popStack(__stack - data->stack );
					return NULL;
				}
				break;

		case 4:	{
					struct kittyChannel *item;
					int _start = getStackNum(__stack-1 );	// start channel id
					int _end = getStackNum(__stack );	// end channel id

					engine_lock();				// most be thread safe!!!
					for (int n=_start;n <= _end;n++)
					{
						if (item = channels -> getChannel(n))
						{
							if (stack_is_number(__stack-3)) item -> reg[0]=getStackNum(__stack-3);
							if (stack_is_number(__stack-2)) item -> reg[1]=getStackNum(__stack-2);
						}
					}
					engine_unlock();

					popStack(__stack - data->stack );
					return NULL;
				}
				break;
		defaut:
				popStack(__stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *amalAmplay(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _amalAmplay, tokenBuffer );
	return tokenBuffer;
}

char *_amalAnimOff( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					struct kittyChannel *item;
					int channel = getStackNum(__stack  );

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
				popStack(__stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *amalAnimOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);
	unsigned int idx;

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
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					struct kittyChannel *item;
					int channel = getStackNum(__stack  );

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
				popStack(__stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *amalAnimOn(struct nativeCommand *cmd, char *tokenBuffer)		// this dummy don't do anything.
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);
	unsigned int idx;

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
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					struct kittyChannel *item;
					int channel = getStackNum(__stack  );

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
				popStack(__stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *amalAnimFreeze(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);
	unsigned int idx;

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

char *_amalMoveFreeze( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	{
					struct kittyChannel *item;
					int channel = getStackNum(__stack  );

					engine_lock();				// most be thread safe!!!
					if (item = channels -> getChannel(channel))
					{
						item -> moveStatus = channel_status::frozen;
					}
					engine_unlock();
					return NULL;
				}
				break;
		defaut:
				popStack(__stack - data->stack );
				setError(22,data->tokenBuffer);
	}
	return NULL;
}

char *amalMoveFreeze(struct nativeCommand *cmd, char *tokenBuffer)
{
	unsigned short next_token = *((unsigned short *) tokenBuffer);
	unsigned int idx;

	if ((next_token == token_nextCmd) || (next_token == token_newLine ))
	{
		struct kittyChannel *item;
		engine_lock();
		for (idx = 0; idx < channels -> _size(); idx++)
		{
			if (item = channels -> item( idx ))
				item -> moveStatus = channel_status::frozen;
		}
		engine_unlock();
		return tokenBuffer;
	}

	stackCmdNormal( _amalMoveFreeze, tokenBuffer );
	return tokenBuffer;
}



