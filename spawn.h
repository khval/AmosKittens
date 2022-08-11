
// Copyright 2015, Kjetil Hvalstrand, MIT license.

extern int spawn_count;

#ifdef __amigaos4__
struct Process *spawn( void (*fn) (), const char *name, BPTR output );		// create a new spawn.
#endif

#ifdef __linux__
// returns pid+1 number for Linux, so it return 0 on error.
int spawn( void (*fn) (), const char *name, BPTR output );	
#endif


void wait_spawns();										// wait for spawns to exit

