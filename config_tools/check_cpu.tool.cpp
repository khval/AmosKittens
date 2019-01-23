

#include <stdio.h>
#include <stdlib.h>

int main(int args,char **arg)
{
	int value = 1;

	if ( *((char *) &value) == 1 )
	{
		printf("#define __LITTLE_ENDIAN__\n");
	}
}