#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#endif

#include "debug.h"
#include <string>
#include <vector>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsMenu.h"
#include "var_helper.h"
#include "errors.h"
#include "engine.h"

extern int last_var;
extern char *(*_do_set) ( struct glueCommands *data, int nextToken );

unsigned int _set_menu_item = 0;
struct NewMenu *amiga_menu_data = NULL;
struct Menu *amiga_menu = NULL;

extern char *_setVar( struct glueCommands *data, int nextToken );

std::vector<struct amosMenuItem *> menuitems;
extern std::vector<struct amos_selected> amosSelected;

char *set_menu_item ( struct glueCommands *data, int nextToken )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (_set_menu_item)
	{
		struct amosMenuItem *i = menuitems[_set_menu_item-1];

		if (i)
		{
			if (i->str) free(i->str);
			i -> str = strdup(getStackString(__stack ));
		}
		_set_menu_item = 0;
	}

	_do_set = _setVar;

	return NULL;
}

struct amos_selected _selected_;

char *_menuChoice( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int i = 0;
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			if (kittyStack[__stack].type == type_none)
			{
				if (!amosSelected.empty())
				{
					_selected_ = amosSelected[0];
					amosSelected.erase(amosSelected.begin());
					ret = -1;
				}
			}
			else
			{
				i = getStackNum(stack);

				switch( i )
				{
					case 1:	ret = _selected_.menu + 1;	break;
					case 2:	ret = _selected_.item + 1;	break;
					case 3:	ret = _selected_.sub + 1;	break;
				}
			}
			break;
	}

	popStack(__stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *menuChoice(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _menuChoice, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_menuMenuStr( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int n;
	struct amosMenuItem *menuItem = NULL;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	menuItem = (struct amosMenuItem *) malloc( sizeof(struct amosMenuItem) );

	_set_menu_item = 0;

	if (menuItem)
	{
		menuItem -> levels = args;
		menuItem -> str = NULL;

		for (n=0;n<args;n++)
		{
			menuItem -> index[n] = getStackNum(data->stack + n);
		}

		menuitems.push_back(menuItem);	

		_set_menu_item = menuitems.size();	
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *menuMenuStr(struct nativeCommand *cmd, char *tokenBuffer )
{
	_do_set = set_menu_item;

	stackCmdParm( _menuMenuStr, tokenBuffer );
	return tokenBuffer;
}

char *_menuSetMenu( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
	}

	popStack(__stack - data->stack );
	return NULL;
}

char *menuSetMenu(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _menuSetMenu, tokenBuffer );
	return tokenBuffer;
}

bool compare_menu( struct amosMenuItem *i, struct amosMenuItem *n )
{
	int e, ie, ne;
	int elements = i -> levels < n -> levels ? i -> levels : n -> levels ;

	printf("%s:%d - start\n",__FUNCTION__,__LINE__);

	for (e = 0; e<elements ; e++ )
	{
		ie  = i -> index[ e ];
		ne =n -> index[ e ];
		if (ie < ne) return false;
		if (ie > ne) return true;
	}

	if (i -> levels > n -> levels) 
	{
		return true;
	}

	return false;
}

void dump_menu()
{
	int n,nn;
	struct amosMenuItem *i;
	for (n=0;n<menuitems.size();n++)
	{
		i = menuitems[n];
		printf("item %d ",n);

		for (nn=0;nn<i->levels;nn++)
		{		
			printf("[%08x]", i -> index[nn]);
		}

		printf(" -> str: %s\n", i -> str ? i -> str : "NULL");		
	}
}

void sort_menu()
{
	int n;
	struct amosMenuItem *temp;
	bool need_to_sort = true;

	while (need_to_sort)
	{
		need_to_sort = false;
		for (n=0;n<menuitems.size()-1;n++)
		{
			if (compare_menu( menuitems[n], menuitems[n+1] ))
			{
				temp = menuitems[n];
				menuitems[n] = menuitems[n+1];
				menuitems[n+1] = temp;
				need_to_sort = true; 
				break;
			}
		}
	}
}


char *menuMenuOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *menuMenuOff(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	return tokenBuffer;
}

char *_menuMenuInactive( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
	}

	popStack(__stack - data->stack );

	setError(23,data->tokenBuffer);	// not implemented

	return NULL;
}

char *menuMenuInactive(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _menuMenuInactive, tokenBuffer );
	return tokenBuffer;
}

char *_menuMenuActive( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
	}

	popStack(__stack - data->stack );

	setError(23,data->tokenBuffer);	// not implemented

	return NULL;
}

char *menuMenuActive(struct nativeCommand *cmd, char *tokenBuffer)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _menuMenuActive, tokenBuffer );
	return tokenBuffer;
}

char *menuMenuCalc(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	setError(23,tokenBuffer);		// not implemented

	return tokenBuffer;
}

char *_menuMenuToBank( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
	}

	popStack(__stack - data->stack );

	setError(23,data->tokenBuffer);	// not implemented

	return NULL;
}

char *menuMenuToBank(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _menuMenuToBank, tokenBuffer );
	return tokenBuffer;
}

char *menuMenuDel(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _menuMenuToBank, tokenBuffer );

	setError(23,tokenBuffer);		// not implemented

	return tokenBuffer;
}


char *_menuMenuX( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int i = 0;
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	setError(23,data->tokenBuffer);		// not implemented

	popStack(__stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *menuMenuX(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _menuMenuX, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_menuMenuY( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	int i = 0;
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	setError(23,data->tokenBuffer);		// not implemented

	popStack(__stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *menuMenuY(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _menuMenuY, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}


