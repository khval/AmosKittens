
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <string>
#include <iostream>
#include <proto/asl.h>

#include "stack.h"
#include "amosKittens.h"
#include "commandsDisc.h"
#include "debug.h"
#include "errors.h"

extern int last_var;
extern struct globalVar globalVars[];
extern unsigned short last_token;
extern int tokenMode;

char *amos_to_amiga_pattern(const char *amosPattern);

char *_cmdSetDir( struct glueCommands *data )
{
	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}


char *_cmdKill( struct glueCommands *data )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;
	char *_str;
	int32 success = false;
	_str = _stackString( stack );

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (_str) success = Delete(_str);

	if (success == false)
	{
		setError(81);
	}

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_cmdRename( struct glueCommands *data )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;
	int32 success = false;

	if (args == 2)
	{
		char *oldName = _stackString( stack - 1 );
		char *newName = _stackString( stack );
		if ((oldName)&&(newName))	success = Rename(  oldName, newName );
	}

	if (success == false)
	{
		setError(81);
	}

	popStack( stack - cmdTmp[cmdStack].stack  );
	return NULL;
}

char *_cmdFselStr( struct glueCommands *data )
{
	int args = stack - cmdTmp[cmdStack].stack ;
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
					_path_ = _stackString( stack -2 );
					_default_ = _stackString( stack -1 );
					_title_ = _stackString( stack );

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

char *_cmdExist( struct glueCommands *data )
{
	int args = stack - cmdTmp[cmdStack-1].stack +1;
	char *_str;
	BPTR lock = 0;

	_str = _stackString( stack );

	if (args==1)
	{
		lock = Lock( _str, SHARED_LOCK );

		if (lock)
		{
			UnLock( lock );
			_num( true ); 
		}
	}

	if (!lock) _num( false );

	popStack( stack - cmdTmp[cmdStack-1].stack  );
	return NULL;
}

char *_cmdDirFirstStr( struct glueCommands *data )
{
	popStack( stack - cmdTmp[cmdStack-1].stack  );
	return NULL;
}

char *_cmdDirNextStr( struct glueCommands *data )
{
	popStack( stack - cmdTmp[cmdStack-1].stack  );
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

char *_cmdDir( struct glueCommands *data )
{
	char *_path = NULL;
	const char *_pattern = NULL;
	int i;
	int _len;
	char c;
	char *str;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	str = _stackString( stack );

	if (str == NULL) return NULL;

	_len = strlen( str );
	
	printf("%s:%d\n",__FUNCTION__,__LINE__);


	if (_len>0) 
	{
		c = str[_len-1];

		if ((c == '/') || ( c == ':'))
		{
			_path = strdup(str);
		}
		else
		{
			for (i=_len-1; i>=0;i--)
			{
				c = str[i];

				if ((c == '/') || ( c == ':'))
				{
					_path = strndup( str, i+1 );
					_pattern = str + i +1;
				}
			}
		}		
	}


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

	return  data -> tokenBuffer ;
}


char *_cmdDirStr( struct glueCommands *data )
{
	int args = stack - data->stack +1 ;
	BPTR lock;
	BPTR oldLock;
	char *_str;

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	_str = _stackString( stack );

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


char *cmdDir(struct nativeCommand *cmd, char *tokenBuffer)
{
	stackCmdNormal( _cmdDir, tokenBuffer );

	return tokenBuffer;
}

char *cmdDirStr(struct nativeCommand *cmd, char *tokenBuffer)
{

	printf("%s:%d\n",__FUNCTION__,__LINE__);

	printf("%08x - %08x\n", last_token, NEXT_TOKEN( tokenBuffer ) );

	if (last_token == 0x0000) printf("last token 0x0000\n");
	if (NEXT_TOKEN( tokenBuffer ) == 0xFFA2) printf("next token 0xFFA2\n");

	if ( ((last_token == 0x0000) || (last_token == 0x0054)) && (NEXT_TOKEN( tokenBuffer ) == 0xFFA2)) 
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

	printf("%s:%d\n",__FUNCTION__,__LINE__);

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

		printf("num blocks %d\n", data.id_NumBlocks);
		printf("used blocks %d\n", data.id_NumBlocksUsed);
		printf("bytes per block %d\n", data.id_BytesPerBlock);


		freeBlocks = data.id_NumBlocks - data.id_NumBlocksUsed;

		// this does not support my disk as it has more then 4 GB free :-/
		// ints are 32bit internaly in AMOS kittens, should bump it up to 64bit, internally.

		_num( freeBlocks * data.id_BytesPerBlock ); 

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


