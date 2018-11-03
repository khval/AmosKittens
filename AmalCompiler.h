
struct kittyChannel;

namespace amal
{
	enum Flags
	{
		flag_none,
		flag_cmd = 1,
		flag_para = 2,
		flag_parenthses =4
	};

	enum Class
	{
		class_cmd_arg,
		class_cmd_normal
	};
};

#define API_AMAL_CALL_ARGS ( struct kittyChannel *self, void **code, unsigned int opt )

struct amalBuf
{
	unsigned int elements;
	unsigned int size;
	void *(**call_array) API_AMAL_CALL_ARGS;
};

struct amalWriterData
{
	int pos;
	const char *at_script;
	unsigned int command_len;
	unsigned int arg_len;
	amal::Class lastClass;
};

struct amalTab
{
	const char *name;

	amal::Class Class;

	unsigned int (*write) (
				struct kittyChannel *channel,
				struct amalTab *self, 
				void *(**call_array) API_AMAL_CALL_ARGS, 
				struct amalWriterData *data,
				unsigned int
				);

	void *(*call) API_AMAL_CALL_ARGS;
};

extern void pushBackAmalCmd( amal::Flags flags, void **code, struct kittyChannel *channel, void *(*cmd)  (struct kittyChannel *self, struct amalCallBack *cb)  ) ;
extern void dumpAmalStack( struct kittyChannel *channel );
extern bool asc_to_amal_tokens( struct kittyChannel  *channel );
extern void amal_run_one_cycle( struct kittyChannel  *channel );
extern void amal_fix_labels( void **code );
extern void amal_clean_up_labels();
extern void freeAmalBuf( struct amalBuf *i);

