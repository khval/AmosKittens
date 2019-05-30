
#ifdef __amigaos4__
#ifdef __linux__
#undef __linux__
#endif
#endif

// --------- debug options --------------- (most common debug options)

#define show_error_at_file_yes
#define show_proc_names_yes
#define show_token_numbers_yes
#define show_debug_printf_yes
#define show_debug_amal_no
#define show_array_no
#define enable_engine_debug_output_no

// --------- debug options pass1 ----------- (most debug options for pretest)

#define show_pass1_tokens_no
#define show_pass1_procedure_fixes_no
#define show_pass1_end_of_file_no

// ------------- CRC options ------------------ (keep this to no, unless you need to find a memory corruption bug)

#define enable_ext_crc_no
#define enable_vars_crc_no
#define enable_bank_crc_no			// can find memory corruption in pass1

// ------------ optimizer ----------------------

#define run_program_yes
#define enable_fast_execution_yes		// Some debug option do not work when this enabled.

//------------- end of options -----------------

void dump_global();
void dump_local( int n );
void dump_prog_stack();
void dump_stack();
void dump_labels();
void dump_banks();
void dump_end_of_program();
void dumpLineAddress();
void dump_680x0_regs();
void dumpScreenInfo();
int getLineFromPointer( char *address );
uint32_t mem_crc( char *mem, uint32_t size );

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
		#ifdef test_app
			#define AmalPrintf printf
		#else
			#define AmalPrintf Printf_iso
		#endif
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
