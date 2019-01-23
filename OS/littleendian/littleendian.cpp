


#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__amigaos__) || defined(__aros__) || defined(__morphos__) 
#include <proto/exec.h>
#include <proto/dos.h>
#endif

#ifdef __linux__
#include <string.h>
#include <stdint.h>
#include "os/linux/stuff.h"
#endif

#include "amosKittens.h"
#include "commands.h"
#include "debug.h"
#include <vector>
#include "AmosKittens.h"
#include "errors.h"
#include "var_helper.h"
#include "pass1.h"

// this should read tokens, and convert every token that needs some help.

extern int nativeCommandsSize;
extern struct nativeCommand nativeCommands[];

extern int QuoteByteLength(char *ptr);
extern int ReferenceByteLength(char *ptr);

#define getBigEndienShort( name ) ((name[0]<<8)|name[1])
#define getBigEndienInt( name ) ((name[0]<<24)|(name[1]<<16)|(name[2]<<8)|name[3])

static void FixQuote(char *ptr)
{
	*((unsigned short *) ptr) = getBigEndienShort(ptr);
}

static void FixReference(char *ptr)
{
	struct reference *ref = (struct reference *) ptr;
	char *tptr = (char *) &ref -> ref;
	ref -> ref = getBigEndienShort(tptr);
}

static void FixExtension(char *ptr)
{
	struct extensionCommand *ext = (struct extensionCommand *) ptr;
	char *tptr = (char *) &ext -> ExtentionTokenTable;
	ext -> ExtentionTokenTable = getBigEndienShort(tptr);
}

static void FixBin(char *ptr)
{}
static void FixHex(char *ptr)
{}
static void FixFloat(char *ptr)
{}
static void FixFor(char *ptr)
{}
static void FixRepeat(char *ptr)
{}
static void FixWhile(char *ptr)
{}
static void FixDo(char *ptr)
{}
static void FixExitIf(char *ptr)
{}
static void FixExit(char *ptr)
{}
static void FixIf(char *ptr)
{}
static void FixElse(char *ptr)
{}
static void FixElseIf(char *ptr)
{}
static void FixOn(char *ptr)
{}
static void FixData(char *ptr)
{}


char *nextToken_littleendian( char *ptr, unsigned short token )
{
	struct nativeCommand *cmd;
	char *ret;

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id )
		{
			ret = ptr;

			switch (token)
			{
				case 0x0000:	break;

				case 0x0006:	FixReference(ptr);
								ret += ReferenceByteLength(ptr);
								break;

				case 0x000c:	FixReference(ptr);
								ret += ReferenceByteLength(ptr);
								break;

				case 0x0012:	FixReference(ptr);
								ret += ReferenceByteLength(ptr);
								break;

				case 0x0018:	FixReference(ptr);
								ret += ReferenceByteLength(ptr);
								break;

				case 0x001E:	FixBin(ptr);	break;

				case 0x0026:	FixQuote(ptr);
								ret += QuoteByteLength(ptr); break;	// skip strings.

				case 0x0036:	FixHex(ptr);	break;
				case 0x0046:	FixFloat(ptr);	break;

				case 0x00b0:	break;
				case 0x00bc:	break;

				case 0x064A:	FixQuote(ptr);
								ret += QuoteByteLength(ptr); break;	// skip strings.

				case 0x0652:	FixQuote(ptr);
								ret += QuoteByteLength(ptr); break;	// skip strings.

				case 0x023C:	FixFor(ptr); break;
				case 0x0246:	break;
				case 0x0250:	FixRepeat(ptr); break;
				case 0x025C:	break;
				case 0x0268:	FixWhile(ptr); break;
				case 0x0274:	break;
				case 0x027E:	FixDo(ptr);	break;
				case 0x0290:	FixExitIf(ptr); break;
				case 0x029E:	FixExit(ptr);	break;
				case 0x02BE:	FixIf(ptr); break;
				case 0x02C6:	break;
				case 0x02D0:	FixElse(ptr); break;
				case 0x25A4:	FixElseIf(ptr); break;
				case 0x02DA:	break;
				case 0x0316:	FixOn(ptr);	break;
				case 0x0376:	FixExtension(ptr);	break;
				case 0x0390: 	break;
				case 0x008C:	break;
				case 0x039E:	break;
				case 0x03AA:	break;
				case 0x0404:	FixData(ptr);	break;

			}

			ret += cmd -> size;
			return ret;
		}
	}

	setError(35,ptr);

	return NULL;
}



char *token_reader_littleendian( char *start, char *ptr, unsigned short lastToken, unsigned short token, char *file_end )
{
	ptr = nextToken_littleendian( ptr, token );

	if ( ptr  >= file_end ) return NULL;

	return ptr;
}

void token_littleendian_fixer( char *start, char *file_end )
{
	char *ptr;
	int token = 0;
	last_tokens[parenthesis_count] = 0;

	ptr = start;
	while (( ptr = token_reader_littleendian(  start, ptr,  last_tokens[parenthesis_count], token, file_end ) ) && ( kittyError.code == 0))
	{
		if (ptr == NULL) break;

		last_tokens[parenthesis_count] = token;
		token = getBigEndienShort(ptr);	// don't swap, but read in as bytes, so works if was executed by acident on big endiene.
		*((short *) ptr) = token; 	// write back fixed token.

		ptr += 2;	// next token.
	}

	nested_count = 0;
}