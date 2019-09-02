
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/diskfont.h>
#include <proto/graphics.h>
#include "amosString.h"

extern struct RastPort font_render_rp;
#endif

#ifdef __linux__
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
#include "kittyErrors.h"
#include "engine.h"

extern int last_var;

extern struct TextFont *open_font( char const *filename, int size );
extern struct TextFont *gfx_font ;

using namespace std;

class font
{
	public:
		string	amosString;
		string	name;
		int		size;

};

vector<font> fonts;

void set_font_buffer( char *buffer, char *name, int size, char *type )
{
	char *c;
	char *b;
	int n;
	memset(buffer,' ',37);	// char 0 to 36
	buffer[37]=0;

	n = 0; b = buffer;
	for (c=name;(*c) && (n<37);c++)
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

void getfonts(char *buffer, int source_type )
{
	int32 afShortage, afSize;
	struct AvailFontsHeader *afh;
	struct AvailFonts *_fonts;
	font tfont;

	afSize = 400;
	do
	{
		afh = (struct AvailFontsHeader *) AllocVecTags(afSize, TAG_END);
		if (afh)
		{
			afShortage = AvailFonts( (char *) afh, afSize, AFF_MEMORY | AFF_DISK);

			if (afShortage)
			{
				FreeVec(afh);
				afSize += afShortage;
			}
			else
			{
				int n;
				_fonts = (AvailFonts *) ((char *) afh + sizeof(uint16_t));
				for (n=0;n<afh -> afh_NumEntries;n++)
				{
					if ((_fonts -> af_Type | source_type ) && (_fonts ->  af_Attr.ta_Style == FS_NORMAL))
					{
						set_font_buffer(buffer, (char *) _fonts -> af_Attr.ta_Name, _fonts -> af_Attr.ta_YSize, (char *) "disc");
						tfont.amosString =buffer;
						tfont.name = _fonts -> af_Attr.ta_Name;
						tfont.size = _fonts -> af_Attr.ta_YSize;
						fonts.push_back(tfont);
					}
					_fonts++;
				}
				FreeVec(afh);
			}
		}
		else
		{
			printf("AllocMem of AvailFonts buffer afh failed\n");
			break;
		}
	} while (afShortage);
}


char *fontsGetRomFonts(struct nativeCommand *cmd, char *tokenBuffer)
{
	char buffer[39];

	fonts.erase( fonts.begin(), fonts.end() );
	getfonts( buffer,  AFF_MEMORY );

	return tokenBuffer;
}

char *fontsGetDiscFonts(struct nativeCommand *cmd, char *tokenBuffer)
{
	char buffer[39];

	fonts.erase( fonts.begin(), fonts.end() );
	getfonts( buffer,  AFF_DISK );

	return tokenBuffer;
}

char *fontsGetAllFonts(struct nativeCommand *cmd, char *tokenBuffer)
{
	char buffer[39];

	fonts.erase( fonts.begin(), fonts.end() );
	getfonts( buffer,  AFF_DISK | AFF_MEMORY );

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

			if ((ret>=0)&&(ret<=(int) fonts.size()))
			{
				if (gfx_font) CloseFont(gfx_font);		
				gfx_font = open_font( fonts[ret].name.c_str(),fonts[ret].size );

				engine_lock();
				if (engine_ready())
				{
					SetFont( &font_render_rp, gfx_font );
				}
				engine_unlock();
			}

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

				if ((index>0) && (index <= fonts.size()))
				{
					popStack( stack - data->stack );
					setStackStr( 
						toAmosString( 
							fonts[ index-1 ].amosString.c_str(), 
							strlen(fonts[ index-1 ].amosString.c_str()) ) );
					return NULL;
				}
				break;
		default:
				setError(22,data->tokenBuffer);
	}

	popStack( stack - data->stack );
	setStackStrDup(toAmosString("",0));
	return NULL;
}

char *fontsFontsStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _fontsFontsStr, tokenBuffer );
	setStackNone();
	return tokenBuffer;
}

