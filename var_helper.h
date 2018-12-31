
// var 

int var_type_is( struct reference *ref, int mask );
char *var_JumpToName(struct reference *ref);
int var_find_proc_ref(struct reference *ref);
void validate_and_fix_globals();

// copy functions

char *_copy_to_char(char *adr, char t);
char *_copy_to_len(char *adr, int _len);

