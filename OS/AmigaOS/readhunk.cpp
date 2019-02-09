#include <stdlib.h>
#include <stdio.h>


char *read_string(FILE *fd)
{

	int numLongs;
	char *txt;

	fread( &numLongs, sizeof(int),1,fd );

	if (numLongs<1) return NULL;

	txt = (char *) malloc( (numLongs*4) + 1 );
	if (txt)
	{
		fread( txt, (numLongs*4), 1, fd );
		return txt;
	}

	return NULL;
}


void readHeader(FILE *fd)
{
	char *txt;
	int tableSize;
	int firstHunkSlot;
	int lastHunkSlot;
	int n;
	int i32;
	int numberOfHunks;

	// read strings
	do
	{
		txt = read_string(fd);
		if (txt)
		{
			printf("%s\n",txt);
			free(txt);
		}
	} while (txt != NULL);

	fread(&tableSize,sizeof(int),1,fd);
	fread(&firstHunkSlot,sizeof(int),1,fd);
	fread(&lastHunkSlot,sizeof(int),1,fd);

	numberOfHunks =lastHunkSlot - firstHunkSlot + 1;
	
	printf("numberOfHunks %d\n",numberOfHunks);

	for (n=0;n<numberOfHunks;n++)
	{
		fread(&i32,sizeof(int),1,fd);
		printf("size: %d\n",i32 * 4);
	}
}

void readCodeBlock(FILE *fd, char **keep_code, int *code_size)
{
	unsigned int size;
	char *code;

	fread(&size,sizeof(int),1,fd);
	printf("size: %d\n",size * 4);

	code = (char *) malloc(size * 4) ;
	if (code)
	{
		fread(code,size*4,1,fd);

		if (keep_code)
		{
			if (*keep_code == NULL) 	// we only get first code block.
			{
				*keep_code = code;
				*code_size = size*4;
			}
		}
		else 	free(code);
	}
}

void readReloc32(FILE *fd)
{
	unsigned int num_offsets;
	unsigned int hunk_number;
	unsigned int skip;
	unsigned int n;

	while (true)
	{
		fread(&num_offsets,sizeof(int),1,fd);
		if (num_offsets == 0) break;
		fread(&hunk_number,sizeof(int),1,fd);

		for (n=0; n<num_offsets;n++)	fread(&skip,sizeof(int),1,fd);
	}
}

#define HUNK_HEADER	0x3F3
#define HUNK_RELOC32 0x3EC
#define HUNK_CODE	0x3E9
#define HUNK_END		0x3F2

int readhunk(const char *name, char **keep_code, int *code_size)
{
	FILE *fd;
	unsigned int hunkid;
	bool _error = false;
	bool _exit = false;

	fd = fopen( name ,"r" );
	if (fd)
	{
		while( !feof(fd) && !_error && !_exit )
		{
			fread( &hunkid, sizeof(int),1,fd );
			switch (hunkid)
			{
				case HUNK_HEADER:
					printf("block header\n");
					readHeader(fd);
					break;

				case HUNK_CODE:
					printf("HUNK Code\n");
					readCodeBlock(fd, keep_code, code_size);
					break;

				case HUNK_RELOC32:
					printf("HUNK RELOC32\n");
					readReloc32(fd);
					break;

				case HUNK_END:
					printf("HUNK End\n");
					break;

				default:
					printf("hunk id %04x not found\n",hunkid);
					_error = true;
					break;
			}
		}

		fclose(fd);
	}
}


