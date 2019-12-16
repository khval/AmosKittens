
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "debug.h"
#include "amosstring.h"

// this functions are used for testing the Amal compiler.

int bobCol( unsigned short bob, unsigned short start, unsigned short end )
{
	AmalPrintf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);
	return 0;
}

