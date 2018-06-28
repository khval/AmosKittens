#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "debug.h"
#include <string>
#include <iostream>
#include <proto/retroMode.h>

#include "stack.h"
#include "amosKittens.h"
#include "commandsGfx.h"
#include "errors.h"
#include "engine.h"

extern int sig_main_vbl;

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

extern int current_screen;


extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern struct retroSprite *sprite;
extern struct retroSpriteObject bobs[30];

char *_boBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num,image;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	num = getStackNum( stack - 3 );
	stack_get_if_int( stack - 2 , &bobs[num].x );
	stack_get_if_int( stack - 1 , &bobs[num].y );
	image = getStackNum( stack );

	if (sprite)
	{
		image = image % sprite -> number_of_frames;

		bobs[num].sprite = sprite ;
		bobs[num].frame = &sprite -> frames[image];

		if (screens[current_screen])
		{
			retroPasteSprite(screens[current_screen], sprite, bobs[num].x, bobs[num].y, image);
		}
	}
char *_boBob( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num;
	struct retroSpriteObject *bob;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	num = getStackNum( stack - 3 );
	bob = &bobs[num];

	stack_get_if_int( stack - 2 , &bob->x );
	stack_get_if_int( stack - 1 , &bob->y );

	bob->image = getStackNum( stack );

	bob->screen_id = current_screen;

	printf( "bob %d,%d,%d,%d\n", num,bob->x,bob->y,bob->image);

	popStack( stack - data->stack );
	return NULL;
}

char *boBob(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _boBob, tokenBuffer );
	return tokenBuffer;
}

char *boBobOff(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *_boNoMask( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	popStack( stack - data->stack );
	return NULL;
}

char *boNoMask(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _boNoMask, tokenBuffer );
	return tokenBuffer;
}

