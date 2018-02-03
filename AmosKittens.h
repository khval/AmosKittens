
#ifndef __amoskittens_h__
#define __amoskittens_h__

enum 
{
	state_none = 0,
	state_subData
};

struct nativeCommand
{
	int id;
	const char *name;
	int size;
	char *(*fn) (struct nativeCommand *cmd, char *tokenBuffer);
};

struct glueCommands
{
	void (*cmd) ( void );
	char *tokenBuffer;
};

struct kittyString
{
	int len;
	char *str;
	int flag;
};

extern BOOL equal_symbol;
extern struct nativeCommand NativeCommand[];
extern int findNativeCommand(unsigned short lastToken,unsigned short token);
extern BOOL findSymbol(unsigned short token);
extern int commandCnt;

extern struct kittyString strStack[];
extern int numStack[];
extern struct glueCommands cmdTmp[];

extern int stack;
extern int cmdStack;

#endif

