#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/retroMode.h>
#endif

#ifdef __linux__
#include <stdint.h>
#include "os/linux/stuff.h"
#endif

#include <vector>
#include <string>
#include <iostream>

#include "stack.h"
#include "amosKittens.h"
#include "commandsDisc.h"
#include "commands.h"
#include "debug.h"
#include "kittyErrors.h"
#include "amosString.h"

extern int last_var;
extern struct globalVar globalVars[];
extern int tokenMode;

std::vector<std::string> devList;

APTR contextDir = NULL;
char *dir_first_pattern = NULL;
struct stringData *dir_first_path = NULL;

extern char *_setVar( struct glueCommands *data, int nextToken );
extern char *(*_do_set) ( struct glueCommands *data, int nextToken ) ;

char *_discInputIn( struct glueCommands *data, int nextToken );
char *_discLineInputFile( struct glueCommands *data, int nextToken );

char *amos_to_amiga_pattern(const char *amosPattern);
void split_path_pattern( struct stringData *str, struct stringData **path, struct stringData **pattern);

int have_drive( struct stringData *name);

char *_discSetDir( struct glueCommands *data, int nextToken )
{
	popStack(__stack - data -> stack  );
	return NULL;
}

char *_discPrintOut( struct glueCommands *data, int nextToken )
{
	int num,n;
	FILE *fd;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	num = getStackNum( data -> stack ) -1;

	if ((num>-1)&&(num<10))
	{
		fd =  kittyFiles[ num ].fd;

		for (n=data->stack+1;n<=__stack;n++)
		{
			switch (kittyStack[n].type)
			{
				case type_int:
					fprintf(fd,"%d", kittyStack[n].integer.value);
					break;
				case type_float:
					fprintf(fd,"%f", kittyStack[n].decimal.value);
					break;
				case type_string:
					if (kittyStack[n].str) fprintf(fd,"%s", &(kittyStack[n].str -> ptr));
					break;
			}

			if (n<=__stack) fprintf(fd,"    ");
		}

		fprintf(fd, "\n");

	}
	popStack(__stack - data -> stack  );
	return NULL;
}

char *_open_file_( struct glueCommands *data, const char *access )
{
	struct stringData *_str;
	int num;
	int args =__stack - data -> stack +1;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 2)
	{
		num = getStackNum(__stack -1 ) -1;

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

			_str = getStackString(__stack );
			if (_str) kittyFiles[ num ].fd = fopen( &_str->ptr, access );

			if (kittyFiles[ num ].fd  == NULL) setError(81,data->tokenBuffer);
		}
	}

	popStack(__stack - data -> stack  );
	return NULL;
}

char *_discOpenOut( struct glueCommands *data, int nextToken )
{
	return _open_file_( data, "w" );
}

char *_discOpenIn( struct glueCommands *data, int nextToken )
{
	return _open_file_( data, "r" );
}

char *_discAppend( struct glueCommands *data, int nextToken )
{
	return _open_file_( data, "a" );
}

char *_discOpenRandom( struct glueCommands *data, int nextToken )
{
	return _open_file_( data, "w+" );
}

char *_discClose( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	int num;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		num = getStackNum(__stack ) -1;

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

	popStack(__stack - data -> stack  );
	return NULL;
}

char *_discKill( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	struct stringData *_str;
	int32 success = false;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args==1)
	{
		_str = getStackString(__stack );
		if (_str) success = Delete( &_str -> ptr );

		if (success == false)
		{
			setError(81,data->tokenBuffer);
		}
	}
	else setError(22,data->tokenBuffer);

	popStack(__stack - data -> stack  );
	return NULL;
}

char *_discRename( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	int32 success = false;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 2)
	{
		struct stringData *oldName = getStackString(__stack - 1 );
		struct stringData *newName = getStackString(__stack );
		if ((oldName)&&(newName))	success = Rename(  &oldName->ptr, &newName->ptr );
	}

	if (success == false)
	{
		setError(81,data->tokenBuffer);
	}

	popStack(__stack - data -> stack  );
	return NULL;
}

char *_discFselStr( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	struct FileRequester	 *filereq;
	struct stringData *ret = NULL;
	char *amigaPattern = NULL;
	char c;
	int l;
	int size;
	bool success = false;
	struct stringData *str = NULL;
	struct stringData *path = NULL;
	struct stringData *pattern = NULL;
	struct stringData *_default_ = NULL;
	struct stringData *_title_ = NULL;
	struct stringData *_title2_ = NULL;
	struct stringData *_title_temp_ = NULL;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (filereq = (struct FileRequester	 *) AllocAslRequest( ASL_FileRequest, TAG_DONE ))
	{
		switch (args)
		{
			case 1:
					str = getStackString(__stack );

					split_path_pattern( str, &path, &pattern );
					amigaPattern = amos_to_amiga_pattern( &(pattern -> ptr) );

					success = AslRequestTags( (void *) filereq, 
						ASLFR_DrawersOnly, FALSE,	
						ASLFR_InitialDrawer, path ? &path -> ptr : "",
						ASLFR_InitialPattern, amigaPattern ? amigaPattern : "",
						ASLFR_DoPatterns, TRUE,
						TAG_DONE );
					break;

			case 3:
					str = getStackString(__stack -2 );

					split_path_pattern( str, &path, &pattern );
					amigaPattern = amos_to_amiga_pattern( &(pattern -> ptr) );

					_default_ = getStackString(__stack -1 );
					_title_ = getStackString(__stack );

					success = AslRequestTags( (void *) filereq, 
						ASLFR_DrawersOnly, FALSE,	
						ASLFR_TitleText, &_title_ -> ptr,
						ASLFR_InitialFile, &_default_ -> ptr,
						ASLFR_InitialDrawer, path ? &path -> ptr : "",
						ASLFR_InitialPattern, amigaPattern ? amigaPattern : "",
						ASLFR_DoPatterns, TRUE,
						TAG_DONE );
					break;

			case 4:
					str = getStackString(__stack -3 );

					split_path_pattern( str, &path, &pattern );
					amigaPattern = amos_to_amiga_pattern( &(pattern -> ptr) );

					_default_ = getStackString(__stack -2 );
					_title_ = getStackString(__stack-1 );
					_title2_ = getStackString(__stack );

					_title_temp_ = alloc_amos_string( _title_ -> size + 1 + _title2_ -> size  );

					if (_title_temp_)
					{
						sprintf(&_title_temp_ -> ptr,"%s\n%s",&_title_ -> ptr ,&_title2_ -> ptr);

						success = AslRequestTags( (void *) filereq, 
							ASLFR_DrawersOnly, FALSE,	
							ASLFR_TitleText, &_title_temp_ -> ptr,
							ASLFR_InitialFile, &_default_ -> ptr,
							ASLFR_InitialPattern, amigaPattern ? amigaPattern  : "",
							ASLFR_DoPatterns, TRUE,
							TAG_DONE );
					}

					break;

		}

		if (path) free(path);
		if (pattern) free(pattern);
		if (amigaPattern) free(amigaPattern);
		path = NULL;
		pattern = NULL;
		amigaPattern = NULL;

		if (success)
		{
			if ((filereq -> fr_File)&&(filereq -> fr_Drawer))
			{
				l = strlen(filereq -> fr_Drawer);

				if (l>1)
				{
					c = filereq -> fr_Drawer[l-1];
					size = strlen(filereq -> fr_Drawer) + strlen(filereq -> fr_File) + (((c == '/') || (c==':')) ? 0 : 1);

					if (ret = (struct stringData *) malloc( sizeof(struct stringData) + size ))
					{
						sprintf( &ret -> ptr, ((c == '/') || (c==':')) ? "%s%s" : "%s/%s",  filereq -> fr_Drawer, filereq -> fr_File ) ;
						ret -> size = size;
					}
				}
				else 	ret = toAmosString(filereq -> fr_File, strlen(filereq -> fr_File));
			}
		}
		 FreeAslRequest( filereq );
	}

	popStack(__stack - data -> stack  );

	if (amigaPattern) free(amigaPattern);

	if (ret) 
	{
		setStackStr(ret);		// we don't need to copy no dup.
	}
	else
	{
		setStackStr(toAmosString("", 0));
	}

	return NULL;
}

char *_discExist( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	struct stringData *_str;
	BPTR lock = 0;
	APTR oldRequest;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args==1)
	{
		_str = getStackString(__stack );

		if (_str -> size == 0) 
		{
			setStackNum( 0 );
			return NULL;
		}

		oldRequest = SetProcWindow((APTR)-1);
		lock = Lock( &_str -> ptr, SHARED_LOCK );
		SetProcWindow(oldRequest);

		if (lock)
		{
			UnLock( lock );
			setStackNum( ~0 ); 
			return NULL;
		}

		setStackNum( 0 );
		return NULL;
	}
	else
	{
		popStack(__stack - data -> stack  );
		setError( 22, data -> tokenBuffer );
	}
	return NULL;
}

char *formated_dir(bool is_dir, struct stringData *path, char *name, int64 size )
{
	char *buffer;
	int _l;
	char c;
	buffer = (char *) malloc( 100 );

	if (buffer)
	{
		if (is_dir)
		{
			sprintf(buffer,"*%-32.32s%10s",  name ,"");
		}
		else
		{
			if (size == -1)	// i have no idea way this has no size :-(
			{
				char *fullname;
				_l = path -> size;
				c = _l ? (&path -> ptr) [_l-1] : 0;

				fullname = (char *) malloc( _l + strlen(name) + 2 ); 
				if (fullname)
				{
					BPTR fd;
					sprintf(fullname, "%s%s%s", &path -> ptr, ((c==':')||(c=='/')) ? "" : ( c==':' ? ":" : "/"), name );

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

			sprintf(buffer," %-32.32s%lld", name, size );
		}
	}

	return buffer;
}


char *dir_item_formated(struct ExamineData *dat, struct stringData *path, const char *pattern)
{
	bool _match = true;
	struct ExamineData *target;
	char *outStr = NULL;

	char matchpattern[100];

	if (pattern)
	{
		if ( pattern[0] )
		{
			ParsePattern( pattern, matchpattern, 100 );
		}
		else
		{
			ParsePattern( "#?", matchpattern, 100 );
		}

		_match = MatchPattern( matchpattern, dat->Name );
	}

	if (_match == false) return NULL;

			if( EXD_IS_LINK(dat) ) /* all links, must check these first ! */
			{
				if( EXD_IS_SOFTLINK(dat) )        /* a FFS style softlink */
				{
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


char *amos_to_amiga_pattern(const char *amosPattern)
{
	int _new_len = 0;
	char *amigaPattern;
	const char *s;
	char *d;
	const char *end_of_path = NULL;

	for (s = amosPattern; *s ;  s++) 
	{
		if ((*s=='/') || (*s==':')) end_of_path=s;
	}

	if (end_of_path) amosPattern=end_of_path+1;

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

void split_path_pattern( struct stringData *str, struct stringData **path, struct stringData **pattern)
{
	bool has_pattern = false;
	int _len;
	char c;
	int i;

	_len = str -> size;
	if (_len>0) 
	{
		c = (&str -> ptr) [_len-1];
		if ((c == '/') || ( c == ':'))
		{
			*path = amos_strdup(str);
			*pattern = toAmosString("",0);
			return;
		}
		else
		{
			*path = NULL;
			for (i=_len-1; i>=0;i--)
			{
				c = (&str->ptr) [i];
		
				if (c=='*') has_pattern = true;

				if ((c == '/') || ( c == ':'))
				{
					if (has_pattern)
					{
						*path = amos_strndup( str, i+1 );
						*pattern = toAmosString( &str-> ptr + i +1, strlen(&str-> ptr + i +1) );
						return;
					}
					else 
					{
						*path = amos_strdup(str);
						*pattern = toAmosString("",0);
						return;
					}
				}
			}
		}		
	}

	*path = toAmosString("",0);
	*pattern = amos_strdup( str );
}


char *_discDir( struct glueCommands *data, int nextToken )
{
	struct stringData *str;
	struct stringData *_path = NULL;
	struct stringData *_pattern = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	str = getStackString(__stack );

	if (str == NULL) return NULL;

	split_path_pattern(str, &_path, &_pattern);
	
	if (_path == NULL) return NULL;

	APTR context = ObtainDirContextTags(EX_StringNameInput, _path ? &_path -> ptr : "",
			EX_DoCurrentDir,TRUE,
			EX_DataFields,(EXF_NAME|EXF_LINK|EXF_TYPE), 
			TAG_END);

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
			if (pattern_match( dat->Name, &_pattern->ptr )) Printf("filename=%s\n", dat->Name);			
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

	popStack(__stack - data -> stack  );

	return  NULL ;
}

char *discDir(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _discDir, tokenBuffer );

	return tokenBuffer;
}

char *_discDirStr( struct glueCommands *data, int nextToken )
{
	int args =__stack - data->stack +1 ;
	BPTR lock;
	BPTR oldLock;
	struct stringData *_str;

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (args == 1)
	{
		_str = getStackString(__stack );

		if (_str)
		{
			lock = Lock( &_str->ptr, SHARED_LOCK );
			if (lock)
			{
				oldLock = SetCurrentDir( lock );
				if (oldLock) UnLock (oldLock );
			}
		}
	}
	else setError(22,data -> tokenBuffer);

	popStack(__stack - data->stack );
	return NULL;
}


char *_set_dir_str( struct glueCommands *data, int nextToken )
{
	struct stringData *_new_path = getStackString(__stack );

	if (_new_path) { chdir( &_new_path->ptr); }

	_do_set = _setVar;
	return NULL;
}


char *discDirStr(struct nativeCommand *cmd, char *tokenBuffer)
{

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( (token_is_fresh) && (NEXT_TOKEN(tokenBuffer) == 0xFFA2 ))
	{
		tokenMode = mode_store;
		_do_set = _set_dir_str;
	}
	else if ( (token_is_fresh) && (NEXT_TOKEN( tokenBuffer ) == 0xFFA2)) 
	{
		printf("%s:%d\n",__FUNCTION__,__LINE__);
		stackCmdNormal( _discDirStr, tokenBuffer );
	}
	else
	{
		char buffer[4000];
		int32 success = NameFromLock(GetCurrentDir(), buffer, sizeof(buffer) );

		if (success)
		{
			struct stringData *ret = toAmosString( buffer, strlen(buffer) );
			setStackStr( ret );
		}
	}

	return tokenBuffer;
}

char *discParent(struct nativeCommand *cmd, char *tokenBuffer)
{
	BPTR lock;
	BPTR oldLock;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	lock = Lock( "/", SHARED_LOCK );
	if (lock)
	{
		oldLock = SetCurrentDir( lock );
		if (oldLock) UnLock (oldLock );
	}

	return tokenBuffer;
}

char *discDfree(struct nativeCommand *cmd, char *tokenBuffer)
{
	struct InfoData info;

	int32 success = GetDiskInfoTags( 
					GDI_LockInput,GetCurrentDir(),
					GDI_InfoData, &info,
					TAG_END);

	if (success)
	{

		uint64_t freeBlocks = (uint64_t) info.id_NumBlocks - (uint64_t) info.id_NumBlocksUsed;
		uint64_t freeBytes = freeBlocks * (uint64_t) info.id_BytesPerBlock;

		// this does not support my disk as it has more then 4 GB free :-/
		// ints are 32bit internaly in AMOS kittens, should bump it up to 64bit, internally.

		if (freeBytes<0x80000000)
		{
			setStackNum( (int) freeBytes ); 
		}
		else
		{
			setStackDecimal( (double) freeBytes );
		}
	}

	return tokenBuffer;
}

char *discKill(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _discKill, tokenBuffer );
	return tokenBuffer;
}

char *discRename(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _discRename, tokenBuffer );
	return tokenBuffer;
}

char *discFselStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _discFselStr, tokenBuffer );
	return tokenBuffer;
}

char *discExist(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _discExist, tokenBuffer );
	return tokenBuffer;
}

char *_discDirFirstStr( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack + 1;
	struct stringData *str = NULL;
	struct stringData *_pattern = NULL;
	stringData *outStr = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			str  = getStackString(__stack );
			if (str == NULL) 
			{
				setError(22, data-> tokenBuffer);
				return NULL;
			}
			break;
		default:
			popStack(__stack - data -> stack  );
			setError(22, data-> tokenBuffer);
			return NULL;
	}

	// delete old context

		if (contextDir)	ReleaseDirContext(contextDir);
		contextDir = NULL;

		if (dir_first_path) free(dir_first_path);
		dir_first_path = NULL;

		if (dir_first_pattern) free(dir_first_pattern);
		dir_first_pattern = NULL;

	// create new context

		split_path_pattern(str, &dir_first_path, &_pattern);
		dir_first_pattern = amos_to_amiga_pattern( (char *) &_pattern-> ptr);

		contextDir = ObtainDirContextTags(EX_StringNameInput, &dir_first_path -> ptr,
	                   EX_DoCurrentDir,TRUE, 
	                   EX_DataFields,(EXF_NAME|EXF_LINK|EXF_TYPE), TAG_END);

	// ready.

	if( contextDir )
	{
		struct ExamineData *dat;

		do
		{
			if ((dat = ExamineDir(contextDir)))  /* until no more data.*/
			{
				char *tmp = dir_item_formated(dat, dir_first_path, dir_first_pattern );
				if (tmp)
				{
					outStr = toAmosString(tmp,strlen(tmp));
					free(tmp);
				}
			}
		} while ((outStr == NULL) && (dat));
	} 
	
	if (outStr)	
	{ 
		setStackStr(outStr);
	}
	else
	{
		setStackStr( toAmosString("",0) );
	}

	return NULL;
}

char *discDirFirstStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _discDirFirstStr, tokenBuffer );
	return tokenBuffer;
}

char *discDirNextStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	struct stringData *outStr = NULL;
	char *tmp;

	if( contextDir )
	{
		struct ExamineData *dat;
		do
		{
			if ((dat = ExamineDir(contextDir)))  /* until no more data.*/
			{
				tmp = dir_item_formated(dat, dir_first_path, dir_first_pattern );
				if (tmp)
				{
					outStr = toAmosString( tmp, strlen(tmp) );
					free(tmp);
				}
			}
		} while ((outStr == NULL) && (dat));
	}
	
	if (outStr)	
	{
		setStackStr(outStr);
	}
	else
	{
		setStackStr(toAmosString("",0));
	}

	return tokenBuffer;
}

void init_dev_first()
{
	char buffer[1000];
	struct DosList *dl;
	ULONG flags;

	flags = LDF_DEVICES|LDF_READ;
	dl = LockDosList(flags);

	devList.clear();

	while(( dl = NextDosEntry(dl,flags) ))
	{
		if (dl -> dol_Port)
		{
			if (DevNameFromPort(dl -> dol_Port,  buffer, sizeof(buffer), TRUE))
			{
				devList.push_back(buffer);
			}
		}
	}

	UnLockDosList(flags);

//	for (n=0;n<devList.size();n++) printf( "%s\n",devList[n].c_str() );
}


unsigned int dev_index = 0;

char *_discDevFirstStr( struct glueCommands *data, int nextToken )
{
	dev_index = 0;
	init_dev_first();

	popStack(__stack - data -> stack  );

	if (devList.size())
	{
		struct stringData *str = toAmosString( devList[0].c_str(),devList[0].length());
		setStackStr( str );
		dev_index++;
	}
	else
	{
		setStackStr( NULL );
	}

	return NULL;
}

char *discDevFirstStr(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdParm( _discDevFirstStr, tokenBuffer );
	return tokenBuffer;
}


char *discDevNextStr(struct nativeCommand *cmd, char *tokenBuffer)
{

	if (dev_index<devList.size())
	{
		char *buf;
		struct stringData *str = NULL; 

		buf = (char *) malloc( 1 + devList[dev_index].length() + 30 + 1 );

		if (buf)
		{
			sprintf( buf, " %s", devList[dev_index].c_str());
			str = toAmosString( buf, strlen(buf) );
			free(buf);
			buf = NULL;
		}

		setStackStr( str );
		dev_index++;
	}
	else
	{
		setStackStr( toAmosString( "",0) ) ;
		devList.clear();
		dev_index = 0;
	}

	return tokenBuffer;
}


char *discSetDir(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _discSetDir, tokenBuffer );
	return tokenBuffer;
}

char *discPrintOut(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _discPrintOut, tokenBuffer );
	return tokenBuffer;
}

char *discOpenIn(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _discOpenIn, tokenBuffer );
	return tokenBuffer;
}

char *discOpenOut(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _discOpenOut, tokenBuffer );
	return tokenBuffer;
}

char *discAppend(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _discAppend, tokenBuffer );
	return tokenBuffer;
}

char *discClose(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _discClose, tokenBuffer );
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
		if (__cmdStack > 0)
		{
			if (cmdTmp[__cmdStack-1].cmd == _discInputIn) valid = true;
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
					if (ret==1) 
					{
						struct stringData *str = toAmosString( buffer,strlen(buffer) );
						setStackStr( str );
					}
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

	proc_names_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (cmd == NULL)
	{
		valid = true;
	}
	else	// if file_input is called from ","
	{
		if (__cmdStack > 0)
		{
			if (cmdTmp[__cmdStack-1].cmd == _discLineInputFile) valid = true;
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
			size_t len = 0;

			getline( &line, &len, fd );

			if (line) 
			{
				struct stringData *str = toAmosString( line, strlen(line) );
				setStackStr( str );		
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


char *_discInputIn( struct glueCommands *data, int nextToken )
{
	if (do_input[parenthesis_count]) do_input[parenthesis_count]( NULL, NULL );
	do_input[parenthesis_count] = do_std_next_arg;

	popStack(__stack - data -> stack  );

	return NULL;
}

char *discInputIn(struct nativeCommand *cmd, char *tokenBuffer)
{
	if (NEXT_TOKEN( tokenBuffer ) == 0x003E)
	{
		input_cmd_context.cmd = _discInputIn;
		input_cmd_context.stack = __stack;
		input_cmd_context.lastVar = *((int *) (tokenBuffer + 2));
		input_cmd_context.tokenBuffer = tokenBuffer;
		stackCmdNormal( _discInputIn, tokenBuffer );

		do_input[parenthesis_count] = file_input;

		tokenBuffer += 6;

		if (NEXT_TOKEN( tokenBuffer ) == 0x005C) tokenBuffer += 2;

	}
	else
	{
		setError(125,tokenBuffer);
	}

	return tokenBuffer;
}

char *_discLineInputFile( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (do_input[parenthesis_count]) do_input[parenthesis_count]( NULL, NULL );
	do_input[parenthesis_count] = do_std_next_arg;

	popStack(__stack - data -> stack  );

	return NULL;
}

char *discLineInputFile(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (NEXT_TOKEN( tokenBuffer ) == 0x003E)
	{
		input_cmd_context.cmd = _discLineInputFile;
		input_cmd_context.stack = __stack;
		input_cmd_context.lastVar = *((int *) (tokenBuffer + 2));
		input_cmd_context.tokenBuffer = tokenBuffer;

		stackCmdNormal( _discLineInputFile, tokenBuffer );

		do_input[parenthesis_count] = file_line_input;

		tokenBuffer += 6;

		if (NEXT_TOKEN( tokenBuffer ) == 0x005C) tokenBuffer += 2;
	}
	else
	{
		setError(23,tokenBuffer);
	}

	return tokenBuffer;
}

char *_discInputStrFile( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack;
	int channel = 0;
	int len = 0;
	struct stringData *newstr;
	FILE *fd;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 2)
	{
		channel = getStackNum(__stack - 1 );
		len = getStackNum(__stack );

		if (( channel >0)&&( channel <11))
		{
			fd = kittyFiles[ channel -1 ].fd ;

			if (fd)
			{
				popStack(__stack - data -> stack  );
				
				newstr = (struct stringData *) malloc( sizeof(struct stringData) + len +1);

				if (newstr)	 if (fgets( &newstr -> ptr, len ,fd ))
				{
					popStack(__stack - data -> stack  );
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

	popStack(__stack - data -> stack  );
	return NULL;
}

char *_discSetInput( struct glueCommands *data, int nextToken )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	dump_stack();

	popStack(__stack - data -> stack  );
	return NULL;
}

char *_discLof( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack + 1;
	int channel = 0;
	FILE *fd;
	int pos,len = 0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		channel = getStackNum(__stack);

		if (( channel >0)&&( channel <11))
		{
			fd = kittyFiles[ channel -1 ].fd ;

			if (fd)
			{
				pos = ftell( fd );
				fseek( fd, 0, SEEK_END );
				len = ftell(fd);
				fseek( fd, pos, SEEK_SET );
			}
			else	setError(97,data->tokenBuffer); // file not open
		}
		else	setError(23,data->tokenBuffer);	// "Illegal function call"
	}

	popStack(__stack - data -> stack  );
	setStackNum( len );

	return NULL;
}

char *_discPof( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack + 1;
	int channel = 0;
	FILE *fd;
	int ret =0;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		channel = getStackNum(__stack);

		if (( channel >0)&&( channel <11))
		{
			fd = kittyFiles[ channel -1 ].fd ;

			if (fd)
			{
				ret = ftell( fd );
			}
			else	setError(97,data->tokenBuffer); // file not open
		}
		else	setError(23,data->tokenBuffer);	// "Illegal function call"
	}

	popStack(__stack - data -> stack  );
	setStackNum( ret );

	dump_stack();
	
	return NULL;
}

char *_discEof( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;	
	int channel = 0;
	FILE *fd;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 1)
	{
		channel = getStackNum(__stack);

		dprintf("channel: %d\n",channel);

		if (( channel >0)&&( channel <11))
		{
			fd = kittyFiles[ channel -1 ].fd ;

			if (fd)
			{
				popStack(__stack - data -> stack  );
				setStackNum( feof( fd ));
				return NULL;
			}
			else	setError(97,data->tokenBuffer); // file not open
		}
		else	setError(23,data->tokenBuffer);	// "Illegal function call"
	}

	popStack(__stack - data -> stack  );
	return NULL;
}


char *discInputStrFile(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _discInputStrFile, tokenBuffer );
	return tokenBuffer;
}

char *discSetInput(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _discSetInput, tokenBuffer );
	return tokenBuffer;
}

char *discLof(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _discLof, tokenBuffer );
	return tokenBuffer;
}

char *discPof(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _discPof, tokenBuffer );
	return tokenBuffer;
}

char *discEof(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _discEof, tokenBuffer );
	return tokenBuffer;
}

char *_discGet( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	int channel = 0;
	int n, index;
	struct kittyField *fields = NULL;
	FILE *fd;
	struct stringData *str;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (args == 2)
	{
		channel = getStackNum(__stack -1 ) ;
		index = getStackNum(__stack ) ;

		if ((channel>0)&&(channel<11) && (index>0))
		{
			fields = kittyFiles[channel-1].fields ;
			fd = kittyFiles[channel-1].fd ;

			printf("Seek to %d\n",(index -1) * kittyFiles[channel-1].fieldsSize);

			fseek( fd, (index -1) * kittyFiles[channel-1].fieldsSize , SEEK_SET);

			for (n=0; n<kittyFiles[channel-1].fieldsCount;n++)
			{
				if (globalVars[ fields -> ref -1 ].var.str) free(globalVars[ fields -> ref -1 ].var.str);

				str = (struct stringData *) malloc( sizeof(struct stringData) + fields -> size + 1);

				fgets( &str -> ptr ,fields -> size+1,fd);

				str -> size = strlen(&str -> ptr);

				if (globalVars[ fields -> ref -1 ].var.str) free(globalVars[ fields -> ref -1 ].var.str);
				globalVars[ fields -> ref -1 ].var.str = str;
				
				fields ++;
			}
		}
	}

	popStack(__stack - data -> stack  );
	return NULL;
}

bool write_file_start_end( int channel, char *start, char *end )
{
	printf("channel %d\n",channel);

	if ((channel>0)&&(channel<11))
	{
		FILE *fd = kittyFiles[channel-1].fd ;
		printf("channel\n");

		if (fd)
		{
			printf("is open\n");

			printf("fwrite( %0lx, (uint32_t) %0lx - (uint32_t) %0lx,  1,  fd );\n", start , end, start );
			printf("fwrite( %0lx, %d, %d, %0lx)\n", start, (uint32_t) end - (uint32_t) start, 1, fd );

			fwrite( start, (uint32_t) end - (uint32_t) start,  1,  fd );
			return true;
		}
	}
	return false;
}

char *_discPut( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	int channel = 0;
	int n, index;
	struct kittyField *fields = NULL;
	FILE *fd;
	char fmt[15];
	char tmp[1000];

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	dump_stack();

	if (args == 2)
	{
		channel = getStackNum(__stack -1 ) ;
		index = getStackNum(__stack ) ;

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

	popStack(__stack - data -> stack  );
	return NULL;
}

char *discOpenRandom(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _discOpenRandom, tokenBuffer );
	return tokenBuffer;
}

char *discField(struct nativeCommand *cmd, char *ptr)
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

		token = *( (short *) ptr);
		ptr += 2;

	} while ((token != 0) && (token != 0x0054 ));

	if (channel) 
	{
		kittyFiles[channel-1].fieldsCount = count ;
		kittyFiles[channel-1].fieldsSize = size ;
	}

	return ptr - 2;
}

char *discGet(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _discGet, tokenBuffer );
	return tokenBuffer;
}

char *discPut(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _discPut, tokenBuffer );
	return tokenBuffer;
}

char *_discMakedir( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	BPTR lock = 0;

	if (args==1)
	{
		struct stringData *_str = getStackString(__stack );

		lock = CreateDir( &_str->ptr );
		if (lock) 
		{
			UnLock( lock );
		}
		else setError( 22, data -> tokenBuffer );
	}
	else setError( 22, data -> tokenBuffer );

	popStack(__stack - data -> stack  );
	return NULL;
}

char *discMakedir(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _discMakedir, tokenBuffer );
	return tokenBuffer;
}

char *_discAssign( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	bool success = false;

	if (args==2)
	{
		struct stringData *_volume = getStackString(__stack-1 );
		struct stringData *_path = getStackString(__stack );

		if ((_volume)&&(_path))
		{
			int vl = _volume -> size;
			if (vl)	 if ( (&_volume -> ptr)[ vl -1 ]==':') (&_volume -> ptr)[ vl -1 ]=0 ;

			if (AssignLate( &_volume->ptr,&_path->ptr)) success = true;
		}
	}

	if (success == false )setError( 22, data -> tokenBuffer );

	popStack(__stack - data -> stack  );
	return NULL;
}

char *discAssign(struct nativeCommand *cmd, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _discAssign, tokenBuffer );
	return tokenBuffer;
}

char *_discReadText( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	NYI(__FUNCTION__);

	popStack(__stack - data -> stack  );
	return NULL;
}

char *discReadText(struct nativeCommand *disc, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _discReadText, tokenBuffer );
	return tokenBuffer;
}


char *findVolumeName(char *name)
{
	char deviceName[30];
	struct Node *nd;
	char *ret = NULL;
	BPTR l;

	struct List *list = (struct List *) AllocDosObjectTags( DOS_VOLUMELIST,
	                             ADO_Type,LDF_VOLUMES,
	                             ADO_AddColon,TRUE, TAG_END);
	if( list )
	{
		int32 success;
		for( nd = GetHead(list); nd; nd = GetSucc(nd))
		{
			success = FALSE;
			l = Lock(nd->ln_Name,SHARED_LOCK);

			if (l)
			{
				success = DevNameFromLock(l, deviceName, sizeof(deviceName), DN_DEVICEONLY);
				UnLock(l);
			}

			if (success)
			{
				if ((strcasecmp(name, nd->ln_Name) == 0)|| (strcasecmp(name,deviceName) == 0))
				{
					ret = strdup( nd->ln_Name );	
					break;
				}
			}
			else
			{
				if ( strcasecmp(name,deviceName) == 0 ) 
				{
					ret = strdup( nd->ln_Name );	
					break;
				}
			}
		}
		FreeDosObject(DOS_VOLUMELIST,list);
	}

	return ret;
}


char *_cmdDiskInfoStr( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;

	struct stringData *path;
	char  *volumeName;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

//	NYI(__FUNCTION__);

	switch (args)
	{
		case 1:
			path = getStackString(__stack );
			volumeName = path ? findVolumeName( &path -> ptr ) : NULL;

			if (volumeName)
			{
				int result = -1;
				struct InfoData info;

				int32 success;
				APTR oldRequest;

//				printf("volumeName: %s\n",volumeName);

				oldRequest = SetProcWindow(NULL);

				result = GetDiskInfoTags( 
						GDI_StringNameInput, volumeName,
						GDI_InfoData, &info, TAG_END );

				SetProcWindow(oldRequest);
				
				if (result)
				{
					struct stringData *ret = alloc_amos_string( strlen(volumeName) +30 );
					if (ret)
					{
						uint64_t freeBlocks = (uint64_t) info.id_NumBlocks - (uint64_t) info.id_NumBlocksUsed;
						uint64_t freeBytes = freeBlocks * (uint64_t) info.id_BytesPerBlock;

						sprintf(&(ret->ptr),"%s%lld",volumeName, freeBytes);
						ret -> size = strlen( &(ret -> ptr) );
						setStackStr(ret);
						return NULL;
					}
				}
			}

			{
				struct stringData ret;
				ret.ptr =0;
				ret.size = 0;
				setStackStrDup(&ret);
			}

			break;
		default:
			popStack(__stack - data -> stack );
			setError(23,data-> tokenBuffer);
			break;
	}

	return NULL;
}

char *cmdDiskInfoStr(struct nativeCommand *disc, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _cmdDiskInfoStr, tokenBuffer );
	return tokenBuffer;
}


extern char *_file_end_;

char *_discRun( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	struct stringData *filename;
	char *newCmd = NULL;
	BPTR l = 0;
	char *progpath = NULL;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);


	switch (args)
	{
		case 1:
			filename = getStackString(__stack );

			l = Lock("PROGDIR:",SHARED_LOCK);

			progpath = (char *) malloc(1000);

			if ((l)&&(progpath))
			{
				NameFromLock(l,progpath,1000);

				newCmd = (char *) malloc( strlen(progpath) + filename -> size + 20 );

				if (newCmd)
				{
					sprintf(newCmd,"%s/amosKittens.exe %c%s%c",progpath, 34,&filename -> ptr,34);
					printf("%s\n",newCmd);

					SystemTags(newCmd, 
//						SYS_Input, NULL,
//						SYS_Output, NULL,
//						SYS_Asynch,TRUE, 
						TAG_END);
				}
			}

			if (l)	UnLock(l);
			if (progpath) free(progpath);
			if (newCmd) free(newCmd);

			break;

		default:
			popStack(__stack - data -> stack );
			setError(23,data-> tokenBuffer);
			break;
	}

	return _file_end_;
}

char *discRun(struct nativeCommand *disc, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdNormal( _discRun, tokenBuffer );
	return tokenBuffer;
}

//---

int have_drive( struct stringData *name)
{
	char buffer[256];
	struct DosList *dl;
	ULONG flags;

	flags = LDF_DEVICES|LDF_READ;
	dl = LockDosList(flags);

	while(( dl = NextDosEntry(dl,flags) ))
	{
		if (dl -> dol_Port)
		{
			if (DevNameFromPort(dl -> dol_Port,  buffer, sizeof(buffer), TRUE))
			{
				if (strcasecmp( &name -> ptr , buffer ) == 0 )
				{
					UnLockDosList(flags);
					return -1;
				}
			}
		}
	}

	UnLockDosList(flags);
	return 0;
}


char *_discDrive( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
	struct stringData *volumeName;
	int ret = 0;
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			volumeName = getStackString(__stack );
			if (volumeName)
			{
				ret = have_drive( volumeName );
			}
			setStackNum(ret);

			break;
		default:
			popStack(__stack - data -> stack );
			setError(23,data-> tokenBuffer);
			break;
	}

	return NULL;
}

char *discDrive(struct nativeCommand *disc, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _discDrive, tokenBuffer );
	return tokenBuffer;
}

char *_discPort( struct glueCommands *data, int nextToken )
{
	int args =__stack - data -> stack +1;
//	struct stringData *volumeName;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	switch (args)
	{
		case 1:
			setStackNum(-1);	// always true (fix me if I'm wrong)
			break;
		default:
			popStack(__stack - data -> stack );
			setError(23,data-> tokenBuffer);
			break;
	}

	return NULL;
}

char *discPort(struct nativeCommand *disc, char *tokenBuffer)
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	stackCmdParm( _discPort, tokenBuffer );
	return tokenBuffer;
}
