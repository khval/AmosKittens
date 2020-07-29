
// for vars

int var_type_is( struct reference *ref, int mask );
struct label *var_JumpToName(struct reference *ref);
int var_find_proc_ref(struct reference *ref);
void validate_and_fix_globals();
struct kittyData *getVar(uint16_t ref);

// For general 

int ReferenceByteLength(char *ptr);
int QuoteByteLength(char *ptr);
char *dupRef( struct reference *ref );

// for labels

struct label *findLabel( char *name, int _proc );
int findLabelRef( char *name, int _proc );

// for procedures

int findProcByName( char *name );
struct globalVar *findProcPtrById( int _proc );
void get_procedures();

