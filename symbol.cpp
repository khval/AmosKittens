
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <proto/retromode.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "amosKittens.h"
#include "commands.h"
#include <debug.h>

bool symbol_order[0x10000];

bool __correct_order( int last_token, int next_token );
bool correct_order( int last_token, int next_token );

void init_symbol_order()
{
	unsigned short last_token;
	unsigned short next_token;
	unsigned int i;

	for (i=0;i<0x10000;i++)
	{
		last_token = 0xFF00 | (i >> 8);
		next_token = 0xFF00 | (i & 0xFF);

//		printf("%04x - %04x,%04x\n",i,last_token,next_token);

		symbol_order[i] = __correct_order( last_token, next_token );

	}
/*
	printf("%d = correct_order( token_semi, token_semi )\n",correct_order( token_semi, token_semi ));
	printf("%d = __correct_order( token_semi, token_semi )\n",__correct_order( token_semi, token_semi ));
	getchar();
*/

}

bool correct_order( int last_token, int next_token )
{
/*
	printf("%d:%s -- %08x -- %08x\n",__LINE__,__FUNCTION__, 
			(last_token & next_token & 0xFFFFFF00),
			((last_token & 0xFF) <<8) | ( 0xFF & next_token));
*/

	if ((last_token & next_token & 0xFFFFFF00) == 0xFF00)
	{
		 return symbol_order[ ((last_token & 0xFF) <<8) | ( 0xFF & next_token) ];
	}

	return true;
}

bool __correct_order( int last_token, int next_token )
{
/*
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
*/

	switch (last_token)
	{
		case token_semi:
			if ((next_token == token_add)
				|| (next_token == token_sub)
				|| (next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)
				|| (next_token == token_more_or_equal )
				|| (next_token == token_less_or_equal	)
				|| (next_token == token_less_or_equal2 )
				|| (next_token == token_more_or_equal2 )
				|| (next_token == token_not_equal )
				|| (next_token == token_equal )
				|| (next_token == token_more )
				|| (next_token == token_less ) 
				|| (next_token == token_or)
				|| (next_token == token_xor)		// don't know the token number yet.
				|| (next_token == token_and)) return false;
			break;

		case token_xor:
			if ((next_token == token_add)
				|| (next_token == token_sub)
				|| (next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_and)
				|| (next_token == token_mod)
				|| (next_token == token_power)
				|| (next_token == token_more_or_equal )
				|| (next_token == token_less_or_equal	)
				|| (next_token == token_less_or_equal2 )
				|| (next_token == token_more_or_equal2 )
				|| (next_token == token_not_equal )
				|| (next_token == token_equal )
				|| (next_token == token_more )
				|| (next_token == token_less )) return false;
			break;

		case token_or:
		case token_and:
			if ((next_token == token_add)
				|| (next_token == token_sub)
				|| (next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)
				|| (next_token == token_more_or_equal )
				|| (next_token == token_less_or_equal	)
				|| (next_token == token_less_or_equal2 )
				|| (next_token == token_more_or_equal2 )
				|| (next_token == token_not_equal )
				|| (next_token == token_equal )
				|| (next_token == token_more )
				|| (next_token == token_less )) return false;
			break;

		case token_more_or_equal:
		case token_less_or_equal:
		case token_less_or_equal2:
		case token_more_or_equal2:
		case token_not_equal:
		case token_equal:
		case token_more:
		case token_less:

			if ((next_token == token_add)
				|| (next_token == token_sub)
				|| (next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)) return false;
			break;

		case token_add:
			if ((next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)) return false;
			break;

		case token_sub:
			if ((next_token == token_mul)
				|| (next_token == token_div)
				|| (next_token == token_mod)
				|| (next_token == token_power)) return false;
			break;

		case token_mul:
			if (next_token == token_power) return false;
			break;

		case token_div	:
			if (next_token == token_power) return false;
			break;
	}
	
	return true;
}


