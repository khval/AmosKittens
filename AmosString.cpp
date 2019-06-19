
#include <stdlib.h>
#include <stdio.h>
#include "amosString.h"

struct stringData *alloc_amos_string( int size )
{
	struct stringData *newstr = (struct stringData *) malloc( sizeof(struct stringData) + size ); 
	newstr -> size = size;
	(&newstr -> ptr)[size]=0;	// unlike AmosPro, Amos kitten strings should 0 terminaled, so they will work with standard OS libraryes and C libs.

	return newstr;
}

