
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdarg.h>

void Printf_iso(const char *fmt,...)
{
	char buffer[2000];
	va_list args;
	va_start (args, fmt);
	vsprintf(buffer,fmt,args);
	va_end(args);

	Printf("%s",buffer);
}

