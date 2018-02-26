
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

char *_addData( struct glueCommands *data );
