
struct kittyChannel;
struct amalCallBack;

#define API_AMAL_CALL_ARGS ( struct kittyChannel *self, void **code, unsigned int opt )

struct amalCallBack
{
	void **code ;
	void *(*cmd) (struct kittyChannel *self, struct amalCallBack *cb);
	void *ret;
	unsigned char last_reg;
	int argStackCount;
	int progStackCount;
	amal::Flags Flags;
};


namespace channel_status
{
	enum status
	{
		uninitialized,	// No amal program.
		initialized,		// Have amal program
		done,		// Amal program is done.
		active,		// Amal program is running.
		paused,		// Same as exit amal prgram at VBL
		frozen		// Stops the amal program, until its unfrozen.
	};
};

struct channelAPI
{
	int (*getMax) ( void );
	int (*getImage) (int object);
	int (*getX) (int object);
	int (*getY) (int object);
	void (*setImage) (int object,int);
	void (*setX) (int object,int);
	void (*setY) (int object,int);
};

struct kittyChannel
{
	unsigned short id;
	unsigned short token;
	unsigned short number;
	char *amal_script;
	char *amal_at;
	char *anim_script;
	char *anim_at;
	int anim_loops;

	// move x and move y
	char *movex_script;
	char *movex_at;
	char *movey_script;
	char *movey_at;
	int deltax;
	int deltay;
	int sleep;
	int sleep_to;
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
	channel_status::status status;
	int reg[10];	// local reg 0 to 9 
	int parenthses;
	int *argStack;
	struct amalBuf amalProg;
	void *(**amalProgCounter) API_AMAL_CALL_ARGS;
	unsigned int argStackCount;
	struct amalCallBack *progStack;
	unsigned int progStackCount;
	unsigned int loopCount; 
	unsigned int last_reg;
	void *(*pushBackFunction)  (struct kittyChannel *self, struct amalCallBack *cb);
};


class ChannelTableClass
{
private:
	struct kittyChannel **tab;
	int allocated;
	int used;
public:
	ChannelTableClass()
	{
		used = 0;
		allocated = 10;
		tab = (struct kittyChannel **) malloc(sizeof(struct kittyChannel *) * allocated );
	}

	~ChannelTableClass()
	{
		if (tab) free(tab);
		tab = NULL;
	}

	struct kittyChannel *newChannel( int channel );
	struct kittyChannel *getChannel( int channel );
	struct kittyChannel *item( int index );
	int _size();
};

extern void setChannelAmal( struct kittyChannel *item, char *str);
extern void setChannelAnim( struct kittyChannel *item, char *str);
extern void setChannelMoveX( struct kittyChannel *item, char *str);
extern void setChannelMoveY( struct kittyChannel *item, char *str);
extern void initChannel( struct kittyChannel *item, int channel );

