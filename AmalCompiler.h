
struct kittyChannel;

#define API_AMAL_CALL_ARGS ( struct kittyChannel *self, void **code, unsigned int opt )

struct amalBuf
{
	unsigned int elements;
	unsigned int size;
	void *(**call_array) API_AMAL_CALL_ARGS;
};

struct amalTab
{
	const char *name;

	unsigned int (*write) (	struct amalTab *self, 
				void *(**call_array) API_AMAL_CALL_ARGS, 
				const char *at_script,
				unsigned int);

	void *(*call) API_AMAL_CALL_ARGS;
};

