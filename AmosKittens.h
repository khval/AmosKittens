
#ifndef __amoskittens_h__
#define __amoskittens_h__

#define PROC_STACK_SIZE 1000
#define VAR_BUFFERS 1000
#define MAX_PARENTHESIS_COUNT 1000

#define token_semi	0x0064
#define token_add		0xFFC0
#define token_sub		0xFFCA
#define token_mul		0xFFE2
#define token_div		0xFFEC
#define token_power	0xFFF6
#define token_or		0xFF4C
#define token_and		0xFF58
#define token_mod		0xFFD4

#define token_more_or_equal	0xFF8E
#define token_less_or_equal		0xFF7A
#define token_less_or_equal2	0xFF84
#define token_more_or_equal2	0xFF98
#define token_not_equal		0xFF66
#define token_equal			0xFFA2
#define token_more			0xFFB6
#define token_less				0xFFAC

#define joy_up 1
#define joy_left 2
#define joy_down 4
#define joy_right 8

extern unsigned int amiga_joystick_dir[4];
extern unsigned int amiga_joystick_button[4];

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

#define cmd_normal		1
#define cmd_index			2 
#define cmd_para			4
#define cmd_loop			8
#define cmd_proc			16
#define cmd_onEol			32
#define cmd_onNextCmd	64
#define cmd_onComma		128
#define cmd_onBreak		256
#define cmd_never			512

enum
{
	type_int = 0,
	type_float,		// 1
	type_string,		// 2
	type_file,			// 3
	type_proc,		// 4
	type_none,		// 5
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
	char *(*cmd) ( struct glueCommands *data, int nextToken );	// can return token location
	char *tokenBuffer;
	
	union
	{
		char *tokenBuffer2;		// a place to store a 2en token buffer pos.
		char *FOR_NUM_TOKENBUFFER;
	};

	union
	{
		int flag;				// should remove flag and use cmd_type
		int cmd_type;
	};

	int lastVar;
	int lastToken;

	union
	{
		int step;	
		int have_to;
	};

	int stack;
	int parenthesis_count;
};

struct proc 
{
	char *name;
	int ref;
};

struct extension 
{
	unsigned char ext;
	unsigned char	__align__;
	unsigned short token;
} __attribute__((packed)) ;

struct extension_lib
{
	struct Library *base;
#ifdef amigaos4
	struct Interface *interface;
#endif
	char	*lookup;
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

	union
	{
		int *sizeTab;
		char *procDataPointer;
	};

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
	char *start;
	char *object_ptr;
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

struct zone
{
	int screen;
	int x0;
	int y0;
	int x1;
	int y1;
};


#define stackIfSuccess()					\
	cmdTmp[cmdStack].cmd = _ifSuccess;		\
	cmdTmp[cmdStack].tokenBuffer = NULL;	\
	cmdTmp[cmdStack].flag = cmd_never;	\
	cmdStack++; \

#define stackIfNotSuccess()					\
	cmdTmp[cmdStack].cmd = _ifNotSuccess;		\
	cmdTmp[cmdStack].tokenBuffer = NULL;	\
	cmdTmp[cmdStack].flag = cmd_never;	\
	cmdStack++; \

#define stackCmdNormal( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_normal;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdTmp[cmdStack].parenthesis_count =parenthesis_count; \
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

#define stackCmdIndex( fn, buf )	{			\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_index;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdTmp[cmdStack].lastToken = last_tokens[parenthesis_count]; \
	cmdTmp[cmdStack].parenthesis_count =parenthesis_count; \
	cmdStack++; } \

#define stackCmdParm( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_para;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdTmp[cmdStack].parenthesis_count =parenthesis_count; \
	cmdStack++; \

#define stackCmdOnBreakOrNewCmd( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_onBreak | cmd_onComma | cmd_onNextCmd | cmd_onEol;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdTmp[cmdStack].parenthesis_count =parenthesis_count; \
	cmdStack++; \


extern struct zone *zones;
extern int zones_allocated;

extern int currentLine;
extern int parenthesis_count;

extern BOOL equal_symbol;
extern struct nativeCommand NativeCommand[];
extern int findNativeCommand(unsigned short lastToken,unsigned short token);
extern BOOL findSymbol(unsigned short token);
extern int commandCnt;

extern struct kittyData kittyStack[];
extern struct glueCommands cmdTmp[];
extern struct proc procStack[];
extern struct kittyBank kittyBanks[];

extern struct extension_lib	kitty_extensions[32];

extern int stack;
extern int cmdStack;
extern int procStackCount;

extern unsigned short last_tokens[MAX_PARENTHESIS_COUNT];

extern char *(*jump_mode) (struct reference *ref, char *ptr);
extern char *jump_mode_goto (struct reference *ref, char *ptr);
extern char *jump_mode_gosub (struct reference *ref, char *ptr);

extern char *var_param_str;
extern int var_param_num;
extern double var_param_decimal;

extern int proc_stack_frame;
extern char *data_read_pointers[PROC_STACK_SIZE];
extern char *_file_start_;
extern char *_file_end_;

extern APTR contextDir;

extern struct kittyFile kittyFiles[10];

extern void do_std_next_arg(nativeCommand *cmd, char *ptr);
extern char *do_to_default( struct nativeCommand *cmd, char *tokenbuffer );

extern void (**do_input) ( struct nativeCommand *cmd, char *tokenBuffer );
extern void (*do_breakdata) ( struct nativeCommand *cmd, char *tokenBuffer );
extern char *(*do_to) ( struct nativeCommand *cmd, char *tokenBuffer );

extern struct glueCommands input_cmd_context;

#endif

