
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

char *nextArg(struct nativeCommand *cmd, char *tokenBuffer);
char *addData(struct nativeCommand *cmd, char *tokenBuffer);
char *subCalc(struct nativeCommand *cmd, char *tokenBuffer);
char *subCalcEnd(struct nativeCommand *cmd, char *tokenBuffer);
char *addData(struct nativeCommand *cmd, char *tokenBuffer);
char *subData(struct nativeCommand *cmd, char *tokenBuffer);
char *breakData(struct nativeCommand *cmd, char *tokenBuffer);
char *setVar(struct nativeCommand *cmd, char *tokenBuffer);
char *mulData(struct nativeCommand *cmd, char *tokenBuffer);
char *divData(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdInput(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdIf(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdThen(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdElse(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdEndIf(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdGoto(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdDo(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdLoop(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdRepeat(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdUntil(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdTrue(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdFalse(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdNotEqual(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdWhile(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdWend(struct nativeCommand *cmd, char *tokenBuffer );

// not used outside of commands.cpp, normally but just for testing.
char *_addStr( struct glueCommands *data );
char *_print( struct glueCommands *data );


// structs are used read chunks of the AMOS file, so they need to be packed.

struct tokenStart {
	char length;
	char level;
} __attribute__((packed)) ;

struct extensionCommand
{
	unsigned char extention_number;
	unsigned char alignment16bit;			// maybe length
	unsigned short ExtentionTokenTable;
} __attribute__((packed)) ;

struct reference
{
	short ref;
	char	length;
	char flags;
} __attribute__((packed)) ;

struct procedure
{
	unsigned int ProcLength;
	unsigned short seed;
	char flags;
	char seed2;
} __attribute__((packed)) ;

struct rem
{
	unsigned short length;
};

