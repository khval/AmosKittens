
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <math.h>
#include <signal.h>

#ifdef __amigaos4__
#include <proto/exec.h>
#include <proto/dos.h>
#include <libraries/retroMode.h>
#include <proto/retroMode.h>
#endif
#include "amosString.h"

void os_text_width(stringData*txt)
{
}

void os_text_height(stringData*txt)
{
}

void os_text_base(stringData*txt)
{
}

void os_text_no_outline(retroScreen *screen, int x , int y, stringData *str, int style)
{
}

void os_text_outline(retroScreen *screen, int x, int y, stringData *str, int a, int b)
{
}

void draw_glyph(retroScreen *screen, TextFont *tf, int a, int b, int c, int d)
{
}

