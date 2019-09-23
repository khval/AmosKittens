#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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
			i -> str = strdup( &(getStackString( stack ) -> ptr));
		}
		_set_menu_item = 0;
	}

	_do_set = _setVar;

	return NULL;
}

struct amos_selected _selected_;

char *_menuChoice( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int i = 0;
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			if (kittyStack[stack].type == type_none)
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

	popStack( stack - data->stack );
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
	int args = stack - data->stack +1 ;
	int n;
	struct amosMenuItem *menuItem = NULL;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	menuItem = (struct amosMenuItem *) malloc( sizeof(struct amosMenuItem) );

	_set_menu_item = 0;

	if (menuItem)
	{
		menuItem -> levels = args;
		menuItem -> str = NULL;
		menuItem -> active = true;

		for (n=0;n<args;n++)
		{
			menuItem -> index[n] = getStackNum(data->stack + n);
		}

		menuitems.push_back(menuItem);	

		_set_menu_item = menuitems.size();	
	}

	popStack( stack - data->stack );
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
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
	}

	popStack( stack - data->stack );
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
	unsigned int n;
	int nn;
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
	unsigned int n;
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



struct NewMenu *alloc_amiga_menu( int items )
{
	ULONG levels[]={NM_TITLE,NM_ITEM,NM_SUB,NM_SUB,NM_SUB,NM_SUB,NM_SUB,NM_SUB,NM_SUB,NM_SUB};
	int n;
	ULONG flag;
	struct NewMenu *_new_ = (struct NewMenu *) AllocVecTags( sizeof(struct NewMenu) * (items+1), TAG_END );

	if (_new_)
	{
		for (n=0;n<items;n++)
		{
			flag = 0;

			if (menuitems[n] -> active == false)
			{
				switch (levels[ menuitems[n] -> levels - 1 ])
				{
					case NM_TITLE: flag |= NM_MENUDISABLED; break;
					case NM_ITEM: flag |= NM_ITEMDISABLED; break;
					case NM_SUB: flag |= NM_ITEMDISABLED; break;
				}
			}

			_new_[n].nm_Type = levels[ menuitems[n] -> levels - 1 ];
			_new_[n].nm_Label = (STRPTR) menuitems[n] -> str;
    			_new_[n].nm_CommKey = 0;     
    			_new_[n].nm_Flags = flag;        
    			_new_[n].nm_MutualExclude = 0 ;
    			_new_[n].nm_UserData = (void *) n;
		}
		_new_[items].nm_Type =NM_END;	// set end of menu
	}

	return _new_;
}

void attach_menu(struct Window *window)
{
	struct VisualInfo *vi;

	if ((amiga_menu)&&(window))
	{
		printf("%s:%d\n",__FUNCTION__,__LINE__);

		if (vi = (VisualInfo *) GetVisualInfoA( window -> WScreen,  NULL))
		{
			printf("%s:%d\n",__FUNCTION__,__LINE__);
			LayoutMenus( amiga_menu, vi, GTMN_NewLookMenus, TRUE, TAG_END );
		}

		printf("%s:%d\n",__FUNCTION__,__LINE__);
		SetMenuStrip(window, amiga_menu);
	}
}

void detach_menu(struct Window *window)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);
	if (amiga_menu)
	{
		printf("%s:%d\n",__FUNCTION__,__LINE__);
		ClearMenuStrip(window);
		FreeMenus( amiga_menu );
		amiga_menu = NULL;
	}
}

extern struct Window *My_Window ;

char *menuMenuOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	detach_menu(My_Window);	// safty.

	sort_menu();
//	dump_menu();

	if (amiga_menu_data) FreeVec(amiga_menu_data);
	amiga_menu_data = alloc_amiga_menu( menuitems.size() );

	if (amiga_menu_data)
	{
		amiga_menu = CreateMenusA( amiga_menu_data,NULL);
		attach_menu(My_Window);
		SetWindowAttrs( My_Window, WA_RMBTrap, FALSE, TAG_END );

		printf("well done\n");
		getchar();
	}

	return tokenBuffer;
}

char *menuMenuOff(struct nativeCommand *cmd, char *tokenBuffer )
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	detach_menu(My_Window);
	SetWindowAttrs( My_Window, WA_RMBTrap, TRUE, TAG_END );

	return tokenBuffer;
}

int findAmosMenuItem(struct glueCommands *data)
{
	int i,n;
	int found;
	int args = stack - data->stack +1 ;
	struct amosMenuItem *menuItem;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	for (i = 0; i<(int) menuitems.size();i++)
	{
		menuItem = menuitems[i];

		if (menuItem -> levels == args)
		{
			found = 0;
			for (n=0;n<args;n++)
			{
				if (menuItem -> index[n] == getStackNum(data->stack + n)) found++;
			}

			if (found == args) return i;
		}
	}
	return -1;
}

char *_menuMenuInactive( struct glueCommands *data, int nextToken )
{
	int i;
	struct amosMenuItem *menuItem;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	i = findAmosMenuItem(data);
	if (i>-1)
	{
		menuItem = menuitems[i];
		menuItem -> active = false;
		popStack( stack - data->stack );

		if (amiga_menu)
		{
			detach_menu(My_Window);
			attach_menu(My_Window);
		}

		return NULL;
	}

	popStack( stack - data->stack );
	setError(22,data->tokenBuffer);	// not implemented

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
	int i;
	struct amosMenuItem *menuItem;

	i = findAmosMenuItem(data);
	if (i>-1)
	{
		menuItem = menuitems[i];
		menuItem -> active = true;
		popStack( stack - data->stack );

		if (amiga_menu)
		{
			detach_menu(My_Window);
			attach_menu(My_Window);
		}

		return NULL;
	}

	popStack( stack - data->stack );
	setError(22,data->tokenBuffer);	// not implemented

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
	int args = stack - data->stack +1 ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			break;
	}

	popStack( stack - data->stack );

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
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	NYI(__FUNCTION__);
	setError(23,data->tokenBuffer);		// not implemented

	popStack( stack - data->stack );
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
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	setError(23,data->tokenBuffer);		// not implemented
	popStack( stack - data->stack );

	return NULL;
}

char *menuMenuY(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _menuMenuY, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *to_menuMenuKey( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;

	printf("%s:%d\n",__FUNCTION__,__LINE__);
	dump_stack();
	getchar();

	switch (args)
	{
		case 2:
				break;
		case 3:
				break;
		default:
				setError(22,data->tokenBuffer);		// not implemented
				break;
	}

	popStack( stack - data->stack );

	return NULL;
}

char *menuKey_do_to ( struct nativeCommand *, char *tokenBuffer ) 
{
	stackCmdNormal( to_menuMenuKey, tokenBuffer );
}

char *_menuMenuKey( struct glueCommands *data, int nextToken )
{
	int i;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	i = findAmosMenuItem(data);
	popStack( stack - data->stack );
	setStackNum(i);
	stack ++;

	do_to[parenthesis_count] = menuKey_do_to;

	return NULL;
}

char *menuMenuKey(struct nativeCommand *cmd, char *tokenBuffer )
{
	stackCmdParm( _menuMenuKey, tokenBuffer );
	return tokenBuffer;
}

char *menuOnMenu(struct nativeCommand *cmd, char *tokenBuffer )
{
	uint16_t next_token  = *((uint16_t *) tokenBuffer) ;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (next_token)
	{
		case token_goto:
			tokenBuffer+=2;
			break;
		case token_gosub:
			tokenBuffer+=2;
			break;
		case token_proc:
			tokenBuffer+=2;
			break;
		default:
			setError(22, tokenBuffer);
	}

//	stackCmdNormal( _menuOnMenu, tokenBuffer );
	return tokenBuffer;
}

char *menuOnMenuOn(struct nativeCommand *cmd, char *tokenBuffer )
{
	return tokenBuffer;
}
