
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

	struct ivar value;
	struct ivar pos;

	int event;	// is reset on dialog command, used read by rdialog command.

	struct ivar params[9];		// index 0 to 8 == P1 to P9
	char *script_action;

	void (*update) (struct zone_base *base, struct cmdcontext *context, int args, int arg1,int arg2,int arg3);
	void (*mouse_event) (struct zone_base *base, struct cmdcontext *context, int mx, int my, int zid);	// default
	void (*render) (struct zone_base *);

	zone_base();
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


struct zone_activelist : zone_base
{
	zone_activelist();
	struct stringArrayData *array;
	int paper;
	int pen;
	int flag;
};

enum 
{
	iz_none,
	iz_button,
	iz_hslider,
	iz_vslider,
	iz_hypertext,
	iz_activelist
};

struct izone
{
	int id;
	int type;
	struct zone_base *custom;
};

struct userDefined
{
	userDefined();	
	char name[4];		// is always 2 chars, +1 zero, +1 pad
	int len;
	int args;
	const char *action;
};

struct iblock
{
	bool (*start_fn)( struct cmdcontext *context, struct cmdinterface *self );
	void (*end_fn)( struct cmdcontext *context);

	void set(
			bool (*start_fn)( struct cmdcontext *, struct cmdinterface * ),
			void (*end_fn)( struct cmdcontext *)
		 );
};

class cmdcontext
{
	public:
		// metods
		struct izone *findZone(int id);
		struct izone *setZone(int id,struct zone_base *obj);
		struct userDefined *findUserDefined( const char *name );
		void dumpUserDefined();
		void resetZoneEvents();
		void dumpZones();
		
		void flushZones();
		void flushVars();
		void flushUserDefined();

		cmdcontext();
		~cmdcontext();

		std::vector<struct izone> zones;

		// data
		int id;
		char *tokenBuffer;
		struct retroBlock *saved_block;
		bool tested;
		int stackp;
		int lstackp;
		struct ivar stack[20];
		struct ivar *vars;

		struct ivar params[9];			// index 0 to 8 == P1 to P9
		struct ivar *current_params;		// points to the current params

		struct ivar *params_backup[10];
		int ui_stackp;
		struct ivar	defaultZoneValue;
			
		char *labels[512];
		int programStackCount;
		char *programStack[10];
		int selected_dialog;
		struct dialog dialog[2];
		struct stringData *script;
		void (*cmd_done)( struct cmdcontext *context, struct cmdinterface *self );
		int args;
		int expected;
		int error;
		char *at;
		int l;
		int image_offset;
		int block_level;

		// if true increment block counter, if false block was skipped, end block not expected.

		struct iblock *iblocks;

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
		std::vector<struct userDefined> userDefineds;
		struct userDefined *ui_current;
		int pass_store;
};

struct cmdinterface
{
	const char *name;
	short len;
	short type;
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
extern void execute_interface_script( struct cmdcontext *context, int32_t label);
extern void do_events_interface_script(  struct cmdcontext *context, int event, int delay );

