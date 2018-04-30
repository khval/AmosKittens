
#ifndef __amoskittens_h__
#define __amoskittens_h__

#define token_semi	0x0064
#define token_add		0xFFC0
#define token_sub		0xFFCA
#define token_mul		0xFFE2
#define token_div		0xFFEC
#define token_power	0xFFF6
#define token_or		0xFF4C
#define token_and		0xFF58

#define token_more_or_equal	0xFF8E
#define token_less_or_equal		0xFF7A
#define token_less_or_equal2	0xFF84
#define token_more_or_equal2	0xFF98
#define token_not_equal		0xFF66
#define token_equal			0xFFA2
#define token_more			0xFFB6
#define token_less				0xFFAC

#define NEXT_TOKEN(ptr) *((unsigned short *) ptr)
#define NEXT_INT(ptr) *((int *) (ptr+2))

enum
{
	mode_standard,		// 0
	mode_alloc,			// 1
	mode_input,			// 2
	mode_goto,			// 3
	mode_logical,			// 4
	mode_store,			// 5
	mode_for				// 6
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
	cmd_loop,
	cmd_proc,
	cmd_never,
	cmd_eol
};

enum
{
	type_int = 0,
	type_float,
	type_string,
	type_file,
	type_proc,
	type_none,
	type_array = 8	,	// I'm sure AMOS don't use this, but we do.
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
	char *tokenBuffer2;		// a place to store a 2en token buffer pos.

	union
	{
		int flag;				// should remove flag and use cmd_type
		int cmd_type;
	};

	int lastVar;
	int step;		// specal to one command, think this can be union if more specals are needed.
	int stack;
};

struct proc 
{
	char *name;
	int ref;
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
		char *tokenBufferPos;	
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
	int proc;	// so vars can be attached to proc.
	int pass1_shared_to;	// pass1 should only use this, as it will change.
	bool isGlobal;
};

struct defFn
{
	char *name;
	char *fnAddr;
	char *skipAddr;
};

struct kittyBank 
{
	int type;
	void *start;
	int length;
};

struct kittyField
{
	int size;
	int ref;
};

struct lineAddr
{
	char *start;
	char *end;
};

struct kittyFile
{
	FILE *fd;
	int fieldsCount;
	int fieldsSize;
	struct kittyField *fields;
};

#define stackIfSuccess()					\
	cmdTmp[cmdStack].cmd = _ifSuccess;		\
	cmdTmp[cmdStack].tokenBuffer = NULL;	\
	cmdTmp[cmdStack].flag = cmd_never;	\
	cmdStack++; \

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

#define stackCmdProc( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_proc;	\
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

#define stackCmdParm( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_para;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdStack++; \

extern int currentLine;

extern void _num( int num );

extern BOOL equal_symbol;
extern struct nativeCommand NativeCommand[];
extern int findNativeCommand(unsigned short lastToken,unsigned short token);
extern BOOL findSymbol(unsigned short token);
extern int commandCnt;

extern struct kittyData kittyStack[];
extern struct glueCommands cmdTmp[];
extern struct proc procStack[];
extern struct kittyBank kittyBanks[];

extern int stack;
extern int cmdStack;
extern int procStackCount;

extern unsigned short last_token;

extern char *(*jump_mode) (struct reference *ref, char *ptr);
extern char *jump_mode_goto (struct reference *ref, char *ptr);
extern char *jump_mode_gosub (struct reference *ref, char *ptr);

extern char *var_param_str;
extern int var_param_num;
extern double var_param_decimal;

extern char *data_read_pointer;
extern char *_file_start_;
extern char *_file_end_;

extern APTR contextDir;

extern struct kittyFile kittyFiles[10];

extern void (*do_input) ( struct nativeCommand *cmd, char *tokenBuffer );
extern void (*do_breakdata) ( struct nativeCommand *cmd, char *tokenBuffer );

extern struct glueCommands input_cmd_context;

#endif

