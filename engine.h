
extern bool start_engine();
extern void wait_engine();

extern void engine_lock();
extern void engine_unlock();
extern bool engine_ready();
extern void engine_ShowMouse( ULONG enable );

extern void set_default_colors( struct retroScreen *screen );
extern void clear_cursor( struct retroScreen *screen );
extern void draw_cursor( struct retroScreen *screen );
extern void atomic_add_key( ULONG eventCode, ULONG Code, ULONG Qualifier, char Char );
extern void run_amal_scripts();

extern bool engine_wait_key;
extern uint32_t engine_back_color;
extern bool engine_stopped;
extern bool engine_mouse_hidden;

extern uint32_t engine_update_flags ;

extern bool synchro_on;

#define hardware_upper_left 128
#define hardware_upper_top 50

#ifdef __amigaos4__
extern Process *EngineTask;
#endif

enum
{
	GID_ICONIFY = 1,
	GID_FULLSCREEN,
	GID_PREFS
};

enum
{
	kitty_to_back = 1,
	kitty_to_front,
	kitty_limit_mouse
};

enum
{
	kitty_key_up,
	kitty_key_down
};

struct keyboard_buffer
{
	ULONG event;
	ULONG Code;
	ULONG Qualifier;
	char	Char;
};

struct amos_selected
{
	ULONG menu;
	ULONG item;
	ULONG sub;
};

struct amosMenuItem
{
	int levels;
	int index[3];
	char *str;
	char *key;
	unsigned short scancode;
	unsigned short qualifier;
	bool active;
};

