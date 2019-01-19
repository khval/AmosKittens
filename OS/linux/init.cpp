
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "joysticks.h"
#include "amoskittens.h"

#include <thread>
#include <mutex>
#include <signal.h>
#include <pthread.h>

pid_t main_task = 0;

std::mutex engine_mx;

BOOL init()
{
	int i;

	for (i=0;i<32;i++)
	{
		kitty_extensions[i].lookup = NULL;
	}


	return TRUE;
}

void closedown()
{
	int i;

	for (i=0;i<32;i++)
	{
		if (kitty_extensions[i].lookup) free(kitty_extensions[i].lookup);
		kitty_extensions[i].lookup = NULL;
	}

}

