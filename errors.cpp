
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "amosKittens.h"
#include "stack.h"
#include "errors.h"
#include "debug.h"

struct errorAt kittyError = { 0, 0 };

extern struct error errorsTestTime[];

// return error code.

char *cmdERRN(struct nativeCommand *cmd, char *tokenBuffer)
{
	setStackNum( kittyError.code );
	return tokenBuffer;
}


void printError( struct errorAt *thisError, struct error *tab )
{
	struct error *e;
	
	for (e=tab; e-> errorText; e++ )
	{
		if (thisError -> code == e->errorCode)
		{
			printf("ERROR: %s\nAt line number: %d\n\n",e->errorText, getLineFromPointer(thisError->pos));
			break;
		}
	}
}

struct error errorsTestTime[]= {
	{ 1,"Bad structure"},
	{ 2,"User function not defined"},
	{ 3,"Variable buffer can't be changed in the middle of a program!"},
	{ 4,"This instruction must be alone on a line"},
	{ 5,"Extension not loaded"},
	{ 6,"Too many direct mode variables"},
	{ 7,"Illegal direct mode"},
	{ 8,"Variable buffer too small"},
	{ 9,"No jumps allowed into the middle of a loop!"},
	{ 10,"Structure too long"},
	{ 11,"This instruction must be used within a procedure"},
	{ 12,"This variable is already defined as SHARED"},
	{ 13,"This array is not defined in the main program"},
	{ 14,"Use empty brakets when defining a shared array"},
	{ 15,"Shared must be alone on a line"},
	{ 16,"Procedure's limits must be alone on a line"},
	{ 17,"Procedure not closed"},
	{ 18,"Procedure not opened"},
	{ 19,"Illegal number of parameters"},
	{ 20,"Undefined procedure"},
	{ 21,"ELSE without IF"},
	{ 22,"IF without ENDIF"},
	{ 23,"ENDIF without IF"},
	{ 24,"ELSE without ENDIF"},
	{ 25,"No THEN in a structured test"},
	{ 26,"Not enough loops to exit"},
	{ 27,"DO without LOOP"},
	{ 28,"LOOP without DO"},
	{ 29,"WHILE without matching WEND"},
	{ 30,"WEND without WHILE"},
	{ 31,"REPEAT without matching UNTIL"},
	{ 32,"UNTIL without REPEAT"},
	{ 33,"FOR without matching NEXT"},
	{ 34,"NEXT without FOR"},
	{ 35,"Syntax error"},
	{ 36,"Out of memory"},
	{ 37,"Variable name's buffer too small"},
	{ 38,"Array not dimensioned"},
	{ 39,"Array already dimensioned"},
	{ 40,"Type mismatch error"},
	{ 41,"Undefined label"},
	{ 42,"Label defined twice"},
	{ 43,"Trap must be immediately followed by an instruction"},
	{ 44,"No ELSE IF after an ELSE"},
	{ 45,"Cannot load included file"},
	{ 46,"Included file is not an AMOS program"},
	{ 47,"Instruction not compatible with AMOS 1.3"},
	{ 48,"This program holds too many banks for AMOS 1.3"},
	{ 49,"This program is compatible with AMOS 1.3"},
	{ 50,"This command must begin your program (but AFTER 'Set Buffer')"},
	{ 51,"Equate not defined"},
	{ 52,"Cannot load equate file"},
	{ 53,"Bad format in equate file"},
	{ 54,"Equate not of the right type"},
	{ 0,NULL } };


struct error errorsRunTime[]= {
	{ 0,""},
	{ 1,"RETURN without GOSUB"},			
	{ 2,"POP without GOSUB"},			
	{ 3,"Error not resumed"},			
	{ 4,"Can't resume to a label"},
	{ 5,"No ON ERROR PROC before this instruction"},	
	{ 6,"Resume label not defined"},
	{ 7,"Resume without error"},		
	{ 8,"Error procedure must RESUME to end"},	
	{ 9,"Program interrupted"},
	{ 10,"End of program"},
	{ 11,"Out of variable space"},
	{ 12,"Cannot open math libraries"},		
	{ 13,"Out of stack space"},	
	{ 14,""},	
	{ 15,"User function not defined"},
	{ 16,"Illegal user function call"}, 		
	{ 17,"Illegal direct mode"},		
	{ 18,""},						
	{ 19,""},						
// 20- Messages normaux
// ~~~~~~~~~~~~~~~~~~~~
	{ 20,"Division by zero"},		
	{ 21,"String too long"},		
	{ 22,"Syntax error"},
	{ 23,"Illegal function call"},			
	{ 24,"Out of memory"},		
	{ 25,"Address error"},				
	{ 26,""},
	{ 27,"Non dimensioned array"},
	{ 28,"Array already dimensioned"},		
	{ 29,"Overflow"},	
	{ 30,"Bad IFF format"},				
	{ 31,"IFF compression not recognised"},		
	{ 32,"Can't fit picture in current screen"},	
	{ 33,"Out of data"},				
	{ 34,"Type mismatch"},				
	{ 35,"Bank already reserved"},			
	{ 36,"Bank not reserved"},		
	{ 37,"Fonts not examined"},			
	{ 38,"Menu not opened"},			
	{ 39,"Menu item not defined"},			
	{ 40,"Label not defined"},			
	{ 41,"No data after this label"},		
	{ 42,""},			
	{ 43,""},	
	{ 44,"Font not available"},			
// Messages d'erreur ecrans/fenetres
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	{ 45,""},
	{ 46,"Block not defined"},			
	{ 47,"Screen not opened"},			
	{ 48,"Illegal screen parameter"},		
	{ 49,"Illegal number of colours"},		
	{ 50,"Valid screen numbers range 0 to 7"},	
	{ 51,"Too many colours in flash"},				
	{ 52,"Flash declaration error"},		
	{ 53,"Shift declaration error"},		
	{ 54,"Text window not opened"},			
	{ 55,"Text window already opened"},			
	{ 56,"Text window too small"},			
	{ 57,"Text window too large"},			
	{ 58,""},
	{ 59,"Bordered text windows not on edge of screen"},	
	{ 60,"Illegal text window parameter"},		
	{ 61,""},
	{ 62,"Text window 0 can't be closed"},		
	{ 63,"This text window has no border"},		
	{ 64,""},
	{ 65,"Block not found"},			
	{ 66,"Illegal block parameters"},		
	{ 67,"Screens can't be animated"}, 		
	{ 68,"Bob not defined"},			
	{ 69,"Screen already in double buffering"},	
	{ 70,"Can't set dual playfield"},		
	{ 71,"Screen not in dual playfield mode"},	
	{ 72,"Scrolling zone not defined"},		
	{ 73,"No zones defined"},			
	{ 74,"Icon not defined"},			
	{ 75,"Rainbow not defined"},			
	{ 76,"Copper not disabled"},			
	{ 77,"Copper list too long"},			
	{ 78,"Illegal copper parameter"},		
// Messages d'erreur disque
// ~~~~~~~~~~~~~~~~~~~~~~~~
	{ 79,"File already exists"},			
	{ 80,"Directory not found"},		//	204		
	{ 81,"File not found"},				//	205	
	{ 82,"Illegal file name"},			//	210
	{ 83,"Disc is not validated"},		//	213		
	{ 84,"Disc is write protected"},	//	214	
	{ 85,"Directory not empty"},		//	216		
	{ 86,"Device not available"},		//	218		
	{ 87,""},							//	220
	{ 88,"Disc full"},					//	221
	{ 89,"File is protected against deletion"},		//	222
	{ 90,"File is write protected"},				//	223
	{ 91,"File is protected against reading"},		//	224
	{ 92,"Not an AmigaDOS disc"},					//	225
	{ 93,"No disc in drive"},						// 226
	{ 94,"I/O error"},				
	{ 95,"File format not recognised"},		
	{ 96,"File already opened"},			
	{ 97,"File not opened"},			
	{ 98,"File type mismatch"},			
	{ 99,"Input too long"},				
	{ 100,"End of file"},
	{ 101,"Disc error"},
	{ 102,"Instruction not allowed here"},
	{ 103,""},	
// Message d'erreur sprites/souris
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	{ 104,""},
	{ 105,"Sprite error"},						
	{ 106,""},								
	{ 107,"Syntax error in animation string"},			
	{ 108,"Next without For in animation string"},			
	{ 109,"Label not defined in animation string"},
	{ 110,"Jump To/Within autotest in animation string"},		
	{ 111,"Autotest already opened"},				
	{ 112,"Instruction only valid in autotest"},			
	{ 113,"Animation string too long"},				
	{ 114,"Label already defined in animation string"},		
	{ 115,"Illegal instruction during autotest"},			
	{ 116,"Amal bank not reserved"},					
	{ 117,""},
	{ 118,""},
	{ 119,""},
// Messages d'erreur dialogues
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
	{ 120,"Interface error: bad syntax"},
	{ 121,"Interface error: out of memory"},
	{ 122,"Interface error: label defined twice"},
	{ 123,"Interface error: label not defined"},
	{ 124,"Interface error: channel already defined"},
	{ 125,"Interface error: channel not defined"},
	{ 126,"Interface error: screen modified"},
	{ 127,"Interface error: variable not defined"},
	{ 128,"Interface error: illegal function call"},
	{ 129,"Interface error: type mismatch"},
	{ 130,"Interface error: buffer to small"},
	{ 131,"Interface error: illegal number of parameters"},
	{ 132,""},
	{ 133,""},
	{ 134,""},
	{ 135,""},
	{ 136,""},
	{ 137,""},
	{ 138,""},
	{ 139,""},
// Messages d'erreur DEVICE / PRINTER / SERIAL
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	{ 140,"Device already opened"},
	{ 141,"Device not opened"},
	{ 142,"Device cannot be opened"},
	{ 143,"Command not supported by device"},
	{ 144,"Device error"},
// Messages Serie
	{ 145,"Serial device already in use"},
	{ 146,""},
	{ 147,"Invalid baud rate"},
	{ 148,"Out of memory (serial device)"},
	{ 149,"Bad parameter"},
	{ 150,"Hardware data overrun"},
	{ 151,""},
	{ 152,""},
	{ 153,""},
	{ 154,""},
	{ 155,"Timeout error"},
	{ 156,"Buffer overflow"},
	{ 157,"No data set ready"},
	{ 158,""},
	{ 159,"Break detected"},
	{ 160,"Selected unit already in use"},
// Message Printer
	{ 161,"User canceled request"},
	{ 162,"Printer cannot output graphics"},
	{ 163,""},
	{ 164,"Illegal print dimensions"},
	{ 165,""},
	{ 166,"Out of memory (printer device)"},
	{ 167,"Out of internal memory (printer device)"},
// Messages libraries
	{ 168,"Library already opened"},
	{ 169,"Library not opened"},
	{ 170,"Cannot open library"},
// Messages parallel
	{ 171,"Parallel device already used"},
	{ 172,"Out of memory (parallel device)"},
	{ 173,"Invalid parallel parameter"},
	{ 174,"Parallel line error"},
	{ 175,""},
	{ 176,"Parallel port reset"},
	{ 177,"Parallel initialisation error"},
// Music errors
	{ 178,"Wave not defined"},				//	0
	{ 179,"Sample not defined"},			//	1
	{ 180,"Sample bank not found"},			//	2
	{ 181,"256 characters for a wave"},		//	3
	{ 182,"Wave 0 and 1 are reserved"},		//	4
	{ 183,"Music bank not found"},			//	5
	{ 184,"Music not defined"},				//	6
	{ 185,"Can't open narrator"},			//	7
	{ 186,"Not a tracker module"},			//	8
	{ 187,"Cannot load med.library"},		//	9
	{ 188,"Cannot start med.library"},		//	10
	{ 189,"Not a med module"},				//	11
	{ 190,""},
	{ 191,""},
	{ 192,""},
// AREXX
// ~~~~~
	{ 193,"Arexx port already opened"},
	{ 194,"Arexx library not found"},
	{ 195,"Cannot open Arexx port"},
	{ 196,"Arexx port not opened"},	
	{ 197,"No Arexx message waiting"},
	{ 198,"Arexx message not answered to"},
	{ 199,"Arexx Device not interactive"},
// Misc
// ~~~~
	{ 200,"Cannot open powerpacker.library (v35)"},
	{ 0,NULL }};
