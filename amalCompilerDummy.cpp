
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(__amigaos4__) || defined(__amigaos__)
#include <proto/exec.h>
#include <proto/dos.h>
#endif

#include "AmalCompiler.h"
#include "channel.h"
#include "AmalCommands.h"
#include "amal_object.h"
#include "amoskittens.h"
#include "commandsScreens.h"
#include "amosstring.h"

#include "debug_amal_test_app.h"

extern void *amalFlushParaCmds( struct kittyChannel *self );
extern void *amalFlushAllCmds( struct kittyChannel *self );
extern void *amalFlushAllParenthsesCmds( struct kittyChannel *self );

extern void dump_object();

// this functions are used for testing the Amal compiler.

int bobCol( unsigned short bob, unsigned short start, unsigned short end )
{
	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return 0;
}

void *amal_call_pause API_AMAL_CALL_ARGS
{
	AmalPrintf("%s:%s:%ld - channel %d\n",__FILE__,__FUNCTION__,__LINE__, self -> id);
	self -> amalStatus = channel_status::paused;
	self -> loopCount = 0;

	dump_object();
	getchar();

	return NULL;
}

void *amal_call_j1 API_AMAL_CALL_ARGS
{
	int num;
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	printf("right 8, left 4, up 1, down 2\n");
	printf("Enter joystick 1 value:\n");
	scanf("%d\n",&num);

	self -> argStack [ self -> argStackCount ] = num;
	amalFlushParaCmds( self );
	return NULL;
}

