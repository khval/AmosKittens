
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#endif

#ifdef __linux__
#include <stdint.h>
#include "os/linux/stuff.h"
#endif

#include "debug.h"
#include <string>
#include <iostream>
#include <vector>
#include <string>

#include "stack.h"
#include "amosKittens.h"
#include "commandsFonts.h"
#include "errors.h"
#include "engine.h"

extern int last_var;

using namespace std;

vector<string> fonts;

void set_font_buffer( char *buffer, char *name, int size, char *type )
{
	char *c;
	char *b;
	int n;
	memset(buffer,' ',37);
	buffer[38]=0;

	n = 0; b = buffer;
	for (c=name;(*c) && (n<38);c++)
	{
		 *b++=*c;
		n++;
	}

	n = 0; b = buffer+37-4;
	for (c=type;(*c) && (n<4);c++)
	{
		 *b++=*c;
		n++;
	}

	b = buffer+37-6;
	while (size)
	{
		n = size % 10;
		size /=10;
		*b-- = n + '0';
	}
}

char *fontsGetRomFonts(struct nativeCommand *cmd, char *tokenBuffer)
{
	char buffer[38];
	int n;

	memset(buffer,' ',37);
	buffer[38]=0;
	
	fonts.erase( fonts.begin(), fonts.end() );

	set_font_buffer(buffer,"topaz.font",8,"disc");
	fonts.push_back(buffer);

	set_font_buffer(buffer,"junk.font",8,"disc");
	fonts.push_back(buffer);

	return tokenBuffer;
}

char *_fontsSetFont( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	int ret = 0;
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1: ret = getStackNum( stack ) ;
			break;
		default:
			setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	return NULL;
}

char *fontsSetFont(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _fontsSetFont, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

char *_fontsFontsStr( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	unsigned int index;
	string font;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:	index = (unsigned int) getStackNum(stack);

				printf("index: %d\n",index);

				if ((index>0) && (index <= fonts.size()))
				{
					popStack( stack - data->stack );
					setStackStrDup( fonts[ index-1 ].c_str() );
					return NULL;
				}
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackStrDup("");
	return NULL;
}

char *fontsFontsStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _fontsFontsStr, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

