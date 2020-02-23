
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
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

#include "debug.h"
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commandsGfx.h"
#include "kittyErrors.h"
#include "engine.h"

int XScreen_formula( struct retroScreen *screen, int x );
int YScreen_formula( struct retroScreen *screen, int y );

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

extern std::vector<int> collided;
extern bool has_collided(int id);
extern void flush_collided();

extern int XSprite_formula(int x);
extern int YSprite_formula(int y);
extern int from_XSprite_formula(int x);
extern int from_YSprite_formula(int y);

#define getSprite(num) &(video -> sprites[num])

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
			screen -> fade_speed = 0;

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

	sprite -> id = num;

	if (stack_is_number( stack - 2)) sprite->x = XSprite_formula(getStackNum( stack - 2 ));
	if (stack_is_number( stack - 1)) sprite->y = YSprite_formula(getStackNum( stack - 1 ));
	sprite->image = getStackNum( stack );
	engine_unlock();

//	printf("sprite %d,%d,%d,%d\n",num,sprite->x,sprite->y,sprite->image);

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

		if (sprite)	if ((pick>0)&&(pick<=sprite->number_of_frames))
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

int spriteColAll( unsigned short Sprite );
int spriteColRange( unsigned short Sprite, unsigned short start, unsigned short end );

char *_hsSpriteCol( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num = 0;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	flush_collided();

	if (args==1)
	{
		num = getStackNum(stack);

//		Printf("Sprite Col(%ld)\n",num);

		if (num>=0) ret = spriteColAll( num );

	}
	else setError(22, data->tokenBuffer);

	popStack( stack - data->stack );
	setStackNum( (int) ret );
	return NULL;
}

char *hsSpriteCol(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _hsSpriteCol, tokenBuffer );
	return tokenBuffer;
}

char *_hsXSprite( struct glueCommands *data, int nextToken )
{
	struct retroSpriteObject *object;
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		object = getSprite( getStackNum(stack) );

		if (object == NULL)
		{
			setError(23,data->tokenBuffer);
			return NULL;
		}

		setStackNum(from_XSprite_formula(object -> x));
		return NULL;	// don't need to pop stack.
	} 

	setError(23,data->tokenBuffer);
	popStack( stack - data->stack );
	return NULL;
}

char *hsXSprite(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _hsXSprite, tokenBuffer );
	return tokenBuffer;
}

char *_hsYSprite( struct glueCommands *data, int nextToken )
{
	struct retroSpriteObject *object;
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		object = getSprite( getStackNum(stack) );

		if (object == NULL)
		{
			setError(23,data->tokenBuffer);
			return NULL;
		}

		setStackNum(from_YSprite_formula( object -> y ));
		return NULL;	// don't need to pop stack.
	} 

	setError(23,data->tokenBuffer);
	popStack( stack - data->stack );
	return NULL;
}

char *hsYSprite(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _hsYSprite, tokenBuffer );
	return tokenBuffer;
}

char *_hsISprite( struct glueCommands *data, int nextToken )
{
	struct retroSpriteObject *object;
	int args = stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		object = getSprite( getStackNum(stack) );

		if (object == NULL)
		{
			setError(23,data->tokenBuffer);
			return NULL;
		}

		setStackNum( object -> image );
		return NULL;	// don't need to pop stack.
	} 

	setError(23,data->tokenBuffer);
	popStack( stack - data->stack );
	return NULL;
}

char *hsISprite(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _hsISprite, tokenBuffer );
	return tokenBuffer;
}



