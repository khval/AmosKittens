
struct kittyChannel;

struct kittyChannel
{
	unsigned short id;
	unsigned short token;
	unsigned short number;
	void (*cmd) (struct kittyChannel *self);
	char *script;
	char *at;
	int deltax;
	int deltay;
	int sleep;
	int sleep_to;
	int count;
	int count_to;
	int frame;
	bool active;
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
		if (*tab) free(*tab);
		*tab = NULL;
	}

	struct kittyChannel *newChannel( int channel );
	struct kittyChannel *getChannel( int channel );
	struct kittyChannel *item( int index );
	int _size();
};

