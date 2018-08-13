#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <proto/exec.h>
#include <proto/dos.h>
#include "debug.h"
#include <string>
#include <iostream>
#include <proto/retroMode.h>

#include "stack.h"
#include "amosKittens.h"
#include "commandsBackgroundGraphics.h"
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

extern struct retroScreen *screens[8] ;
extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];
extern struct retroSprite *icons;


struct retroBlock
{
	int id; 
	int x0;
	int y0;
	int x1;
	int y1;
};

struct retroIcon 
{
	int id; 
	int x0;
	int y0;
	int x1;
	int y1;
};

std::vector<struct retroBlock> blocks;	// 0 is not used.


char *_bgPasteIcon( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:	if (icons)
				{
					int x = getStackNum( stack-2 );
					int y = getStackNum( stack-1 );
					int image = getStackNum( stack );
					int flags = image & 0xC000;
					image &= 0x3FFF;

					retroPasteSprite(screens[current_screen],icons,x,y,image-1,flags);
				}
				break;
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *bgPasteIcon(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _bgPasteIcon, tokenBuffer );
	return tokenBuffer;
}

char *_bgGetIcon( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int num;
	struct retroSpriteObject *bob;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 5:	// get bob i,x,y to x2,y2
				{
					int image = getStackNum( stack-4 );
					int x0 = getStackNum( stack-3 );
					int y0 = getStackNum( stack-2 );
					int x1 = getStackNum( stack-1 );
					int y1 = getStackNum( stack );

					if (icons==NULL)
					{
						icons = (struct retroSprite *) AllocVecTags(  sizeof(struct retroSprite), AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_END );
					}

					if (icons)
					{
						retroGetSprite(screens[current_screen],icons,image-1,x0,y0,x1,y1);
					}
				}
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *bgGetIcon(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _bgGetIcon, tokenBuffer );
	return tokenBuffer;
}

char *_bgGetIconPalette( struct glueCommands *data, int nextToken )
{
	int n;
	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ((icons)&&(screen))
	{
		for (n=0;n<256;n++)
		{
			retroScreenColor( screen, n, icons -> palette[n].r, icons -> palette[n].g, icons -> palette[n].b );
		}
	}

	popStack( stack - data->stack );
	return NULL;
}

char *bgGetIconPalette(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _bgGetIconPalette, tokenBuffer );
	return tokenBuffer;
}

char *_bgDelIcon( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	switch (args)
	{
		case 1:
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *bgDelIcon(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _bgDelIcon, tokenBuffer );
	return tokenBuffer;
}

char *_bgMaskIconMask( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	switch (args)
	{
		case 1:
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *bgMaskIconMask(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _bgMaskIconMask, tokenBuffer );
	return tokenBuffer;
}

char *_bgGetBlock( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	switch (args)
	{
		case 5:	{
					struct retroBlock block;
					block.id = getStackNum(stack-4);
					block.x0 = getStackNum(stack-3);
					block.y0 = getStackNum(stack-2);
					block.x1 = getStackNum(stack-1);
					block.y1 = getStackNum(stack);
					blocks.push_back(block);
				}
				break;
		case 6:	{
					struct retroBlock block;
					int flags;
					block.id = getStackNum(stack-5);
					block.x0 = getStackNum(stack-4);
					block.y0 = getStackNum(stack-3);
					block.x1 = getStackNum(stack-2);
					block.y1 = getStackNum(stack-1);
					flags = getStackNum(stack);
					blocks.push_back(block);
				}
				break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *bgGetBlock(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _bgGetBlock, tokenBuffer );
	return tokenBuffer;
}

char *_bgPutBlock( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	switch (args)
	{
		case 3:
//			n = getStackNum(stack-2);
//			x = getStackNum(stack-1);
//			y = getStackNum(stack);
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *bgPutBlock(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _bgPutBlock, tokenBuffer );
	return tokenBuffer;
}

char *_bgDelBlock( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	switch (args)
	{
		case 1:
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *bgDelBlock(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _bgDelBlock, tokenBuffer );
	return tokenBuffer;
}

char *_bgGetCBlock( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	switch (args)
	{
		case 1:
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *bgGetCBlock(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _bgGetCBlock, tokenBuffer );
	return tokenBuffer;
}

char *_bgPutCBlock( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	switch (args)
	{
		case 1:
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *bgPutCBlock(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _bgPutCBlock, tokenBuffer );
	return tokenBuffer;
}

char *_bgDelCBlock( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	switch (args)
	{
		case 1:
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *bgDelCBlock(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _bgDelCBlock, tokenBuffer );
	return tokenBuffer;
}


