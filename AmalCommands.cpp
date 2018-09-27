
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "AmalCompiler.h"
#include "channel.h"
#include "AmalCommands.h"
#include "amal_object.h"

extern void pushBackAmalCmd( struct kittyChannel *channel, void *cmd ) ;
extern int amreg[26];
extern void dumpAmalRegs();

#ifdef test_app
	#define amal_mouse_x 1000
	#define amal_mouse_y 2000
#else
	extern int engine_mouse_x;
	extern int engine_mouse_y;

	#define amal_mouse_x engine_mouse_x
	#define amal_mouse_y engine_mouse_y
#endif

void *amal_call_pause API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> status = channel_status::paused;
	self -> loopCount = 0;
	return NULL;
}

void *amal_set_num API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount ] = (int) code[1];

	return code+1;
}

void *amal_call_x API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> last_reg = 5;
	self -> argStack [ self -> argStackCount ] = self -> objectAPI -> getX( self -> number );
	return NULL;
}

void *amal_call_y API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> last_reg = 6;
	self -> argStack [ self -> argStackCount ] = self -> objectAPI -> getY( self -> number );
	return NULL;
}

void *amal_call_reg API_AMAL_CALL_ARGS
{
	unsigned char c;
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	c = (unsigned char) (int) code[1];
	self -> last_reg = c;

	if ((c>='0')&&(c<='9'))
	{
		self -> argStack [ self -> argStackCount ] = self -> reg[c-'0'];
	} else if ((c>='A')&&(c<='Z'))
	{
		self -> argStack [ self -> argStackCount ] = amreg[ c - 'A'];
	}

	printf("R%c is %d\n",c, self -> argStack [ self -> argStackCount ]);

	return code+1;
}

void *amal_call_on API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_direct API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_wait API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_if API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_jump API_AMAL_CALL_ARGS
{
	void **ret;
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	ret = (void **) code[1];
	if (ret)
	{
		if (self -> loopCount>9)
		{
			self -> status = channel_status::paused;
			self -> loopCount = 0;
			return ret-1;
		}
		else
		{
			self -> loopCount++;
			return ret-1;
		}
	}

	return code+1;
}

void *amal_call_exit API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_let API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_autotest API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_move API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

static void *add (struct kittyChannel *self, struct amalCallBack *cb)
{
	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] + self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}
	return NULL;
}

void *amal_call_add API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStackCount  ++;
	pushBackAmalCmd( code, self, add ); 
	return NULL;
}

static void *sub (struct kittyChannel *self, struct amalCallBack *cb)
{
	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] - self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}
	return NULL;
}

void *amal_call_sub API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStackCount  ++;
	pushBackAmalCmd( code, self, sub ); 
	return NULL;
}

static void *mul (struct kittyChannel *self, struct amalCallBack *cb)
{
	dumpAmalStack( self );
	getchar();

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] * self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}
	return NULL;
}

void *amal_call_mul API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStackCount  ++;
	pushBackAmalCmd( code, self, mul ); 
	return NULL;
}

static void *div (struct kittyChannel *self, struct amalCallBack *cb)
{
	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] / self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}
	return NULL;
}

void *amal_call_div API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( code, self, div ); 
	return NULL;
}

void *amal_call_and API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *not_equal (struct kittyChannel *self, struct amalCallBack *cb)
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	int args = self -> argStackCount - cb -> argStackCount + 1 ;

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] > self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}

	return NULL;
}

void *amal_call_not_equal API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( code, self, not_equal ); 
	return NULL;
}

void *less (struct kittyChannel *self, struct amalCallBack *cb)
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	int args = self -> argStackCount - cb -> argStackCount + 1 ;

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] > self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}

	return NULL;
}

void *amal_call_less API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( code, self, less ); 
	return NULL;
}

void *more (struct kittyChannel *self, struct amalCallBack *cb)
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	int args = self -> argStackCount - cb -> argStackCount + 1 ;

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] > self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}

	return NULL;
}

void *amal_call_more API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( code, self, more ); 
	return NULL;
}

void *less_or_equal  (struct kittyChannel *self, struct amalCallBack *cb)
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	int args = self -> argStackCount - cb -> argStackCount + 1 ;

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] <= self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}

	return NULL;
}

void *amal_call_less_or_equal API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( code, self, less_or_equal ); 
	return NULL;
}

void *more_or_equal  (struct kittyChannel *self, struct amalCallBack *cb)
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	int args = self -> argStackCount - cb -> argStackCount + 1 ;

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] >= self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}

	return NULL;
}

void *amal_call_more_or_equal API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStackCount  ++;
	self -> argStack [ self -> argStackCount ] = 0;
	pushBackAmalCmd( code, self, more_or_equal ); 
	return NULL;
}

void *amal_call_equal API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount ] = 0;	
	return NULL;
}

void *amal_call_for API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_to API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_next API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_play API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_end API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_xm API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount  ] = amal_mouse_x;
	getchar();	
	return NULL;
}

void *amal_call_ym API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount  ] = amal_mouse_y;	
	return NULL;
}

void *amal_call_k1 API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_k2 API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_j0 API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_j1 API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_z API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}


void *amal_call_xh API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_yh API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_sx API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_sy API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_bobCol API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_spriteCol API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_Col API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_vumeter API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_next_cmd API_AMAL_CALL_ARGS
{
	void *ret;
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	while (self -> progStackCount)
	{
		struct amalCallBack *cb;
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

		self -> progStackCount --;

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

		cb = &self -> progStack[ self -> progStackCount ];

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

		ret =cb -> cmd( self, cb );

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

		if (ret) return ret;
	}

	printf("----\n");

	return NULL;
}

void *amal_call_parenthses_start API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_parenthses_end API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_end_label API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_nextArg API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	return NULL;
}

void *amal_call_anim API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
//	pushBackAmalCmd( code, self, NULL ) ;
	return NULL;
}

void *while_status  (struct kittyChannel *self, struct amalCallBack *cb)
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	printf("args: %d\n", self -> argStackCount - cb -> argStackCount + 1 );
	dumpAmalStack( self );

	if ( ! self -> argStack [ self -> argStackCount ] )
	{
		char *start_location = (char *) self -> amalProg.call_array;
		int offset_location ;
		void **code = cb -> code;

		offset_location = (int) code[1];

		printf("code at %08x\n",code);
		printf("%08x\n",code[0]);
		printf("%08x\n",code[1]);

		return start_location + offset_location + 4;
	}

	return NULL;
}

void *amal_call_while API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount ] = 0;	// set default value. 
	pushBackAmalCmd( code, self, while_status ); 
	printf("ptr %08x\n",code[1]);
	return code +1;
}

void *amal_call_wend API_AMAL_CALL_ARGS
{
	char *location;
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	printf("ptr %08x ------ \n", ((void **) code[1]) );
	
	location = (char *) self -> amalProg.call_array;
	location += (int) code[1];

	self -> status = channel_status::paused;

	return location - sizeof(void *) ;
}

void *set_reg (struct kittyChannel *self, struct amalCallBack *cb)
{
	unsigned int c;

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	printf("args: %d\n", self -> argStackCount - cb -> argStackCount + 1 );

	printf("c = %d\n",  cb-> last_reg);

	c = cb-> last_reg;

	if (c==5)
	{
		self -> objectAPI -> setX( self -> number, self -> argStack [ self -> argStackCount ] );
	}
	else if (c==6)
	{
		self -> objectAPI -> setY( self -> number, self -> argStack [ self -> argStackCount ] );
	}
	if ((c>='0')&&(c<='9'))
	{
		self -> reg[ c - '0' ] = self -> argStack [ self -> argStackCount ];
		printf("R%c=%d\n",c, self -> reg[ c - '0' ]);
	}
	else if ((c>='A')&&(c<='Z'))
	{
		amreg[ c - 'A' ] = self -> argStack [ self -> argStackCount ];
		printf("R%c=%d\n",c, amreg[ c - 'A' ]);
	}

	return NULL;
}

void *amal_call_set API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	self -> argStack [ self -> argStackCount ] = 0;	// set default value. 
	pushBackAmalCmd( code, self, set_reg ); 
	return NULL;
}

void *callback_inc  (struct kittyChannel *self, struct amalCallBack *cb)
{
	unsigned char c = self -> last_reg;
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	if ((c>='0')&&(c<='9'))
	{
		self -> reg[ c - '0' ] ++;
		printf("R%c=%d\n",c, self -> reg[ c - '0' ]);
	}
	else 	if ((c>='A')&&(c<='Z'))
	{
		amreg[ c - 'A' ] ++;
		printf("R%c=%d\n",c, amreg[ c - 'A' ]);
	}

	return NULL;
}	

void *amal_call_inc API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	pushBackAmalCmd( code, self, callback_inc ); 
	return NULL;
}


