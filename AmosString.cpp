
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "amoskittens.h"
#include "amosString.h"
#include "amosstring.h"

struct stringData *alloc_amos_string( int size )
{
	struct stringData *newstr = (struct stringData *) malloc( sizeof(struct stringData) + size ); 
	newstr -> size = size;
	(&newstr -> ptr)[size]=0;	// unlike AmosPro, Amos kitten strings be should 0 terminaled, so they will work with standard OS libraryes and C libs.
	return newstr;
}

struct stringData *amos_strdup( struct stringData *var )
{
	struct stringData *newstr = (struct stringData *) malloc( sizeof(struct stringData) + var -> size ); 
	newstr -> size = var -> size;
	(&newstr -> ptr)[var -> size]=0;	// unlike AmosPro, Amos kitten strings should be 0 terminaled, so they will work with standard OS libraryes and C libs.
	memcpy(&(newstr -> ptr),&(var -> ptr),var->size);
	return newstr;
}

struct stringData *amos_strndup( struct stringData *var, int len )
{
	struct stringData *newstr;
	if (var->size<len) len = var->size;
	newstr = (struct stringData *) malloc( sizeof(struct stringData) + len ); 
	newstr -> size = len;
	(&newstr -> ptr)[len]=0;	// unlike AmosPro, Amos kitten strings should be 0 terminaled, so they will work with standard OS libraryes and C libs.
	memcpy(&(newstr -> ptr),&(var -> ptr),len);
	return newstr;
}

struct stringData *amos_mid( struct stringData *string, int start, int len )
{
	struct stringData *newstr;

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
	newstr -> size = len;
	(&newstr -> ptr)[len]=0;	// unlike AmosPro, Amos kitten strings should be 0 terminaled, so they will work with standard OS libraryes and C libs.
	memcpy(&(newstr -> ptr),&(var -> ptr) + (var -> size - len),len);
	return newstr;
}

int amos_instr( struct stringData *var,int start,struct stringData *find  )
{
	char *p = &(var -> ptr) + start;
	int l; 
	int n;

	if (var -> size < find -> size) return 0;

	l = var -> size - start - find -> size;
	for (n=0;n<l;n++)
	{
		if (strncmp(p+n,&(find -> ptr),find -> size))
		{
			return n+1;
		}
	}

	return 0;
}

struct stringData *toAmosString( const char *txt,int len)
{
	struct stringData *newstr;
	int _l = strlen(txt);
	if (_l<len) len = _l;
	newstr = (struct stringData *) malloc( sizeof(struct stringData) + len ); 
	newstr -> size = len;
	(&newstr -> ptr)[len]=0;	// unlike AmosPro, Amos kitten strings should be 0 terminaled, so they will work with standard OS libraryes and C libs.
	memcpy(&(newstr -> ptr),txt,len);
	return newstr;
}


