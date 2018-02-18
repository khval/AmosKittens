
#ifndef __amoskittens_h__
#define __amoskittens_h__

enum
{
	mode_standard,
	mode_alloc,
	mode_input,
	mode_goto,
	mode_logical
};

enum 
{
	state_none = 0,
	state_subData,
	state_hidden_subData 
};

enum
{
	cmd_first = 0,
	cmd_index, 
	cmd_para,
	cmd_loop
};

enum
{
	type_int = 0,
	type_float,
	type_string,
	type_file,
	type_array = 8		// I'm sure AMOS don't use this, but we do.
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
	char *(*cmd) ( struct glueCommands *data );	// can return token location
	char *tokenBuffer;
	int flag;
	int lastVar;
	int step;
	int stack;
};

struct kittyData
{
	union		// we don't need to wast space.
	{
		int len;
		int value;
		int count;
	};
	
	union
	{
		char *str;
		char **str_array;
		int *int_array;
		double *float_array;		
	};

	int index;
	int cells;
	int *sizeTab;

	double decimal;
	int state;
	int type;
};

struct label
{
	char *tokenLocation;
	char *name;
};

struct globalVar
{
	struct kittyData var;
	char *varName;
};

#define stackCmdNormal( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_first;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdStack++; \

#define stackCmdLoop( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_loop;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdStack++; \

#define stackCmdIndex( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_index;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdStack++; \

#define cmdParm( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_para;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdStack++; \

extern void _num( int num );

extern BOOL equal_symbol;
extern struct nativeCommand NativeCommand[];
extern int findNativeCommand(unsigned short lastToken,unsigned short token);
extern BOOL findSymbol(unsigned short token);
extern int commandCnt;

extern struct kittyData kittyStack[];
extern struct glueCommands cmdTmp[];

extern int stack;
extern int cmdStack;
extern unsigned short last_token;

#endif

