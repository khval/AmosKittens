
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

char *nextArg(struct nativeCommand *cmd, char *tokenBuffer);
char *subCalc(struct nativeCommand *cmd, char *tokenBuffer);
char *subCalcEnd(struct nativeCommand *cmd, char *tokenBuffer);
char *breakData(struct nativeCommand *cmd, char *tokenBuffer);
char *setVar(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdInput(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdLineInput(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdIf(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdThen(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdElse(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdEndIf(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdGoto(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdGosub(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdReturn(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdDo(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdLoop(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdRepeat(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdUntil(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdTrue(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdFalse(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdNotEqual(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdWhile(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdWend(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdLess(struct nativeCommand *cmd, char *tokenBuffer);
char *cmdMore(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdEnd(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdFor(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdTo(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdStep(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdNext(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdProcedure(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdProcAndArgs(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdProc(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdEndProc(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdPopProc(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdOn(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdBracket(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdBracketEnd(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdShared(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdGlobal(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdParam(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdParamFloat(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdParamStr(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdRead(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdData(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdRestore(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdExit(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdExitIf(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdEveryOn(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdEveryOff(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdEvery(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdWait(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdTimer(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdBreakOff(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdBreakOn(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdCloseWorkbench(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdCloseEditor(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdElseIf(struct nativeCommand *cmd, char *tokenBuffer );

char *cmdAmosToBack(struct nativeCommand *cmd, char *tokenBuffer );
char *cmdAmosToFront(struct nativeCommand *cmd, char *tokenBuffer );

char *cmdNot(struct nativeCommand *cmd, char *tokenBuffer );

// not used outside of commands.cpp, normally but just for testing.
char *_addStr( struct glueCommands *data );
char *_print( struct glueCommands *data );
char *_gosub( struct glueCommands *data );
char *_procedure( struct glueCommands *data );

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
	char *EndOfProc;	// maybe not used like this in AMOS, but I don't care, does not break anything
	unsigned short seed;
	char flags;
	char seed2;
} __attribute__((packed)) ;

struct rem
{
	unsigned short length;
};

extern bool every_on;
extern int every_timer;
extern char *on_every_gosub_location;
extern char *on_every_proc_location;
extern struct timeval every_before, every_after;

