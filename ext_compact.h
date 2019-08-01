#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#endif
#include "amosKittens.h"

struct PacPicContext
{
	// for when uncompressing or when compressing.

	int w;
	int h;
	int ll;
	int d;
	unsigned char *raw;
	unsigned short mode;

	// when compressing data

	unsigned char *data;
	unsigned char *rledata;
	unsigned char *points;
	
	int data_used;
	int rledata_used;
	int points_used;

	unsigned char rrle;
	unsigned char last_rle;
	bool first_rle;
	bool ready_to_encode;
};

extern char *ext_cmd_unpack(nativeCommand *cmd, char *ptr);
extern char *ext_cmd_spack(nativeCommand *cmd, char *tokenBuffer);

