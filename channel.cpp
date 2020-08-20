#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(__amigaos4__) || defined(__amigaos__)
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#include "debug.h"
#include <string>

#include "amosKittens.h"
#include "amalCompiler.h"
#include "channel.h"

extern void *set_reg (struct kittyChannel *self, struct amalCallBack *cb);


kittyChannel::~kittyChannel()
{
	dprintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (anim_script) freeString(anim_script);
	anim_script = NULL;

	if (amal_script) freeString(amal_script);
	amal_script = NULL;

	if (movex_script) freeString(movex_script);
	movex_script = NULL;

	if (movey_script) freeString(movey_script);
	movey_script = NULL;

	if ( amalProg.call_array ) sys_free( amalProg.call_array );
	amalProg.call_array = NULL;

	if (amalPlayBank) delete amalPlayBank;
	amalPlayBank = NULL;

}

kittyChannel::kittyChannel( int channel )
{
		id = channel;
		token = 0;
		number = 0;
		amal_script = NULL;
		amal_at = NULL;
		anim_script = NULL;
		anim_at = NULL;
		movex_script = NULL;
		movex_at = NULL;
		movey_script = NULL;
		movey_at = NULL;

		progStack = NULL;
		progStackCount = 0;
		argStack = NULL;
		argStackCount = 0;

		amalProg.prog_crc = 0;
		amalProg.used = 0;
		amalProg.elements = 0;
		amalProg.size = 0;
		amalProg.amalProgCounter = NULL;
		amalProg.amalAutotest = NULL;
		amalProg.directProgCounter = NULL;
		amalProg.call_array = NULL;
		amalPlayBank = NULL;

		parenthses = 0;
		objectAPI = NULL;
		pushBackFunction = NULL;

		count = 0;
		count_to = 0;
		deltax = 0;
		deltay = 0;

		anim_sleep = 0;
		anim_sleep_to = 0;

		move_sleep = 0;
		move_sleep_to = 0;
		move_count = 0; 
		move_count_to = 0; 

		animStatus = channel_status::uninitialized;
		amalStatus = channel_status::uninitialized;
}

ChannelTableClass::~ChannelTableClass()
{
	int n;

	dprintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (tab)
	{
		for (n=0;n<used;n++)
		{
			if (tab[n]) delete tab[n];
			tab[n] = NULL;
		}
		freeStruct(tab);
	}
	tab = NULL;
	used = 0;
	allocated = 0;
}

struct kittyChannel * ChannelTableClass::newChannel(  int channel )
{
	struct kittyChannel *item = new kittyChannel( channel );

	if (used < allocated )
	{
		tab[used] = item;
		used ++;
		return item;
	}
	else
	{
		struct kittyChannel **tmp;
		int old_allocated = allocated;
		allocated += 20;
		tmp = allocStruct(kittyChannel *,allocated );

		if (tmp)
		{
			memcpy(tmp,tab,sizeof(struct kittyChannel *) * old_allocated );
			freeStruct(tab);

			tab = tmp;
			tab[used] = item;
			used ++;
			return item;
		}
		else
		{
			// resvert 
			if (item) delete item ;
			allocated = old_allocated;
		}
	}

	return NULL;
}

struct kittyChannel *ChannelTableClass::getChannel(int id)
{
	for (unsigned int n=0;n<used;n++) if (tab[n] -> id == id) return tab[n];
	return NULL;
}

struct kittyChannel *ChannelTableClass::findChannelByItem(int token, int number)
{
	for (unsigned int n  = 0 ; n < used;n++ )
	{
		if ( (tab[n] -> token == token) && (tab[n] -> number == number) )
		{	
			return tab[n];
		}
	}

	return NULL;
}


struct kittyChannel *ChannelTableClass::item(int index)
{
	return tab[index];
}

unsigned int ChannelTableClass::_size()
{
	return used;
}

void setChannelAnim(  kittyChannel *item, struct stringData *str , bool enable )
{
	if (item -> anim_script) freeString(item -> anim_script);

	item -> anim_script = str;
	item -> anim_at = &str->ptr;
	item -> anim_loops = 0;

	if (enable)
	{
		item -> animStatus = channel_status::active;
	}
}

void setChannelAmal(  kittyChannel *item, struct stringData *str)
{
	item -> amalStatus = channel_status::uninitialized;
	if (item -> amal_script) freeString(item -> amal_script);
	item -> amal_script = str;
	item -> amal_at = &str->ptr;

	// reset for AMAL move command.
	item -> move_sleep = 0;
	item -> move_sleep_to = 0;
	item -> move_count = 0; 
	item -> move_count_to = 0; 
}

void setChannelMoveX( kittyChannel *item, struct stringData *str)
{
	if (item -> movex_script) freeString(item -> movex_script);
	item -> movex_script = str;
	item -> movex_at = &str->ptr;
	item -> deltax = 0;
}

void setChannelMoveY(  kittyChannel *item, struct stringData *str)
{
	if (item -> movey_script) freeString(item -> movey_script);
	item -> movey_script = str;
	item -> movey_at = &str->ptr;
	item -> deltay = 0;
}


