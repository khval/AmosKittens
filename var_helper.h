
// var 

int var_type_is( struct reference *ref, int mask );
struct label *var_JumpToName(struct reference *ref);
int var_find_proc_ref(struct reference *ref);
void validate_and_fix_globals();

// copy functions


char *_copy_until_char(char *adr, char t);
char *_copy_until_len(char *adr, int _len);
char *_copy_until_len_or_char(char *adr, int _len, char t);

