
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
	(&newstr -> ptr)[size]=0;	// unlike AmosPro, Amos kitten strings should 0 terminaled, so they will work with standard OS libraryes and C libs.

	return newstr;
}

struct stringData *amos_strdup( struct stringData *var )
{
}

struct stringData *amos_strndup( struct stringData *var, int len )
{
}

struct stringData *amos_mid( struct stringData *var, int start, int size )
{
}

struct stringData *amos_right( struct stringData *var, int size )
{
}

struct stringData *amos_instr( struct stringData *var,int start,struct stringData *find  )
{
}

struct stringData *toAmosString( const char *txt,int len)
{
}




