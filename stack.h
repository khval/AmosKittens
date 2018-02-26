
extern int stack;

extern void _num( int num );
extern void setStackDecimal( double decimal );
extern void setStackStr( char *str );
extern void setStackStrDup(const char *str);

extern bool stackStrAddValue(struct kittyData *item0, struct kittyData *item1);
extern bool stackStrAddDecimal(struct kittyData *item0, struct kittyData *item1);

extern void popStack(int n);

extern int _stackInt( int n );
extern char *_stackString( int n );

