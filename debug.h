
#ifdef __amigaos4__
#ifdef __linux__
#undef __linux__
#endif
#endif

// --------- debug options ---------------

#define show_error_at_file_yes
#define show_proc_names_yes
#define show_token_numbers_yes
#define show_debug_printf_yes
#define show_debug_amal_yes
#define show_pass1_tokens_yes
#define show_array_yes
#define enable_engine_debug_output_yes

//------------- end of options -----------------

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

#ifdef __amigaos4__
#define AmalPrintf Printf_iso
#endif

#ifdef __linux__
#define AmalPrintf printf
#endif

#else
#define AmalPrintf(fmt,...)
#endif

#ifdef __amigaos4__
void Printf_iso(const char *fmt,...);
#endif

#ifdef __linux__
#define Printf_iso(fmt,...) fprintf(engine_fd,fmt,__VA_ARGS__)
#endif
