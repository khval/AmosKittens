
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>

#include "commandsBanks.h"

#include "AmalCompiler.h"
#include "channel.h"


amalBankPlay::amalBankPlay( char *start)
{
	offset_tab = (unsigned short *) (start + 6) ;
	size_tab = offset_tab + 48;	// next 48
	name_tab = (char *) ( size_tab +48);
	move_data = ((char *)  name_tab) + (48*8);
}


namespace amalBank
{

BOOL deNext( struct amalPlayContext *data )
{
	if (data -> repeat)
	{
		printf("repeat\n");
		data -> repeat --;
		return FALSE;
	}
	{
		signed char value = *data -> data; 

		if (value & 0x80)
		{
			data -> repeat=(value & 127);
			data -> value = 0;
			data -> size+= data -> repeat;

			printf("got repeat\n");
		}
		else 
		{
			printf("got data\n");
			data -> value = value & 0x40 ? 0x80 | value : value & 0x3F;
			data -> size++;
		}

		data -> data++;	// get next
	}
	return TRUE;
}

#define amal_play_adr(start,item ) (start+( amalPlayBank -> offset_tab[ item ]*2+4))


void initPlayContext(struct amalPlayContext *pc, signed char *data)
{
	pc -> repeat =0;
	pc -> data = data;
}

void getMove( struct amalBankPlay *amalPlayBank, int id, char *start )
{
	int sp;
	char d;
	char *ap;

	ap = amal_play_adr( start,  id );

	sp = *((unsigned short *) ap);
	ap+=2;

	amalPlayBank -> lx = *((unsigned short *) ap)-2-4;

	ap+=2;
	d = *((char *) ap);

	initPlayContext(&( amalPlayBank -> cdx), (signed char *) ap );
	initPlayContext(&( amalPlayBank -> cdy), (signed char *) ap +amalPlayBank -> lx + 2);
}


int play( struct kittyChannel *self, int id )
{
	struct amalBankPlay *amalPlayBank;

	amalPlayBank = self -> amalPlayBank;

	if (amalPlayBank == NULL)
	{
		struct kittyBank *bank = findBank( 4 );

		if (bank)
		{
			self -> amalPlayBank = new amalBankPlay( bank -> start );
			getMove( amalPlayBank, id , bank -> start );
		}
	}

	if (amalPlayBank == NULL) return channel_status::error;

	if (( amalPlayBank -> lx > 0 || amalPlayBank -> cdx.repeat > 0))
	{
		if (deNext( &amalPlayBank -> cdx )) amalPlayBank -> lx--;
		deNext( &amalPlayBank -> cdy );

		self -> objectAPI -> setX( self -> number, self -> objectAPI -> getX( self -> number ) + amalPlayBank -> cdx.value );
		self -> objectAPI -> setY( self -> number, self -> objectAPI -> getY( self -> number ) + amalPlayBank -> cdy.value );
		return channel_status::active;
	}
	else	// done
	{
		delete self -> amalPlayBank;
		self -> amalPlayBank = NULL;
		return channel_status::done;
	}

	return channel_status::error;
}

}

