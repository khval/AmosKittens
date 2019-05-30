
extern int stack;

extern void	remove_parenthesis(int black_at_stack );
extern void	unLockPara();

extern void	setStackParenthesis();
#define 		setStackPtr(adr) setStackNum((int) adr) 
extern void	setStackNum( int num );
extern void	setStackDecimal( double decimal );
extern void	setStackStr( char *str );
extern void	setStackStrDup(const char *str);
extern void	setStackNone( void );

extern int		getStackNum( int n );
extern double	getStackDecimal( int n );
extern char*	getStackString( int n );

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

extern void popStack(int n);
extern void correct_for_hidden_sub_data();

extern char *flushCmdParaStack( int nextToken );
extern bool dropProgStackToProc( char *(*fn) (struct glueCommands *data, int nextToken ) );
extern bool dropProgStackToFlag( int flag );
extern bool dropProgStackAllFlag( int flag );

extern void stack_get_if_int( int n, int *ret );

#define incStack 	stack++; kittyStack[stack].state = state_none;	kittyStack[stack].type = type_none; 

