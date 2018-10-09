
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <proto/dos.h>
#include <proto/exec.h>

#include "AmalCompiler.h"
#include "channel.h"
#include "AmalCommands.h"

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
	printf("AllocVecTags %d\n",size); 
	return AllocVecTags( size, AVT_Type, MEMF_SHARED, TAG_END );
}

#define amalFreeBuffer( ptr ) { FreeVec( ptr ); ptr = NULL; }

#ifdef test_app
int amreg[26];

void print_code( void **adr );

void dumpAmalRegs()
{
	int i;
	for (i=0;i<26;i++) printf("R%c is %d\n", 'A'+i,amreg[i]);
}

#endif

void *(*amal_cmd_equal) API_AMAL_CALL_ARGS = NULL;

unsigned int (*amal_to_writer) ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num) = NULL;


void dumpAmalStack( struct kittyChannel *channel )
{
	int s;
	Printf("Amal Stack\n");
	for (s=0;s<=channel -> argStackCount;s++) 
	{
		Printf("stack %ld: value %ld\n",s, channel -> argStack[s] );
	}
}

void dumpAmalProgStack( struct kittyChannel *channel )
{
	int s;
	printf("Amal Stack\n");
	for (s=0;s<=channel -> progStackCount;s++) 
	{
		Printf("stack %ld: flags %lx\n",s, channel -> progStack[s].flags );
	}
}

void pushBackAmalCmd( amal::flags flags, void **code, struct kittyChannel *channel, void *(*cmd)  (struct kittyChannel *self, struct amalCallBack *cb)  ) 
{
	if (channel -> progStack)
	{
		struct amalCallBack *CallBack = &channel -> progStack[ channel -> progStackCount ];
		if (CallBack)
		{
			CallBack -> flags = flags;
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

unsigned int numAmalWriter (	struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	call_array[0] = self -> call;

	printf("Writing %-8d to %08x - num\n",num, &call_array[1]);

	*((int *) &call_array[1]) = num;
	return 2;
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

int writeAmalStringToBuffer( const char *s, char *d )
{
	int le = 0;
	while ((*s != 0)&&(*s != ' ')&&( *s != ';')) { *d++=*s++; le++; }
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
	int size;
	int le;

	call_array[0] = self -> call;

	struct amalBuf *amalProg = &channel -> amalProg;

	printf("Writing %-8d to %08x - script\n",num, &call_array[1]);

// find script find the size

	s = AmalAtStringArg( data -> at_script );
	anim_script_len =amalStringLength( s );
	size = 2 + ((anim_script_len + sizeof(void *)) / sizeof(void *) );

// check if size is ok, or need a new buffer.

	if (data -> pos > amalProg -> elements - size )	// larger writer, takes max 6 elements.
	{
		reAllocAmalBuf(amalProg,size + 20);	// add another 20 elements if buffer is low.

		// now that call array is new, need to reset it.
		call_array = &amalProg -> call_array[data -> pos];
	}

// write the script into buffer.

	le = writeAmalStringToBuffer( s, (char *) (&call_array[2]) );
	data -> arg_len = le ? le+1 : 0;
	*((int *) &call_array[1]) = ((le + sizeof(void *)) / sizeof(void *) ) ;

	return 2 + ((le + sizeof(void *)) / sizeof(void *) );
}



unsigned int stdAmalWriterJump (	struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	char labelname[20];
	int le;
	const char *s;
	char *d;

	printf("writing %08x to %08x - jump\n", self -> call, &call_array[0]);

	call_array[0] = self -> call;

	s = data -> at_script;
	while ((*s != 0)&&(*s != ' ')) s++;
	while (*s == ' ') s++;

	le = 0;
	d = labelname;
	while ((*s != 0)&&(*s != ' ')&&( *s != ';')&&(le<18)) { *d++=*s++; le++; }
	*d = 0;

	data -> arg_len = le ? le+1 : 0;

	printf("label: %s\n",labelname);

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

bool let = false;

unsigned int stdAmalWriterLet ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	let = true;
	return 0;
}

unsigned int stdAmalWriterExit_Or_X ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	if (let)
	{
		printf("writing %08x to %08x - X\n", amal_call_x, &call_array[0]);
		call_array[0] = amal_call_x;
	}
	else
	{
		printf("writing %08x to %08x - exit\n", amal_call_exit, &call_array[0]);
		call_array[0] = amal_call_exit;
	}
	return 1;
}

unsigned int stdAmalWriterNextCmd ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	printf("writing %08x to %08x - ;\n", self -> call, &call_array[0]);
	call_array[0] = self -> call;
	let = false;
	return 1;
}

unsigned int stdAmalWriter ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	printf("writing %08x to %08x\n", self -> call, &call_array[0]);
	call_array[0] = self -> call;
	return 1;
}

unsigned int stdAmalWriterEqual ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
	if (amal_cmd_equal)
	{
		printf("writing [code block] to %08x ==\n", &call_array[0]);
		call_array[0] = amal_cmd_equal;
		amal_cmd_equal = NULL;
	}
	else
	{
		printf("writing %08x to %08x =\n", self -> call, &call_array[0]);
		call_array[0] = self -> call;
	}

	return 1;
}

unsigned int amal_for_to ( struct kittyChannel *channel, struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				struct amalWriterData *data,
				unsigned int num)
{
		printf("writing [code block] to %08x - for \n", &call_array[0]);
		char *current_location = (char *) (&call_array[1]);
		char *start_location = (char *) channel -> amalProg.call_array;

		call_array[0] = amal_call_next_cmd;

		call_array[1] = amal_call_while;
		call_array[2] = NULL;
		amalloops.push_back( (void **) (current_location - start_location) );

		printf("start location:    %08x\n",start_location);
		printf("current location: %08x\n",current_location);

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
		int i;
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
	printf("writing %08x to %08x\n", amal_call_reg, &call_array[0]);
	call_array[0] = amal_call_reg;
	*((int *) &call_array[1]) = num;

	last_reg[nest] =num;

	return 2;
}

struct amalTab amalSymbols[] =
{
	{";",stdAmalWriterNextCmd,amal_call_next_cmd },
	{"(",stdAmalWriter,amal_call_parenthses_start },
	{")",stdAmalWriter,amal_call_parenthses_end },
	{",",stdAmalWriter,amal_call_nextArg },
	{"+",stdAmalWriter,amal_call_add},		// +
	{"-",stdAmalWriter,amal_call_sub},			// -
	{"*",stdAmalWriter,amal_call_mul},		// *
	{"/",stdAmalWriter,amal_call_div},			// /
	{"&",stdAmalWriter,amal_call_and},		// &
	{"<>",stdAmalWriter,amal_call_not_equal},	// <>
	{"<",stdAmalWriter,amal_call_less},		// <
	{">",stdAmalWriter,amal_call_more},		// >
	{"=",stdAmalWriterEqual,amal_call_set},	// =
	{NULL, NULL,NULL }
};

struct amalTab amalCmds[] =
{
	{"@{never used}",stdAmalWriter,NULL},
	{"@{number}",numAmalWriter,amal_set_num},	//number token, reserved.
	{"O",stdAmalWriter,amal_call_on},	// On
	{"D",stdAmalWriter,NULL},	// Direct
	{"W",stdAmalWriter,NULL},	// Wait
	{"I",stdAmalWriter,NULL},	 // If
	{"X",stdAmalWriterExit_Or_X,NULL},	// eXit
	{"Y",stdAmalWriter,amal_call_y},	// eXit
	{"L",stdAmalWriterLet,NULL},	// Let
	{"AU",stdAmalWriter,NULL},	// AUtotest
	{"A",stdAmalWriterScript,amal_call_anim},	// Anim
	{"M",stdAmalWriter,amal_call_move},	// Move
	{"P",stdAmalWriter,amal_call_pause},	// Pause
	{"R0",stdAmalWriterReg,NULL },	// R0
	{"R1",stdAmalWriterReg,NULL },	// R0
	{"R2",stdAmalWriterReg,NULL },	// R0
	{"R3",stdAmalWriterReg,NULL },	// R0
	{"R4",stdAmalWriterReg,NULL },	// R0
	{"R5",stdAmalWriterReg,NULL },	// R0
	{"R6",stdAmalWriterReg,NULL },	// R0
	{"R7",stdAmalWriterReg,NULL },	// R0
	{"R8",stdAmalWriterReg,NULL },	// R0
	{"R9",stdAmalWriterReg,NULL },	// R0
	{"RA",stdAmalWriterReg,NULL },	// R0
	{"RB",stdAmalWriterReg,NULL },	// R0
	{"RC",stdAmalWriterReg,NULL },	// R0
	{"RD",stdAmalWriterReg,NULL },	// R0
	{"RE",stdAmalWriterReg,NULL },	// R0
	{"RF",stdAmalWriterReg,NULL },	// R0
	{"RG",stdAmalWriterReg,NULL },	// R0
	{"RH",stdAmalWriterReg,NULL },	// R0
	{"RI",stdAmalWriterReg,NULL },	// R0
	{"RJ",stdAmalWriterReg,NULL },	// R0
	{"RK",stdAmalWriterReg,NULL },	// R0
	{"RL",stdAmalWriterReg,NULL },	// R0
	{"RM",stdAmalWriterReg,NULL },	// R0
	{"RN",stdAmalWriterReg,NULL },	// R0
	{"RO",stdAmalWriterReg,NULL },	// R0
	{"RP",stdAmalWriterReg,NULL },	// R0
	{"RQ",stdAmalWriterReg,NULL },	// R0
	{"RR",stdAmalWriterReg,NULL },	// R0
	{"RS",stdAmalWriterReg,NULL },	// R0
	{"RT",stdAmalWriterReg,NULL },	// R0
	{"RU",stdAmalWriterReg,NULL },	// R0
	{"RV",stdAmalWriterReg,NULL },	// R0
	{"RW",stdAmalWriterReg,NULL },	// R0
	{"RX",stdAmalWriterReg,NULL },	// R0
	{"RY",stdAmalWriterReg,NULL },	// R0
	{"RZ",stdAmalWriterReg,NULL },	// R0
	{"F",stdAmalWriterFor,NULL},				// For
	{"T",stdAmalWriterTo,amal_call_nextArg},	// To
	{"N",stdAmalWriterWend,amal_call_wend},	// Next
	{"PL",stdAmalWriter,NULL},	// Play
	{"E",stdAmalWriter,NULL},	// End
	{"XM",stdAmalWriter,amal_call_xm},	// XM
	{"YM",stdAmalWriter,amal_call_ym},	// YM
	{"K1",stdAmalWriter,NULL},	// k1		mouse key 1
	{"K2",stdAmalWriter,NULL},	// k2		mouse key 2
	{"J0",stdAmalWriter,NULL},	// j0		joy0
	{"J1",stdAmalWriter,NULL},	// J1		Joy1
	{"J",stdAmalWriterJump,amal_call_jump},	// Jump
	{"Z",stdAmalWriter,NULL},	// Z(n)	random number
	{"XH",stdAmalWriter,NULL},	// x hardware
	{"YH",stdAmalWriter,NULL},	// y hardware
	{"XS",stdAmalWriter,NULL},	// screen x
	{"YS",stdAmalWriter,NULL},	// screen y
	{"BC",stdAmalWriter,NULL},	// Bob Col(n,s,e)	// only with Synchro
	{"SC",stdAmalWriter,NULL},	// Sprite Col(m,s,e)	// only with Synchro
	{"C",stdAmalWriter,NULL},	// Col
	{"V",stdAmalWriter,NULL},	// Vumeter
	{"@while",stdAmalWriter,amal_call_while },
	{"@set",stdAmalWriter,amal_call_set },
	{"@reg",stdAmalWriter,amal_call_reg },
	{NULL, NULL,NULL }
};

void print_code( void **adr )
{
	if (*adr == NULL)
	{
		printf("%08X - %08X - %d\n",adr,0,0);
	}

	for (struct amalTab *tab = amalCmds; tab -> name ; tab++ )
	{
		if ( tab->call == *adr ) 
		{
			printf("%08X - %08X - name %s\n",adr,*adr, tab -> name);
			return;
		}
	}
	
	{
		unsigned int c = (unsigned int) *adr;

		if (((c>='0')&&(c<='9')) || ((c>='A')&&(c<='Z')))
		{
			printf("%08X - %08X - R%c\n",adr,*adr,*adr);
		}
		else
		{
			printf("%08X - %08X - %d\n",adr,*adr,*adr);
		}

	}}

struct amalTab *find_amal_symbol(const char *str)
{
	char next_c;
	int l;

	for (struct amalTab *tab = amalSymbols; tab -> name ; tab++ )
	{
		l = strlen(tab->name);

		if (strncasecmp(str, tab -> name, l) == 0) return tab;
	}
	return NULL;
}

struct amalTab *find_amal_command(const char *str)
{
	char next_c;
	int l;
	struct amalTab *symbol = NULL;

	for (struct amalTab *tab = amalCmds; tab -> name ; tab++ )
	{
		l = strlen(tab->name);

		if (strncasecmp(str, tab -> name, l) == 0)
		{
			next_c = *(str+l);
			symbol = find_amal_symbol( str + l );

			if ((next_c == ' ') || (symbol) || (next_c == 0))
			{
				 return tab;
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
		while ((*c>='a')&&(*c<='z'))	{ c++; printf("skip\n"); }
		
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

		printf(" i -> call_array = %08x\n", i -> call_array );

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
	char txt[30];
	const char *script = channel -> amal_script;
	struct amalBuf *amalProg = &channel -> amalProg;
	struct amalWriterData data;

	allocAmalBuf( amalProg, 60 );

	printf("script: '%s'\n",script);

	data.pos = 0;

	s=script;
	while (*s)
	{
		found = find_amal_command(s);

		if (!found) found = find_amal_symbol(s);
		
		if (found)
		{
			data.at_script = s;
			data.command_len = strlen(found -> name);
			data.arg_len = 0;

			data.pos += found -> write( channel, found, &amalProg -> call_array[data.pos], &data, 0 );
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
				printf("code bad at: %s\n",s);

				amalProg -> call_array[data.pos] = 0;
				return false;
			}
		}
		else
		{
			printf("script: %s\n",channel -> amal_script);
			printf("code bad at: %s\n",s);
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

	Printf("channel -> amalProgCounter = %08lx\n",channel -> amalProgCounter);

	return true;
}

void amal_run_one_cycle(struct kittyChannel  *channel)
{
	void *ret;
	void *(**call) ( struct kittyChannel *self, void **code, unsigned int opt );

	Printf("%s\n", channel -> amalProg.call_array ? "has amal program code" : "do not have amal program code");

	for (call = channel -> amalProgCounter ;  *call ; call ++ )
	{
		Printf("%08lx\n",call);
		ret = (*call) ( channel, (void **) call, 0 );
		if (ret) 
		{
			Printf("code offset set %08lx\n",ret);
			call = (void* (**)(kittyChannel*, void**, unsigned int)) ret;
		}

		if (channel -> status == channel_status::paused) 	// if amal program gets paused, we break loop
		{
			channel -> status = channel_status::active;
			call++;
			break;
		}
	}

	Printf("<temp exit loop>\n");

	channel -> amalProgCounter = call;	// save counter.
	if (*call == NULL) 
	{
		Printf("%s - no more data\n",__FUNCTION__);
		channel -> status = channel_status::done;
		getchar();
	}

}

bool amal_find_label(char *name, unsigned int *ref_pos)
{
	int i;
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

void amal_fix_labels( void **code )
{
	int i;
	unsigned int ref_pos = 0xFFFFFFFE;

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (i=0;i<looking_for_labels.size();i++)
	{
		if (amal_find_label(looking_for_labels[i].name,&ref_pos))
		{
			if (ref_pos != 0xFFFFFFFF)
			{
				code[ looking_for_labels[i].pos + 1] = &code[ref_pos];
			}
			return;
		}
	}

	Printf("labels %ld, Fixed labels %ld\n",looking_for_labels.size(), fixed );
}


void dump_amal_labels()
{
	int i;

	printf("looking for labels\n");

	for (i=0;i<looking_for_labels.size();i++)
	{
		printf("pos 0x%08x, name %s\n",looking_for_labels[i].pos,looking_for_labels[i].name);
	}

	printf("found labels\n");

	for (i=0;i<found_labels.size();i++)
	{
		printf("pos 0x%08x, name %s\n",found_labels[i].pos,found_labels[i].name);
	}
}


#ifdef test_app

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
	printf("%s\n", channel -> amalProg.call_array ? "has amal program code" : "do not have amal program code");

	// init amal Prog Counter.
	channel -> status = channel_status::active;
	channel -> objectAPI = &test_api;

	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
	while ( ( channel -> status == channel_status::active ) && ( *channel -> amalProgCounter ) )
	{
	Printf("%s:%s:%ld\n",__FILE__,__FUNCTION__,__LINE__);
		amal_run_one_cycle(channel);
		dump_object();
		dumpAmalRegs();
		getchar();
	}
}

void dump_object()
{
	printf("x: %d, y: %d\n", obj_x, obj_y);
}



int main(int args, char **arg)
{
	struct kittyChannel  channel;

	initChannel( &channel, 999 );

	amalBuf *amalProg = &channel.amalProg;

	amalProg->call_array = NULL;
	amalProg->size = 0;
	amalProg->elements = 0;

	if (args==2)
	{
		channel.amal_script = strdup( (char *) arg[1]);

		if (channel.amal_script)
		{
			remove_lower_case(channel.amal_script);
			printf("amal script: %s\n",channel.amal_script);

			if (asc_to_amal_tokens( &channel ))
			{
				amal_fix_labels( (void **) amalProg -> call_array );

				dump_object();
				dump_amal_labels();


				test_run( &channel );
			}

			free(channel.amal_script);
			dumpAmalRegs();
			amal_clean_up_labels( );
		}
	}

	return 0;
}
#endif

