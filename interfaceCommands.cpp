
#include <stdint.h>

#ifdef _MSC_VER
#include <string.h>
#include "vs_missing_string_functions.h"
#define strdup _strdup
#define Printf printf
#endif

#include "debug.h"

#if defined(__amigaos4__) || defined(__amigaos)
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/retroMode.h>
#include <proto/kittyCompact.h>
#include <string.h>
extern struct RastPort font_render_rp;
#endif

#ifdef __linux__
#include <string.h>
#include "os/linux/stuff.h"
#include <retromode.h>
#include <retromode_lib.h>
#define Printf printf
#ifdef test_app
#define engine_fd stdout
#else
extern FILE *engine_fd;
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "AmalCompiler.h"
#include "pass1.h"
#include "AmosKittens.h"
#include "interfacelanguage.h"
#include "commandsBanks.h"
#include "kittyErrors.h"
#include "engine.h"
#include "bitmap_font.h"
#include "amosstring.h"

extern struct TextFont *topaz8_font;

extern int sig_main_vbl;

extern struct retroVideo *video;
extern struct retroRGB DefaultPalette[256];


