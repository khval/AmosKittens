
extern bool start_engine();
extern void wait_engine();

extern void engine_lock();
extern void engine_unlock();
extern void set_default_colors( struct retroScreen *screen );
extern void draw_cursor( struct retroScreen *screen );

extern int engine_mouse_key;
extern int engine_mouse_x;
extern int engine_mouse_y;
extern bool engine_started;

