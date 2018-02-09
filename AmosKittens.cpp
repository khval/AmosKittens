
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "amosKittens.h"
#include "commands.h"

int stack = 0;
int cmdStack = 0;
unsigned short last_token = 0;
int last_var = 0;

BOOL allocMode = FALSE;

void _str(const char *str);
void _num( int num );

struct kittyData kittyStack[100];
struct globalVar globalVars[1000];	// 0 is not used.

int global_var_count = 0;

 int numStack[100];
 struct glueCommands cmdTmp[100];	

void dumpGlobal()
{
	int n;
	int i;

	for (n=1;n<sizeof(globalVars)/sizeof(struct globalVar);n++)
	{
		if (globalVars[n].varName == NULL) return;

		printf("%d\n",globalVars[n].var.type);

		switch (globalVars[n].var.type)
		{
			case type_int:
				printf("%s=%d\n", globalVars[n].varName, globalVars[n].var.value );
				break;
			case type_float:
				printf("%s=%f\n", globalVars[n].varName, globalVars[n].var.decimal );
				break;
			case type_string:
				printf("%s=\"%s\"\n", globalVars[n].varName, globalVars[n].var.str ? globalVars[n].var.str : "NULL" );
				break;
			case type_int | type_array:

				printf("%s(%d)=",
						globalVars[n].varName,
						globalVars[n].var.count);

				for (i=0; i<globalVars[n].var.count; i++)
				{
					printf("%d,",globalVars[n].var.int_array[i]);
				}
				printf("\n");

				break;
			case type_float | type_array:
				break;
			case type_string | type_array:
				break;
		}
	}
}


int findVar( char *name )
{
	int n;

	for (n=1;n<sizeof(globalVars)/sizeof(struct globalVar);n++)
	{
		if (globalVars[n].varName == NULL) return 0;

		if (strcasecmp( globalVars[n].varName, name)==0)
		{
			return n;
		}
	}
	return 0;
}

char *nextCmd(nativeCommand *cmd, char *ptr)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack)
	{
		cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	}

	allocMode = FALSE;

	return ptr;
}

char *cmdNewLine(nativeCommand *cmd, char *ptr)
{
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmdStack)
	{
		cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	}
	return ptr;
}

char *cmdPrint(nativeCommand *cmd, char *ptr)
{
	cmdNormal( _print, ptr );
	return ptr;
}

void dump_stack()
{
	int n;

	for (n=0; n<=stack;n++)
	{
		printf("stack[%d]=",n);

		switch( kittyStack[n].type )
		{		
			case type_int:
				printf("%d\n",kittyStack[n].value);
				break;
			case type_float:
				printf("%f\n",kittyStack[n].decimal);
				break;
			case type_string:
				if (kittyStack[n].str)
				{
					printf("'%s' stack is %d cmd stack is %d\n", kittyStack[n].str, stack, cmdStack);
				}
				else
				{
					printf("no string found\n");
				}
				break;
		}
	}
}

void _array_index_var( glueCommands *self )
{
	int tmp_cells;
	int varNum;
	int n = 0;
	int mul;
	int index;

	struct kittyData *var;

	tmp_cells = stack - self -> stack;

	varNum = self -> lastVar;

//	varNum = *((unsigned short *) (self -> tokenBuffer + 2));
//	printf("buffer %08x, varNum %04x\n",self -> tokenBuffer, varNum);

	var = &globalVars[varNum].var;

	for (n=0; n<var -> cells;n++) printf("data %d\n", var -> sizeTab[n] );

	index = 0; mul  = 1;
	for (n = self -> stack+1;n<=stack; n++ )
	{
		index += (mul * kittyStack[n].value);
		mul *= var -> sizeTab[n- self -> stack -1];
	}

	var -> index = index;
	stack -=  tmp_cells;		// should use garbage collector here ;-) memory leaks works to for now.


	if ((index >= 0)  && (index<var->count))
	{
		// we going over write it.
		if (kittyStack[n].str) free(kittyStack[n].str);
		kittyStack[n].str = NULL;

		// change stack
		switch (var -> type & 7)
		{
			case type_int:
				kittyStack[stack].type = (var -> type & 7);
				kittyStack[stack].value = var -> int_array[index];
				break;
			case type_float:
				kittyStack[stack].type = (var -> type & 7);
				kittyStack[stack].decimal = var -> float_array[index];
				break;
			case type_string:
				kittyStack[stack].type = (var -> type & 7);
				kittyStack[stack].str = strdup(var -> str_array[index]);
				kittyStack[stack].len = strlen(kittyStack[stack].str);
				break;
		}
	}
}

void _alloc_mode_off( glueCommands *self )
{
	int size = 0;
	int n;
	int varNum;
	int count;
	struct kittyData *var;

	allocMode = FALSE;

	printf("self.stack = %d\n",self -> stack);

	printf("****\n");

	// skip the var token +2

	varNum = *((unsigned short *) (self -> tokenBuffer + 2));

	printf("varNum %d\n",varNum);

	var = &globalVars[varNum].var;

	dump_stack();

	var -> cells = stack - self -> stack;

	var -> sizeTab = (int *) malloc( sizeof(int) * var -> cells );


	for (n= 0; n<var -> cells; n++ ) 
	{
		var -> sizeTab[n] = kittyStack[self -> stack + 1 +  n].value;
	}

	var -> count = 1 ;
	for (n= 0; n<var -> cells;n++) var -> count *= var -> sizeTab[n];

	switch (var -> type)
	{
		case type_int:
				size = var -> count * sizeof(int);
				var -> int_array = (int *) malloc( size ) ;
				break;
		case type_float:
				size = var -> count * sizeof(double);
				var -> float_array = (double *) malloc( size ) ;
				break;
		case type_string:
				size = var -> count * sizeof(char *);
				var -> str_array = (char **) malloc( size ) ;
				break;
	}

	memset( var -> str, 0, size );	// str is a union :-)

	printf("name %s, cells %d, size %d, sizeTab %08x\n", 
		globalVars[varNum].varName, 
		var -> cells,
		var -> count,
		var ->sizeTab );

	var -> type |= type_array; 	

	stack -=  var -> cells;	// should use garbage collector here ;-) memory leaks works to for now.

	getchar();
}

char *cmdDim(nativeCommand *cmd, char *ptr)
{
	allocMode = TRUE;
	cmdNormal( _alloc_mode_off, ptr );
	return ptr;
}

const char *types[]={"","#","$",""};

char *cmdVar(nativeCommand *cmd, char *ptr)
{
	char *tmp;
	struct reference *ref = (struct reference *) ptr;


	unsigned short next_token = *((short *) (ptr+sizeof(struct reference)+ref->length));

	printf("next token %04x\n", next_token);


	last_var = 0;
	if (ref -> ref == 0)
	{
		int found = 0;


		tmp = (char *) malloc( ref->length + 2 );
		if (tmp)
		{
			tmp[ ref->length -2 ] =0;
			tmp[ ref->length -1 ] =0;

				printf("%s:%d --- length %d \n",__FUNCTION__,__LINE__, ref -> length);

			memcpy(tmp, ptr + sizeof(struct reference), ref->length );
			sprintf(tmp + strlen(tmp),"%s", types[ ref -> flags & 3 ] );


			found = findVar(tmp);
			if (found)
			{
				free(tmp);		//  don't need tmp
				ref -> ref = found;
			}
			else
			{
				global_var_count ++;
				ref -> ref = global_var_count;
				globalVars[global_var_count].varName = tmp;	// tmp is alloced and used here.
				globalVars[global_var_count].var.str = strdup("");
				globalVars[global_var_count].var.len = 0;
			}

				printf("%s:%d\n",__FUNCTION__,__LINE__);

			last_var = ref -> ref;

			// we should not free tmp, see code above.
		}
	}


	if (next_token == 0x0074)	// ( symbol
	{
		printf("found a array, hurray :)\n");

		if (allocMode == FALSE)
		{

			cmdIndex( _array_index_var, ptr );
		}
	}
	else
	{
		if (ref -> ref)
		{
			switch (ref -> flags & 3)
			{
				case 0:
					_num(globalVars[ref -> ref].var.value);
					break;
				case 1:
					break;
				case 2:
					_str(globalVars[ref -> ref].var.str);
					break;
			}
		}
	}



	return ptr + ref -> length ;
}

char *cmdQuote(nativeCommand *cmd, char *ptr)
{
	unsigned short length = *((unsigned short *) ptr);
	unsigned short length2 = length;
	char *txt;

	length2 += (length & 1);		// align to 2 bytes

	kittyStack[stack].str = strndup( ptr + 2, length );
	kittyStack[stack].len = strlen( kittyStack[stack].str );
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = 2;

	printf("%s\n", kittyStack[stack].str);

	if (cmdStack) if (stack)
	{
		 if (kittyStack[stack-1].state == state_none) if (cmdTmp[cmdStack-1].flag == cmd_para ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	}

	return ptr + length2;
}

char *cmdNumber(nativeCommand *cmd, char *ptr)
{
	unsigned short next_token = *((short *) (ptr+4) );


	printf("Stack[%d].value=%d --- last token %04x, next token %04x\n",stack,kittyStack[stack].value, (unsigned short) last_token, next_token);


	// check if - or + comes before * or /

	if (
		((last_token==0xFFC0) || (last_token==0xFFCA))
	&&
		((next_token==0xFFE2) || (next_token == 0xFFEC))
	) {

		printf("---hidden ( symbol \n");

		// hidden ( condition.
		kittyStack[stack].str = NULL;
		kittyStack[stack].state = state_hidden_subData;
		stack++;
	}


	kittyStack[stack].value = *((int *) ptr);
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = 0;


	// check it last command was * or /. and next command is not a * or /


	if (cmdStack) if (stack)
	{
//		printf("**** kittyStack[stack-1].state %d\n",kittyStack[stack-1].state);
//		printf("**** cmdTmp[cmdStack-1].flag %d\n",cmdTmp[cmdStack].flag);


		 if (kittyStack[stack-1].state == state_none) if (cmdTmp[cmdStack-1].flag == cmd_para ) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	}

	if (
		((last_token==0xFFE2) || (last_token == 0xFFEC))
	&&
		((next_token!=0xFFE2) && (next_token != 0xFFEC))
	) {

		printf("---hidden ( symbol maybe ? \n");

		// it maybe a hidden ) condition.
		if (stack > 0)
		{
			if (kittyStack[stack-1].state == state_hidden_subData)
			{
				printf("---hidden ) symbol yes it is\n");

				kittyStack[stack-1] = kittyStack[stack];
				stack --;
				if (cmdStack) if (stack) if (kittyStack[stack-1].state == state_none) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
			}
		}
	}

	return ptr;
}


struct nativeCommand Symbol[]=
{
	{0x0000,	"", 2,	cmdNewLine},
	{0x0006, "", sizeof(struct reference),cmdVar},
	{0x0026,"\"",2, cmdQuote },
	{0x003E,"",4,cmdNumber },
	{0x0054,":", 0, nextCmd },
	{0x005C,",", 0, nextArg},
	{0x0064,";", 0, addData},
	{0x0074,"(", 0, subCalc},
	{0x007C,")", 0, subCalcEnd},
	{0x0084,"[", 0, NULL },
	{0x008C,"]", 0, NULL },
	{0x0476, "Print",0,cmdPrint },
	{0x0640, "Dim",0,cmdDim },
	{0xFFC0,"+",0, addData},
	{0xFFCA,"-", 0, subData},
	{0xFFA2,"=", 0, setVar},
	{0xFFE2,"*", 0, mulData},
	{0xFFEC,"/", 0, divData}

};

char *executeToken( char *ptr, unsigned short token )
{
	struct nativeCommand *cmd;
	int size = sizeof(Symbol)/sizeof(struct nativeCommand);
	char *ret;

	for (cmd = Symbol ; cmd < Symbol + size ; cmd++ )
	{
		if (token == cmd->id ) 
		{
			printf("'%20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
						__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token);
	
			ret = cmd -> fn( cmd, ptr ) ;
			if (ret) ret += cmd -> size;
			return ret;
		}
	}

	printf("'%20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
					__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state, token);	

	return NULL;
}


void _num( int num )
{
								printf("\n'%20s:%08d stack is %d cmd stack is %d flag %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (kittyStack[stack].str) free(kittyStack[stack].str);	// we should always set ptr to NULL, if not its not freed.

	kittyStack[stack].str = NULL;
	kittyStack[stack].value = num;
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = 0;

	if (cmdStack) if (stack)
	{
		 if (kittyStack[stack-1].state == state_none) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	}
								printf("'%20s:%08d stack is %d cmd stack is %d flag %d\n\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
}

void _str(const char *str)
{
								printf("\n'%20s:%08d stack is %d cmd stack is %d flag %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);

	if (kittyStack[stack].str) free(kittyStack[stack].str);	// we should always set ptr to NULL, if not its not freed.

	kittyStack[stack].str = strdup( str );
	kittyStack[stack].len = strlen( kittyStack[stack].str );
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = 2;

	if (cmdStack) if (stack)
	{
		 if (kittyStack[stack-1].state == state_none) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	}
								printf("'%20s:%08d stack is %d cmd stack is %d flag %d\n\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
}

void _castNumToStr( int num )
{
	char tmp[100];
								printf("'%20s:%08d stack is %d cmd stack is %d flag %d\n",__FUNCTION__,__LINE__, stack, cmdStack, kittyStack[stack].state);
	sprintf(tmp,"%d",num);
	kittyStack[stack].str = strdup( tmp );
	kittyStack[stack].len = strlen( kittyStack[stack].str );
	kittyStack[stack].state = state_none;
	kittyStack[stack].type = 2;

	if (cmdStack) if (stack)
	{
		 if (kittyStack[stack-1].state == state_none) cmdTmp[--cmdStack].cmd(&cmdTmp[cmdStack]);
	}
}

void paramiter_testing()
{
	_str("Amos");
	executeToken( NULL, 0x0064);
	_str(" Profsonal");
	executeToken( NULL, 0x0064 );
	_str(" can run on X");

	executeToken( NULL, 0x0064 );

	_castNumToStr( 1000 );
	executeToken( NULL, 0xFFCA );
	_str("fsonal");

	executeToken( NULL, 0xFFCA );	// -
//	executeToken( NULL, 0xFFC0 );	// +

	executeToken( NULL, 0x0074 );	// (
	_str("X");						// "X"

	executeToken( NULL, 0xFFC0 );	// +
	_str("A1000");					// "A1000"

	executeToken( NULL, 0xFFCA );	// -
	_str("A");						// "A"

	executeToken( NULL, 0x007C );	// )

	printf("--------------------------\n");

	if (kittyStack[stack].str)
	{
		printf("'%s' stack is %d cmd stack is %d\n", kittyStack[stack].str, stack, cmdStack);
	}
	else
	{
		printf("nothing\n");
	}
}

char *token_reader( char *start, char *ptr, unsigned short lastToken, unsigned short token, int tokenlength )
{
	ptr = executeToken( ptr, token );

	if ( ( (long long int) ptr - (long long int) start)  >= tokenlength ) return NULL;

	return ptr;
}

void code_reader( char *start, int tokenlength )
{
	char *ptr;
	int token = 0;
	last_token = 0;
	
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	ptr = start;
	while ( ptr = token_reader(  start, ptr,  last_token, token, tokenlength ) )
	{
		printf("%s:%d\n",__FUNCTION__,__LINE__);

		if (ptr == NULL) break;

		last_token = token;
		token = *((short *) ptr);
		ptr += 2;	// next token.
		
	}

	printf("%s:%d\n",__FUNCTION__,__LINE__);

}

int main()
{
	int tokenlength;
	FILE *fd;
	int amos_filesize;
	char amosid[17];
	char *data;
	int n;

	amosid[16] = 0;	// /0 string.

	stack = 0;
	cmdStack = 0;

	memset(globalVars,0,sizeof(globalVars));

//	fd = fopen("amos-test/var.amos","r");
//	fd = fopen("amos-test/var_num.amos","r");
//	fd = fopen("amos-test/math.amos","r");
	fd = fopen("amos-test/dim.amos","r");
	if (fd)
	{
		fseek(fd, 0, SEEK_END);
		amos_filesize = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		fread( amosid, 16, 1, fd );
		fread( &tokenlength, 4, 1, fd );

		data = (char *) malloc(amos_filesize);
		if (data)
		{
			fread(data,amos_filesize,1,fd);
			code_reader( data, tokenlength );
		}

		fclose(fd);
	}

	printf("--------------------------\n");

	dumpGlobal();
	dump_stack();

	return 0;
}
