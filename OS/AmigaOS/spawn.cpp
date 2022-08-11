
// Copyright 2015, Kjetil Hvalstrand, MIT license.

#include <proto/exec.h>
#include <proto/dos.h>

#include <stdlib.h>
#include <stdio.h>

#include "spawn.h"

int spawn_count = 0;

extern struct Task *main_task;

void spawn_died(int32 ret,int32 x)
{
	spawn_count--;
	Signal( main_task ,SIGF_CHILD);
}

struct Process *spawn( void (*fn) (), const char *name, BPTR output )
{
	struct Process *ret = NULL;

	if (output)
	{
		ret = (struct Process *) CreateNewProcTags (
	   			NP_Entry, 		(ULONG) fn,
				NP_Name,	   	name,
				NP_StackSize,   262144,
				NP_Child,		TRUE,
				NP_Priority,	0,
				NP_ExitData, 	IDOS,
				NP_FinalCode, spawn_died,
				NP_Output, output,
				TAG_DONE);
	}
	else
	{
		ret = (struct Process *) CreateNewProcTags (
	   			NP_Entry, 		(ULONG) fn,
				NP_Name,	   	name,
				NP_StackSize,   262144,
				NP_Child,		TRUE,
				NP_Priority,	0,
				NP_ExitData, 	IDOS,
				NP_FinalCode, spawn_died,
				TAG_DONE);
	}


	if (ret) spawn_count ++;
	return ret;
}

void wait_spawns()
{
	while (spawn_count) Delay(1);
}
