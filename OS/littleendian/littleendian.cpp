


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

static void FixQuote(unsigned char *ptr)
{
	*((unsigned short *) ptr) = getBigEndienShort(ptr);
}

static void FixReference(unsigned char *ptr)
{
	struct reference *ref = (struct reference *) ptr;
	unsigned char *tptr = (unsigned char *) &ref -> ref;
	ref -> ref = getBigEndienShort(tptr);
}

static void FixExtension(unsigned char *ptr)
{
	struct extensionCommand *ext = (struct extensionCommand *) ptr;
	unsigned char *tptr = (unsigned char *) &ext -> ExtentionTokenTable;
	ext -> ExtentionTokenTable = getBigEndienShort(tptr);
}

static void FixBin(unsigned char *ptr)
{}
static void FixHex(unsigned char *ptr)
{}
static void FixFloat(unsigned char *ptr)
{}
static void FixFor(unsigned char *ptr)
{}
static void FixRepeat(unsigned char *ptr)
{}
static void FixWhile(unsigned char *ptr)
{}
static void FixDo(unsigned char *ptr)
{}
static void FixExitIf(unsigned char *ptr)
{}
static void FixExit(unsigned char *ptr)
{}
static void FixIf(unsigned char *ptr)
{}
static void FixElse(unsigned char *ptr)
{}
static void FixElseIf(unsigned char *ptr)
{}
static void FixOn(unsigned char *ptr)
{}
static void FixData(unsigned char *ptr)
{}


static unsigned int line_number = 0;

unsigned char *nextToken_littleendian( unsigned char *ptr, unsigned short token, unsigned char *file_end )
{
	struct nativeCommand *cmd;
	unsigned char *ret;
	bool fixed;

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id )
		{
			ret = ptr;

			fixed = true;

			switch (token)
			{
				case 0x0000:	line_number++;
								break;

				case 0x0006:	FixReference(ptr);
								ret += ReferenceByteLength( (char *) ptr);
								break;

				case 0x000c:	FixReference(ptr);
								ret += ReferenceByteLength( (char *) ptr);
								break;

				case 0x0012:	FixReference(ptr);
								ret += ReferenceByteLength( (char *) ptr);
								break;

				case 0x0018:	FixReference(ptr);
								ret += ReferenceByteLength( (char *) ptr);
								break;

//				case 0x001E:	FixBin(ptr);	break;

				case 0x0026:	FixQuote(ptr);
								ret += QuoteByteLength( (char *) ptr); break;	// skip strings.

/*
				case 0x0036:	FixHex(ptr);	break;
				case 0x0046:	FixFloat(ptr);	break;

				case 0x00b0:	break;
				case 0x00bc:	break;
*/
				case 0x064A:	FixQuote(ptr);
								ret += QuoteByteLength( (char *) ptr); break;	// skip strings.

				case 0x0652:	FixQuote(ptr);
								ret += QuoteByteLength( (char *) ptr); break;	// skip strings.

/*
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
	*/
				default: fixed == false;
			}

			if ((cmd->size) && (fixed == false))
			{
				printf("warning token: %04x not converted\n", token);
			}

			printf("found token %04x - name %s\n",token,cmd -> name);


			ret += cmd -> size;
			return ret;
		}
	}

	if (ptr<file_end)
	{
		printf("token %04x not found, at line_number %d (last token before %04x)\n", token, line_number, last_tokens[parenthesis_count] );
	
		printf("ptr at %08x file end at %08x\n",ptr,file_end);
		setError(35,(char *) ptr);
	}

	return NULL;
}



unsigned char *token_reader_littleendian( char *start, unsigned char *ptr, unsigned short lastToken, unsigned short token, unsigned char *file_end )
{
	ptr = nextToken_littleendian( ptr, token, file_end );

	if ( ptr  >= file_end ) return NULL;

	return ptr;
}

void token_littleendian_fixer( char *start, char *file_end )
{
	unsigned char *ptr;
	int token = 0;
	last_tokens[parenthesis_count] = 0;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	ptr = (unsigned char *) start;
	while (( ptr = token_reader_littleendian(  start, ptr,  last_tokens[parenthesis_count], token, (unsigned char *) file_end ) ) && ( kittyError.code == 0))
	{
		if (ptr == NULL) break;

		last_tokens[parenthesis_count] = token;
		token = getBigEndienShort(ptr);	// don't swap, but read in as bytes, so works if was executed by acident on big endiene.
		*((short *) ptr) = token; 	// write back fixed token.

		ptr += 2;	// next token.
	}

	nested_count = 0;
}