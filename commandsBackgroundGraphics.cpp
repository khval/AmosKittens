#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
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
#include "commandsBackgroundGraphics.h"
#include "errors.h"
#include "engine.h"
#include "commandsbanks.h"
		
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

struct retroIcon 
{
	int id; 
	int x0;
	int y0;
	int x1;
	int y1;
};

std::vector<struct retroBlock> blocks;	
std::vector<struct retroBlock> cblocks;	


char *_bgPasteIcon( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:	if (icons)
				{
					int x = getStackNum( stack-2 );
					int y = getStackNum( stack-1 );
					int image = getStackNum( stack );

					retroPasteIcon(screens[current_screen],icons,x,y,image-1);
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
	struct retroScreen *screen = screens[current_screen];
	struct kittyBank *bank1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 5:	// get icon i,x,y to x2,y2
				{
					int image = getStackNum( stack-4 );
					int x0 = getStackNum( stack-3 );
					int y0 = getStackNum( stack-2 );
					int x1 = getStackNum( stack-1 );
					int y1 = getStackNum( stack );

					if (screen)
					{
						if (icons==NULL)
						{
							icons = (struct retroSprite *) sys_public_alloc_clear(sizeof(struct retroSprite));
						}

						if (icons)
						{
							retroGetSprite(screen,icons,image-1,x0,y0,x1,y1);
						}

						bank1 = findBank(2);

						if (!bank1) 
						{
							if (bank1 = __ReserveAs( bank_type_icons, 2, sizeof(void *),NULL, NULL))							
							{
								bank1 -> object_ptr = (char *) icons;
							} 
						}
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
//	int args = stack - data->stack +1 ;
	struct retroScreen *screen = screens[current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

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

extern bool del_sprite_object( struct retroSprite *sprite, int del);
char *_bgDelIcon( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int del, delTo;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("args: %d\n",args);

	switch (args)
	{
		case 1:
			del = getStackNum(stack);
			del_sprite_object(icons, del-1);
			break;

		case 2:
			del = getStackNum(stack-1);
			delTo = getStackNum(stack);

			while (delTo>=del)
			{
				del_sprite_object(icons, del-1);
				delTo--;
			}

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

char *_bgMakeIconMask( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int pick = 0;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		switch (kittyStack[stack].type)
		{
			case type_none:

				if (icons)
				{
					for (pick = 0;pick<icons->number_of_frames;pick++)
					{
						icons -> frames[pick].retroFlag = 1 ;
					}
					success = true;
				}

				break;

			case type_int:

				pick = getStackNum(stack);

				if (icons)
				{
					if ((pick>0)&&(pick<=icons->number_of_frames))
					{
						icons -> frames[pick-1].retroFlag = 1 ;
						success = true;
					}
				}
				break;

		}
	}

	if (success == false) setError(22, data->tokenBuffer);

	popStack( stack - data->stack );

	return NULL;
}

char *bgMakeIconMask(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	setStackNone();
	stackCmdNormal( _bgMakeIconMask, tokenBuffer );
	return tokenBuffer;
}

void del_block(std::vector<struct retroBlock> &blocks,int id)
{
	int _index = -1;
	unsigned int b;

	for (b=0;b<blocks.size();b++)
	{
		if (blocks[b].id == id) 
		{
			_index = b;
			break;
		}	
	}

	if (_index>-1)
	{
		if (blocks[_index].mem) 
		{
			free(blocks[_index].mem);
			blocks[_index].mem = NULL;
		}
		blocks.erase(blocks.begin()+_index);
	}
}


void retroPutBlock(struct retroScreen *screen, struct retroBlock *block,  int x, int y, unsigned char bitmask)
{
	unsigned char *sslice,*dslice;
	int _x,_y,dx,dy;

	for (_y=0;_y<block->h;_y++)
	{
		dy = _y+y;
		if ((dy>=0)&&(dy<screen->realHeight))
		{
			dslice = screen->Memory[0] + (screen->bytesPerRow*dy);
			sslice = block->mem + (block->w * _y);

			if (block->mask)
			{
				for (_x=0;_x<block->w;_x++)
				{
					dx = _x+x;
					if ((dx>=0)&&(dx<screen->realWidth))
					{
						if (sslice[_x]) dslice[dx]= sslice[_x] & bitmask;
					}
				}
			}
			else
			{
				for (_x=0;_x<block->w;_x++)
				{
					dx = _x+x;
					if ((dx>=0)&&(dx<screen->realWidth))
					{
						dslice[dx]= sslice[_x] & bitmask;
					}
				}
			}
		}
	}
}

struct retroBlock *findBlock(std::vector<struct retroBlock> &blocks,int id)
{
	unsigned int b;

	for (b=0;b<blocks.size();b++)
	{
		if (blocks[b].id == id) return &blocks[b];
	}
	return NULL;
}

void put_block(struct retroScreen *screen,struct retroBlock *block,  int x, int y, unsigned char bitmask)
{
	retroPutBlock(screen, block,   x,  y, bitmask);
}

char *_bgGetBlock( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 5:	{
					struct retroBlock block;
					block.id = getStackNum(stack-4);
					block.x = getStackNum(stack-3);
					block.y = getStackNum(stack-2);
					block.w = getStackNum(stack-1);
					block.h = getStackNum(stack);
					block.mask = 0;

					del_block( blocks, block.id );
					block.mem  = (unsigned char *) malloc( block.w * block.h );		
					retroGetBlock(screens[current_screen],&block, block.x, block.y);
					blocks.push_back(block);
				}
				break;
		case 6:	{
					struct retroBlock block;
					block.id = getStackNum(stack-5);
					block.x = getStackNum(stack-4);
					block.y = getStackNum(stack-3);
					block.w = getStackNum(stack-2);
					block.h = getStackNum(stack-1);
					block.mask = getStackNum(stack);

					del_block( blocks, block.id );	// delete old
					block.mem  = (unsigned char *) malloc( block.w * block.h );
					retroGetBlock(screens[current_screen],&block, block.x, block.y);

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
	struct retroScreen *screen;
	struct retroBlock *block = NULL;
	int x=0,y=0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:
			{
				int id = getStackNum(stack-2);
				x = getStackNum(stack-1);
				y = getStackNum(stack);
				block = findBlock(blocks, id);
			}		
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );

	if (block)
	{
		screen = screens[ current_screen ];
		if (screen)
		{
			if (block) put_block(screen, block, x,y, 255);
		}
	}

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
	int id;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			id = getStackNum(stack);
			del_block( blocks, id );
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
		case 5:	{
					struct retroBlock block;
					block.id = getStackNum(stack-4);
					block.x = getStackNum(stack-3);
					block.y = getStackNum(stack-2);
					block.w = getStackNum(stack-1);
					block.h = getStackNum(stack);
					block.mask = 0;

					del_block( cblocks, block.id );
					block.mem  = (unsigned char *) malloc( block.w * block.h );		
					retroGetBlock(screens[current_screen],&block, block.x, block.y);
					cblocks.push_back(block);
				}
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
	struct retroScreen *screen;
	struct retroBlock *block = NULL;
	int x=0,y=0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			block = findBlock(blocks, getStackNum(stack));
			if (block)
			{
				x = block -> x;
				y = block -> y;
			}
			break;
		case 3:
			block = findBlock(cblocks, getStackNum(stack-2));
			x = getStackNum(stack-1);
			y = getStackNum(stack);
			x -= x & 8;
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );

	if (block)
	{
		screen = screens[ current_screen ];
		if (screen)
		{
			if (block) put_block(screen, block, x,y, 255);
		}
	}

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
	int id;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			id = getStackNum(stack);
			del_block( cblocks, id );
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

char *_bgIconBase( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int pick = 0;

	void *ret = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		pick = getStackNum(stack);

		if (icons)
		{
			if ((pick>0)&&(pick<=icons->number_of_frames))
			{
				ret = &icons -> frames[pick-1] ;

				// success quit here.

				popStack( stack - data->stack );
				setStackNum( (int) ret );
				return NULL;
			}
		}
	}

	// failed quit here.
	
	popStack( stack - data->stack );
	setError(22, data->tokenBuffer);
	return NULL;
}

char *bgIconBase(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _bgIconBase, tokenBuffer );
	return tokenBuffer;
}

