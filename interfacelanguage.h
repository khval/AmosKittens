
struct ivar
{
	int type;
	int num;
	struct stringData *str;
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
	union 
	{
		int pos;
		int value;
	};
	int event;	// is reset on dialog command, used read by rdialog command.

	char *script_action;

	void (*update) (struct zone_base *base, struct cmdcontext *context, int args, int arg1,int arg2,int arg3);
	void (*mouse_event) (struct zone_base *base, struct cmdcontext *context, int mx, int my, int zid);	// default
	void (*render) (struct zone_base *);
};

#define I_FUNC_RENDER (void (*) (struct zone_base *))

struct zone_button : zone_base
{
	zone_button();

	char *script_render;
};

struct zone_hslider : zone_base
{
	zone_hslider();

	int trigger;
	int total;
	int step;
};

struct zone_vslider : zone_base
{
	zone_vslider();

	int trigger;
	int total;
	int step;
};

struct zone_edit : zone_base
{
	zone_edit();

	struct stringData *string;
	int max;
	int pen;
	int paper;
	int x1;
	int y1;
};

struct zone_hypertext : zone_base
{
	zone_hypertext();

	void *address;
	int buffer;
	int paper;
	int pen;
};

enum 
{
	iz_none,
	iz_button,
	iz_hslider,
	iz_vslider,
	iz_hypertext
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
	struct retroBlock *saved_block;
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
	struct stringData *script;
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
	bool exit_run;
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

extern void isetvarstr( struct cmdcontext *context,int index, struct stringData *str);
extern void isetvarnum( struct cmdcontext *context,int index,int num);

extern stringData *igetvarstr( struct cmdcontext *context, int index);
extern int igetvarnum( struct cmdcontext *context, int index);

extern void init_interface_context( struct cmdcontext *context, int id, struct stringData *script, int x, int y, int varSize, int bufferSize  );
extern void cleanup_interface_context( struct cmdcontext *context );
extern void execute_interface_script( struct cmdcontext *context, int32_t label);
extern void do_events_interface_script(  struct cmdcontext *context, int event, int delay );

