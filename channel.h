
struct kittyChannel;
struct amalCallBack;

#define API_AMAL_CALL_ARGS ( struct kittyChannel *self, void **code, unsigned int opt )

struct amalCallBack
{
	void **code ;
	void *(*cmd) (struct kittyChannel *self, struct amalCallBack *cb);
	void *ret;
	int last_reg;
	int argStackCount;
	int progStackCount;
	amal::Flags Flags;
};


namespace channel_status
{
	enum status
	{
		uninitialized	= 0x01,	// No amal program.
		done		= 0x02,	// Amal program is done.
		initialized		= 0x04,	// Have amal program
		active		= 0x08,	// Amal program is running.
		paused		= 0x10,	// Same as exit amal prgram at VBL
		frozen		= 0x20,	// Stops the amal program, until its unfrozen.
		wait			= 0x40,	// only Execute autotest
		direct		= 0x80,	// exit autotest, early, and AMAL start ptr.
		error			= 0x100	// error (used by play command)
	};
};

struct channelAPI
{
	int (*getMax) ( void );
	int (*getImage) (unsigned int object);
	int (*getX) (unsigned int object);
	int (*getY) (unsigned int object);
	void (*setImage) (unsigned int object,int);
	void (*setX) (unsigned int object,int);
	void (*setY) (unsigned int object,int);
	struct retroScreen *(*getScreen)( unsigned int object );
};

struct amalPlayContext
{
	signed char *data;
	int repeat;
	int size;
	signed char value;
};

class amalBankPlay
{
	public:
		unsigned short *offset_tab;
		unsigned short *size_tab;
		char *name_tab;
		char *move_data;
		int lx;

		struct amalPlayContext cdx;
		struct amalPlayContext cdy;

	amalBankPlay(char *start);
};


class kittyChannel
{
	public:

	kittyChannel( int channel );
	~kittyChannel();

	unsigned short id;
	unsigned short token;
	unsigned short number;
	struct stringData *amal_script;
	char *amal_at;
	struct stringData *anim_script;
	char *anim_at;
	int anim_loops;

	// move x and move y
	struct stringData *movex_script;
	char *movex_at;
	struct stringData *movey_script;
	char *movey_at;
	int deltax;
	int deltay;
	int anim_sleep;
	int anim_sleep_to;
	int move_sleep;
	int move_sleep_to;
	int count;
	int count_to;
	
	// amal move
	int move_from_x;
	int move_from_y;
	int move_delta_x;
	int move_delta_y;
	int move_count;
	int move_count_to;

	struct channelAPI *objectAPI;

	uint32_t animStatus;
	uint32_t amalStatus;
	uint32_t moveStatus;

	int reg[10];	// local reg 0 to 9 
	int parenthses;
	int *argStack;
	struct amalBuf amalProg;
	unsigned int argStackCount;
	struct amalCallBack *progStack;
	unsigned int progStackCount;
	unsigned int loopCount; 
	unsigned int autotest_loopCount;
	unsigned int last_reg;

	void *(*pushBackFunction)  (struct kittyChannel *self, struct amalCallBack *cb);
	unsigned short next_arg;
	unsigned short let;

	struct amalBankPlay *amalPlayBank;
};


class ChannelTableClass
{
private:
	struct kittyChannel **tab;
	unsigned int allocated;
	unsigned int used;
public:
	ChannelTableClass()
	{
		used = 0;
		allocated = 10;
		tab = (struct kittyChannel **) malloc(sizeof(struct kittyChannel *) * allocated );
	}

	~ChannelTableClass();

	struct kittyChannel *newChannel( int channel );
	struct kittyChannel *getChannel( int channel );
	struct kittyChannel *item( int index );
	struct kittyChannel *findChannelByItem(int token, int number);
	int _size();
};

extern void setChannelAmal( struct kittyChannel *item, struct stringData *str);
extern void setChannelAnim( struct kittyChannel *item, struct stringData *str, bool enable);
extern void setChannelMoveX( struct kittyChannel *item, struct stringData *str);
extern void setChannelMoveY( struct kittyChannel *item, struct stringData *str);
extern void initChannel( struct kittyChannel *item, int channel );

