
char *token_reader_pass1( char *start, char *ptr, unsigned short lastToken, unsigned short token, char *file_end );
void pass1_reader( char *start, char *file_end );

#define max_nested_commands 1000

enum
{
	nested_if,
	nested_then,
	nested_then_else,
	nested_then_else_if,
	nested_else,
	nested_else_if,
	nested_while,
	nested_repeat,
	nested_do,
	nested_for,
	nested_proc,
	nested_defFn,
	nested_data
};

struct nested
{
	int cmd;
	char *ptr;
};

extern int nested_count;
//extern struct nested nested_command[ max_nested_commands ];

#define addNestLoop( enum_cmd ) \
	nested_command[ nested_count ].cmd = enum_cmd; \
	nested_command[ nested_count ].ptr = ptr; \
	nested_count++;	\
	nest_loop_count++;

#define addNest( enum_cmd ) \
	nested_command[ nested_count ].cmd = enum_cmd; \
	nested_command[ nested_count ].ptr = ptr; \
	nested_count++;

#define addNestAmal( name , adr ) \
	amal_nested_command[ nested_count ].cmd = nested_ ## name; \
	amal_nested_command[ nested_count ].offset = (unsigned int) (adr) - (unsigned int) channel -> amalProg.call_array; \
	nested_count++;

#define IS_LAST_NEST_TOKEN(name) ((nested_count>0) && (nested_command[ nested_count -1 ].cmd == nested_ ## name ))
#define GET_LAST_NEST ((nested_count>0) ? nested_command[ nested_count -1 ].cmd : -1)
#define GET_LAST_AMAL_NEST ((nested_count>0) ? amal_nested_command[ nested_count -1 ].cmd : -1)

