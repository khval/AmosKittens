
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "amoskittens.h"
#include "amosString.h"
#include "amosstring.h"
#include "debug.h"

struct stringData *alloc_amos_string( int size )
{
	struct stringData *newstr = (struct stringData *) malloc( sizeof(struct stringData) + size ); 

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (newstr)
	{
		newstr -> size = size;
		(&newstr -> ptr)[size]=0;	// unlike AmosPro, Amos kitten strings be should 0 terminaled, so they will work with standard OS libraryes and C libs.
	}
	return newstr;
}

struct stringData *amos_strdup( struct stringData *var )
{
	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	struct stringData *newstr = (struct stringData *) malloc( sizeof(struct stringData) + var -> size ); 

	if (newstr)
	{
		newstr -> size = var -> size;
		(&newstr -> ptr)[var -> size]=0;	// unlike AmosPro, Amos kitten strings should be 0 terminaled, so they will work with standard OS libraryes and C libs.
		memcpy(&(newstr -> ptr),&(var -> ptr),var->size);
		return newstr;
	}
	else
	{
		printf("%s:%s:%d -> failed\n",__FILE__,__FUNCTION__,__LINE__);
	}

	return NULL;
}

struct stringData *amos_strndup( struct stringData *var, int len )
{
	struct stringData *newstr;
	if (var->size<len) len = var->size;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	newstr = (struct stringData *) malloc( sizeof(struct stringData) + len ); 

	if (newstr)
	{
		newstr -> size = len;
		(&newstr -> ptr)[len]=0;	// unlike AmosPro, Amos kitten strings should be 0 terminaled, so they will work with standard OS libraryes and C libs.
		memcpy(&(newstr -> ptr),&(var -> ptr),len);
	}
	return newstr;
}

struct stringData *amos_mid( struct stringData *string, int start, int len )
{
	struct stringData *newstr;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if ( start >= string->size ) start = string->size;
	if ( (string->size - start) <len) len = (string->size - start);
	if (len<0) len =0;

	newstr = (struct stringData *) malloc( sizeof(struct stringData) + len ); 
	newstr -> size = len;

	if (len>0)
	{
		memcpy(&(newstr->ptr),&(string->ptr)+start,len);
		(&newstr->ptr)[len]=0;	
	}
	return newstr;
}

struct stringData *amos_right( struct stringData *var, int len )
{
	struct stringData *newstr;
	if (var->size<len) len = var->size;
	newstr = (struct stringData *) malloc( sizeof(struct stringData) + len ); 

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (newstr)
	{
		newstr -> size = len;
		(&newstr -> ptr)[len]=0;	// unlike AmosPro, Amos kitten strings should be 0 terminaled, so they will work with standard OS libraryes and C libs.
		memcpy(&(newstr -> ptr),&(var -> ptr) + (var -> size - len),len);
	}
	return newstr;
}

int amos_instr( struct stringData *string,int start,struct stringData *find  )
{
	char *p = &(string -> ptr) + start;
	int l; 
	int n;

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (string -> size < find -> size) return 0;

	l = string -> size - start - (find -> size -1);
	for (n=0;n<l;n++)
	{
		if (memcmp(p+n,&(find -> ptr),find -> size)==0)
		{
			return n+start+1;
		}
	}

	return 0;
}

struct stringData *toAmosString( const char *txt,int len)
{
	struct stringData *newstr;
	int _l = strlen(txt);

	proc_names_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (_l<len) len = _l;

	newstr = (struct stringData *) malloc( sizeof(struct stringData) + len ); 
	if (newstr)
	{
		newstr -> size = len;
		(&newstr -> ptr)[len]=0;	// unlike AmosPro, Amos kitten strings should be 0 terminaled, so they will work with standard OS libraryes and C libs.
		memcpy(&(newstr -> ptr),txt,len);
	}
	return newstr;
}


