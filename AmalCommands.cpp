
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>

#if defined(__amigaos4__) || defined(__amigaos__)
#include <proto/exec.h>
#include <proto/dos.h>
#endif

#ifdef __linux__
#include "os/linux/stuff.h"
#define Printf printf
#endif

#include "AmalCompiler.h"
#include "channel.h"
#include "AmalCommands.h"
#include "amal_object.h"
#include "amoskittens.h"
#include "commandsScreens.h"
#include "amosstring.h"

#ifdef test_app
#include "debug_amal_test_app.h"
#else
#include "debug.h"
#endif 

extern void pushBackAmalCmd( amal::Flags flags, struct kittyChannel *channel, void *cmd ) ;
extern int amreg[26];
extern void dumpAmalRegs();
extern struct retroScreen *screens[8] ;

extern void dumpAmalProgStack( struct kittyChannel *channel );

void *amalFlushParaCmds( struct kittyChannel *self );
void *amalFlushAllCmds( struct kittyChannel *self );
void *amalFlushAllParenthsesCmds( struct kittyChannel *self );

#ifdef test_app
	#define amal_mouse_x 1000
	#define amal_mouse_y 2000
	#define engine_mouse_key 3
#else
	extern int engine_mouse_x;
	extern int engine_mouse_y;
	extern int engine_mouse_key;
	#define amal_mouse_x engine_mouse_x
	#define amal_mouse_y engine_mouse_y
#endif

#ifdef show_debug_amal_no
#define dumpAmalStack(...)
#endif

void *cb_add(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_inc(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_move(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_parenthses_default(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_z(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_div(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_equal_reg(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_less(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_less_or_equal(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_more(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_more_or_equal(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_mul(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_not_equal(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_set_reg(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_sub(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_while_status(struct kittyChannel *self, struct amalCallBack *cb);

void *cb_and(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_xor(struct kittyChannel *self, struct amalCallBack *cb);
void *cb_or(struct kittyChannel *self, struct amalCallBack *cb);

extern bool has_collided(int id);

struct amalDebugitem amalDebugList[] =
	{
		{cb_move, "move"},
		{cb_add, "add"},
		{cb_sub, "sub"},
		{cb_mul, "mul"},
		{cb_div, "div"},
		{cb_not_equal, "not_equal"},
		{cb_less, "less"},
		{cb_more, "more"},
		{cb_and, "and"},
		{cb_xor, "xor"},
		{cb_less_or_equal, "less_or_equal"},
		{cb_more_or_equal, "more_or_equal"},
		{cb_z, "z"},
		{cb_parenthses_default, "parenthses_default"},
		{cb_while_status, "while_status"},
		{cb_set_reg, "set_reg"},
		{cb_equal_reg, "equal_reg"},
		{cb_inc, "inc"},
		{NULL,"End Of List"}
	};

void *autotest_start API_AMAL_CALL_ARGS
{
	void **new_code;
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	new_code = (void **) ((char *) self -> amalProg.call_array + (unsigned int) code[1]);
	if (new_code) return new_code-1;

	return NULL;
}

void *autotest_end API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	return NULL;
}



void *amal_call_j0 API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount ] = amiga_joystick_dir[0] | (amiga_joystick_button[0] << 4);
	amalFlushParaCmds( self );
	return NULL;
}

#ifndef test_app

void *amal_call_pause API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> amalStatus |= channel_status::paused;
	self -> loopCount = 0;

	printf("Amal Status %d\n",self -> amalStatus);

	return NULL;
}

void *amal_call_j1 API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount ] = amiga_joystick_dir[1] | (amiga_joystick_button[1] << 4);
	amalFlushParaCmds( self );
	return NULL;
}
#endif

void *amal_call_screen_width API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	Printf("**** NOT YET WORKING %s ****\n",__FUNCTION__);
	return NULL;
}

void *amal_call_screen_height API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	Printf("**** NOT YET WORKING %s ****\n",__FUNCTION__);
	return NULL;
}

void *amal_set_num API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount ] = (int) code[1];

	AmalPrintf("num %d\n",(int) code[1] );

	amalFlushParaCmds( self );

	return code+1;
}

void *amal_call_x API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> last_reg = -1;
	self -> argStack [ self -> argStackCount ] = self -> objectAPI -> getX( self -> number );
	AmalPrintf("%d\n",self -> objectAPI -> getX( self -> number ));
	return NULL;
}

void *amal_call_y API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> last_reg = -2;
	self -> argStack [ self -> argStackCount ] = self -> objectAPI -> getY( self -> number );
	AmalPrintf("%d\n",self -> objectAPI -> getY( self -> number ));
	return NULL;
}

void *amal_call_image API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> last_reg = -3;
	self -> argStack [ self -> argStackCount ] = self -> objectAPI -> getY( self -> number );
	return NULL;
}

void *amal_call_reg API_AMAL_CALL_ARGS
{
	unsigned char c;
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	c = (unsigned char) (int) code[1];
	self -> last_reg = c;

	if ((c>='0')&&(c<='9'))
	{
		self -> argStack [ self -> argStackCount ] = self -> reg[c-'0'];
	} else if ((c>='A')&&(c<='Z'))
	{
		self -> argStack [ self -> argStackCount ] = amreg[ c - 'A'];
	}

	amalFlushParaCmds( self );

	return code+1;
}

void *amal_call_on API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> amalStatus &= ~channel_status::wait;
	return NULL;
}

void *amal_call_direct API_AMAL_CALL_ARGS	// jumps out of autotest.
{
	void *(**ret) API_AMAL_CALL_ARGS;

	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	getchar();

	ret = (void *(**) API_AMAL_CALL_ARGS) code[1];
	if (ret) return ret-1;

	return code+1;
}

void *amal_call_wait API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> amalStatus |= channel_status::wait;

	Printf("Amal Status %ld\n",self -> amalStatus);

	return NULL;
}

void *amal_call_if API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount ] = 0;
	return code+1;
}

void *amal_call_jump_autotest API_AMAL_CALL_ARGS
{
	void **ret;

	amalFlushAllCmds( self );	// comes after "IF", we need to flush, no ";" symbol.

	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	AmalPrintf("self -> loopCount %ld\n", self -> loopCount);

	ret = (void **) code[1];
	if (ret)
	{
		if (self -> autotest_loopCount>100)		// so in autotest we need to make sure we do not get stuck!!!.
		{
			AmalPrintf("self -> status = channel_status::paused\n");
			self -> amalStatus |= channel_status::paused;
			self -> autotest_loopCount = 0;
			return ret-1;
		}
		else
		{
			self -> autotest_loopCount++;
			AmalPrintf("[exit] self -> autotest_loopCount %ld\n", self -> autotest_loopCount);
			return ret-1;
		}
	}
	else 	AmalPrintf("Amal Jump did not find a ret value\n");

	return code+1;
}

void *amal_call_jump API_AMAL_CALL_ARGS
{
	void **ret;

	amalFlushAllCmds( self );	// comes after "IF", we need to flush, no ";" symbol.

	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	AmalPrintf("self -> loopCount %ld\n", self -> loopCount);

	ret = (void **) code[1];
	if (ret)
	{
		if (self -> loopCount>9)
		{
			AmalPrintf("self -> status = channel_status::paused\n");
			self -> amalStatus |= channel_status::paused;
			self -> loopCount = 0;
			return ret-1;
		}
		else
		{
			self -> loopCount++;
			AmalPrintf("[exit] self -> loopCount %ld\n", self -> loopCount);
			return ret-1;
		}
	}
	else 	AmalPrintf("Amal Jump did not find a ret value\n");

	return code+1;
}

void *amal_call_let API_AMAL_CALL_ARGS
{
	amalFlushAllCmds( self );	// comes after "IF", we need to flush, no ";" symbol.
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	return NULL;
}

void *amal_call_autotest API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	return NULL;
}

void *cb_move  (struct kittyChannel *self, struct amalCallBack *cb)
{
	int args = self -> argStackCount - cb -> argStackCount + 1 ;

	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	if (args == 3)
	{
		if (( self -> move_count == 0)&&( self -> move_count_to == 0))
		{
			self -> move_from_x = self -> objectAPI -> getX( self -> number );
			self -> move_from_y = self -> objectAPI -> getY( self -> number );

			self -> move_delta_x = self -> argStack [ self -> argStackCount - 2 ];
			self -> move_delta_y = self -> argStack [ self -> argStackCount - 1 ];
			self -> move_count = 0;
			self -> move_count_to = self -> argStack [self -> argStackCount ];

			// reset stack
			self -> argStackCount = cb -> argStackCount;
			return cb -> code - 1;
		}
		else	if (self -> move_count < self -> move_count_to)
		{
			int dxp,dyp;
			self -> move_count ++;

			dxp = self -> move_delta_x * self -> move_count / self -> move_count_to;
			dyp = self -> move_delta_y * self -> move_count / self -> move_count_to;
			self -> objectAPI -> setX( self -> number, self -> move_from_x + dxp );
			self -> objectAPI -> setY( self -> number, self -> move_from_y + dyp );

			self -> amalStatus |= channel_status::paused;

			// reset stack
			self -> argStackCount = cb -> argStackCount;
			return cb -> code - 1;
		}
	}

	self -> move_count = 0;
	self -> move_count_to = 0;

	// reset stack
	self -> argStackCount = cb -> argStackCount;

	return NULL;
}	

void *amal_call_move API_AMAL_CALL_ARGS
{
	amalFlushAllCmds( self );	// comes after "IF", we need to flush, no ";" symbol.

	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	pushBackAmalCmd( amal::flag_cmd, code, self, cb_move ); 
	self -> argStack [ self -> argStackCount ] = 0;	// 
	return NULL;
}

void *cb_add (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

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
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, cb_add ); 
	return NULL;
}

void *cb_sub (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	dumpAmalStack( self );

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
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, cb_sub ); 
	return NULL;
}

void *cb_mul (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	dumpAmalStack( self );

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
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, cb_mul ); 
	return NULL;
}

void *cb_div (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
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
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, cb_div ); 
	return NULL;
}

void *cb_and (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] & self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret;
	}
	return NULL;
}

void *amal_call_and API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, cb_and ); 
	return NULL;
}

void *cb_xor (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] ^ self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret ;
	}
	return NULL;
}

void *amal_call_xor API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, cb_xor ); 
	return NULL;
}

void *cb_not_equal (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] > self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret ? ~0 : 0;
	}

	return NULL;
}

void *amal_call_not_equal API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_para, code, self, cb_not_equal ); 
	return NULL;
}

void *cb_less (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] < self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret ? ~0 : 0 ;;
	}

	return NULL;
}

void *amal_call_less API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStackCount  ++;
	self -> argStack [ self -> argStackCount ] = 0;	
	pushBackAmalCmd( amal::flag_para, code, self, cb_less ); 
	return NULL;
}

void *cb_more (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] > self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret ? ~0 : 0 ;;
	}

	return NULL;
}

void *amal_call_more API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	self -> argStack [ self -> argStackCount + 1 ] = 0;	// 
	self -> argStackCount  ++;
	pushBackAmalCmd( amal::flag_cmd, code, self, cb_more ); 
	return NULL;
}

void *cb_less_or_equal  (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] <= self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret ? ~0 : 0 ;
	}

	return NULL;
}

void *amal_call_less_or_equal API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStackCount  ++;
	self -> argStack [ self -> argStackCount ] = 0;	// 
	pushBackAmalCmd( amal::flag_cmd, code, self, cb_less_or_equal ); 
	return NULL;
}

void *cb_more_or_equal  (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] >= self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret ? ~0 : 0 ;
	}

	return NULL;
}

void *amal_call_more_or_equal API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStackCount  ++;
	self -> argStack [ self -> argStackCount ] = 0;
	pushBackAmalCmd( amal::flag_cmd, code, self, cb_more_or_equal ); 
	return NULL;
}

void *amal_call_play API_AMAL_CALL_ARGS
{
		AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	return NULL;
}

void *amal_call_end API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	return NULL;
}

void *amal_call_xm API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount  ] = amal_mouse_x;
	return NULL;
}

void *amal_call_ym API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount  ] = amal_mouse_y;	
	return NULL;
}

void *amal_call_k1 API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount  ] = engine_mouse_key & 1 ? ~0 : 0;
	return NULL;
}

void *amal_call_k2 API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount  ] = engine_mouse_key & 2 ? ~0 : 0;
	return NULL;
}

void *cb_z  (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount ] = rand() % (self -> argStack [ self -> argStackCount ]+1);
	return NULL;
}

void *amal_call_z API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	pushBackAmalCmd( amal::flag_para ,code, self, cb_z ); 
	return NULL;
}

void *cb_xh  (struct kittyChannel *self, struct amalCallBack *cb)
{
	int args = self -> argStackCount - cb -> argStackCount + 1 ;
	struct retroScreen *screen;
	int s,x = 0;
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	switch (args)
	{
		case 2:
			s = self -> argStack [ self -> argStackCount -1 ];			
			x = self -> argStack [ self -> argStackCount  ];
			if (screen = screens[s]) x = XHard_formula( screen, x );
			break;
	}

	// reset stack
	self -> argStackCount = cb -> argStackCount;
	self -> argStack [ self -> argStackCount ] = x;
	return NULL;
}

void *amal_call_xh API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> pushBackFunction = cb_xh;
	return NULL;
}

void *cb_yh  (struct kittyChannel *self, struct amalCallBack *cb)
{
	int args = self -> argStackCount - cb -> argStackCount + 1 ;
	struct retroScreen *screen;
	int s,y = 0;
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	switch (args)
	{
		case 2:
			s = self -> argStack [ self -> argStackCount -1 ];			
			y = self -> argStack [ self -> argStackCount ];
			if (screen = screens[s]) y = YHard_formula( screen, y );
			break;
	}

	// reset stack
	self -> argStackCount = cb -> argStackCount;
	self -> argStack [ self -> argStackCount ] = y;
	return NULL;
}

void *amal_call_yh API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> pushBackFunction = cb_yh;
	return NULL;
}

void *cb_xscreen  (struct kittyChannel *self, struct amalCallBack *cb)
{
	struct retroScreen *screen;
	int args = self -> argStackCount - cb -> argStackCount + 1 ;
	int s,x = 0;
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	switch (args)
	{
		case 2:
			s = self -> argStack [ self -> argStackCount -1 ];			
			x = self -> argStack [ self -> argStackCount  ];
			if (screen = screens[s]) x = XScreen_formula( screen, x );
			break;
	}

	// reset stack
	self -> argStackCount = cb -> argStackCount;
	self -> argStack [ self -> argStackCount ] = x;
	return NULL;
}

void *amal_call_xscreen API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> pushBackFunction = cb_xscreen;
	return NULL;
}

void *cb_yscreen  (struct kittyChannel *self, struct amalCallBack *cb)
{
	struct retroScreen *screen;
	int args = self -> argStackCount - cb -> argStackCount + 1 ;
	int s,y = 0;
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	switch (args)
	{
		case 2:
			s = self -> argStack [ self -> argStackCount -1 ];			
			y = self -> argStack [ self -> argStackCount  ];
			if (screen = screens[s]) y = YScreen_formula( screen, y );
			break;
	}

	// reset stack
	self -> argStackCount = cb -> argStackCount;
	self -> argStack [ self -> argStackCount ] = y;
	return NULL;
}

void *amal_call_yscreen API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	self -> pushBackFunction = cb_yscreen;
	return NULL;
}

int bobColRange( unsigned short bob, unsigned short start, unsigned short end );

void *cb_bobCol  (struct kittyChannel *self, struct amalCallBack *cb)
{
	int args = self -> argStackCount - cb -> argStackCount + 1 ;
	int16_t r = 0;
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	dumpAmalStack( self );
	if (args == 3)
	{
		uint16_t bob = self -> argStack [ self -> argStackCount -2 ];	
		uint16_t start = self -> argStack [ self -> argStackCount -1 ];			
		uint16_t end = self -> argStack [ self -> argStackCount  ];

		r = bobColRange( bob, start, end );
	}
	
	// reset stack
	self -> argStackCount = cb -> argStackCount;
	self -> argStack [ self -> argStackCount ] = r;
	return NULL;
}

void *amal_call_bobCol API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> pushBackFunction = cb_bobCol;
	return NULL;
}

void *cb_spriteCol  (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	// reset stack
	self -> argStackCount = cb -> argStackCount;
	self -> argStack [ self -> argStackCount ] = 0;
}

void *amal_call_spriteCol API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> pushBackFunction = cb_spriteCol;
	return NULL;
}

void *cb_col  (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	int ret = has_collided(self -> argStack [ self -> argStackCount ]) ? ~0 : 0;

	// reset stack
	self -> argStackCount = cb -> argStackCount;
	self -> argStack [ self -> argStackCount ] = ret;
}


void *amal_call_col API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	self -> pushBackFunction = cb_col;
	return NULL;
}


void *cb_vumeter  (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	Printf("**** NOT YET WORKING %s ****\n",__FUNCTION__);
}

void *amal_call_vumeter API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> pushBackFunction = cb_vumeter;
	return NULL;
}

void *amalFlushParaCmds( struct kittyChannel *self )
{
	void *ret;

//	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	while (self -> progStackCount)
	{
		struct amalCallBack *cb;
		cb = &self -> progStack[ self -> progStackCount -1 ];

		if (cb -> Flags & amal::flag_para)
		{
			self -> progStackCount --;
			ret =cb -> cmd( self, cb );
			if (ret) return ret;
		}
		else break;
	}
	return NULL;
}

void *amalFlushAllParenthsesCmds( struct kittyChannel *self )
{
	void *ret;

	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	while (self -> progStackCount)
	{
		struct amalCallBack *cb;
		cb = &self -> progStack[ self -> progStackCount -1 ];

		if (cb -> Flags & (amal::flag_para | amal::flag_parenthses))
		{
			self -> progStackCount --;
			ret =cb -> cmd( self, cb );
			if (ret) return ret;
		}
		else break;
	}
	return NULL;
}

void *amalFlushAllCmds( struct kittyChannel *self )
{
	void *ret;

	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	while (self -> progStackCount)
	{
		struct amalCallBack *cb;
		self -> progStackCount --;
		cb = &self -> progStack[ self -> progStackCount ];
		ret =cb -> cmd( self, cb );
		if (ret) return ret;
	}
	return NULL;
}

void *amal_call_next_cmd API_AMAL_CALL_ARGS
{
	void *ret;
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	ret = amalFlushAllCmds( self );

	return ret ? ret : NULL;
}


void *cb_parenthses_default  (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	dumpAmalStack( self );
	return NULL;
}

void *amal_call_parenthses_start API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	if (self -> pushBackFunction)
	{
		pushBackAmalCmd( amal::flag_parenthses ,code, self, self -> pushBackFunction ); 
		self -> pushBackFunction = NULL;
	}
	else
	{
		pushBackAmalCmd( amal::flag_parenthses ,code, self, cb_parenthses_default ); 
		self -> argStack [ self -> argStackCount ] = 0;	// 
	}

	return NULL;
}

void *amal_call_parenthses_end API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	amalFlushAllParenthsesCmds( self );	
	return NULL;
}

void *amal_call_end_label API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_nextArg API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	amalFlushParaCmds( self );

	self -> argStackCount  ++;
	self -> argStack [ self -> argStackCount ] = 0;	// 

	return NULL;
}


struct stringData *getRealAnimString(  struct kittyChannel *self, const char *str)
{
	const char *c;
	const char *c2;
	int reg ;
	int regv;
	char numstr[10];
	std::string tmp;

	for (c=str;*c;c++)
	{
		reg = -1;

		if (*c=='R')	// look for regs;
		{
			c2=c+1;

			if ((*c2>='0')&&(*c2<='9'))
			{
				c++;
				reg = *c2-'0';
				regv = self -> reg[ reg ] ;
			}
			else 	if ((*c2>='0')&&(*c2<='9'))
			{
				reg = (*c2-'A')+10;
				regv = amreg[ reg ];
			}
		}

		if ( reg == -1) 
			tmp += *c;
		else
		{
			sprintf(numstr,"%d",regv);
			tmp += numstr;
		}
	}

	return toAmosString( tmp.c_str(), tmp.size() );
}

void *amal_call_anim API_AMAL_CALL_ARGS
{
	int le;
	struct stringData *animCode ;
	
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	le = (int) code[1];
	animCode = getRealAnimString(self, (char *) &code[2]);

#ifdef test_app

	if (animCode)
	{
		printf("str: %s\n", &(animCode->ptr));
		free( animCode );
	}

#else 

	setChannelAnim( self, animCode, true );
#endif

	return code+1+le;	// 
}

void *cb_while_status  (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	dumpAmalStack( self );

	if ( ! self -> argStack [ self -> argStackCount ] )
	{
		char *start_location = (char *) self -> amalProg.call_array;
		int offset_location ;
		void **code = cb -> code;

		offset_location = (int) code[1];
		return start_location + offset_location + 4;
	}

	return NULL;
}

void *amal_call_while API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount ] = 0;	// set default value. 
	pushBackAmalCmd( amal::flag_cmd ,code, self, cb_while_status ); 
	return code +1;
}

void *amal_call_wend API_AMAL_CALL_ARGS
{
	char *location;
	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	location = (char *) self -> amalProg.call_array;
	location += (int) code[1];

	self -> amalStatus |= channel_status::paused;

	return location - sizeof(void *) ;
}

void *cb_set_reg (struct kittyChannel *self, struct amalCallBack *cb)
{
	int c;
	char chr[2] = {0,0};

	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	c = cb-> last_reg;

	switch (c)
	{
		case -1: 	self -> objectAPI -> setX( self -> number, self -> argStack [ self -> argStackCount ] );
				return NULL;;
		case -2:
				self -> objectAPI -> setY( self -> number, self -> argStack [ self -> argStackCount ] );
				return NULL;;
		case -3:
				self -> objectAPI -> setImage( self -> number, self -> argStack [ self -> argStackCount ] );
				return NULL;;
	}

	if ((c>='0')&&(c<='9'))
	{
		self -> reg[ c - '0' ] = self -> argStack [ self -> argStackCount ];
		chr[0] = c;
		AmalPrintf("R%s=%ld\n", chr, self -> reg[ c - '0' ]);
		return NULL;
	}
	
	if ((c>='A')&&(c<='Z'))
	{
		amreg[ c - 'A' ] = self -> argStack [ self -> argStackCount ];
		chr[0]=c;
		AmalPrintf("R%s=%ld\n",chr, amreg[ c - 'A' ]);
		return NULL;
	}

	return NULL;
}

void *amal_call_set API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStack [ self -> argStackCount ] = 0;	// set default value. 
	pushBackAmalCmd( amal::flag_cmd ,code, self,  cb_set_reg ); 
	return NULL;
}

void *cb_equal_reg (struct kittyChannel *self, struct amalCallBack *cb)
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	dumpAmalStack( self );

	if (self -> argStackCount+1 >= 2)
	{
		int ret = (self -> argStack [ cb -> argStackCount - 1 ] == self -> argStack [ cb -> argStackCount ]);
		self -> argStackCount -= 1;
		self -> argStack[ self -> argStackCount ] = ret ? ~0 : 0 ;
	}
	return NULL;
}

void *amal_call_equal API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> argStackCount ++;
	self -> argStack [ self -> argStackCount ] = 0;	// set default value. 
	pushBackAmalCmd( amal::flag_para ,code, self, cb_equal_reg ); 
	return NULL;
}


void *cb_inc  (struct kittyChannel *self, struct amalCallBack *cb)
{
	unsigned char c = self -> last_reg;
	char chr[2] = {c,0};
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	if ((c>='0')&&(c<='9'))
	{
		self -> reg[ c - '0' ] ++;
	}
	else 	if ((c>='A')&&(c<='Z'))
	{
		amreg[ c - 'A' ] ++;
		chr[0]=c;
	}

	return NULL;
}	

void *amal_call_inc API_AMAL_CALL_ARGS
{
	amalFlushAllCmds( self );	// comes after "IF", we need to flush, no ";" symbol.
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	pushBackAmalCmd( amal::flag_para ,code, self, cb_inc ); 
	return NULL;
}

void *amal_call_then API_AMAL_CALL_ARGS
{
	void **new_code;
	amalFlushAllCmds( self );	// comes after "IF", we need to flush, no ";" symbol.
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);

	if (self -> argStack [ self -> argStackCount ] == 0)
	{
		new_code = (void **) ((char *) self -> amalProg.call_array + (unsigned int) code[1]);
		if (new_code) return new_code-1;
	}

	return code+1;
}

void *amal_call_else API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	void **new_code;

	new_code = (void **) ((char *) self -> amalProg.call_array + (unsigned int) code[1]);
	if (new_code) return new_code-1;

	return code + 1;
}

void *amal_flush_prog API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	amalFlushAllCmds( self );
	return NULL;
}
