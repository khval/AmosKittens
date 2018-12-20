 
void dump_global();
void dump_prog_stack();
void dump_stack();
void dump_labels();
void dump_banks();
void dump_end_of_program();
void dumpLineAddress();
void dump_680x0_regs();
void dumpScreenInfo();
int getLineFromPointer( char *address );

// --------- debug options ---------------

#define show_error_at_file_no
#define show_proc_names_no
#define show_token_numbers_no
#define show_debug_printf_no
#define show_debug_amal_no
#define show_pass1_tokens_no
#define show_array_no
#define enable_engine_debug_output_yes

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

#ifdef show_debug_amal_yes
#define AmalPrintf Printf
#else
#define AmalPrintf(fmt,...)
#endif
