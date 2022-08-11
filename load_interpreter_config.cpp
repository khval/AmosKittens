
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <math.h>
#include <proto/exec.h>
#include "load_interpreter_config.h"

extern ULONG flags;

#define cmpID( a, b ) (*((int *) a) == *((int *) ((void *) b)))
#define Leek( adr )	*((int *) (adr))
#define Peek( adr ) *((char *) (adr))
#define Peek_str( adr, n ) strndup( adr , n )

const char *config_name = "AMOSPro_Interpreter_Config";

char *ST_str[STMX];

void process_load( char *mem )
{
	char *STAD;
	char *A;
	int ST;
	int L;			// string length

	if (cmpID(mem,"PId1"))
	{
	         STAD=mem+Leek(mem+4)+8;
		if(cmpID(STAD,"PIt1"))
		{
			// Strings
			A=STAD+8;

			for (ST=1 ; ST<=STMX; ST++)
			{
				L=Peek(A+1) ;
				 if (L==0xFF) break;
				ST_str[ST-1]=Peek_str(A+2,L);

				printf("%-3d:%s\n",ST, ST_str[ST-1]);

				A+=L+2;
			} 
		} 
	} 
}

BOOL load_config( const char *name )
{
	FILE *fd;
	size_t filesize;
	char *bank;
	BOOL ret = FALSE;

	fd = fopen(name,"r");
	if (fd)
	{	
		fseek(fd,0,SEEK_END);
		filesize = ftell(fd);
		fseek(fd,0,SEEK_SET);

		bank = (char *) malloc( filesize );

		if (bank)
		{
			if (fread( bank, filesize, 1, fd )==1)
			{
				process_load( bank );
			}

			ret = TRUE;
			free(bank);
		}

		fclose(fd);
	}
	return ret;
}

char *get_path(char *name)
{
	int nlen = name ? strlen(name) : 0;
	int n;

	for (n=nlen-1; n>0; n--)
	{
		if ((name[n]==':')||(name[n]=='/'))
		{
			return strndup(name,n);
		}
	}

	return NULL;
}

char *safe_addpart(char *path, char *name)
{
	if ( (path) && (name) )
	{
		int size;
		int pl = strlen(path);
		char *newname;
		BOOL has_divider = TRUE;		// if path is "" then we don't add a divider

		if (pl>0) has_divider = ((path[pl-1] == '/') || (path[pl-1] == ':')) ? TRUE : FALSE;

		size = pl + strlen(name) + 2;		// one extra for divider, one extra for \0 = two extra
		newname = (char *) malloc( size );
		if (newname)
		{
			sprintf(newname,"%s%s%s", path,has_divider ? "" : "/", name);
			return newname;
		}		
	}

	if ( (path == NULL) && (name) )	// we don't have path, so we do normal strdup().
	{
		return strdup(name);
	}

	return NULL;
}

BOOL try_config( const char *path, char *config_name)
{
	BOOL config_loaded = FALSE;

	char *config_full_name = NULL;

	config_full_name = safe_addpart( (char *) path, config_name );

	printf("try: '%s'\n", config_full_name);

	if (config_full_name)
	{
		config_loaded = load_config(config_full_name);
		free(config_full_name);
	}

	printf("config loaded: %s\n",config_loaded ? "True" : "False");

	return config_loaded;
}

BOOL load_config_try_paths( char *filename)
{
	BOOL config_loaded = FALSE;
	const char **path;
	char *_path;	// tmp path
	const char *paths[] =
		{
			"s:",
			"progdir:",
			"amospro_system:s",
			NULL,
		};

	_path = get_path( filename );
	if (_path)
	{
		config_loaded = try_config( _path,  (char *) config_name);
		free(_path);
	}

	path = paths;
	while ((config_loaded == FALSE)&&(*path))
	{
		config_loaded = try_config( *path,  (char *) config_name);
		path++;
	}

	return config_loaded;
}

