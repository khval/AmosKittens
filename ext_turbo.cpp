
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <string.h>
#include <stdint.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#endif

#include "debug.h"
#include <string>
#include <iostream>
#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsBanks.h"
#include "commandsBlitterObject.h"
#include "errors.h"
#include "engine.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;
extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern int current_screen;
extern struct retroSprite *sprite ;
extern struct retroSprite *icons ;

char *_ext_cmd_range( struct glueCommands *data, int nextToken )
{
	int ret = 0,_min,_max;
	int args = stack - data->stack +1;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	dump_stack();

	switch (args)
	{
		case 3:
			ret = getStackNum( stack-2 );
			_min = getStackNum( stack-1 );
			_max = getStackNum( stack );
	
			if (ret<_min) ret=_min;
			if (ret>_max) ret=_max;
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );
	setStackNum(ret);

	return  NULL ;
}

char *ext_cmd_range(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	do_to[parenthesis_count] = do_to_default;
	stackCmdParm( _ext_cmd_range, tokenBuffer );
	return tokenBuffer;
}
