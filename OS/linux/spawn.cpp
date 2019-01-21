#include "stdafx.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "spawn.h"

int spawn_count = 0;

extern pid_t main_task;

void spawn_died(int32_t ret,int32_t x)
{
	spawn_count--;
	kill( main_task ,SIGQUIT );
}

int spawn( void (*fn) (), const char *name, BPTR output )
{
	int ret = 0;

	if (output)
	{

	}
	else
	{

	}


	if (ret) spawn_count ++;
	return ret;
}

void wait_spawns()
{
	while (spawn_count) sleep(1);
}
