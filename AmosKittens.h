
#ifndef __amoskittens_h__

#if defined(_MSC_VER) || defined(_linux_)
typedef bool BOOL;
typedef void* APTR;
#endif

#ifdef _MSC_VER
#define PACKED 
#else
#define PACKED __attribute__((packed))
#endif
#define __amoskittens_h__

#define PROC_STACK_SIZE 1000
#define VAR_BUFFERS 1000
#define MAX_PARENTHESIS_COUNT 1000

#define token_newLine	0x0000
#define token_index		0x0074
#define token_semi		0x0064
#define token_comma	0x005C
#define token_nextCmd	0x0054
#define token_add		0xFFC0
#define token_sub		0xFFCA
#define token_mul		0xFFE2
#define token_div		0xFFEC
#define token_power	0xFFF6
#define token_or		0xFF4C
#define token_and		0xFF58
#define token_xor		0xFF3E
#define token_mod		0xFFD4

#define token_parenthesis_start	0x0074
#define token_parenthesis_end	0x007C

#define token_more_or_equal		0xFF8E
#define token_less_or_equal		0xFF7A
#define token_less_or_equal2		0xFF84
#define token_more_or_equal2	0xFF98
#define token_not_equal			0xFF66
#define token_equal			0xFFA2
#define token_more				0xFFB6
#define token_less				0xFFAC

#define token_trap				0x259A

#define token_goto				0x02A8
#define token_gosub			0x02B2
#define token_proc				0x0386

#define joy_up 1
#define joy_down 2
#define joy_left 4
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

#define cmd_normal			0x0001
#define cmd_index			0x0002 
#define cmd_para			0x0004
#define cmd_loop			0x0008
#define cmd_proc			0x0010
#define cmd_onEol			0x0020
#define cmd_onNextCmd		0x0040
#define cmd_onComma		0x0080
#define cmd_onBreak		0x0100
#define cmd_never			0x0200
#define cmd_exit			0x0400
#define cmd_true			0x0800
#define cmd_false			0x1000

enum
{
	type_undefined = 0,
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

enum 
{
	glue_option_for_int = 1,
	glue_option_for_float
};


struct kittyForInt
{
	int step;	
	int have_to;
};

struct kittyForDouble 
{
	double step;	
	double have_to;
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
	int token;

	int optionsType;
	struct kittyForInt optionsInt;
	struct kittyForDouble optionsFloat;

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
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
	unsigned char ext;
	unsigned char	__align__;
	unsigned short token;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
} PACKED;

struct extension_lib
{
	struct Library *base;
#ifdef amigaos4
	struct Interface *interface;
#endif
	char	*lookup;
	uint32_t crc;
};

struct dataBase
{
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
	uint16_t type;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
} PACKED;

struct stringData : dataBase
{
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
	uint16_t size;
	char ptr;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
} PACKED;

struct desimalData :  dataBase
{
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
	double value;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
} PACKED;

struct valueData : dataBase
{
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
	int value;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
} PACKED;

struct stringArrayData : dataBase
{
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
	uint16_t size;
	struct stringData *ptr;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
} PACKED;

struct desimalArrayData : dataBase
{
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
	uint16_t size;
	struct desimalData ptr;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
} PACKED;

struct valueArrayData : dataBase
{
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
	uint16_t size;
	struct valueData ptr;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
} PACKED;

struct kittyData
{
	int count;
	
	union
	{
		struct stringData *str;
		struct valueArrayData *int_array;
		struct desimalArrayData *float_array;
		struct stringArrayData *str_array;
		char *tokenBufferPos;
	};

	union		// we don't need to wast space.
	{
		int index;
		int prev_last_token;
	};

	int cells;

	union
	{
		int *sizeTab;
		char *procDataPointer;
	};

	struct valueData integer;
	struct desimalData decimal;
	int state;
	int type;
};

struct kittyVideoInfo
{
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
	uint16_t videoWidth;
	uint16_t videoHeight;
	uint16_t display_x;
	uint16_t display_y; 
#ifdef _MSC_VER
#pragma pack(pop)
#endif
} PACKED ; 

struct kittyInfo		// where amos programs look for info about the editor.
{
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
	struct kittyVideoInfo *video;
	uint32_t dummy[6];
	uint16_t rgb[8];
#ifdef _MSC_VER
#pragma pack(pop)
#endif
} PACKED ;

struct label
{
	int proc;
	char *loopLocation;
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

struct stackFrame
{
	int id;
//	struct kittyData *var;
	char *dataPointer;
};

struct defFn
{
	char *name;
	char *fnAddr;
	char *skipAddr;
};

struct kittyBank 
{
	int id;
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
	unsigned int file;
	unsigned int lineNumber;
	unsigned int srcStart;
	unsigned int srcEnd;
	unsigned int start;
	unsigned int end;
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

struct sampleHeader
{
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
	char		name[8];
	uint16_t	frequency;
	uint32_t	bytes;
	uint8_t	ptr;
#ifdef _MSC_VER
#pragma pack(pop)
#endif
} PACKED ;

struct envel
{
	int startDuration;
	int duration;
	int volume;
};

struct wave
{
	int id;
	int	bytesPerSecond;	// bytesPerSecond
	struct envel envels[7];
	struct sampleHeader sample;	// this one most be last
};

struct kittyDevice
{
	int id;
	struct MsgPort *port;
	struct IORequest *io;
	bool sendt;
	int error;
};

struct kittyLib
{
	int id;
	struct Library *base;
};

#define stackIfSuccess()					\
	cmdTmp[cmdStack].cmd = _ifSuccess;		\
	cmdTmp[cmdStack].tokenBuffer = NULL;	\
	cmdTmp[cmdStack].flag = cmd_never | cmd_true;	\
	cmdStack++; \

#define stackIfNotSuccess()					\
	cmdTmp[cmdStack].cmd = _ifNotSuccess;		\
	cmdTmp[cmdStack].tokenBuffer = NULL;	\
	cmdTmp[cmdStack].flag = cmd_never | cmd_false;	\
	cmdStack++; \

#define stackCmdNormal( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_normal | cmd_onNextCmd | cmd_onEol;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdTmp[cmdStack].token = 0; \
	cmdTmp[cmdStack].parenthesis_count =parenthesis_count; \
	cmdStack++; \
	token_is_fresh = false; 

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

#define stackCmdFlags( fn, buf, flags )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = flags;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdStack++; \

#define stackCmdIndex( fn, buf )	{			\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_index ;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdTmp[cmdStack].token = token_index ; \
	cmdTmp[cmdStack].parenthesis_count =parenthesis_count; \
	cmdStack++; } \

#define stackCmdParm( fn, buf )				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_para | cmd_onComma | cmd_onNextCmd | cmd_onEol;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdTmp[cmdStack].token = 0; \
	cmdTmp[cmdStack].parenthesis_count =parenthesis_count; \
	cmdStack++; \
	token_is_fresh = false; 

#define stackCmdMathOperator(fn,_buffer,_token)				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = _buffer;	\
	cmdTmp[cmdStack].flag = cmd_para | cmd_onComma | cmd_onNextCmd | cmd_onEol; \
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdTmp[cmdStack].token = _token; \
	cmdTmp[cmdStack].parenthesis_count =parenthesis_count; \
	cmdStack++; \

#define stackCmdOnBreakOrNewCmd(fn,buf,_token)				\
	cmdTmp[cmdStack].cmd = fn;		\
	cmdTmp[cmdStack].tokenBuffer = buf;	\
	cmdTmp[cmdStack].flag = cmd_para | cmd_onBreak | cmd_onComma | cmd_onNextCmd | cmd_onEol;	\
	cmdTmp[cmdStack].lastVar = last_var;	\
	cmdTmp[cmdStack].stack = stack; \
	cmdTmp[cmdStack].token = _token; \
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

extern struct extension_lib	kitty_extensions[32];

extern int stack;
extern int cmdStack;
extern int procStackCount;

//extern unsigned short last_tokens[MAX_PARENTHESIS_COUNT];

extern char *(*jump_mode) (struct reference *ref, char *ptr);
extern char *jump_mode_goto (struct reference *ref, char *ptr);
extern char *jump_mode_gosub (struct reference *ref, char *ptr);

extern struct stringData *var_param_str;
extern int var_param_num;
extern double var_param_decimal;

extern int proc_stack_frame;

extern struct stackFrame procStcakFrame[PROC_STACK_SIZE];

extern char *_file_start_;
extern char *_file_end_;

extern APTR contextDir;

extern struct kittyFile kittyFiles[10];

extern void do_std_next_arg(nativeCommand *cmd, char *ptr);
extern char *do_to_default( struct nativeCommand *cmd, char *tokenbuffer );

extern void (**do_input) ( struct nativeCommand *cmd, char *tokenBuffer );
extern char *(**do_to) ( struct nativeCommand *cmd, char *tokenBuffer );

extern void (*do_breakdata) ( struct nativeCommand *cmd, char *tokenBuffer );
extern bool token_is_fresh;
extern struct glueCommands input_cmd_context;

#endif

