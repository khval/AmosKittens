 
void dump_global();
void dump_prog_stack();
void dump_stack();
void dumpLabels();
void dump_end_of_program();
void dumpLineAddress();
int getLineFromPointer( char *address );

#define show_proc_names_yes
#define show_token_numbers_yes
#define show_debug_printf_yes

#ifdef show_debug_printf_yes
#define dprintf printf
#else
#define dprintf 
#endif

#ifdef show_proc_names_yes
#define proc_names_printf printf
#else
#define proc_names_printf(fmt,...)
#endif

