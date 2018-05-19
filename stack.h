
extern int stack;

#define setStackNum(num) _num(num)
#define setStackPtr(adr) _num((int) adr) 

extern void _num( int num );
extern void setStackDecimal( double decimal );
extern void setStackStr( char *str );
extern void setStackStrDup(const char *str);

extern bool stackStrAddValue(struct kittyData *item0, struct kittyData *item1);
extern bool stackStrAddDecimal(struct kittyData *item0, struct kittyData *item1);

extern bool stackStrAddStr(struct kittyData *item0,	struct kittyData *item1);

extern bool stackMoreStr(struct kittyData *item0,	struct kittyData *item1);
extern bool stackLessStr(struct kittyData *item0,	struct kittyData *item1);

extern bool stackMoreOrEqualStr(struct kittyData *item0,struct kittyData *item1);
extern bool stackLessOrEqualStr(struct kittyData *item0,struct kittyData *item1);

extern bool stackEqualStr(struct kittyData *item0, struct kittyData *item1);

extern bool stackMoreOrEqualStr(struct kittyData *item0, struct kittyData *item1);
extern bool stackLessOrEqualStr(struct kittyData *item0, struct kittyData *item1);

extern void popStack(int n);
extern void correct_for_hidden_sub_data();

extern void unLockPara();
extern void remove_parenthesis(int black_at_stack );

extern char *flushCmdParaStack();
extern bool dropProgStackToProc( char *(*fn) (struct glueCommands *data) );
extern bool dropProgStackToType( int type );

// return value or 0 or NULL

extern int _stackInt( int n );
extern double _stackDecimal( int n );
extern char *_stackString( int n );

// only set ret var if type is correct.

extern void stack_get_if_int( int n, int *ret );

