
#ifndef __amoskittens_h__
#define __amoskittens_h__

enum 
{
	state_none = 0,
	state_subData
};

enum
{
	cmd_first = 0,
	cmd_para
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
	int flag;
};

struct kittyString
{
	int len;
	char *str;
	int flag;
};

#define cmdNormal( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_first;	\
	cmdStack++; \

#define cmdParm( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_para;	\
	cmdStack++; \


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

