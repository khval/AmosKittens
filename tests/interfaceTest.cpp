
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <math.h>
#include <signal.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <libraries/retroMode.h>
#include <proto/retroMode.h>
#endif

#include "config.h"
#include "amosString.h"
#include <amosKittens.h>
#include "interfacelanguage.h"

struct KittyInstance instance;
int breakpoint;

struct Library 			*RetroModeBase = NULL;
struct RetroModeIFace 	*IRetroMode = NULL;

struct Library			*kittyCompactBase = NULL;
struct kittyCompactIFace	*IkittyCompact = NULL;

extern void draw_HyperText(struct zone_hypertext *zh);

struct TextFont *topaz8_font = NULL;

int main(int args, char **arg)
{
	int n;
	struct cmdcontext context;

	const char *script =    "IF     0VA 1=;" 
   "[" 
   "SIze   1VATW160+ SW MI,40;" 
   "BAse   SWidth SX -2/,SHeight SY- 2/;" 
   "SAve   1;" 
   "BOx    0,0,1,SX,SY;" 
   "PRint  1VACX,SY2/ TH2/ -,1VA,3;" 
   "RUn    0,%1111;" 
   "]" 
   "IF     0VA 2=;" 
   "[" 
   "SIze   SWidth 1VATW80+ MIn,64;" 
   "BAse   SWidth SX -2/,SHeight SY- 2/;" 
   "SAve   1;" 
   "BOx    0,0,1,SX,SY;" 
   "PRint  1VACX,16,1VA,3;" 
   "BUtton 1,16,SY24-,64,16,0,0,1;[UNpack 0,0,13BP+; PRint 12ME CX BP+,4,12ME,3;][BR0;BQ;]" 
   "KY     27,0;" 
   "BUtton 2,SX72-,SY24-,64,16,0,0,1;[UNpack 0,0,13BP+; PRint 11ME CX BP+,4,11ME,3;][BR0;BQ;]" 
   "KY     13,0;" 
   "RUn    0,3;" 
   "]" 
   "IF     0VA 3=;" 
   "[" 
   "SIze   1VATW160+ SW MI,40;" 
   "BAse   SWidth SX -2/,SHeight SY- 2/;" 
   "BOx    0,0,1,SX,SY;" 
   "PRint  1VACX,SY2/ TH2/ -,1VA,3;" 
   "]" 
   "EXit;" ;

	context.script = toAmosString_char( (char *) script,strlen(script));

	if (context.script)
	{
		printf("%s\n",&context.script ->ptr);
		
		printf("\n\n");

		execute_interface_script( &context, 0);

		sys_free(context.script);
	}

	return 0;
}



void setError( int _code, char * _pos ) 
{}

void engine_lock()
{}

void engine_unlock()
{}

bool engine_ready()
{
	return true;
}

int engine_wait_key;

void *findBank(int id)
{}

uint8_t getByte( char *adr, int &pos )
{
	uint8_t ret = *((uint8_t *) (adr + pos));
	pos+=1;
	return ret;
}

uint16_t getWord( char *adr, int &pos )
{
	short ret = *((uint16_t *) (adr + pos));
	pos+=2;
	return ret;
}

uint32_t getLong( char *adr, int &pos )
{
	short ret = *((uint32_t *) (adr + pos));
	pos+=4;
	return ret;
}

void getResourceStr(int)
{
}

