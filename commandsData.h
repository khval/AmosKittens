
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <proto/exec.h>
#include "amosKittens.h"

bool correct_order( int last_token, int next_token );
void correct_for_hidden_sub_data();

char *powerData(struct nativeCommand *cmd, char *tokenBuffer);
char *addData(struct nativeCommand *cmd, char *tokenBuffer);
char *subData(struct nativeCommand *cmd, char *tokenBuffer);
char *mulData(struct nativeCommand *cmd, char *tokenBuffer);
char *divData(struct nativeCommand *cmd, char *tokenBuffer);
char *orData(struct nativeCommand *cmd, char *tokenBuffer);
char *andData(struct nativeCommand *cmd, char *tokenBuffer);
char *xorData(struct nativeCommand *cmd, char *tokenBuffer);

char *lessData(struct nativeCommand *cmd, char *tokenBuffer);
char *moreData(struct nativeCommand *cmd, char *tokenBuffer);

char *lessOrEqualData(struct nativeCommand *cmd, char *tokenBuffer);
char *moreOrEqualData(struct nativeCommand *cmd, char *tokenBuffer);

char *_addData( struct glueCommands *data );
char *_equalData( struct glueCommands *data );

