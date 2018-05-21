
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/keymap.h>
#include <proto/diskfont.h>
#include <diskfont/diskfonttag.h>
#include "debug.h"
#include <string>
#include <iostream>
#include <proto/dos.h>
#include <vector>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "errors.h"
#include "engine.h"

extern std::vector<struct keyboard_buffer> keyboardBuffer;

extern int last_var;
extern ULONG *codeset_page;
int _scancode;
int _keyshift;
int keyState[256];

char *cmdWaitKey(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (engine_started)
	{
		engine_wait_key = true;
		do
		{
			Delay(1);
		} while ((engine_wait_key == true) && (engine_started));
	}
	else
	{
		getchar();
	}

	return tokenBuffer;
}

char *cmdInkey(struct nativeCommand *cmd, char *tokenBuffer )
{
	char buf[2];

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	_scancode = 0;
	buf[0]=0;

	if (engine_started)
	{
		struct InputEvent event;
		bzero(&event,sizeof(struct InputEvent));

		event.ie_NextEvent = 0;
       		event.ie_Class     = IECLASS_RAWKEY;
		event.ie_SubClass  = 0;

		engine_lock();
		if (! keyboardBuffer.empty() )
		{

			ULONG actual;
			ULONG code;
			char buffer[20];

			if (code = keyboardBuffer[0].Code)
			{
				if ((code & IECODE_UP_PREFIX) == 0)	// button pressed.
				{
					_scancode = code;

					event.ie_Code = keyboardBuffer[0].Code;
					event.ie_Qualifier = keyboardBuffer[0].Qualifier;
					actual = MapRawKey(&event, buffer, 20, 0);

					if (actual)
					{
						buf[0] = buffer[0];
						buf[1]=0;
					}
				}
			}	

			keyboardBuffer.erase(keyboardBuffer.begin());
		}
		engine_unlock();
	}

	setStackStrDup(buf);
	return tokenBuffer;
}

char *cmdScancode(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	setStackNum(_scancode);
	return tokenBuffer;
}

char *cmdKeyShift(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	setStackNum(_keyshift);
	return tokenBuffer;
}


char *cmdClearKey(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	engine_lock();
	keyboardBuffer.erase(keyboardBuffer.begin(),keyboardBuffer.begin()+keyboardBuffer.size());
	engine_unlock();

	_scancode = 0;

	setStackNum(0);
	return tokenBuffer;
}


char *_cmdKeyState( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	bool success = false;
	int ret = 0;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		int key = _stackInt( stack );

		if ((key>-1)&&(key<256))
		{
			ret = keyState[key];
			success = true;
		}
	}

	if (success == false) setError(22);

	popStack( stack - data->stack );
	setStackNum(ret);
	return NULL;
}

char *cmdKeyState(struct nativeCommand *cmd, char *tokenBuffer )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _cmdKeyState, tokenBuffer );
	return tokenBuffer;
}
