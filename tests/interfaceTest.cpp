
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
	struct stringData *scriptAmos;

printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

#if 0
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
#endif

#if 1
	const char *script = 

	"IF     0VA 1=;["
		"SIze   1VATW160+ SW MI,40;"
		"BAse   SWidth SX -2/,SHeight SY- 2/;"
		"SAve   1;"
		"BOx    0,0,1,SX,SY;"
		"PRint  1VACX,SY2/ TH2/ -,1VA,3;"
		"RUn    0,%1111;]"

	"IF     0VA 2=;["
		"SIze   SWidth 1VATW80+ MIn,64;"
		"BAse   SWidth SX -2/,SHeight SY- 2/;"
		"SAve   1;"
		"BOx    0,0,1,SX,SY;"
		"PRint  1VACX,16,1VA,3;"
		"BUtton 1,16,SY24-,64,16,0,0,1;[UNpack 0,0,13BP+; PRint 12ME CX BP+,4,12ME,3;][BR0;BQ;]KY     27,0;"
		"BUtton 2,SX72-,SY24-,64,16,0,0,1;[UNpack 0,0,13BP+; PRint 11ME CX BP+,4,11ME,3;][BR0;BQ;]KY     13,0;"
		"RUn    0,3;]"

	"IF     0VA 3=;["
		"SIze   1VATW160+ SW MI,40;"
		"BAse   SWidth SX -2/,SHeight SY- 2/;"
		"BOx    0,0,1,SX,SY;"
		"PRint  1VACX,SY2/ TH2/ -,1VA,3;]"

	"EXit;"
	"LA 1;SIze   SW,SH;"
	"BAse   SWidth SX -2/,SHeight SY- 2/;PUzzle  21;"
	"SVar    2,0;LIne 24,0,7,SX288-;"
	"LIne 0,SY4-,10,SX;"
	"VLine SX16-,24,17,SY4-;"
	"BUtton 1,0,0,24,16,0,0,1;[UNpack 0,0,1 BP+;][BR0;]KY     27,0;KY     $DF,0;"
	"BUtton 2,SX16-,10,16,7,0,0,1;[UNpack 0,0,13 BP+;][SV2,0 2VA1- MAx;ZC4,2VA;BR0;]KY     $CC,0;"
	"BUtton 3,XA,YB,16,7,0,0,1;[UNpack 0,0,15 BP+;][SV2,3VA 2VA1+ MIn;ZC4,2VA;BR0;]KY     $CD,0;"
	"HText  5,0,11,SX16-8/,SY16-8/,0VA,2VA,8,2,3;[]SVar 3,ZVar;"
	"VSlide 4,SX11-,25,8,SY30-,0,SH8/2-,3VA,SH16-8/2-;[SV2,ZP;ZC 5,2VA;]"
	"BB     6,SX288-,0,96,7 ME;"
	"BB     7,SX96-,0,96,8 ME;"
	"BB     8,SX192-,0,96,3 ME;"
	"BUtton 0,24,0,SX160-,10,0,0,0;[][SM;]"
	"EXit;"
	"UI     BB,5;[SZone  P5;BUtton P1,P2,P3,P4,10,0,0,1;[SW 0;LIne 0,0,BP3* 20+,SX;PRint ZV CX BP+,1,ZV,3;][BR0;]]";
#endif

#if 0
	const char *script =    "KY     $DF,0;";
#endif

#if 0
	const char *script =    "UX 100,200;UI UX,2;[ KY $DF,0;]";
#endif

printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	scriptAmos = toAmosString_char( script,strlen(script));

printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (scriptAmos)
	{

printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		init_interface_context( &context, 1, scriptAmos, 0, 0, 10, 1000  );

printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

		// its copied so we can free it now.
		sys_free(scriptAmos);
		scriptAmos = NULL;	
	}

printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	if (context.script)
	{
		printf("%s\n",&context.script ->ptr);
		
		printf("\n\n");

		execute_interface_script( &context, 0);

		cleanup_interface_context( &context );
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

