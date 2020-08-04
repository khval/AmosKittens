
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <math.h>
#include <signal.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <libraries/retroMode.h>
#include <proto/retroMode.h>
#endif

#include "config.h"
#include "amosString.h"
#include <amosKittens.h>
#include "interfacelanguage.h"

struct KittyInstance instance;
int breakpoint;

struct retroSprite *patterns = NULL;
std::vector<struct kittyBank> kittyBankList;

struct Library 			*RetroModeBase = NULL;
struct RetroModeIFace 	*IRetroMode = NULL;

struct Library			*kittyCompactBase = NULL;
struct kittyCompactIFace	*IkittyCompact = NULL;

extern void draw_HyperText(struct zone_hypertext *zh);

struct TextFont *topaz8_font = NULL;


const char *arg_names[] = 
	{
		"-script",
		"-file",
		NULL
	};

enum
{
	arg_script,
	arg_file
};

char *script = NULL;
char *file = NULL;

void get_args(int args, char **arg)
{
	int n,nn;
	int arg_type = 0;
	bool arg_type_found;

	for (n=1;n<args;n++)
	{
		arg_type_found = false;

		for (nn = 0; arg_names[nn]; nn++)
		{
			if (strcasecmp(arg[n],arg_names[nn])==0)
			{
				arg_type = nn;
				arg_type_found = true;
				break;
			}
		}

		if (arg_type_found == false)
		{
			switch (arg_type)
			{
				case arg_script:
						script = arg[n]; 
						break;

				case arg_file:
						file = arg[n]; 
						break;
			}
		}
	}
}

void readFile( const char *file )
{
	int length;
	FILE *fd;

	script = NULL;

	fd = fopen(file,"r");
	if (fd)
	{
		fseek(fd,0,SEEK_END);
		length = ftell(fd);
		fseek(fd,0,SEEK_SET);

		script = (char *) malloc( length + 1 );
		if (script)	script[fread( script, 1, length, fd )]=0;
		fclose(fd);
	}
	else printf("file not found '%s'\n",file);
}

int main(int args, char **arg)
{
	int n;
	class cmdcontext *context = new cmdcontext();
	struct stringData *scriptAmos = NULL;

	get_args( args, arg);

	printf("file: %s\n", file ? file : "<NULL>");

	if (file)
	{
		readFile( file );

		if (script)	// str is allocated
		{
			scriptAmos = toAmosString( script, strlen(script));
			free (script);
		}
		script = NULL;
	}
	else if (script)	// str from arg
	{
		scriptAmos = toAmosString( script, strlen(script));
	}

	if (scriptAmos)
	{
		init_interface_context( context, 1, scriptAmos, 0, 0, 10, 1000  );

		// its copied so we can free it now.
		sys_free(scriptAmos);
		scriptAmos = NULL;	
	}

	if (context -> script)
	{
		printf("%s\n",&context -> script ->ptr);
		execute_interface_script( context, 0);
		delete context;
	}


	return 0;
}

void makeMaskForAll()
{}

void setError( int _code, char * _pos ) 
{}

void engine_lock()
{}

void engine_unlock()
{}

bool engine_ready()
{
	return true;
}

int engine_wait_key;

void *findBank(int id)
{}

uint8_t getByte( char *adr, int &pos )
{
	uint8_t ret = *((uint8_t *) (adr + pos));
	pos+=1;
	return ret;
}

uint16_t getWord( char *adr, int &pos )
{
	short ret = *((uint16_t *) (adr + pos));
	pos+=2;
	return ret;
}

uint32_t getLong( char *adr, int &pos )
{
	short ret = *((uint32_t *) (adr + pos));
	pos+=4;
	return ret;
}


