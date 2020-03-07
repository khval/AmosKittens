#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <stdint.h>
#include <string>
#include <iostream>

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

#include <amosKittens.h>
#include <stack.h>

#include "debug.h"

#include "commandsBackgroundGraphics.h"
#include "kittyErrors.h"
#include "engine.h"
#include "commandsbanks.h"
		
extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;
extern int tokenlength;

extern struct retroVideo *video;

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
	int args =__stack - data->stack +1 ;
	struct retroScreen *screen;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 3:	if ((instance.icons) && (screen = instance.screens[instance.current_screen]))
				{
					int x = getStackNum(__stack-2 );
					int y = getStackNum(__stack-1 );
					int image = getStackNum(__stack );

					switch (screen -> autoback)
					{
						case 0:	retroPasteIcon( screen, screen -> double_buffer_draw_frame,  instance.icons,x,y,image-1);
								break;
						default:	retroPasteIcon( screen, 0, instance.icons,x,y,image-1);
								if (screen -> Memory[1]) retroPasteIcon( screen, 1,  instance.icons,x,y,image-1);
								break;
					}		
				}
				break;
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );
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
	int args =__stack - data->stack +1 ;
	struct retroScreen *screen = instance.screens[instance.current_screen];
	struct kittyBank *bank1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 5:	// get icon i,x,y to x2,y2
				{
					int image = getStackNum(__stack-4 );
					int x0 = getStackNum(__stack-3 );
					int y0 = getStackNum(__stack-2 );
					int x1 = getStackNum(__stack-1 );
					int y1 = getStackNum(__stack );

					if (screen)
					{
						if (instance.icons==NULL)
						{
							instance.icons = (struct retroSprite *) sys_public_alloc_clear(sizeof(struct retroSprite));
						}

						if (instance.icons)
						{
							retroGetSprite(screen,instance.icons,image-1,x0,y0,x1,y1);
						}

						bank1 = findBank(2);

						if (!bank1) 
						{
							if (bank1 = __ReserveAs( bank_type_icons, 2, sizeof(void *),NULL, NULL))							
							{
								bank1 -> object_ptr = (char *) instance.icons;
							} 
						}
					}
				}
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );
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
//	int args =__stack - data->stack +1 ;
	struct retroScreen *screen = instance.screens[instance.current_screen];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ((instance.icons)&&(screen))
	{
		for (n=0;n<256;n++)
		{
			retroScreenColor( screen, n, 
				instance.icons -> palette[n].r, 
				instance.icons -> palette[n].g, 
				instance.icons -> palette[n].b );
		}
	}

	popStack(__stack - data->stack );
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
	int args =__stack - data->stack +1 ;
	int del, delTo;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			del = getStackNum(__stack);
			del_sprite_object(instance.icons, del-1);
			break;

		case 2:
			del = getStackNum(__stack-1);
			delTo = getStackNum(__stack);

			while (delTo>=del)
			{
				del_sprite_object(instance.icons, del-1);
				delTo--;
			}

			break;

		default:
			setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );
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
	int args =__stack - data->stack +1 ;
	int pick = 0;
	bool success = false;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		switch (kittyStack[__stack].type)
		{
			case type_none:

				if (instance.icons)
				{
					for (pick = 0;pick<instance.icons->number_of_frames;pick++)
					{
						instance.icons -> frames[pick].retroFlag = 1 ;
					}
					success = true;
				}

				break;

			case type_int:

				pick = getStackNum(__stack);

				if (instance.icons)
				{
					if ((pick>0)&&(pick<=instance.icons->number_of_frames))
					{
						instance.icons -> frames[pick-1].retroFlag = 1 ;
						success = true;
					}
				}
				break;

		}
	}

	if (success == false) setError(22, data->tokenBuffer);

	popStack(__stack - data->stack );

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


struct retroBlock *findBlock(std::vector<struct retroBlock> &blocks,int id)
{
	unsigned int b;

	for (b=0;b<blocks.size();b++)
	{
		if (blocks[b].id == id) return &blocks[b];
	}
	return NULL;
}


char *_bgGetBlock( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 5:	{
					struct retroBlock block;
					block.id = getStackNum(__stack-4);
					block.x = getStackNum(__stack-3);
					block.y = getStackNum(__stack-2);
					block.w = getStackNum(__stack-1);
					block.h = getStackNum(__stack);
					block.mask = 0;

					del_block( blocks, block.id );
					block.mem  = (unsigned char *) malloc( block.w * block.h );		
					retroGetBlock(instance.screens[instance.current_screen],0,&block, block.x, block.y);
					blocks.push_back(block);
				}
				break;
		case 6:	{
					struct retroBlock block;
					block.id = getStackNum(__stack-5);
					block.x = getStackNum(__stack-4);
					block.y = getStackNum(__stack-3);
					block.w = getStackNum(__stack-2);
					block.h = getStackNum(__stack-1);
					block.mask = getStackNum(__stack);

					del_block( blocks, block.id );	// delete old
					block.mem  = (unsigned char *) malloc( block.w * block.h );
					retroGetBlock(instance.screens[instance.current_screen],0,&block, block.x, block.y);

					blocks.push_back(block);

				}
				break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );
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
	int args =__stack - data->stack +1 ;
	struct retroScreen *screen;
	struct retroBlock *block = NULL;
	int id;
	int x=0,y=0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			id = getStackNum(__stack);
			block = findBlock(blocks, id);
			if (block)
			{
				x=block->x;
				y=block->y;
			}
			break;

		case 3:
			id = getStackNum(__stack-2);
			x = getStackNum(__stack-1);
			y = getStackNum(__stack);
			block = findBlock(blocks, id);
			popStack(__stack - data->stack );
			break;

		default:
			popStack(__stack - data->stack );
			setError(22,data->tokenBuffer);
	}

	if (block)
	{
		screen = instance.screens[ instance.current_screen ];
		if (screen)
		{
			if (block) 
			{
				switch (screen -> autoback)
				{
					case 0:	retroPutBlock(screen, screen -> double_buffer_draw_frame, block, x,y, 255);
							break;
					default:	retroPutBlock(screen, 0, block, x,y, 255);
							if (screen -> Memory[1]) retroPutBlock(screen, 1, block, x,y, 255);
							break;
				}	
			}
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
	int args =__stack - data->stack +1 ;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			switch (kittyStack[__stack].type)
			{
				case type_none:
					while (blocks.size()) del_block( blocks, blocks.size() -1 ); 
					setError(22,data->tokenBuffer);			
					break;

				case type_int: 
					del_block( blocks, kittyStack[__stack].integer.value ); 
					break;

				default:
					setError(22,data->tokenBuffer);
			}
			break;
		default:
			popStack(__stack - data->stack );
			setError(22,data->tokenBuffer);
	}

	return NULL;
}

char *bgDelBlock(struct nativeCommand *cmd, char *tokenBuffer)
{
	// some thing to do with drawing, not sure.
	stackCmdNormal( _bgDelBlock, tokenBuffer );
	setStackNone();

	return tokenBuffer;
}

char *_bgGetCBlock( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);


	switch (args)
	{
		case 5:	{
					struct retroScreen *screen;
					struct retroBlock block;

					block.id = getStackNum(__stack-4);
					block.x = getStackNum(__stack-3);
					block.y = getStackNum(__stack-2);
					block.w = getStackNum(__stack-1);
					block.h = getStackNum(__stack);
					block.mask = 0;

					del_block( cblocks, block.id );
					block.mem  = (unsigned char *) malloc( block.w * block.h );		
					screen = instance.screens[instance.current_screen];

					retroGetBlock(screen ,screen -> double_buffer_draw_frame,&block, block.x, block.y);
					cblocks.push_back(block);
				}
				break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );
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
	int args =__stack - data->stack +1 ;
	struct retroScreen *screen;
	struct retroBlock *block = NULL;
	int x=0,y=0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			block = findBlock(blocks, getStackNum(__stack));
			if (block)
			{
				x = block -> x;
				y = block -> y;
			}
			break;
		case 3:
			block = findBlock(cblocks, getStackNum(__stack-2));
			x = getStackNum(__stack-1);
			y = getStackNum(__stack);
			x -= x & 8;
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );

	screen = instance.screens[ instance.current_screen ];
	if (screen)
	{
		if (block)
		{
			switch (screen -> autoback)
			{
				case 0:	retroPutBlock(screen, screen -> double_buffer_draw_frame, block, x,y, 255);
						break;
				default:	retroPutBlock(screen, 0, block, x,y, 255);
						if (screen -> Memory[1]) retroPutBlock(screen, 1, block, x,y, 255);
						break;
			}
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
	int args =__stack - data->stack +1 ;
	int id;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			id = getStackNum(__stack);
			del_block( cblocks, id );
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack(__stack - data->stack );
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
	int args =__stack - data->stack +1 ;
	int pick = 0;

	void *ret = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		pick = getStackNum(__stack);

		if (instance.icons)
		{
			if ((pick>0)&&(pick<=instance.icons->number_of_frames))
			{
				ret = &instance.icons -> frames[pick-1] ;
				popStack(__stack - data->stack );
				setStackNum( (int) ret );
				return NULL;
			}
		}
	}

	// failed quit here.
	
	popStack(__stack - data->stack );
	setError(22, data->tokenBuffer);
	return NULL;
}

char *bgIconBase(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _bgIconBase, tokenBuffer );
	return tokenBuffer;
}

//------


char *_bgVrevBlock( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	struct retroBlock *block = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		block = findBlock(blocks, getStackNum(__stack));
		if (block) block -> flag ^= flag_block_vrev;
		popStack(__stack - data->stack );
		return NULL;
	}

	// failed quit here.
	
	popStack(__stack - data->stack );
	setError(22, data->tokenBuffer);
	return NULL;
}

char *bgVrevBlock(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _bgVrevBlock, tokenBuffer );
	return tokenBuffer;
}

// -----


char *_bgHrevBlock( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	struct retroBlock *block = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		block = findBlock(blocks, getStackNum(__stack));
		if (block) block -> flag ^= flag_block_hrev;
		popStack(__stack - data->stack );
		return NULL;
	}

	// failed quit here.
	
	popStack(__stack - data->stack );
	setError(22, data->tokenBuffer);
	return NULL;
}

char *bgHrevBlock(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _bgHrevBlock, tokenBuffer );
	return tokenBuffer;
}




