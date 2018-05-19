
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

extern ULONG *codeset_page;

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

int _scancode;

char *cmdInkey(struct nativeCommand *cmd, char *tokenBuffer )
{
	char buf[2];

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	_scancode = 0;
	buf[0]=0;

	if (engine_started)
	{
		if (! keyboardBuffer.empty() )
		{
			struct InputEvent event;
			ULONG actual;
			char buffer[20];

			bzero(&event,sizeof(struct InputEvent));

			event.ie_NextEvent = 0;
       			event.ie_Class     = IECLASS_RAWKEY;
			event.ie_SubClass  = 0;

			if (_scancode = keyboardBuffer[0].Code)
			{
				event.ie_Code = keyboardBuffer[0].Code;
				event.ie_Qualifier = keyboardBuffer[0].Qualifier;
				actual = MapRawKey(&event, buffer, 20, 0);

				if (actual)
				{
					buf[0] = buffer[0];
					buf[1]=0;
				}
			}	

			keyboardBuffer.erase(keyboardBuffer.begin());
		}
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
