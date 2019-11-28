
// var 

int var_type_is( struct reference *ref, int mask );
struct label *var_JumpToName(struct reference *ref);
int var_find_proc_ref(struct reference *ref);
void validate_and_fix_globals();

int ReferenceByteLength(char *ptr);
int QuoteByteLength(char *ptr);
char *dupRef( struct reference *ref );
struct label *findLabel( char *name, int _proc );
int findLabelRef( char *name, int _proc );
int findProc( char *name );

