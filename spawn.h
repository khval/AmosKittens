
extern int spawn_count;

struct Process *spawn( void (*fn) (), const char *name, BPTR output );		// create a new spawn.
void wait_spawns();										// wait for spawns to exit

