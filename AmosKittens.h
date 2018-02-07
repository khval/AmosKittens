
#ifndef __amoskittens_h__
#define __amoskittens_h__

enum 
{
	state_none = 0,
	state_subData,
	state_hidden_subData 
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
	void (*cmd) ( struct glueCommands *data );
	char *tokenBuffer;
	int flag;
	int lastVar;
};

struct kittyData
{
	union		// we don't need to wast space.
	{
		int len;
		int value;
	};
	
	char *str;
	double decimal;
	int state;
	int type;
};

struct globalVar
{
	struct kittyData var;
	char *varName;
};

#define cmdNormal( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_first;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdStack++; \

#define cmdParm( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_para;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdStack++; \


extern BOOL equal_symbol;
extern struct nativeCommand NativeCommand[];
extern int findNativeCommand(unsigned short lastToken,unsigned short token);
extern BOOL findSymbol(unsigned short token);
extern int commandCnt;

extern struct kittyData kittyStack[];
extern int numStack[];
extern struct glueCommands cmdTmp[];

extern int stack;
extern int cmdStack;

#endif

