
struct ivar
{
	int type;
	int num;
	char *str;
};

struct dialog
{
	int x;
	int y;
	int width;
	int height;
};

struct cmdcontext
{
	int stackp;
	int lstackp;
	struct ivar stack[10];
	struct ivar vars[512];	
	uint32_t labels[512];
	void (*cmd_done)( struct cmdcontext *context, struct cmdinterface *self );
	int args;
	int error;
	char *at;
	int l;
	int ink0;
	int ink1;
	int ink3;
	struct dialog dialog;
};

struct cmdinterface
{
	const char *name;
	int type;
	void (*cmd)( struct cmdcontext *context, struct cmdinterface *self );
};

enum
{
	i_normal,
	i_parm
};


extern void isetvarstr( struct cmdcontext *context,int index, char *str);
extern void isetvarnum( struct cmdcontext *context,int index,int num);
extern void init_interface_context( struct cmdcontext *context, int x, int y );
extern void execute_interface_script( struct cmdcontext *context, char *script);

