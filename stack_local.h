
extern void remove_parenthesis(int black_at_stack );
extern void unLockPara();
extern bool stackStrAddValue(struct kittyData *item0, struct kittyData *item1);
extern bool stackStrAddDecimal(struct kittyData *item0, struct kittyData *item1);
extern bool stackStrAddStr(struct kittyData *item0,	struct kittyData *item1);
extern bool stackMoreStr(struct kittyData *item0,	struct kittyData *item1);
extern bool stackLessStr(struct kittyData *item0,	struct kittyData *item1);
extern bool stackMoreOrEqualStr(struct kittyData *item0,struct kittyData *item1);
extern bool stackLessOrEqualStr(struct kittyData *item0,struct kittyData *item1);
extern bool stackEqualStr(struct kittyData *item0, struct kittyData *item1);
extern bool stackNotEqualStr(struct kittyData *item0, struct kittyData *item1);
extern bool stackMoreOrEqualStr(struct kittyData *item0, struct kittyData *item1);
extern bool stackLessOrEqualStr(struct kittyData *item0, struct kittyData *item1);
extern void correct_for_hidden_sub_data();
//extern unsigned short getLastProgStackToken();

#define getLastProgStackFn()	((cmdStack) ? cmdTmp[cmdStack-1].cmd : NULL)
#define getLastProgStackToken() ((cmdStack) ? cmdTmp[cmdStack-1].token : 0 )
#define getLastLastProgStackToken() (((cmdStack-1)>0) ? cmdTmp[cmdStack-2].token : 0 )

#define setStackHiddenCondition()			\
			kittyStack[__stack].str = NULL;		\
			kittyStack[__stack].state = state_hidden_subData;	\
			stack++;

extern char *flushCmdParaStack( int nextToken );
extern bool dropProgStackToProc( char *(*fn) (struct glueCommands *data, int nextToken ) );
extern bool dropProgStackToFlag( int flag );
extern bool dropProgStackAllFlag( int flag );
extern void	setStackParenthesis();