
struct windowclass
{
	struct Window *win;
	ULONG window_left;
	ULONG window_top;
	ULONG window_width;
	ULONG window_height;
};

extern struct windowclass window_save_state;
extern struct Window *My_Window;

extern void open_fullscreen(ULONG ModeID);
extern void save_window_attr(windowclass *self);
extern void close_engine_window();
extern bool open_engine_window( int window_left, int window_top, int window_width, int window_height );


