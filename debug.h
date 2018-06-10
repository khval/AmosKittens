 
void dump_global();
void dump_prog_stack();
void dump_stack();
void dump_labels();
void dump_end_of_program();
void dumpLineAddress();
int getLineFromPointer( char *address );

// --------- debug options ---------------

#define show_proc_names_no
#define show_token_numbers_no
#define show_debug_printf_no
#define show_pass1_tokens_no

//------------- end of options -----------------

#ifdef show_debug_printf_yes
#define dprintf printf
#else
#define dprintf(fmt,...)
#endif

#ifdef show_proc_names_yes
#define proc_names_printf printf
#else
#define proc_names_printf(fmt,...)
#endif

#ifdef show_pass1_tokens_yes
#define pass1_printf printf
#else
#define pass1_printf(fmt,...)
#endif

