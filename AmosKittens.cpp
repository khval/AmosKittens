
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "amosKittens.h"
#include "commands.h"

int stack = 0;
int cmdStack = 0;
int last_token = 0;


 struct kittyString strStack[100];
 int numStack[100];
 struct glueCommands cmdTmp[100];

char *cmdNewLine(nativeCommand *cmd, char *ptr)
{
	if (cmdStack)
	{
		cmdTmp[--cmdStack].cmd();
	}
	return ptr;
}

void _print( void)
{
	int n;
	printf("PRINT: ");

	for (n=0;n<=stack;n++)
	{
		if (strStack[n].str) printf("%s", strStack[n].str);
		if (n<=stack) printf("    ");
	}
	printf("\n");
}

char *cmdPrint(nativeCommand *cmd, char *ptr)
{
	cmdNormal( _print, ptr );
	return ptr;
}

char *cmdQuote(nativeCommand *cmd, char *ptr)
{
	unsigned short length = *((unsigned short *) ptr);
	unsigned short length2 = length;
	char *txt;

	length2 += (length & 1);		// align to 2 bytes

	strStack[stack].str = strndup( ptr + 2, length );
	strStack[stack].len = strlen( strStack[stack].str );
	strStack[stack].flag = state_none;

	printf("%s\n", strStack[stack].str);

	if (cmdStack) if (stack)
	{
		 if ((strStack[stack-1].flag == state_none) && (cmdTmp[ cmdStack - 1].flag == cmd_para )) cmdTmp[--cmdStack].cmd();
	}

	return ptr + length2;
}


struct nativeCommand Symbol[]=
{
	{0x0000,	"", 2,	cmdNewLine},
	{0x0476, "Print",0,cmdPrint },
	{0x0026,"\"",2, cmdQuote },
	{0x005C,",", 0, nextArg},
	{0x0064,";", 0, addStr},
	{0x0074,"(", 0, subCalc},
	{0x007C,")", 0, subCalcEnd},
	{0x0084,"[", 0, NULL },
	{0x008C,"]", 0, NULL },
	{0xFFC0,"+",0, addData},
	{0xFFCA,"-", 0, subData},
	{0xFFA2,"=", 0, setVar },
	{0xFFE2,"*", 0, mulVar },
	{0xFFEC,"/", 0, divVar}

};

char *executeToken( char *ptr, unsigned short token )
{
	struct nativeCommand *cmd;
	int size = sizeof(Symbol)/sizeof(struct nativeCommand);

	for (cmd = Symbol ; cmd < Symbol + size ; cmd++ )
	{
		if (token == cmd->id ) 
		{
			printf("'%20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
						__FUNCTION__,__LINE__, stack, cmdStack, strStack[stack].flag, token);
	
			return cmd -> fn( cmd, ptr ) + cmd -> size;
		}
	}

	printf("'%20s:%08d stack is %d cmd stack is %d flag %d token %04x\n",
					__FUNCTION__,__LINE__, stack, cmdStack, strStack[stack].flag, token);	

	return NULL;
}

void _str(const char *str)
{
	printf("\n'%20s:%08d stack is %d cmd stack is %d flag %d\n",__FUNCTION__,__LINE__, stack, cmdStack, strStack[stack].flag);

	strStack[stack].str = strdup( str );
	strStack[stack].len = strlen( strStack[stack].str );
	strStack[stack].flag = state_none;

	if (cmdStack) if (stack)
	{
		 if ((strStack[stack-1].flag == state_none) && (cmdTmp[ cmdStack - 1].flag == cmd_para )) cmdTmp[--cmdStack].cmd();
	}
		
}

void _castNumToStr( int num )
{
	char tmp[100];

	printf("'%20s:%08d stack is %d cmd stack is %d flag %d\n",__FUNCTION__,__LINE__, stack, cmdStack, strStack[stack].flag);

	sprintf(tmp,"%d",num);
	strStack[stack].str = strdup( tmp );
	strStack[stack].len = strlen( strStack[stack].str );
	strStack[stack].flag = state_none;

	if (cmdStack) if (stack)
	{
		 if ((strStack[stack-1].flag == state_none) && (cmdTmp[ cmdStack - 1].flag == cmd_para )) cmdTmp[--cmdStack].cmd();
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

	if (strStack[stack].str)
	{
		printf("'%s' stack is %d cmd stack is %d\n", strStack[stack].str, stack, cmdStack);
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

	fd = fopen("amos-test/print.amos","r");
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

	for (n=0; n<=stack;n++)
	{
		if (strStack[n].str)
		{
			printf("'%s' stack is %d cmd stack is %d\n", strStack[n].str, stack, cmdStack);
		}
		else
		{
			printf("no string found\n");
		}
	}

	return 0;
}
