
// for vars

void validate_and_fix_globals();

int var_type_is( struct reference *ref, int mask );
struct label *var_JumpToName(struct reference *ref);
int var_find_proc_ref(struct reference *ref);
struct kittyData *getVar(uint16_t ref);
struct globalVar *add_var_from_ref( struct reference *ref, char **tmp, int type );
void add_str_var(const char *_name,const char *_value);
int findVarPublic( char *name, int type );
struct kittyData *findPublicVarByName( char *name, int type );
bool str_var_is( struct kittyData *var, const char *value );

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

