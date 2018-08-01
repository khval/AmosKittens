
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <proto/exec.h>
#include <string>
#include <iostream>
#include <proto/asl.h>

#include "stack.h"
#include "amosKittens.h"
#include "commandsDisc.h"
#include "commands.h"
#include "debug.h"
#include "errors.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;

APTR contextDir = NULL;
char *dir_first_pattern = NULL;
char *dir_first_path = NULL;

extern char *_setVar( struct glueCommands *data, int nextToken );
extern char *(*_do_set) ( struct glueCommands *data, int nextToken ) ;


char *_cmdInputIn( struct glueCommands *data, int nextToken );
char *_cmdLineInputFile( struct glueCommands *data, int nextToken );

char *amos_to_amiga_pattern(const char *amosPattern);
void split_path_pattern(const char *str, char **path, const char **pattern);

char *_cmdSetDir( struct glueCommands *data, int nextToken )
{
	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_cmdPrintOut( struct glueCommands *data, int nextToken )
{
	int num,n;
	FILE *fd;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	num = getStackNum( data -> stack ) -1;

	if ((num>-1)&&(num<10))
	{
		fd =  kittyFiles[ num ].fd;

		for (n=data->stack+1;n<=stack;n++)
		{
			switch (kittyStack[n].type)
			{
				case type_int:
					fprintf(fd,"%d", kittyStack[n].value);
					break;
				case type_float:
					fprintf(fd,"%f", kittyStack[n].decimal);
					break;
				case type_string:
					if (kittyStack[n].str) fprintf(fd,"%s", kittyStack[n].str);
					break;
			}

			if (n<=stack) fprintf(fd,"    ");
		}

		fprintf(fd, "\n");

	}
	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_open_file_( struct glueCommands *data, const char *access )
{
	char *_str;
	int num;
	int args = stack - cmdTmp[cmdStack-1].stack +1;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 2)
	{
		num = getStackNum( stack -1 ) -1;

		if ((num>-1)&&(num<10))
		{
			if ( kittyFiles[ num ].fd )
			{
				fclose( kittyFiles[ num ].fd );
				kittyFiles[ num ].fd = NULL;
			}
			
			if ( kittyFiles[ num ].fields )
			{
				free( kittyFiles[ num ].fields );
				kittyFiles[ num ].fields = NULL;
			}

			_str = getStackString( stack );
			if (_str) kittyFiles[ num ].fd = fopen( _str, access );

			if (kittyFiles[ num ].fd  == NULL) setError(81,data->tokenBuffer);
		}
	}

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_cmdOpenOut( struct glueCommands *data, int nextToken )
{
	return _open_file_( data, "w" );
}

char *_cmdOpenIn( struct glueCommands *data, int nextToken )
{
	return _open_file_( data, "r" );
}

char *_cmdAppend( struct glueCommands *data, int nextToken )
{
	return _open_file_( data, "a" );
}

char *_cmdOpenRandom( struct glueCommands *data, int nextToken )
{
	return _open_file_( data, "w+" );
}

char *_cmdClose( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;
	int num;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 1)
	{
		num = getStackNum( stack ) -1;

		if ((num>-1)&&(num<10))
		{
			fclose( kittyFiles[ num ].fd );
			kittyFiles[ num ].fd = NULL;

			if (kittyFiles[ num ].fields)
			{
				free(kittyFiles[ num ].fields);
				kittyFiles[ num ].fields = NULL;
			}
		}
	}

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_cmdKill( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;
	char *_str;
	int32 success = false;
	_str = getStackString( stack );

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (_str) success = Delete(_str);

	if (success == false)
	{
		setError(81,data->tokenBuffer);
	}

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_cmdRename( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;
	int32 success = false;

	if (args == 2)
	{
		char *oldName = getStackString( stack - 1 );
		char *newName = getStackString( stack );
		if ((oldName)&&(newName))	success = Rename(  oldName, newName );
	}

	if (success == false)
	{
		setError(81,data->tokenBuffer);
	}

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_cmdFselStr( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack].stack +1;
	struct FileRequester	 *filereq;
	char *ret = NULL;
	char *amigaPattern = NULL;
	char c;
	int l;
	bool success = false;
	const char *_path_ = NULL;
	const char *_default_ = NULL;
	const char *_title_ = NULL;

	if (filereq = (struct FileRequester	 *) AllocAslRequest( ASL_FileRequest, TAG_DONE ))
	{

		switch (args)
		{
			case 3:
					_path_ = getStackString( stack -2 );
					_default_ = getStackString( stack -1 );
					_title_ = getStackString( stack );

					amigaPattern = amos_to_amiga_pattern( (char *) _path_);

					success = AslRequestTags( (void *) filereq, 
						ASLFR_DrawersOnly, FALSE,	
						ASLFR_TitleText, _title_,
						ASLFR_InitialFile, _default_,
						ASLFR_InitialPattern, amigaPattern ? amigaPattern : "",
						ASLFR_DoPatterns, TRUE,
						TAG_DONE );
					break;
		}

		if (success)
		{
			if ((filereq -> fr_File)&&(filereq -> fr_Drawer))
			{
				l = strlen(filereq -> fr_Drawer);

				if (l>1)
				{
					c = filereq -> fr_Drawer[l-1];

					if (ret = (char *) malloc( strlen(filereq -> fr_Drawer) + strlen(filereq -> fr_File) +2 ))
					{
						sprintf( ret, ((c == '/') || (c==':')) ? "%s%s" : "%s/%s",  filereq -> fr_Drawer, filereq -> fr_File ) ;
					}
				}
				else 	ret = strdup(filereq -> fr_File);
			}
		}
		 FreeAslRequest( filereq );
	}

	popStack( stack - cmdTmp[cmdStack-1].stack  );
	if (ret) setStackStr(ret);		// we don't need to copy no dup.

	if (amigaPattern) free(amigaPattern);

	return NULL;
}

char *_cmdExist( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;
	char *_str;
	BPTR lock = 0;

	_str = getStackString( stack );

	if (args==1)
	{
		lock = Lock( _str, SHARED_LOCK );

		if (lock)
		{
			UnLock( lock );
			setStackNum( true ); 
		}
	}

	if (!lock) setStackNum( false );

	popStack( stack - cmdTmp[cmdStack-1].stack  );
	return NULL;
}

char *formated_dir(bool is_dir, const char *path, char *name, int64 size )
{
	char *buffer;
	int _l;
	char c;
	buffer = (char *) malloc( 100 );

	if (buffer)
	{
		if (is_dir)
		{
			sprintf(buffer,"*%-32.32s%10s\n",  name ,"");
		}
		else
		{
			if (size == -1)	// i have no idea way this has no size :-(
			{
				char *fullname;
				_l = strlen(path);
				c = _l ? path[_l-1] : 0;
				fullname = (char *) malloc( _l + strlen(name) + 2 ); 
				if (fullname)
				{
					BPTR fd;
					sprintf(fullname, "%s%s%s", path, ((c==':')&&(c=='/')) ? "" : (_l ? "/" : ""), name );
					fd = Open(fullname, MODE_OLDFILE);
					if (fd)
					{
						size = GetFileSize( fd );
						if (size == -1) PrintFault(IoErr(),NULL);
						Close(fd);
					}
					free(fullname);
				}
			}

			sprintf(buffer," %-32.32s%lld\n", name, size );
		}
	}

	return buffer;
}


char *dir_item_formated(struct ExamineData *dat, const char *path, const char *pattern)
{
	bool _match = true;
	struct ExamineData *target;
	char *outStr = NULL;

	char matchpattern[100];

	if (pattern)
	{
		ParsePattern( pattern, matchpattern, 100 );
		_match = MatchPattern( matchpattern, dat->Name );
	}

	if (_match == false) return NULL;


			if( EXD_IS_LINK(dat) ) /* all links, must check these first ! */
			{
				if( EXD_IS_SOFTLINK(dat) )        /* a FFS style softlink */
				{
					CONST_STRPTR target_type = "unavailable"; /* default  */
			                APTR oldwin = SetProcWindow((APTR)-1); 
					target = ExamineObjectTags(EX_StringNameInput, dat->Name,TAG_END);
					SetProcWindow(oldwin);

					if( target )
					{
						outStr = formated_dir( EXD_IS_DIRECTORY(target), path, dat->Name, target -> FileSize  );					
						FreeDosObject(DOS_EXAMINEDATA,target);
					}
				}
				else if( EXD_IS_FILE(dat) )       /* hardlink file */
				{
					outStr = formated_dir( false, path, dat->Name,  dat -> FileSize );
				}
				else if( EXD_IS_DIRECTORY(dat) )  /* hardlink dir */
				{
					outStr = formated_dir( true, path, dat->Name, 0 );
				}
			}
			else if( EXD_IS_FILE(dat) )           /* a plain file */
			{
				outStr = formated_dir( false, path, dat->Name, dat -> FileSize );
			}
			else if ( EXD_IS_DIRECTORY(dat) )     /* a plain directory */
			{
				outStr = formated_dir( true, path, dat->Name, 0 );
			}

	return outStr;
}



char *_cmdDirNextStr( struct glueCommands *data, int nextToken )
{
	char *outStr = NULL;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if( contextDir )
	{
		struct ExamineData *dat;

		do
		{
			if ((dat = ExamineDir(contextDir)))  /* until no more data.*/
			{
				outStr = dir_item_formated(dat, dir_first_path, dir_first_pattern );
			}
		} while ((outStr == NULL) && (dat));
	}
	
	popStack( stack - cmdTmp[cmdStack-1].stack  );
	if (outStr) setStackStr(outStr);

	return NULL;
}

char *amos_to_amiga_pattern(const char *amosPattern)
{
	int _new_len = 0;
	char *amigaPattern;
	const char *s;
	char *d;

	for (s = amosPattern; *s ;  s++) _new_len += (*s == '*') ? 2 : 1;

	amigaPattern = (char *) malloc(_new_len + 1); // need to terminate string +1

	if (amigaPattern)
	{
		d = amigaPattern;

		for (s = amosPattern; *s ;  s++) 
		{
			if (*s == '*')
			{
				*d++='#'; *d++='?';
			}
			else *d ++= *s;
		}
		*d = 0;
	}
	
	return amigaPattern;
}


bool pattern_match( char *name , const char *pattern )
{
	char *n;
	const char *p;

	if (pattern == NULL) return true;
	n = name;
	p = pattern;

	do
	{
		while (*p == '*') p++;
		while ((*n != *p)&&(*n)) n++;	// seek 

		while ((*n == *p)&&(*n)&&(*p))
		{
			n++; p++;
		}

		if ((*p != '*') && ( *p != 0)) p = pattern;	// reset pattern if not wild case.
	}
	while (*p == '*');

	return (*p == 0);	// if we are at end of patten then it match.
}

void split_path_pattern(const char *str, char **path, const char **pattern)
{
	int _len;
	char c;
	int i;

	_len = strlen( str );

	if (_len>0) 
	{
		c = str[_len-1];

		if ((c == '/') || ( c == ':'))
		{
			*path = strdup(str);
		}
		else
		{
			*path = NULL;

			for (i=_len-1; i>=0;i--)
			{
				c = str[i];

				if ((c == '/') || ( c == ':'))
				{
					*path = strndup( str, i+1 );
					*pattern = str + i +1;
				}
			}

			if (*path == NULL)
			{
				*path = strdup("");
				*pattern = str;	
			}
		}		
	}
	else
	{
		*path = strdup("");
		*pattern = str;
	}
}


char *_cmdDir( struct glueCommands *data, int nextToken )
{
	char *str;
	char *_path = NULL;
	const char *_pattern = NULL;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	str = getStackString( stack );

	if (str == NULL) return NULL;

	split_path_pattern(str, &_path, &_pattern);
	
	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (_path == NULL) return NULL;

	APTR context = ObtainDirContextTags(EX_StringNameInput, _path,
	                   EX_DoCurrentDir,TRUE, /* for ExamineObjectTags() */
	                   EX_DataFields,(EXF_NAME|EXF_LINK|EXF_TYPE), TAG_END);

	if( context )
	{
	    struct ExamineData *dat, *target;

	    while((dat = ExamineDir(context)))  /* until no more data.*/
	    {
	        if( EXD_IS_LINK(dat) ) /* all links, must check these first ! */
	        {
	            if( EXD_IS_SOFTLINK(dat) )        /* a FFS style softlink */
	            {
	                CONST_STRPTR target_type = "unavailable"; /* default  */
	                APTR oldwin = SetProcWindow((APTR)-1); 
	                target = ExamineObjectTags(EX_StringNameInput,
	                                                   dat->Name,TAG_END);
	                SetProcWindow(oldwin);

	                if( target )
	                {
	                    if( EXD_IS_FILE(target) )
	                    {
	                        target_type = "file";
	                    }
	                    if( EXD_IS_DIRECTORY(target) )
	                    {
	                        target_type = "dir";
	                    }
	                    FreeDosObject(DOS_EXAMINEDATA,target);
	                    /* Free target data when done */
	                }
	                Printf("softlink=%s points to %s and it's a %s\n",
	                                 dat->Name,dat->Link,target_type);
	            }
	            else if( EXD_IS_FILE(dat) )       /* hardlink file */
	            {
	                Printf("file hardlink=%s points to %s\n",
	                                 dat->Name, dat->Link);
	            }
	            else if( EXD_IS_DIRECTORY(dat) )  /* hardlink dir */
	            {
	                Printf("dir hardlink=%s points to %s\n",
	                                 dat->Name, dat->Link);
	            }
	        }
	        else if( EXD_IS_FILE(dat) )           /* a plain file */
	        {
			if (pattern_match( dat->Name, _pattern )) Printf("filename=%s\n", dat->Name);			
	        }
	        else if ( EXD_IS_DIRECTORY(dat) )     /* a plain directory */
	        {
	            Printf("dirname=%s\n",  dat->Name);
	        }
	    }
       
	    if( ERROR_NO_MORE_ENTRIES != IoErr() )
	    {
	        PrintFault(IoErr(),NULL); /* failure - why ? */
	    }
	}
	else
	{
	    PrintFault(IoErr(),NULL);     /* failure - why ? */
	}
	
	ReleaseDirContext(context);             /* NULL safe */

	popStack( stack - cmdTmp[cmdStack-1].stack  );

	return  NULL ;
}

char *cmdDir(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdDir, tokenBuffer );

	return tokenBuffer;
}

char *_cmdDirStr( struct glueCommands *data, int nextToken )
{
	int args = stack - data->stack +1 ;
	BPTR lock;
	BPTR oldLock;
	char *_str;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	_str = getStackString( stack );

	if (_str)
	{
		lock = Lock( _str, SHARED_LOCK );
		if (lock)
		{
			oldLock = SetCurrentDir( lock );
			if (oldLock) UnLock (oldLock );
		}
	}

	popStack( stack - data->stack );
	return NULL;
}


char *_set_dir_str( struct glueCommands *data, int nextToken )
{
	char *_new_path = getStackString( stack );

//	printf("%s\n",_new_path);

	if (_new_path) { chdir(_new_path); }

	_do_set = _setVar;
	return NULL;
}


char *cmdDirStr(struct nativeCommand *cmd, char *tokenBuffer)
{

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	dprintf("%08x - %08x\n", last_token, NEXT_TOKEN( tokenBuffer ) );

	if ( ((last_token == 0x0000) || (last_token == 0x0054)) && (NEXT_TOKEN(tokenBuffer) == 0xFFA2 ))
	{
		tokenMode = mode_store;
		_do_set = _set_dir_str;
	}
	else if ( ((last_token == 0x0000) || (last_token == 0x0054)) && (NEXT_TOKEN( tokenBuffer ) == 0xFFA2)) 
	{
		printf("%s:%d\n",__FUNCTION__,__LINE__);
		stackCmdNormal( _cmdDirStr, tokenBuffer );
	}
	else
	{
		char buffer[4000];
		int32 success = NameFromLock(GetCurrentDir(), buffer, sizeof(buffer) );

		printf("%s:%d\n",__FUNCTION__,__LINE__);
		setStackStrDup( success ? buffer : (char *) "" );
	}

	return tokenBuffer;
}

char *cmdParent(struct nativeCommand *cmd, char *tokenBuffer)
{
	BPTR lock;
	BPTR oldLock;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	lock = Lock( "/", SHARED_LOCK );
	if (lock)
	{
		oldLock = SetCurrentDir( lock );
		if (oldLock) UnLock (oldLock );
	}

	return tokenBuffer;
}

char *cmdDfree(struct nativeCommand *cmd, char *tokenBuffer)
{
	struct InfoData data;


	int32 success = GetDiskInfoTags( 
					GDI_LockInput,GetCurrentDir(),
					GDI_InfoData, &data,
					TAG_END);

	if (success)
	{
		unsigned int freeBlocks;

		dprintf("num blocks %d\n", data.id_NumBlocks);
		dprintf("used blocks %d\n", data.id_NumBlocksUsed);
		dprintf("bytes per block %d\n", data.id_BytesPerBlock);


		freeBlocks = data.id_NumBlocks - data.id_NumBlocksUsed;

		// this does not support my disk as it has more then 4 GB free :-/
		// ints are 32bit internaly in AMOS kittens, should bump it up to 64bit, internally.

		setStackNum( freeBlocks * data.id_BytesPerBlock ); 

		// print the real size here... 
		printf("free bytes %lld\n", (long long int) freeBlocks * (long long int) data.id_BytesPerBlock );

	}

	return tokenBuffer;
}

char *cmdKill(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdKill, tokenBuffer );
	return tokenBuffer;
}

char *cmdRename(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdRename, tokenBuffer );
	return tokenBuffer;
}

char *cmdFselStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdFselStr, tokenBuffer );
	return tokenBuffer;
}

char *cmdExist(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdExist, tokenBuffer );
	return tokenBuffer;
}

char *_cmdDirFirstStr( struct glueCommands *data, int nextToken )
{
	char *str;
	const char *_pattern;
	char *outStr = NULL;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (contextDir)
	{
		ReleaseDirContext(contextDir);
		contextDir = NULL;
	}

	str  = getStackString( stack );
	if (str == NULL) return NULL;

	if (dir_first_path)
	{
		free(dir_first_path);
		dir_first_path = NULL;
	}

	split_path_pattern(str, &dir_first_path, &_pattern);

	if (dir_first_pattern) free(dir_first_pattern);
	dir_first_pattern = amos_to_amiga_pattern( (char *) _pattern);

	getchar();

	contextDir = ObtainDirContextTags(EX_StringNameInput, dir_first_path,
	                   EX_DoCurrentDir,TRUE, 
	                   EX_DataFields,(EXF_NAME|EXF_LINK|EXF_TYPE), TAG_END);


	if( contextDir )
	{
		struct ExamineData *dat;

		do
		{
			if ((dat = ExamineDir(contextDir)))  /* until no more data.*/
			{
				outStr = dir_item_formated(dat, dir_first_path, dir_first_pattern );

			}
		} while ((outStr == NULL) && (dat));
	} 
	
	popStack( stack - cmdTmp[cmdStack-1].stack  );
	if (outStr) setStackStr(outStr);

	return NULL;
}

char *cmdDirFirstStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdDirFirstStr, tokenBuffer );
	return tokenBuffer;
}

char *cmdDirNextStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdDirNextStr, tokenBuffer );
	return tokenBuffer;
}

char *cmdSetDir(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdSetDir, tokenBuffer );
	return tokenBuffer;
}

char *cmdPrintOut(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdPrintOut, tokenBuffer );
	return tokenBuffer;
}

char *cmdOpenIn(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdOpenIn, tokenBuffer );
	return tokenBuffer;
}

char *cmdOpenOut(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdOpenOut, tokenBuffer );
	return tokenBuffer;
}

char *cmdAppend(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdAppend, tokenBuffer );
	return tokenBuffer;
}

char *cmdClose(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdClose, tokenBuffer );
	return tokenBuffer;
}

extern char *_setVar( struct glueCommands *data, int nextToken );

void file_input( struct nativeCommand *cmd, char *tokenBuffer )
{
	int idx = 0;
	bool valid = false;
	FILE *fd;

	if (cmd == NULL)
	{
		valid = true;
	}
	else	// if file_input is called from ","
	{
		if (cmdStack > 0)
		{
			if (cmdTmp[cmdStack-1].cmd == _cmdInputIn) valid = true;
		}
	}

	if (valid == false) return;

	idx = last_var - 1;
	if (idx>-1)
	{
		if ((input_cmd_context.lastVar>0)&&(input_cmd_context.lastVar<11))
		{
			fd = kittyFiles[ input_cmd_context.lastVar -1 ].fd ;
		}
		else
		{
			// set some error here..
			return;
		}

		if (fd)
		{
			char buffer[10000];
			int ret = 0;
			int num = 0;
			double decimal;

			switch ( globalVars[idx].var.type & 7 )
			{
				case type_int:

					ret = fscanf( fd, "%d", &num );
					if (ret==1) setStackNum( num );
					break;

				case type_float:

					ret = fscanf( fd, "%lf", &decimal );
					if (ret==1) setStackDecimal( decimal );
					break;

				case type_string:

					ret = fscanf( fd, "%[^,],", buffer );
					if (ret==1) setStackStrDup( buffer );
					break;
			}
			
			if (ret == 1)
			{
				struct glueCommands varData;
				varData.lastVar = last_var;
				_setVar( &varData, 0 );
			}
			else
			{
				// set some error here.
			}
		}
	}
}

#if defined(__AMIGAOS4__)

int getline( char **line,size_t *len, FILE *fd )
{
	char c;
	int written = 0;
	int allocated = 100;
	char *dest,*new_dest,*d;

	*len = 0;

	dest = (char *) malloc( allocated + 1);	// one byte for safety.
	d = dest;

	if (dest == 0) 
	{
		if (*line) free(*line);
		*line = NULL;
		return -1;
	}

	do
	{
		c = fgetc(fd);
		if (feof(fd)) break;

		*d++=c;
		written = d-dest;
		if (written == allocated )
		{
			allocated += 100;
			new_dest = (char *) malloc( allocated + 1 );

			if (new_dest)
			{
				memcpy(new_dest,dest, written );
				free(dest)	;
				dest = new_dest;
				d = dest + written;
			}
			else	// failed to allocate mem...
			{
				free(dest);
				if (*line) free(*line);
				*line = NULL;
				return -1;
			}
		}

	} while (c != '\n');

	*d = 0;

	if (*line) free(*line);
	*line = dest;
	*len = written;

	return written;
}

#endif


void file_line_input( struct nativeCommand *cmd, char *tokenBuffer )
{
	int idx = 0;
	bool valid = false;
	FILE *fd;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmd == NULL)
	{
		valid = true;
	}
	else	// if file_input is called from ","
	{
		if (cmdStack > 0)
		{
			if (cmdTmp[cmdStack-1].cmd == _cmdLineInputFile) valid = true;
		}
	}

	if (valid == false) return;

	idx = last_var - 1;
	if (idx>-1)
	{
		if ((input_cmd_context.lastVar>0)&&(input_cmd_context.lastVar<11))
		{
			fd = kittyFiles[ input_cmd_context.lastVar -1 ].fd ;
		}
		else
		{
			setError(23,tokenBuffer);	// "Illegal function call"
			return;
		}

		if (fd)
		{
			char *line = NULL;
			ssize_t read;
			size_t len = 0;
			int ret = 0;

			getline( &line, &len, fd );

			if (line) 
			{
				setStackStr( line );		
				struct glueCommands varData;
				varData.lastVar = last_var;
				_setVar( &varData, 0 );
			}
			else
			{
				setError(100,tokenBuffer);	// end of file
			}
		}
		else
		{
			setError(97,tokenBuffer); // file not open
		}
	}
}


char *_cmdInputIn( struct glueCommands *data, int nextToken )
{
	getchar();

	if (do_input) do_input( NULL, NULL );

	do_input = NULL;

	popStack( stack - cmdTmp[cmdStack].stack  );

	return NULL;
}

char *cmdInputIn(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (NEXT_TOKEN( tokenBuffer ) == 0x003E)
	{
		input_cmd_context.cmd = _cmdInputIn;
		input_cmd_context.stack = stack;
		input_cmd_context.lastVar = *((int *) (tokenBuffer + 2));
		input_cmd_context.tokenBuffer = tokenBuffer;
		stackCmdNormal( _cmdInputIn, tokenBuffer );

		do_input = file_input;

		tokenBuffer += 6;

		if (NEXT_TOKEN( tokenBuffer ) == 0x005C) tokenBuffer += 2;

	}
	else
	{
		setError(125,tokenBuffer);
	}

	return tokenBuffer;
}

char *_cmdLineInputFile( struct glueCommands *data, int nextToken )
{

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	getchar();

	if (do_input) do_input( NULL, NULL );

	do_input = NULL;

	popStack( stack - cmdTmp[cmdStack].stack  );

	return NULL;
}

char *cmdLineInputFile(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (NEXT_TOKEN( tokenBuffer ) == 0x003E)
	{
		input_cmd_context.cmd = _cmdLineInputFile;
		input_cmd_context.stack = stack;
		input_cmd_context.lastVar = *((int *) (tokenBuffer + 2));
		input_cmd_context.tokenBuffer = tokenBuffer;

		stackCmdNormal( _cmdLineInputFile, tokenBuffer );

		do_input = file_line_input;

		tokenBuffer += 6;

		if (NEXT_TOKEN( tokenBuffer ) == 0x005C) tokenBuffer += 2;
	}
	else
	{
		setError(23,tokenBuffer);
	}

	return tokenBuffer;
}

char *_cmdInputStrFile( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack;
	int channel = 0;
	int len = 0;
	char *newstr;
	FILE *fd;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 2)
	{
		channel = getStackNum(stack - 1 );
		len = getStackNum(stack );

		if (( channel >0)&&( channel <11))
		{
			fd = kittyFiles[ channel -1 ].fd ;

			if (fd)
			{
				popStack( stack - cmdTmp[cmdStack].stack  );
				
				newstr = (char *) malloc(len +1);

				if (newstr)	 if (fgets( newstr, len ,fd ))
				{
					popStack( stack - cmdTmp[cmdStack].stack  );
					setStackStr(newstr);
					return NULL;
				}
				else
				{
					free(newstr);
				}
				
				// set some error here

				return NULL;
			}
			else	setError(97,data->tokenBuffer); // file not open
		}
		else	setError(23,data->tokenBuffer);	// "Illegal function call"
	}

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_cmdSetInput( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	dump_stack();
	getchar();

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_cmdLof( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack;
	int channel = 0;
	FILE *fd;
	int pos,len;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	dump_stack();

	if (args == 1)
	{
		channel = getStackNum(stack);

		if (( channel >0)&&( channel <11))
		{
			fd = kittyFiles[ channel -1 ].fd ;

			if (fd)
			{
				popStack( stack - cmdTmp[cmdStack].stack  );

				pos = ftell( fd );
				fseek( fd, 0, SEEK_END );
				len = ftell(fd);
				fseek( fd, pos, SEEK_SET );

				setStackNum( len );
				return NULL;
			}
			else	setError(97,data->tokenBuffer); // file not open
		}
		else	setError(23,data->tokenBuffer);	// "Illegal function call"
	}

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_cmdPof( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack;
	int channel = 0;
	FILE *fd;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	dump_stack();

	if (args == 1)
	{
		channel = getStackNum(stack);

		if (( channel >0)&&( channel <11))
		{
			fd = kittyFiles[ channel -1 ].fd ;

			if (fd)
			{
				popStack( stack - cmdTmp[cmdStack].stack  );
				setStackNum( ftell( fd ));
				return NULL;
			}
			else	setError(97,data->tokenBuffer); // file not open
		}
		else	setError(23,data->tokenBuffer);	// "Illegal function call"
	}

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_cmdEof( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;	
	int channel = 0;
	FILE *fd;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 1)
	{
		channel = getStackNum(stack);

		dprintf("channel: %d\n",channel);

		if (( channel >0)&&( channel <11))
		{
			fd = kittyFiles[ channel -1 ].fd ;

			if (fd)
			{
				popStack( stack - cmdTmp[cmdStack].stack  );
				setStackNum( feof( fd ));
				return NULL;
			}
			else	setError(97,data->tokenBuffer); // file not open
		}
		else	setError(23,data->tokenBuffer);	// "Illegal function call"
	}

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}


char *cmdInputStrFile(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _cmdInputStrFile, tokenBuffer );
	return tokenBuffer;
}

char *cmdSetInput(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdSetInput, tokenBuffer );
	return tokenBuffer;
}

char *cmdLof(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _cmdLof, tokenBuffer );
	return tokenBuffer;
}

char *cmdPof(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _cmdPof, tokenBuffer );
	return tokenBuffer;
}

char *cmdEof(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdParm( _cmdEof, tokenBuffer );
	return tokenBuffer;
}

char *_cmdGet( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;
	int channel = 0;
	int n, index;
	struct kittyField *fields = NULL;
	FILE *fd;
	char *str;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	dump_stack();

	if (args == 2)
	{
		channel = getStackNum( stack -1 ) ;
		index = getStackNum( stack ) ;

		if ((channel>0)&&(channel<11) && (index>0))
		{
			fields = kittyFiles[channel-1].fields ;
			fd = kittyFiles[channel-1].fd ;

			printf("Seek to %d\n",(index -1) * kittyFiles[channel-1].fieldsSize);

			fseek( fd, (index -1) * kittyFiles[channel-1].fieldsSize , SEEK_SET);

			for (n=0; n<kittyFiles[channel-1].fieldsCount;n++)
			{
				if (globalVars[ fields -> ref -1 ].var.str) free(globalVars[ fields -> ref -1 ].var.str);

				str = (char *) malloc( fields -> size + 1);
				fgets(str,fields -> size+1,fd);
				globalVars[ fields -> ref -1 ].var.str = str;
				
				fields ++;
			}
			printf("\n");
		}
	}

	getchar();

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_cmdPut( struct glueCommands *data, int nextToken )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;
	int channel = 0;
	int n, index;
	struct kittyField *fields = NULL;
	FILE *fd;
	char fmt[15];
	char tmp[1000];

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	dump_stack();

	if (args == 2)
	{
		channel = getStackNum( stack -1 ) ;
		index = getStackNum( stack ) ;

		if ((channel>0)&&(channel<11) && (index>0))
		{
			fields = kittyFiles[channel-1].fields ;
			fd = kittyFiles[channel-1].fd ;

			printf("Seek to %d\n",(index -1) * kittyFiles[channel-1].fieldsSize);

			fseek( fd, (index -1) * kittyFiles[channel-1].fieldsSize , SEEK_SET);

			for (n=0; n<kittyFiles[channel-1].fieldsCount;n++)
			{
				printf(" [%d,%d] ", fields -> size, fields -> ref );

				sprintf( fmt, "%%-%ds",fields -> size);
				sprintf( tmp, fmt, globalVars[ fields -> ref -1 ].var.str );
				tmp[ fields -> size ] = 0;

				fputs(tmp,fd);

				fields ++;
			}
			printf("\n");
		}
	}

	getchar();

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *cmdOpenRandom(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdOpenRandom, tokenBuffer );
	return tokenBuffer;
}

char *cmdField(struct nativeCommand *cmd, char *ptr)
{
	int count = 0;
	int size = 0;
	int channel = 0;
	unsigned short token;
	struct reference *ref;
	struct kittyField *fields = NULL;

	token = *( (unsigned short *) ptr);
	ptr +=2;

	do 
	{
		switch (token)
		{
			case 0x003E:	// number.

					if (count == 0)
					{
						channel = *((int *) ptr);

						if (kittyFiles[channel-1].fields == NULL)
						{
							kittyFiles[channel-1].fields = (struct kittyField *) malloc( sizeof(struct kittyField) * 100 );
						}

						fields = kittyFiles[channel-1].fields;
					}
					else if (fields)
					{
						fields[count-1].size = *((int *) ptr);
						size += fields[count-1].size;
					}
					ptr+=2;
					break;					

			case 0x005C:	
				count ++;
				break;

			case 0x01E6:	// as
				break;

			case 0x0006:	// var

				if ((fields)&&(count))
				{
					ref = (struct reference *) ptr;
					fields[count-1].ref = ref->ref;
					ptr += sizeof(struct reference *);
					ptr += ref -> length;
				}
				break;

			default:
				// error, unexpected token
				break;
		}

		last_token = token;
		token = *( (short *) ptr);
		ptr += 2;

	} while ((token != 0) && (token != 0x0054 ));

	if (channel) 
	{
		kittyFiles[channel-1].fieldsCount = count ;
		kittyFiles[channel-1].fieldsSize = size ;
	}

	dprintf("size %d, count %d\n", size, count);
	getchar();

	return ptr - 2;
}

char *cmdGet(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdGet, tokenBuffer );
	return tokenBuffer;
}

char *cmdPut(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);
	stackCmdNormal( _cmdPut, tokenBuffer );
	return tokenBuffer;
}

char *cmdAt(struct nativeCommand *cmd, char *tokenBuffer)
{
	return tokenBuffer;
}

