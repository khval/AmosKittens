
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
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commandsGfx.h"
#include "kittyErrors.h"
#include "engine.h"

extern int sig_main_vbl;

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

extern int current_screen;


extern struct retroScreen *screens[8] ;
extern struct retroSprite *sprite;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];

char *_hsGetSpritePalette( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		if ((sprite)&&(screen))
		{
			switch (kittyStack[stack].type)
			{
				case type_none:

					for (n=0;n<256;n++)
					{
						retroScreenColor( screen, n, sprite -> palette[n].r, sprite -> palette[n].g, sprite -> palette[n].b );		
					}
					break;

				case type_int:
					{
						int mask = kittyStack[stack].integer.value;

						for (n=0;n<256;n++)
						{
							if (mask & n) retroScreenColor( screen, n, sprite -> palette[n].r, sprite -> palette[n].g, sprite -> palette[n].b );		
						}
					}
					break;

			}
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *hsGetSpritePalette(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _hsGetSpritePalette, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_hsSprite( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num;
	struct retroSpriteObject *sprite;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	engine_lock();
	num = getStackNum( stack - 3 );
	sprite = &video -> sprites[num];
	stack_get_if_int( stack - 2 , &sprite->x );
	stack_get_if_int( stack - 1 , &sprite->y );
	sprite->image = getStackNum( stack );
	engine_unlock();

	popStack( stack - data->stack );
	return NULL;
}

char *hsSprite(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _hsSprite, tokenBuffer );
	return tokenBuffer;
}

/*

char *_hsGetSprite( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *hsGetSprite(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _hsSprite, tokenBuffer );
	return tokenBuffer;
}

*/

char *_hsSpriteOff( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		struct kittyData *s = &kittyStack[stack];
		engine_lock();
		if (s -> type == type_int)
		{
			video -> sprites[ s -> integer.value ].image = -1;	// not deleted, just gone.
		}
		else
		{
			int n;
			for (n=0;n<64;n++)
			{
				video -> sprites[n].image = -1;
			}
		}
		engine_unlock();
		return NULL;
	}

	popStack( stack - data->stack );
	return NULL;
}

char *hsSpriteOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _hsSpriteOff, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_hsSpriteBase( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int pick = 0;
	void *ret = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		pick = getStackNum(stack);

		if (sprite)	if ((pick>0)&&(pick<sprite->number_of_frames))
		{
			ret = &sprite -> frames[pick-1] ;
		}

		if (NULL == ret) setError(23,data->tokenBuffer);
	}
	else setError(22, data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum( (int) ret );
	return NULL;
}

char *hsSpriteBase(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _hsSpriteBase, tokenBuffer );
	return tokenBuffer;
}

char *_hsSetSpriteBuffer( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
	}
	else setError(22, data->tokenBuffer);

	popStack( stack - data->stack );
	return NULL;
}

char *hsSetSpriteBuffer(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _hsSetSpriteBuffer, tokenBuffer );
	return tokenBuffer;
}
