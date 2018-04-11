 
void dump_global();
void dump_prog_stack();
void dump_stack();
void dumpLabels();
void dump_end_of_program();

#define show_proc_names_no
#define show_token_numbers_no
#define show_debug_printf_no

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

