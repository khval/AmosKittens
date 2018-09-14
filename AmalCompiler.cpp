
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AmalCompiler.h"
#include "channel.h"
#include "AmalCommands.h"

void pushBackAmalCmd( struct kittyChannel *channel, void *cmd ) 
{
	if (channel -> progStack)
	{
		struct amalCallBack *CallBack = channel -> progStack[ channel -> progStackCount ];
		if (CallBack)
		{
			CallBack -> cmd = cmd;
			CallBack -> argStackCount = channel -> argStackCount;
			CallBack -> progStackCount = channel -> progStackCount;
			channel -> progStackCount ++;
			return;
		}
	}
	else
	{
		channel -> progStack = (struct amalCallBack **) malloc(sizeof(struct amalCallBack *)*500);

		if (channel -> progStack)
		{
			struct amalCallBack *CallBack = channel -> progStack[ channel -> progStackCount ];
			if (CallBack)
			{
				CallBack -> cmd = cmd;
				CallBack -> argStackCount = channel -> argStackCount;
				CallBack -> progStackCount = channel -> progStackCount;
				channel -> progStackCount ++;
				return;
			}
		}
	}
}

unsigned int numAmalWriter (	struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				unsigned int num)
{
	call_array[0] = self -> call;

	printf("saves num %d\n",num);

	*((int *) &call_array[1]) = num;
	return 2;
}

unsigned int stdAmalWriter (	struct amalTab *self, 
				void *(**call_array) ( struct kittyChannel *self, void **code, unsigned int opt ), 
				unsigned int)
{
	printf("writing %08x to %08x\n", self -> call, &call_array[0]);
	call_array[0] = self -> call;
	return 1;
}

struct amalTab amalCmds[] =
{
	{"@{never used}",stdAmalWriter,NULL},
	{"@{number}",numAmalWriter,amal_set_num},	//number token, reserved.
	{"O",stdAmalWriter,amal_call_on},	// On
	{"D",stdAmalWriter,NULL},	// Direct
	{"W",stdAmalWriter,NULL},	// Wait
	{"I",stdAmalWriter,NULL},	 // If
	{"J",stdAmalWriter,NULL},	// Jump
	{"X",stdAmalWriter,NULL},	// eXit
	{"L",stdAmalWriter,NULL},	// Let
	{"AU",stdAmalWriter,NULL},	// AUtotest
	{"M",stdAmalWriter,NULL},	// Move
	{"A",stdAmalWriter,amal_call_anim},	// Anim
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
	{"+",stdAmalWriter,NULL},	// +
	{"-",stdAmalWriter,NULL},	// -
	{"*",stdAmalWriter,NULL},	// *
	{"/",stdAmalWriter,NULL},	// /
	{"&",stdAmalWriter,NULL},	// &
	{"<>",stdAmalWriter,NULL},	// <>
	{"<",stdAmalWriter,NULL},	// <
	{">",stdAmalWriter,NULL},	// >
	{"=",stdAmalWriterEqual,amal_call_set},	// =
	{"F",stdAmalWriterFor,NULL},				// For
	{"T",stdAmalWriterTo,amal_call_nextArg},	// To
	{"N",stdAmalWriterWend,amal_call_wend},		// Next
	{"PL",stdAmalWriter,NULL},	// Play
	{"E",stdAmalWriter,NULL},	// End
	{"XM",stdAmalWriter,NULL},	// XM
	{"YM",stdAmalWriter,NULL},	// YM

	{"K1",stdAmalWriter,NULL},	// k1		mouse key 1
	{"K2",stdAmalWriter,NULL},	// k2		mouse key 2
	{"J0",stdAmalWriter,NULL},	// j0		joy0
	{"J1",stdAmalWriter,NULL},	// J1		Joy1
	{"Z",stdAmalWriter,NULL},	// Z(n)	random number

	{"XH",stdAmalWriter,NULL},	// x hardware
	{"YH",stdAmalWriter,NULL},	// y hardware
	{"XS",stdAmalWriter,NULL},	// screen x
	{"YS",stdAmalWriter,NULL},	// screen y
	{"BC",stdAmalWriter,NULL},	// Bob Col(n,s,e)	// only with Synchro
	{"SC",stdAmalWriter,NULL},	// Sprite Col(m,s,e)	// only with Synchro
	{"C",stdAmalWriter,NULL},	// Col
	{"V",stdAmalWriter,NULL},	// Vumeter
	{";",stdAmalWriter,amal_call_next_cmd },
	{"(",stdAmalWriter,amal_call_parenthses_start },
	{")",stdAmalWriter,amal_call_parenthses_end },
	{":",stdAmalWriter,NULL },
	{",",stdAmalWriter,amal_call_nextArg },
	{NULL, NULL,NULL }
};

struct amalTab *find_amal_command(const char *str)
{
	for (struct amalTab *tab = amalCmds; tab -> name ; tab++ )
	{
		if (strncasecmp(str, tab -> name, strlen(tab->name)) == 0) return tab;
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
		while ((*c>='a')&&(*c<='z'))	c++;
		
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
	i -> call_array = (void *(**) API_AMAL_CALL_ARGS) malloc(i -> size);
}

void reAllocAmalBuf( struct amalBuf *i, int e )
{
	void *(**new_array) API_AMAL_CALL_ARGS;
	int new_elements = i -> elements + e;
	int new_size = sizeof(void *) * i -> elements;

	new_array = (void *(**) API_AMAL_CALL_ARGS) malloc( new_size );

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (i -> call_array)
	{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		if (new_array)
		{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

			memcpy( new_array, i -> call_array, i->size );
			i->elements = new_elements;
			i->size = new_size;
		}
		else
		{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

			new_elements = 0;
			new_size = 0;
		}

	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
		free((void *) i -> call_array);

		i -> call_array = new_array;
	}
	else
	{
	printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		new_elements = 0;
		new_size = 0;
	}

	i -> elements = new_elements;
	i -> size = new_size;
}

bool asc_to_amal_tokens( struct kittyChannel  *channel )
{
	int pos = 0;
	const char *s;
	struct amalTab *found;
	char txt[30];
	const char *script = channel -> script;
	struct amalBuf *amalProg = &channel -> amalProg;

	allocAmalBuf( amalProg, 10 );

	s=script;
	while (*s)
	{
		found = find_amal_command(s);
		
		if (found)
		{
			printf("Command found\n");

			pos += found -> write( found, &amalProg -> call_array[pos], 0 );
			s += strlen(found -> name);
		}
		else 	if (*s == ' ') 
		{
			printf("skip a space...\n");

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
			pos += found -> write( found, &amalProg -> call_array[pos], num );
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
				printf("%s\n",txt);
				s = l;
			}
			else
			{
				printf("unkown data at %s\n",s);
				amalProg -> call_array[pos] = 0;
				return false;
			}
		}
		else
		{
			printf("unkown data at %s\n",s);
			amalProg -> call_array[pos] = 0;
			return false;
		}

		if (pos > amalProg -> elements - 4 )
		{
			printf("%d > %d\n", pos , amalProg -> elements - 4);

			printf("overflow\n");

			reAllocAmalBuf(amalProg,4);
		}

	}
	amalProg -> call_array[pos] = 0;
	return true;
}

#ifdef test_app

void test_run(struct kittyChannel  *channel)
{
	void *ret;
	void *(**call) ( struct kittyChannel *self, void **code, unsigned int opt );

	printf("%s\n", channel -> amalProg.call_array ? "has amal program code" : "do not have amal program code");

	for (call = channel -> amalProg.call_array ;  *call ; call ++ )
	{
		ret = (*call) ( channel, (void **) call, 0 );
		if (ret) call = (void* (**)(kittyChannel*, void**, unsigned int)) ret;
	}
	channel -> amalProgCounter = call;	// save counter.
}

int main(int args, char **arg)
{
	struct kittyChannel  channel;

	amalBuf *amalProg = &channel.amalProg;

	amalProg->call_array = NULL;
	amalProg->size = 0;
	amalProg->elements = 0;

	if (args==2)
	{
		channel.script = strdup( (char *) arg[1]);

		if (channel.script)
		{
			remove_lower_case(channel.script);
			printf("%s\n",channel.script);

			if (asc_to_amal_tokens( &channel ))
			{
				test_run( &channel );
			}

			free(channel.script);
		}
	}

	return 0;
}
#endif

