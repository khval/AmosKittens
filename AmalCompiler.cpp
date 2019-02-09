
#include "stdafx.h"

#ifdef _MSC_VER
#include <string.h>
#include "vs_missing_string_functions.h"
#define strdup _strdup
#define Printf printf
#endif

#include "debug.h"

#if defined(__amigaos4__) || defined(__amigaos)
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#endif

#ifdef __linux__
#include <string.h>
#include <stdint.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#define Printf printf
#ifdef test_app
#define engine_fd stdout
#endif
#endif



#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "AmalCompiler.h"
#include "channel.h"
#include "AmalCommands.h"
#include "pass1.h"
#include "AmosKittens.h"

bool let = false;
bool next_arg = false;

int nest = 0;
char last_reg[1000] ;


struct AmalLabelRef
{
	unsigned int pos;
	char *name;
};

std::vector<void **> amalloops;
static std::vector<struct AmalLabelRef> looking_for_labels;
static std::vector<struct AmalLabelRef> found_labels;

void reAllocAmalBuf( struct amalBuf *i, int e );
void dump_amal_labels();

void *amalAllocBuffer( int size ) 
{
	return sys_public_alloc_clear(size);
}

#define amalFreeBuffer( ptr ) { sys_free( ptr ); ptr = NULL; }

#ifdef test_app
	struct nested nested_command[ max_nested_commands ];
	int nested_count = 0;
	int parenthesis_count;
	int amreg[26];
	void print_code( void **adr );
#else
	extern int amreg[26];
#endif

void dumpAmalRegs(struct kittyChannel *channel)
{
	char a[2]={0,0};
	char b[2]={0,0};

	int i;
	for (i=0;i<26;i++) 
	{
		a[0]='A'+i;
		b[0]='0'+i;

		if (i<10)
		{
			Printf_iso("R%s is %3d    R%s is %3d\n",a, amreg[i], b, channel -> reg[i] );
		}
		else	Printf_iso("R%s is %3d\n",a, amreg[i]);
	}
}


void *(*amal_cmd_equal) API_AMAL_CALL_ARGS = NULL;

unsigned int (*amal_to_writer) ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num) = NULL;


void dumpAmalStack( struct kittyChannel *channel )
{
	unsigned int s;
	Printf("Amal Stack\n");
	for (s=0;s<=channel -> argStackCount;s++) 
	{
		Printf_iso("stack %d: value %d\n",s, channel -> argStack[s] );
	}

	if (channel -> argStackCount>50)
	{
		Printf("Amal sick puppy :-(, stack is growing too quickly... \n");

		if (channel -> amal_script)
		{
			Printf_iso("script '%s'\n", channel -> amal_script);
		}
	}
}

void dumpAmalProgStack( struct kittyChannel *channel )
{
	unsigned int s;
	Printf("Amal Stack\n");
	for (s=0;s<=channel -> progStackCount;s++) 
	{
		AmalPrintf("stack %d: flags %x\n",s, channel -> progStack[s].Flags );
	}
}

void pushBackAmalCmd( amal::Flags flags, void **code, struct kittyChannel *channel, void *(*cmd)  (struct kittyChannel *self, struct amalCallBack *cb)  ) 
{
	if (channel -> progStack)
	{
		struct amalCallBack *CallBack = &channel -> progStack[ channel -> progStackCount ];
		if (CallBack)
		{
			CallBack -> Flags = flags;
			CallBack -> cmd = cmd;
			CallBack -> argStackCount = channel -> argStackCount;
			CallBack -> progStackCount = channel -> progStackCount;
			CallBack -> last_reg = channel -> last_reg;
			CallBack -> code = code;
			channel -> progStackCount ++;
			return;
		}
	}
}

unsigned int AmalWriterIf (	struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	call_array[0] = self -> call;

	printf("Writing %-8d to %08x/%08x - If\n",num, (unsigned int) &call_array[0], (unsigned int) &call_array[1]);

	*((int *) &call_array[1]) = 0;
	addNestAmal( if, &call_array[1] );
	amal_cmd_equal = amal_call_equal;

	next_arg =true;

	getchar();

	return 2;
}

// DO NOT ADD TO TABLE.
unsigned int AmalWriterCondition (	struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	call_array[0] = self -> call;

	printf("Writing %-8d to %08x/%08x - Condition\n", 0, (unsigned int) &call_array[0], (unsigned int) &call_array[1]);

	*((int *) &call_array[1]) = 0;

	// modify the nest.
	nested_command[ nested_count -1 ].cmd = num;
	nested_command[ nested_count -1 ].ptr = (char *) &call_array[1];

	let =false;

	return 2;
}

unsigned int AmalWriterNum (	struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	call_array[0] = self -> call;

	printf("Writing %-8d to %08x/%08x - num\n",num, (unsigned int) &call_array[0], (unsigned int) &call_array[1]);

	*((int *) &call_array[1]) = num;
	let = false;
	next_arg = false;
	return 2;
}

int animScriptLength( const char *str, const char *valid_chars )
{
	const char *c;
	const char *vc;
	bool valid;
	int l = 0;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("str: %s\n", str);

	for (c = str ; (*c!=0) && (*c!=';') ; c++)
	{
		valid = false;
		for (vc=valid_chars; *vc ; vc++) if (*vc == *c) { valid = true; break; }
		if (valid == false) break;		
		l ++;
	}

	return l;
}


int amalStringLength( const char *str )
{
	const char *c;
	int l = 0;
	for (c = str ; (*c==0) | (*c==';') ; c++) l ++;
	return l;
}

const char *AmalAtStringArg( const char  *at_script )
{
	const char *s = at_script;
	while ((*s != 0)&&(*s != ' ')) s++;
	while (*s == ' ') s++;
	return s;
}

int writeAmalStringToBuffer( const char *s, char *d, int _max_len )
{
	int le = 0;
	while ((*s != 0)&&(*s != ' ')&&( *s != ';')&&(le < _max_len)) { *d++=*s++; le++; }
	*d = 0;
	return le;
}

unsigned int stdAmalWriterScript (	struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	const char *s;
	int anim_script_len;
	unsigned int size;
	int le;
	unsigned int offset;

	call_array[0] = self -> call;

	struct amalBuf *amalProg = &channel -> amalProg;

	printf("Writing %-8d to %08x/%08x - script\n",num,(unsigned int) &call_array[0],(unsigned int) &call_array[1]);

// find script find the size

	s = AmalAtStringArg( data -> at_script );
	anim_script_len =animScriptLength( s, "R,0123456789() " );
	size = 2 + ((anim_script_len + sizeof(void *)) / sizeof(void *) );

// check if size is ok, or need a new buffer.

	if (data -> pos > amalProg -> elements - size )	// larger writer, takes max 6 elements.
	{
		reAllocAmalBuf(amalProg,size + 20);	// add another 20 elements if buffer is low.

		// now that call array is new, need to reset it.
		call_array = &amalProg -> call_array[data -> pos];
	}

// write the script into buffer.

	le = writeAmalStringToBuffer( s, (char *) (&call_array[2]) , anim_script_len );
	data -> arg_len = le ? le+1 : 0;

	offset = ((le + sizeof(void *)) / sizeof(void *) );
	*((int *) &call_array[1]) = offset ;
	return 2 + offset;
}

void amal_clean_up_labels( )
{
	struct AmalLabelRef label;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	while ( ! found_labels.empty() )
	{
		label = found_labels.back();

		if (label.name)
		{
			free(label.name);
			label.name = NULL;
		}

		found_labels.pop_back();
	}

	while ( ! looking_for_labels.empty() )
	{
		label = looking_for_labels.back();

		if (label.name)
		{
			free(label.name);
			label.name = NULL;
		}

		looking_for_labels.pop_back();
	}
}


unsigned int stdAmalWriterJump (	struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	char labelname[20];
	int le = 0;
	const char *s;
	char *d;

	printf("writing %08x to %08x - jump\n", (unsigned int) self -> call,(unsigned int) &call_array[0]);

	call_array[0] = self -> call;
	s = data -> at_script + data -> command_len;

	while (*s == ' ') 
	{
		s++; le++;
	}

	d = labelname;
	while ((*s != 0) && (*s != ' ') && (*s != ';') && (*s != ':') && (le < 18)) { printf("data: %c\n", *s); *d++ = *s++; le++; }
	*d = 0;

	if ((*s == ';') || (*s ==':')) le++;	// if next command is ; we can skip it.

	data -> arg_len = le ;
	{
		struct AmalLabelRef label;
		char *current_location;
		char *start_location;

		current_location = (char *) &call_array[0];
		start_location = (char *) channel -> amalProg.call_array;

		label.pos 	= (int) (current_location - start_location) / sizeof(void *);
		label.name = strdup( labelname );
		looking_for_labels.push_back( label );
	}

	*((int *) &call_array[1]) = 0;
	return 2;
}


unsigned int stdAmalWriterIgnore ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	printf("data ignored\n");
	return 0;
}

unsigned int stdAmalWriterLet ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	let = true;
	return 0;
}

unsigned int stdAmalWriterX ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	printf("writing %08x to %08x - X\n", (unsigned int) amal_call_x,(unsigned int) &call_array[0]);
	call_array[0] = amal_call_x;

	return 1;
}

unsigned int stdAmalWriterExit( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	printf("writing %08x to %08x - exit\n", (unsigned int) amal_call_exit, (unsigned int) &call_array[0]);
	call_array[0] = amal_call_exit;

	return 1;
}


void fix_condition_branch( void *adr )
{
	void **ptr = (void **) nested_command[ nested_count-1 ].ptr;
	*ptr = adr;
}

unsigned int stdAmalWriterNextCmd ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	printf("writing %08x to %08x - ;\n", (unsigned int) self -> call, (unsigned int) &call_array[0]);

	// this command doubles as "end if"

	switch (GET_LAST_NEST)
	{
		case nested_if:
		case nested_then:
		case nested_else:
			fix_condition_branch( (void *) &call_array[0] );
			nested_count --;
			break;
	}

	call_array[0] = self -> call;
	amal_cmd_equal = NULL;
	let = false;
	return 1;
}

unsigned int stdAmalWriter ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	printf("writing %08x to %08x - %s\n", (unsigned int) self -> call, (unsigned int) &call_array[0], self->name);
	call_array[0] = self -> call;
	return 1;
}

unsigned int stdAmalWriterSymbol ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	printf("writing %08x to %08x - %s\n", (unsigned int) self -> call, (unsigned int) &call_array[0], self->name);
	call_array[0] = self -> call;
	next_arg = true;
	return 1;
}


unsigned int stdAmalWriterEqual ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	if (amal_cmd_equal)
	{
		printf("writing %08x to %08x ==\n", (unsigned int) amal_cmd_equal, (unsigned int) &call_array[0]);
		call_array[0] = amal_cmd_equal;
	}
	else
	{
		printf("writing %08x to %08x =\n", (unsigned int) self -> call,(unsigned int)  &call_array[0]);
		call_array[0] = self -> call;
		amal_cmd_equal = amal_call_equal;
	}

	return 1;
}

unsigned int amal_for_to ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
		printf("writing [code block] to %08x - for \n", (unsigned int) &call_array[0]);
		char *current_location = (char *) (&call_array[1]);
		char *start_location = (char *) channel -> amalProg.call_array;

		call_array[0] = amal_call_next_cmd;

		call_array[1] = amal_call_while;
		call_array[2] = NULL;
		amalloops.push_back( (void **) (current_location - start_location) );

		printf("start location:    %08x\n",(unsigned int) start_location);
		printf("current location: %08x\n",(unsigned int) current_location);

		call_array[3] = amal_call_reg;
		*((int *) &call_array[4]) = last_reg[nest];

		call_array[5] = amal_call_less_or_equal;
		amal_to_writer = NULL;
		nest++;
		return 6;
}

unsigned int stdAmalWriterFor (  struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	amal_to_writer = amal_for_to;
	return 0;
}

unsigned int stdAmalWriterTo (  struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	unsigned int ret = 0;

	if (amal_to_writer) 
	{
		ret = amal_to_writer( channel, self, call_array, data, num );
		amal_to_writer = NULL;
	}

	return ret;
}

unsigned int stdAmalWriterWend (  struct kittyChannel *channel,struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	nest--;

	call_array[0] = amal_call_inc;
	call_array[1] = amal_call_reg;
	*((int *) &call_array[2]) = last_reg[nest];
	call_array[3] = amal_call_next_cmd;
	call_array[4] = amal_call_wend;

	if (!amalloops.empty())
	{
		int elements;
		void **code;
		int rel_location = (int) amalloops.back() ;
		char *current_location;
		char *start_location;

		current_location = (char *) &call_array[4];
		start_location = (char *) channel -> amalProg.call_array;
		code = (void **)  (start_location + rel_location);
		elements = (int) (current_location - ((char*) code)) / sizeof(void *);

		code[1] = (void *) (current_location - start_location) ;

		*((void **) &call_array[5]) = (void **) rel_location;
		amalloops.pop_back();
	}

	return 6;
}

unsigned int stdAmalWriterReg (  struct kittyChannel *channel,struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int )
{
	int num = *(data -> at_script + 1);
	printf("writing %08x to %08x\n",(unsigned int) amal_call_reg,(unsigned int) &call_array[0]);
	call_array[0] = amal_call_reg;
	*((int *) &call_array[1]) = num;

	last_reg[nest] =num;

	return 2;
}

struct amalTab amalSymbols[] =
{
	{";",amal::class_cmd_normal,stdAmalWriterNextCmd,amal_call_next_cmd },
	{"(",amal::class_cmd_arg,stdAmalWriterSymbol,amal_call_parenthses_start },
	{")",amal::class_cmd_arg,stdAmalWriterSymbol,amal_call_parenthses_end },
	{",",amal::class_cmd_arg,stdAmalWriterSymbol,amal_call_nextArg },
	{"+",amal::class_cmd_arg,stdAmalWriterSymbol,amal_call_add},			// +
	{"-",amal::class_cmd_arg,stdAmalWriterSymbol,amal_call_sub},			// -
	{"*",amal::class_cmd_arg,stdAmalWriterSymbol,amal_call_mul},			// *
	{"/",amal::class_cmd_arg,stdAmalWriterSymbol,amal_call_div},			// /
	{"&",amal::class_cmd_arg,stdAmalWriterSymbol,amal_call_and},			// &
	{"<>",amal::class_cmd_arg,stdAmalWriterSymbol,amal_call_not_equal},	// <>
	{"<",amal::class_cmd_arg,stdAmalWriterSymbol,amal_call_less},			// <
	{">",amal::class_cmd_arg,stdAmalWriterSymbol,amal_call_more},			// >
	{"=",amal::class_cmd_arg,stdAmalWriterEqual,amal_call_set},		// =
	{NULL, amal::class_cmd_arg,NULL,NULL }
};

struct amalTab amalCmds[] =
{
	{"@{never used}",amal::class_cmd_arg,stdAmalWriter,NULL},
	{"@{number}",amal::class_cmd_arg,AmalWriterNum,amal_set_num},	//number token, reserved.
	{"O",amal::class_cmd_normal,stdAmalWriter,amal_call_on},		// On
	{"D",amal::class_cmd_normal,stdAmalWriter,NULL},				// Direct
	{"W",amal::class_cmd_normal,stdAmalWriter,NULL},				// Wait
	{"I",amal::class_cmd_normal,AmalWriterIf,amal_call_if},			 // If

	{"X",amal::class_cmd_arg,stdAmalWriterX,NULL},		// X
	{"X",amal::class_cmd_normal,stdAmalWriterExit,NULL},		// X

	{"Y",amal::class_cmd_normal,stdAmalWriter,amal_call_y},			// Y
	{"L",amal::class_cmd_normal,stdAmalWriterLet,NULL},			// Let
	{"AU",amal::class_cmd_normal,stdAmalWriter,NULL},				// AUtotest
	{"A",amal::class_cmd_normal,stdAmalWriterScript,amal_call_anim},	// Anim
	{"M",amal::class_cmd_normal,stdAmalWriter,amal_call_move},		// Move
	{"P",amal::class_cmd_normal,stdAmalWriter,amal_call_pause},		// Pause

	{"R0",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R0
	{"R1",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R1
	{"R2",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R2
	{"R3",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R3
	{"R4",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R4
	{"R5",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R5
	{"R6",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R6
	{"R7",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R7
	{"R8",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R8
	{"R9",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// R9
	{"RA",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RA
	{"RB",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RB
	{"RC",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RC
	{"RD",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RD
	{"RE",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RE
	{"RF",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RF
	{"RG",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RG
	{"RH",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RH
	{"RI",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RI
	{"RJ",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RJ
	{"RK",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RK
	{"RL",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RL
	{"RM",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RM
	{"RN",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RN
	{"RO",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RO
	{"RP",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RP
	{"RQ",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RQ
	{"RR",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RR
	{"RS",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RS
	{"RT",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RT
	{"RU",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RU
	{"RV",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RV
	{"RW",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RW
	{"RX",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RX
	{"RY",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RY
	{"RZ",amal::class_cmd_arg,stdAmalWriterReg,NULL },	// RZ

	{"F",amal::class_cmd_normal,stdAmalWriterFor,NULL},			// For (should be null)
	{"T",amal::class_cmd_arg,stdAmalWriterTo,amal_call_nextArg},		// To
	{"N",amal::class_cmd_normal,stdAmalWriterWend,amal_call_wend},	// Next (should be null)
	{"PL",amal::class_cmd_normal,stdAmalWriter,NULL},				// Play
	{"E",amal::class_cmd_normal,stdAmalWriter,NULL},				// End
	{"XM",amal::class_cmd_arg,stdAmalWriter,amal_call_xm},			// XM
	{"YM",amal::class_cmd_arg,stdAmalWriter,amal_call_ym},			// YM
	{"K1",amal::class_cmd_arg,stdAmalWriter,NULL},					// k1		mouse key 1
	{"K2",amal::class_cmd_arg,stdAmalWriter,NULL},					// k2		mouse key 2
	{"J0",amal::class_cmd_arg,stdAmalWriter,amal_call_j0},			// j0		joy0
	{"J1",amal::class_cmd_arg,stdAmalWriter,amal_call_j1},			// J1		Joy1
	{"J",amal::class_cmd_normal,stdAmalWriterJump,amal_call_jump},	// Jump
	{"Z",amal::class_cmd_arg,stdAmalWriter,amal_call_z},				// Z(n)	random number
	{"XH",amal::class_cmd_arg,stdAmalWriter,amal_call_xh},			// x hardware
	{"YH",amal::class_cmd_arg,stdAmalWriter,amal_call_yh},			// y hardware
	{"XS",amal::class_cmd_arg,stdAmalWriter,amal_call_sx},			// screen x
	{"YS",amal::class_cmd_arg,stdAmalWriter,amal_call_sy},			// screen y
	{"BC",amal::class_cmd_arg,stdAmalWriter,amal_call_bobCol},		// Bob Col(n,s,e)	// only with Synchro
	{"SC",amal::class_cmd_arg,stdAmalWriter,amal_call_spriteCol},		// Sprite Col(m,s,e)	// only with Synchro
	{"C",amal::class_cmd_arg,stdAmalWriter,amal_call_col},			// Col
	{"V",amal::class_cmd_normal,stdAmalWriter,NULL},				// Vumeter
	{"@while",amal::class_cmd_normal,stdAmalWriter,amal_call_while },
	{"@set",amal::class_cmd_arg,stdAmalWriter,amal_call_set },
	{"@reg",amal::class_cmd_arg,stdAmalWriter,amal_call_reg },
	{NULL, amal::class_cmd_arg,NULL,NULL }
};

void print_code( void **adr )
{
	if (*adr == NULL)
	{
		printf("%08X - %08X - %d\n",(unsigned int) adr,(unsigned int) 0,0);
	}

	for (struct amalTab *tab = amalCmds; tab -> name ; tab++ )
	{
		if ( tab->call == *adr ) 
		{
			printf("%08X - %08X - name %s\n",(unsigned int) adr,(unsigned int) *adr, tab -> name);
			return;
		}
	}
	
	{
		unsigned int c = (unsigned int) *adr;

		if (((c>='0')&&(c<='9')) || ((c>='A')&&(c<='Z')))
		{
			printf("%08X - %08X - R%c\n",(unsigned int) adr,(unsigned int) *adr,(int) *adr);
		}
		else
		{
			printf("%08X - %08X - %d\n",(unsigned int) adr,(unsigned int) *adr,(int) *adr);
		}

	}
}

struct amalTab *find_amal_symbol(const char *str)
{
	int l;

	for (struct amalTab *tab = amalSymbols; tab -> name ; tab++ )
	{
		l = strlen(tab->name);

		if (strncasecmp(str, tab -> name, l) == 0) return tab;
	}
	return NULL;
}

struct amalTab *find_amal_command(const char *str , int class_flags )
{
	char next_c;
	int l;
	struct amalTab *symbol = NULL;

	for (struct amalTab *tab = amalCmds; tab -> name ; tab++ )
	{
		l = strlen(tab->name);

//		printf("tab -> Class: %d, class_flags %d \n", tab -> Class , class_flags);

		if ( tab -> Class & class_flags )
		{
			if (strncasecmp(str, tab -> name, l) == 0)
			{
				next_c = *(str+l);
				symbol = find_amal_symbol( str + l );

				if (next_c != ':')	// chack if its a label
				{	
					if ((next_c == ' ') || (symbol) || (next_c == 0))
					{
						 return tab;
					}
					else if (find_amal_command(str+l, (int) amal::class_cmd_arg | (int) amal::class_cmd_normal))	
					{
						return tab;
					}
					else if (*str=='J')	// if command is jump.
					{
						return tab;
					}
				}
			}
		}
	}
	return NULL;
}

void remove_lower_case(char *txt)
{
	char *c;
	char *d;
	bool space_repeat;

	d=txt;
	for (c=txt;*c;c++)
	{
		// remove noice.
		while (((*c>='a')&&(*c<='z'))||(*c=='#'))	{ c++;  }
		
		space_repeat = false;
		if (d!=txt) 
		{
			char ld = *(d-1);
			if (	((ld==' ')||(ld==',')||(ld==';'))	&&	(*c==' ')	)	space_repeat = true;
		}

		if (space_repeat == false)
		{
			*d++=*c;
		}
	}
	*d = 0;
}

void allocAmalBuf( struct amalBuf *i, int e )
{
	i -> elements = e;
	i -> size = sizeof(void *) * i -> elements;
	i -> call_array = (void *(**) API_AMAL_CALL_ARGS) amalAllocBuffer(i -> size);
}

void freeAmalBuf( struct amalBuf *i)
{
	i -> elements = 0;
	i -> size = 0;
	if ( i -> call_array ) amalFreeBuffer( i -> call_array );
	i -> call_array = NULL;
}

void reAllocAmalBuf( struct amalBuf *i, int e )
{
	void *(**new_array) API_AMAL_CALL_ARGS;
	int new_elements = i -> elements + e;
	int new_size = sizeof(void *) * new_elements;

	new_array = (void *(**) API_AMAL_CALL_ARGS) amalAllocBuffer  ( new_size );

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (i -> call_array)
	{

		if (new_array)
		{
			printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

			memcpy( new_array, i -> call_array, i->size );
			i->elements = new_elements;
			i->size = new_size;
		}
		else
		{
			new_elements = 0;
			new_size = 0;
		}

		printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		printf(" i -> call_array = %08x\n",(unsigned int) i -> call_array );

		amalFreeBuffer(i->call_array);
		i -> call_array = new_array;
	}
	else
	{
		new_elements = 0;
		new_size = 0;
	}

	i -> elements = new_elements;
	i -> size = new_size;
}

bool asc_to_amal_tokens( struct kittyChannel  *channel )
{
	const char *s;
	struct amalTab *found;
	struct amalTab write_cmd;
	char txt[30];
	const char *script = channel -> amal_script;
	struct amalBuf *amalProg = &channel -> amalProg;
	struct amalWriterData data;

	nested_count = 0;

#ifdef test_app
// 1000 to avoid reallocs.
// 20 to test reallocs.
	allocAmalBuf( amalProg, 1000 );	
#else
	allocAmalBuf( amalProg, 60 );
#endif

	printf("script: '%s'\n",script);

	data.pos = 0;

	s=script;
	while (*s)
	{
		printf("LET = %s, next arg %s\n", let ? "TRUE" : "FALSE", next_arg ? "TRUE" : "FALSE");

		if ((let)||(next_arg))
		{
			found = find_amal_command(s, amal::class_cmd_arg);
		}
		else
		{
			found = find_amal_command(s, amal::class_cmd_normal);
		}

		if (!found) found = find_amal_symbol(s);
		
		if (found)
		{
			data.at_script = s;
			data.command_len = strlen(found -> name);
			data.arg_len = 0;

			if ( found -> Class == amal::class_cmd_normal  )
			{
				switch (GET_LAST_NEST)
				{
					case nested_if:
						fix_condition_branch( &amalProg -> call_array[data.pos] );
						write_cmd.call = amal_call_then; 
						data.pos += AmalWriterCondition( channel, &write_cmd , &amalProg -> call_array[data.pos], &data, nested_then);
						amal_cmd_equal = NULL;
						break;

					case nested_then:
						fix_condition_branch( &amalProg -> call_array[data.pos]  );	// skip over else
						write_cmd.call = amal_call_else; 

//						data.pos += AmalWriterCondition( channel, &write_cmd , &amalProg -> call_array[data.pos], &data, nested_else);

						nested_count--;

						amal_cmd_equal = NULL;
						break;
				}
			}

			data.pos += found -> write( channel, found, &amalProg -> call_array[data.pos], &data, 0 );
			data.lastClass = found -> Class;
			s += data.command_len + data.arg_len;
		}
		else 	if (*s == ' ') 
		{
			s++;	// skip space..
		}
		else if ((*s >= '0')&&(*s<='9'))
		{
			printf("found number\n");

			unsigned int num = 0;
			while ((*s >= '0')&&(*s<='9'))
			{
				num = (num *10) + (*s-'0');
				s++;
			}

			found = &amalCmds[1];
			data.at_script = s;
			data.command_len =0;
			data.arg_len = 0;

			data.pos += found -> write( channel, found, &amalProg -> call_array[data.pos], &data, num );
			data.lastClass = found -> Class;
		}
		else if ((*s >= 'A')&&(*s<='Z'))
		{
			char *t = txt;
			const char *l = s;

			while ((*l >= 'A')&&(*l<='Z')&&((int) (l-s)<25))
			{
				*t++ = *l++;
			}
			*t = 0;

			if (*l==':')
			{
				struct AmalLabelRef label;
				label.pos = data.pos;
				label.name = strdup( txt );
				found_labels.push_back( label );
				s = l+1;
			}
			else
			{
				printf("code bad at: '%s'\n",s);
				printf("*l was %c\n",*l);

				amalProg -> call_array[data.pos] = 0;
				return false;
			}
		}
		else
		{
			printf("script: %s\n",channel -> amal_script);
			printf("code bad at: '%s'\n",s);
			amalProg -> call_array[data.pos] = 0;
			return false;
		}

		if (data.pos > amalProg -> elements - 6 )	// larger writer, takes max 6 elements.
		{
			reAllocAmalBuf(amalProg,20);	// add another 20 elements if buffer is low.
		}
	}

	amalProg -> call_array[data.pos++] = amal_call_next_cmd;
	amalProg -> call_array[data.pos] = 0;

	// setup default stack of 500.

	channel -> argStack = (int *) malloc( sizeof(int) * 500 );
	channel -> argStackCount = 0;

	// setup callback buffer.

	channel -> progStack = (struct amalCallBack *) malloc(sizeof(struct amalCallBack *)*500);
	channel -> progStackCount = 0;
	channel -> amalProgCounter = channel -> amalProg.call_array;

	AmalPrintf("channel -> amalProgCounter = %08x\n",(unsigned int) channel -> amalProgCounter);

	return true;
}

void amal_run_one_cycle(struct kittyChannel  *channel)
{
	void *ret;
	void *(**call) ( struct kittyChannel *self, void **code, unsigned int opt );

//	Printf("%s\n", channel -> amalProg.call_array ? "has amal program code" : "do not have amal program code");

	for (call = channel -> amalProgCounter ;  *call ; call ++ )
	{
		ret = (*call) ( channel, (void **) call, 0 );
		if (ret) 
		{
			call = (void* (**)(kittyChannel*, void**, unsigned int)) ret;
		}

		if (channel -> status == channel_status::paused) 	// if amal program gets paused, we break loop
		{
			channel -> status = channel_status::active;
			call++;
			break;
		}
	}

	channel -> amalProgCounter = call;	// save counter.
	if (*call == NULL) 
	{
		Printf("%s:%s:%ld - amal program ended\n",__FILE__,__FUNCTION__,__LINE__);
		channel -> status = channel_status::done;
	}
}

bool amal_find_label(char *name, unsigned int *ref_pos)
{
	unsigned int i;
	*ref_pos = 0xFFFFFFFF;

	for (i=0;i<found_labels.size();i++)
	{
		if (strcmp(found_labels[i].name, name)==0)
		{
			*ref_pos = found_labels[i].pos;
			return true;
		}
	}
	return false;
}

bool amal_fix_labels( void **code )
{
	unsigned int i;
	unsigned int ref_pos = 0xFFFFFFFE;
	int fixed = 0;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (i=0;i<looking_for_labels.size();i++)
	{
		if (amal_find_label(looking_for_labels[i].name,&ref_pos))
		{
			if (ref_pos != 0xFFFFFFFF)
			{
				code[ looking_for_labels[i].pos + 1] = &code[ref_pos];
				fixed ++;
			}
		}
	}

	Printf("looking for labels %ld, Fixed labels %ld\n",looking_for_labels.size(), fixed );

	return (fixed == looking_for_labels.size());
}


void dump_amal_labels()
{
	unsigned int i;

	AmalPrintf("looking for labels\n");

	for (i=0;i<looking_for_labels.size();i++)
	{
		AmalPrintf("pos 0x%08x, name %s\n",looking_for_labels[i].pos,looking_for_labels[i].name);
	}

	AmalPrintf("found labels\n");

	for (i=0;i<found_labels.size();i++)
	{
		AmalPrintf("pos 0x%08x, name %s\n",found_labels[i].pos,found_labels[i].name);
	}
}


#ifdef test_app

unsigned int amiga_joystick_dir[4];
unsigned int amiga_joystick_button[4];
struct retroScreen *screens[8];

int obj_x = 100, obj_y = 50, obj_image = 20;

int getMax ( void )
{
	return 1;
}

int getImage (int object)
{
	return obj_image;
}

int getX (int object)
{
	return obj_x;
}

int getY (int object)
{
	return obj_y;
}

void setImage (int object,int image)
{
	obj_image = image;
}

void setX (int object,int x)
{
	obj_x = x;
}

void setY (int object,int y)
{
	obj_y = y;
}

struct channelAPI test_api = 
{
	getMax,
	getImage,
	getX,
	getY,
	setImage,
	setX,
	setY
};

void dump_object();

void test_run(struct kittyChannel  *channel)
{
	// init amal Prog Counter.
	channel -> status = channel_status::active;
	channel -> objectAPI = &test_api;

	while ( ( channel -> status == channel_status::active ) && ( *channel -> amalProgCounter ) )
	{
	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
		amal_run_one_cycle(channel);
		dump_object();
		dumpAmalRegs( channel );
		getchar();
	}
}

void dump_object()
{
	printf("x: %d, y: %d\n", obj_x, obj_y);
}

int main(int args, char **arg)
{
	int n;
	struct kittyChannel  channel;

	for (n=0;n<10;n++) channel.reg[n]=0;

	initChannel( &channel, 999 );

	amalBuf *amalProg = &channel.amalProg;

	amalProg->call_array = NULL;
	amalProg->size = 0;
	amalProg->elements = 0;

	amiga_joystick_dir[0] = 7;

	if (args==2)
	{
		channel.amal_script = strdup( (char *) arg[1]);

		if (channel.amal_script)
		{
			remove_lower_case(channel.amal_script);
			printf("amal script: %s\n",channel.amal_script);

			if (asc_to_amal_tokens( &channel ))
			{
				dump_object();
				dump_amal_labels();

				if (amal_fix_labels( (void **) amalProg -> call_array ))
				{
					test_run( &channel );
				}
			}

			free(channel.amal_script);
			dumpAmalRegs( &channel );
			amal_clean_up_labels( );
		}
	}

	return 0;
}
#endif

