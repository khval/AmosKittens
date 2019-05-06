
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

struct ibutton
{
	int x;
	int y;
	int w;
	int h;
};

struct zone_base
{
	int x0,y0,x1,y1,w,h;
};

struct zone_button : zone_base
{
	void (*render) (struct zone_button *zl);
	void (*mouse_event) (struct cmdcontext *context, int mx, int my, struct zone_button *zb);

	char *script_render;
	char *script_action;
};

struct zone_slider : zone_base
{
	void (*render) (struct zone_slider *zl);
	void (*mouse_event) (struct cmdcontext *context, int mx, int my, struct zone_slider *zb);

	int pos;
	int position;
	int trigger;
	int total;
	int step;
};

enum 
{
	iz_none,
	iz_button,
	iz_slider
};

struct izone
{
	int id;
	int type;
	struct zone_base *custom;
};

struct cmdcontext
{
	int id;
	char *tokenBuffer;
	bool tested;
	int stackp;
	int lstackp;
	struct ivar stack[20];
	struct ivar *vars;
	char *labels[512];
	int programStackCount;
	char *programStack[10];
	int selected_dialog;
	struct dialog dialog[2];
	struct izone *zones;
	void (*cmd_done)( struct cmdcontext *context, struct cmdinterface *self );
	int args;
	int error;
	char *script;
	char *at;
	int l;
	int ink0;
	int ink1;
	int ink3;
	int image_offset;
	int block_level;
	void (**block_fn)( struct cmdcontext *context, struct cmdinterface *self );
	int max_vars;
	int last_zone;
	int xgcl;
	int ygcl;
	int xgc;
	int ygc;
	bool has_return_value;
	int return_value;
	bool mouse_key;
	bool button_action;
};

struct cmdinterface
{
	const char *name;
	int type;
	void (*pass)( struct cmdcontext *context, struct cmdinterface *self );
	void (*cmd)( struct cmdcontext *context, struct cmdinterface *self );
};

enum
{
	i_normal,
	i_parm
};


extern void isetvarstr( struct cmdcontext *context,int index, char *str);
extern void isetvarnum( struct cmdcontext *context,int index,int num);
extern void init_interface_context( struct cmdcontext *context, int id, char *script, int x, int y, int varSize, int bufferSize  );
extern void cleanup_inerface_context( struct cmdcontext *context );
extern void execute_interface_script( struct cmdcontext *context, int32_t label);

