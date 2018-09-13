
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "AmalCompiler.h"
#include "channel.h"
#include "AmalCommands.h"

extern void pushBackAmalCmd( struct kittyChannel *channel, void *cmd ) ;

void *amal_set_num API_AMAL_CALL_ARGS
{
	int num;
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);

	num = (int) code[1];
	printf("number should be %d\n",num);

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
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
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

void *amal_call_add API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_sub API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_mul API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_div API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_and API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_not_equal API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_less API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_more API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	return NULL;
}

void *amal_call_equal API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
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
	return NULL;
}

void *amal_call_ym API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
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

void *amal_call_nextCmd API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
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
	return NULL;
}

void *amal_call_anim API_AMAL_CALL_ARGS
{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
//	pushBackAmalCmd( self, NULL ) ;
	return NULL;
}

