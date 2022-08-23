
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void decode(const char *txt)
{
	const char *t;
	for (t=txt;*t;t++)
	{
		printf("'%c' %d\n",*t,*t);
	}
}

char symb1[]={226,128,153};
char symb2[]={195,167};

char squote[]={226,128,156};
char equote[]={226,128,157};

void fix_symb( char *txt, char *symb, int symblen, char nchar)
{
	char *e = txt + strlen(txt);
	char *s = txt;
	char *d = txt;

	const char *t;
	for (s=txt;*s;s++,d++)
	{
		if ((e-s)>=symblen)
		{
			if (memcmp(s,symb,symblen)==0)
			{
				s+=symblen-1;
				*d = nchar;
			}
			else *d=*s;
		}
		else *d=*s;
	}
	*d=0;
}


int main(int args, const char **arg)
{
	FILE *fd;
	if (args != 2) return 0;

	fd = fopen(arg[1],"r");
	if (fd)
	{
		char buffer[10000];
		char *l;
		size_t len;

		if (!feof(fd))
		{
			do
			{
				l = fgets( buffer, sizeof(buffer), fd  );

//				if (l) decode(l);
				if (l) fix_symb(l, symb1,sizeof(symb1),'\'');
				if (l) fix_symb(l, symb2,sizeof(symb2),'c');
				if (l) fix_symb(l, squote,sizeof(squote),'\"');
				if (l) fix_symb(l, equote,sizeof(equote),'\"');
				if (l) printf("%s",l);


			} while (l);
		}
		fclose(fd);
	}

	return 0;
}

