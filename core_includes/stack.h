
extern void	remove_parenthesis(  this_instance_first int black_at_stack );
extern void	unLockPara( this_instance_one );

extern void	setStackParenthesis( this_instance_one );
#define 		setStackPtr(adr) setStackNum( this_instance_first (int) adr) 
extern void	setStackNum( this_instance_first int num );
extern void	setStackDecimal( this_instance_first double decimal );
extern void	setStackStr( this_instance_first struct stringData *str );
extern void	setStackStrDup( this_instance_first struct stringData *str);
extern void	setStackNone( this_instance_one );

#define getLastProgStackFn()	((__cmdStack) ? cmdTmp[__cmdStack-1].cmd : NULL)
#define getLastProgStackToken() ((__cmdStack) ? cmdTmp[__cmdStack-1].token : 0 )

#define setStackHiddenCondition()			\
			kittyStack[instance_stack].str = NULL;		\
			kittyStack[instance_stack].state = state_hidden_subData;	\
			instance_stack++;

extern int		getStackNum( this_instance_first int n );
extern double	getStackDecimal( this_instance_first int n );
extern struct stringData *getStackString( this_instance_first int n );
extern bool dropProgStackToProc( this_instance_first char *(*fn) (struct glueCommands *data, int nextToken ) );
extern bool dropProgStackToFlag( this_instance_first int flag );
extern bool dropProgStackAllFlag( this_instance_first int flag );
extern bool stack_is_number( this_instance_first int n );

#if __amoskittens__

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

#endif

extern void popStack( this_instance_first int n );
extern void correct_for_hidden_sub_data( this_instance_one );

extern char *flushCmdParaStack( int nextToken );

extern void stack_get_if_int( this_instance_first int n, int *ret );

#define incStack 	__stack++; kittyStack[__stack].state = state_none;	kittyStack[__stack].type = type_none; 

