


#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <sys/file.h>

#if defined(__amigaos__) || defined(__aros__) || defined(__morphos__) 
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <string.h>
#include <stdint.h>
#include "os/linux/stuff.h"
#endif

#include "amosKittens.h"
#include "commands.h"
#include "debug.h"
#include <vector>
#include "AmosKittens.h"
#include "kittyerrors.h"
#include "var_helper.h"
#include "pass1.h"

extern unsigned short token_not_found;

// this should read tokens, and convert every token that needs some help.

extern std::vector<struct fileContext *> files;
extern std::vector<struct lineAddr> linesAddress;

extern int nativeCommandsSize;
extern struct nativeCommand nativeCommands[];

extern int QuoteByteLength(char *ptr);
extern int ReferenceByteLength(char *ptr);

static unsigned short lastToken = 0;

void collect_lines( struct fileContext &lastFile, char *filename );

struct fileContext *new_fileContext( int id, unsigned int newFileSize );

unsigned char *cmd_include( struct fileContext &lastFile, unsigned char *ptr)
{
	unsigned short token;
	unsigned short length;
	unsigned short length2;
	char *filename;

	ptr += 2;	// next token
	token = *((unsigned short *) (ptr));

	if ((token == 0x0026) || ( token == 0x002E)) 
	{
		ptr +=2;
		length = *((unsigned short *) ptr);
		length2 = length + (length & 1);		// align to 2 bytes
		filename = strndup( (char *) ptr + 2,  length );

		if (filename)
		{
			collect_lines( lastFile, filename );
			free(filename);
		}

		ptr += length2;
	}

	token = *((unsigned short *) (ptr+2));
	return ptr;
}

void addLineAddress( struct fileContext &file, int sizeOfToken )
{
	struct lineAddr line;
	line.lineNumber = file.lineNumber;
	line.file = file.file;
	line.srcStart = file.lineStart -  file.start;
	line.srcEnd = (file.ptr + sizeOfToken) - file.start;

	if (line.srcEnd > line.srcStart)
	{
		linesAddress.push_back( line );	

#ifdef show_include_debug_yes
		printf("File %d Line %d starts at %d, ends at %d\n",line.file, line.lineNumber, line.srcStart, line.srcEnd);
#endif
	}
	else
	{
#ifdef show_include_debug_yes
		printf("File %d Line %d starts at %d, ends at %d -- ignored\n",line.file,line.lineNumber, line.srcStart, line.srcEnd);
#endif
	}

	file.lineStart = file.ptr + sizeOfToken;
}


unsigned char *nextToken_include( struct fileContext &file, unsigned short token )
{
	struct nativeCommand *cmd;

	for (cmd = nativeCommands ; cmd < nativeCommands + nativeCommandsSize ; cmd++ )
	{
		if (token == cmd->id )
		{
#ifdef show_include_debug_yes
			printf("file %s:%08X, token %04X - %s\n",file.name, file.ptr, token,cmd -> name);
#endif

			switch (token)
			{
				case 0x0000:	addLineAddress( file, 4);  file.lineNumber++; break;
				case 0x0006:	file.ptr += ReferenceByteLength( (char *) file.ptr+2);	break;
				case 0x000c:	file.ptr += ReferenceByteLength( (char *) file.ptr+2);	break;
				case 0x0012:	file.ptr += ReferenceByteLength( (char *) file.ptr+2);	break;
				case 0x0018:	file.ptr += ReferenceByteLength( (char *) file.ptr+2);	break;
				case 0x0026:	file.ptr += QuoteByteLength( (char *) file.ptr+2);		break;	// skip strings.
				case 0x064A:	file.ptr += QuoteByteLength( (char *) file.ptr+2);		break;	// skip strings.					
				case 0x0652:	file.ptr += QuoteByteLength( (char *) file.ptr+2);		break;	// skip strings.
				case 0x25B2:
							addLineAddress( file, 0);
							file.ptr = cmd_include( file, file.ptr);
							file.lineStart = file.ptr + 2;
							break;
			}

			lastToken = token;

			file.ptr += cmd -> size;
			return file.ptr;
		}
	}

	if (file.ptr<file.end)
	{
		printf("token %04x not found, in file %s at line number %d)\n", token, file.name, file.lineNumber );
		token_not_found = token;
		return NULL;
	}

	addLineAddress( file, 0);
	return NULL;
}

unsigned char *token_reader_include( struct fileContext &file, unsigned short token )
{
	file.ptr = nextToken_include( file, token );


	if ( file.ptr  >= file.end ) return NULL;

	return file.ptr;
}

char *get_name(char *path,char *name)
{
	char *c;
//	char *filename = NULL;
	bool is_path = false;
	bool is_volume = false;

	for (c=name;*c;c++)
	{
		if (*c==':') is_volume = true;
		if (*c=='/') is_path = true;
	}

	if (is_volume) return strdup(name);

	if (path)
	{
		BPTR fd;

		if (is_path)
		{
			if ( fd = Lock( name, SHARED_LOCK ) )
			{
				UnLock( fd );
				return strdup(name);
			}

			{
				char *tmp = (char *) malloc( strlen(path) + strlen(name) + 1 );
				if (tmp)
				{
					sprintf(tmp,"%s%s", path, name );
					return tmp;
				}
			}
		}
		else
		{
			char *tmp = (char *) malloc( strlen(path) + strlen(name) + 1 );
			if (tmp)
			{
				sprintf(tmp,"%s%s", path, name );
				return tmp;
			}
		}
	}

	return name ? strdup(name) : NULL;
}

void collect_lines( struct fileContext &lastFile, char *filename )		// this function should be called recursive.... for etch include.
{
	std::vector<struct kittyLib> libsList;
	struct fileContext *file;					// the file context is private, and is trown away when function ends.
	FILE *fd;
	char amosid[18];
	char *tmp_filename = NULL;
	unsigned short token;
	int l;
	char *c;

	if (filename == NULL) return;

	l = strlen(filename);

	file = new_fileContext( files.size(), 0 );

	for (c=filename + (l ? l-1: 0)  ; c>=filename ;c--)
	{
		if ((*c=='/') || (*c==':'))
		{
			file -> name = strdup(c+1);
			file -> path = strndup(filename, c - filename+1 );
			break;
		}
	}

	if ((file -> path == NULL) && (lastFile.path)) file -> path = strdup(lastFile.path);

	tmp_filename = get_name( file -> path , file -> name ? file -> name : filename );

	if (file -> name == NULL)  file -> name = strdup(file -> name ? file -> name : filename);

	file -> file = files.size();

	files.push_back(file);

	fd = filename ? fopen(tmp_filename ? tmp_filename : filename,"r") : NULL;
	if ( ! fd) 
	{
		if (tmp_filename) free(tmp_filename);
		return;
	}

	if (tmp_filename) free(tmp_filename);
	tmp_filename = NULL;

	fseek(fd, 0, SEEK_END);
	file -> length = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	fread( amosid, 16, 1, fd );
	fread( &(file -> tokenLength), 4, 1, fd );

	file -> start = (unsigned char *) malloc(file -> length);
	file -> lineStart = file -> start + 2;

	if (file -> start)
	{
		int r;
		int _file_code_start_ = ftell(fd);
		file -> end = file -> start + file -> tokenLength;
		r = fread(file -> start, file -> tokenLength ,1,fd);

		if (file -> file == 0)
		{
			file -> bankSize = file -> length - file -> tokenLength - _file_code_start_;

			if (file -> bankSize > 8)
			{
				if (file -> bankSize>6) file -> bank = (unsigned char *) malloc( file -> bankSize );
				if (file -> bank)	fread( file -> bank, file -> bankSize, 1, fd );
			}
		}

		file -> ptr = file -> lineStart;
		token = *((unsigned short *) (file -> ptr));
		while ( file -> ptr = token_reader_include(  *file,  token ) ) 
		{
			if (file->ptr == NULL) break;
			file->ptr += 2;	// next token.
			token = *((unsigned short *) (file->ptr));
		}
	}
	fclose( fd );

}

unsigned int get_size_from_lines()
{
	unsigned int fileSize = 0;
	unsigned int size = 0;
	unsigned int n;
	unsigned int start = 0;

	for (n=0;n<linesAddress.size();n++)
	{
		size = (linesAddress[n].srcEnd  - linesAddress[n].srcStart);
		linesAddress[n].start = start;
		linesAddress[n].end = start + size;
		start+=size;
		fileSize += size;
	}

	return fileSize;
}

void join_files( struct fileContext *newFile )
{
	unsigned int size = 0;
	unsigned int n;
	unsigned char *dest;
	unsigned char *src;

	for (n=0;n<linesAddress.size();n++)
	{
		size = (linesAddress[n].srcEnd  - linesAddress[n].srcStart);
		dest = newFile -> start +linesAddress[n].start;
		src = files[ linesAddress[n].file ] -> start;
		memcpy( dest ,  src + linesAddress[n].srcStart , size  );
	}
}

void free_file(struct fileContext *file)
{
	if (file -> name) free(file -> name);
	if (file -> path) free(file -> path);
	if (file -> start) free(file -> start);
	if (file -> bank) free(file -> bank);
	file -> name = NULL;
	file -> path = NULL;
	file -> start = NULL;
	file -> bank = NULL;
	freeStruct(file);
}

void clean_up_inc_files()
{
	while (files.size())
	{
		free_file( files[0] );
		files.erase(files.begin() );
	}
}

// its tempting to use C++ code, but C is more portable.

void init_fileContext( struct fileContext *file )
{
	file -> file = 0;
	file -> length = 0;
	file -> lineNumber = 0;
	file -> bankSize = 0;
	file -> name = NULL;
	file -> path = NULL;
	file -> start = NULL;
	file -> bank = NULL;
}

struct fileContext *new_fileContext( int id, unsigned int newFileSize )
{
	struct fileContext *file = allocStruct(fileContext,1);
	
	if (file)
	{
		init_fileContext( file );
		file -> file = id;
		file -> tokenLength = newFileSize;
		if (newFileSize>0) file -> start = (unsigned char *) malloc( newFileSize );
		if (file -> start) file -> end = file -> start + file -> tokenLength;
	}
	return file;
}

struct fileContext *newFile( char *name )
{
	struct fileContext lastFile ;
	int newFileSize;
	struct fileContext *newFile;
	lastFile.path = NULL;
	lastFile.name = NULL;

	init_fileContext( &lastFile );
	collect_lines( lastFile, name );
	newFileSize = get_size_from_lines();

	newFile = new_fileContext( 0, newFileSize );
	if (newFile) 
	{
		join_files( newFile );

		if (files.size()>0)	// grab the bank from the first file.
		{
			newFile -> bank = files[0] -> bank;
			newFile -> bankSize = files[0] -> bankSize;
			files[0] -> bank = NULL;
		}
	}

	clean_up_inc_files();

	return newFile;
}


