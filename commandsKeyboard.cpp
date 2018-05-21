
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <proto/dos.h>
#include <vector>

#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/keymap.h>
#include <proto/diskfont.h>
#include <diskfont/diskfonttag.h>
#include "debug.h"
#include <proto/retroMode.h>

#include "stack.h"
#include "amosKittens.h"
#include "commands.h"
#include "commandsData.h"
#include "errors.h"
#include "engine.h"

extern std::vector<struct keyboard_buffer> keyboardBuffer;

extern int last_var;
extern ULONG *codeset_page;
extern struct globalVar globalVars[];
int _scancode;
int _keyshift;
int keyState[256];
extern struct retroScreen *screens[8] ;
extern int current_screen;

char *_setVar( struct glueCommands *data );

void _input_arg( struct nativeCommand *cmd, char *tokenBuffer );
void _inputLine_arg( struct nativeCommand *cmd, char *tokenBuffer );
void __print_text(const char *txt, int maxchars);

int input_count = 0;
std::string input_str;

using namespace std;


void get_char( char *buf)
{
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

					printf("event code %d\n",event.ie_Code);

					if (actual)
					{
						printf("set ascii - %d\n", buffer[0]);

						buf[0] = buffer[0];
						buf[1]=0;

						if (buf[0]==13) buf[0]=10;
					}
				}
			}	

			keyboardBuffer.erase(keyboardBuffer.begin());
		}
		engine_unlock();
	}
}

void kitty_getline(string &input)
{
	bool done = false;
	char buf[2];
	buf[1] = 0;
	int rightx;
	int charsPerRow;
	int charspace;

	int cursx = 0;
	int scrollx = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	input = "";

	if (engine_started)
	{
		engine_lock();
		rightx = screens[current_screen]->locateX;
		charsPerRow = (screens[current_screen]-> realWidth / 8);
		charspace = charsPerRow-rightx - 1;
		engine_unlock();

		done = false;

		do
		{
			get_char(buf);
			WaitTOF();

			if (buf[0] != 0) printf("--%d--\n",buf[0]);

			switch (buf[0])
			{
				case 0:
					break;

				case 8:
					printf("<backspace>\n");
					break;

				case 10:
					printf("<enter>\n");
					done=true;
					break;

				default:
					input += buf;
					cursx ++;

					engine_lock();
					screens[current_screen]->locateX = rightx;
					engine_unlock();

					if (cursx - scrollx > charspace) scrollx++;
					__print_text(input.c_str() + scrollx,charspace);
					break;	
			}

		} while (done == false);
	}
	else
	{
		getline(cin, input);
	}
}

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

	get_char(buf);

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

char *_Input(struct glueCommands *data)
{
	int args = stack - data -> stack +1;
	_input_arg( NULL, NULL );
	popStack( stack - data -> stack  );
	do_input = NULL;
	do_breakdata = NULL;
	return NULL;
}

char *_LineInput(struct glueCommands *data)
{
	int args = stack - data -> stack +1;
	_inputLine_arg( NULL, NULL );
	popStack( stack - data -> stack  );
	do_input = NULL;
	do_breakdata = NULL;
	return NULL;
}

void _input_arg( struct nativeCommand *cmd, char *tokenBuffer )
{
	int args = 0;
	int index = 0;
	int idx;
	size_t i;
	std::string arg = "";
	struct glueCommands data;
	bool success = false;
	int num;
	double des;

	if (cmd == NULL)
	{
		args = stack - cmdTmp[cmdStack].stack + 1;
	}
	else
	{
		if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _Input)
		{
			args = stack - cmdTmp[cmdStack-1].stack + 1;
		}
	}
	
	if ((input_count == 0)&&(stack))		// should be one arg.
	{
		char *str = _stackString( stack-args+1 );
		if (str)  __print_text( str ,0 );
	}
	else if (input_str.empty())
	{
		proc_names_printf("??? ");
	}

	do
	{
		do
		{
			while (input_str.empty()) kitty_getline( input_str);

			i = input_str.find(",");	
			if (i != std::string::npos)
			{
				arg = input_str.substr(0,i); input_str.erase(0,i+1);
			}
			else	
			{
				arg = input_str; input_str = "";
			}
		}
		while ( arg.empty() );

		if (last_var)
		{
			switch (globalVars[last_var -1].var.type & 7)
			{	
				case type_string:
					success = true; break;
				case type_int:
					success = arg.find_first_not_of( "-0123456789" ) == std::string::npos; break;
				case type_float:
					success = arg.find_first_not_of( "-0123456789." ) == std::string::npos; break;
			}
		}
	}
	while (!success);

	switch (globalVars[last_var -1].var.type & 7)
	{	
		case type_string:
			setStackStrDup(arg.c_str()); break;

		case type_int:
			sscanf(arg.c_str(),"%d",&num); _num(num); break;

		case type_float:
			sscanf(arg.c_str(),"%lf",&des); setStackDecimal(des); break;
	}

	data.lastVar = last_var;
	_setVar( &data );
	input_count ++;
}

void _inputLine_arg( struct nativeCommand *cmd, char *tokenBuffer )
{
	int args = 0;
	int index = 0;
	int idx;
	size_t i;
	std::string arg = "";
	struct glueCommands data;
	bool success = false;
	int num;
	double des;

	if (cmd == NULL)
	{
		args = stack - cmdTmp[cmdStack].stack + 1;
	}
	else
	{
		if (cmdStack) if (cmdTmp[cmdStack-1].cmd == _Input)
		{
			args = stack - cmdTmp[cmdStack-1].stack + 1;
		}
	}
	
	if ((input_count == 0)&&(stack))		// should be one arg.
	{
		char *str = _stackString( stack-args+1 );
		if (str) proc_names_printf("%s", str);
	}
	else if (input_str.empty())
	{
		proc_names_printf("??? ");
	}

	do
	{
		do
		{
			while (input_str.empty()) kitty_getline(input_str);
			arg = input_str; input_str = "";
		}
		while ( arg.empty() );

		if (last_var)
		{
			switch (globalVars[last_var -1].var.type & 7)
			{	
				case type_string:
					success = true; break;
				case type_int:
					success = arg.find_first_not_of( "-0123456789" ) == std::string::npos; break;
				case type_float:
					success = arg.find_first_not_of( "-0123456789." ) == std::string::npos; break;
			}
		}
	}
	while (!success);

	switch (globalVars[last_var -1].var.type & 7)
	{	
		case type_string:
			setStackStrDup(arg.c_str()); break;

		case type_int:
			sscanf(arg.c_str(),"%d",&num); _num(num); break;

		case type_float:
			sscanf(arg.c_str(),"%lf",&des); setStackDecimal(des); break;
	}

	data.lastVar = last_var;
	_setVar( &data );
	input_count ++;
}

void breakdata_inc_stack( struct nativeCommand *cmd, char *tokenBuffer )
{
	stack++;
}

char *cmdInput(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	input_count = 0;
	input_str = "";

	do_input = _input_arg;
	do_breakdata = breakdata_inc_stack;
	stackCmdNormal( _Input, tokenBuffer );

	return tokenBuffer;
}

char *cmdLineInput(nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	input_count = 0;
	input_str = "";

	do_input = _inputLine_arg;
	do_breakdata = breakdata_inc_stack;
	stackCmdNormal( _LineInput, tokenBuffer );

	return tokenBuffer;
}
