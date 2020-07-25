
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

#include <amosKittens.h>
#include <stack.h>
#include "commandsGfx.h"
#include "kittyErrors.h"
#include "engine.h"
#include "amal_object_sprite.h"

int XScreen_formula( struct retroScreen *screen, int x );
int YScreen_formula( struct retroScreen *screen, int y );

extern int sig_main_vbl;

extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

extern struct retroRGB DefaultPalette[256];

extern std::vector<int> collided;
extern bool has_collided(int id);
extern void flush_collided();

#define getSprite(num) &(instance.video -> sprites[num])

char *_hsGetSpritePalette( struct glueCommands *data, int nextToken )
{
	int n;
	int args = __stack - data->stack +1 ;
	struct retroScreen *screen = instance.screens[instance.current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		if ((instance.sprites)&&(screen))
		{
			screen -> fade_speed = 0;

			switch (kittyStack[__stack].type)
			{
				case type_none:

					for (n=0;n<256;n++)
					{
						retroScreenColor( screen, n, instance.sprites -> palette[n].r, instance.sprites -> palette[n].g, instance.sprites -> palette[n].b );		
					}
					break;

				case type_int:
					{
						int mask = kittyStack[__stack].integer.value;

						for (n=0;n<256;n++)
						{
							if (mask & n) retroScreenColor( screen, n, instance.sprites -> palette[n].r, instance.sprites -> palette[n].g, instance.sprites -> palette[n].b );		
						}
					}
					break;

			}
		}
	}

	popStack(__stack - data->stack );
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
	int args = __stack - data->stack +1 ;
	int num;
	struct retroSpriteObject *sprite;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	num = getStackNum(__stack - 3 );

	engine_lock();
	sprite = &instance.video -> sprites[num];

	sprite -> id = num;

	if (stack_is_number(__stack - 2)) sprite->x = XSprite_formula(getStackNum(__stack - 2 ));
	if (stack_is_number(__stack - 1)) sprite->y = YSprite_formula(getStackNum(__stack - 1 ));
	sprite->image = getStackNum(__stack );
	engine_unlock();

//	printf("sprite %d,%d,%d,%d\n",num,sprite->x,sprite->y,sprite->image);

	popStack(__stack - data->stack );
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
	int args = __stack - data->stack +1 ;
	struct retroScreen *screen = screens[instance.current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	popStack(__stack - data->stack );
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
	int args = __stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		struct kittyData *s = &kittyStack[__stack];
		engine_lock();
		if (s -> type == type_int)
		{
			instance.video -> sprites[ s -> integer.value ].image = -1;	// not deleted, just gone.
		}
		else
		{
			int n;
			for (n=0;n<64;n++)
			{
				instance.video -> sprites[n].image = -1;
			}
		}
		engine_unlock();
		return NULL;
	}

	popStack(__stack - data->stack );
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
	int args = __stack - data->stack +1 ;
	int pick = 0;
	void *ret = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		pick = getStackNum(__stack);

		if (instance.sprites)	if ((pick>0)&&(pick<=instance.sprites->number_of_frames))
		{
			ret = &instance.sprites -> frames[pick-1] ;
		}

		if (NULL == ret) setError(23,data->tokenBuffer);
	}
	else setError(22, data->tokenBuffer);

	popStack(__stack - data->stack );
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
	int args = __stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
	}
	else setError(22, data->tokenBuffer);

	popStack(__stack - data->stack );
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
	int args = __stack - data->stack +1 ;
	int num = 0;
	int ret = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	flush_collided();

	if (args==1)
	{
		num = getStackNum(__stack);

//		Printf("Sprite Col(%ld)\n",num);

		if (num>=0) ret = spriteColAll( num );

	}
	else setError(22, data->tokenBuffer);

	popStack(__stack - data->stack );
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
	int args = __stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		object = getSprite( getStackNum(__stack) );

		if (object == NULL)
		{
			setError(23,data->tokenBuffer);
			return NULL;
		}

		setStackNum(from_XSprite_formula(object -> x));
		return NULL;	// don't need to pop stack.
	} 

	setError(23,data->tokenBuffer);
	popStack(__stack - data->stack );
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
	int args = __stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		object = getSprite( getStackNum(__stack) );

		if (object == NULL)
		{
			setError(23,data->tokenBuffer);
			return NULL;
		}

		setStackNum(from_YSprite_formula( object -> y ));
		return NULL;	// don't need to pop stack.
	} 

	setError(23,data->tokenBuffer);
	popStack(__stack - data->stack );
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
	int args = __stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		object = getSprite( getStackNum(__stack) );

		if (object == NULL)
		{
			setError(23,data->tokenBuffer);
			return NULL;
		}

		setStackNum( object -> image );
		return NULL;	// don't need to pop stack.
	} 

	setError(23,data->tokenBuffer);
	popStack(__stack - data->stack );
	return NULL;
}

char *hsISprite(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _hsISprite, tokenBuffer );
	return tokenBuffer;
}

char *hwSpriteUpdateOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *hwSpriteUpdate(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return tokenBuffer;
}

