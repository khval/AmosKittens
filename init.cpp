
#include <stdio.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/diskfont.h>
#include <diskfont/diskfonttag.h>
#include <proto/keymap.h>
#include <proto/Amigainput.h>
#include <string.h>
#include "joysticks.h"
#include "amoskittens.h"

struct Library 					 *AmosExtensionBase = NULL;
struct AmosExtensionIFace		 *IAmosExtension = NULL;

struct Library					*DataTypesBase = NULL;
struct DataTypesIFace			*IDataTypes = NULL;

extern struct Library		 	*DOSBase;
extern struct DOSIFace			*IDOS;

extern struct Library			*RetroModeBase;
extern struct RetroModeIFace		*IRetroMode;

struct Library 			*AslBase = NULL;
struct AslIFace 		*IAsl = NULL;

struct LocaleIFace		*ILocale  = NULL;
struct Library			*LocaleBase = NULL;

struct Library			*DiskfontBase = NULL;
struct DiskfontIFace		*IDiskfont = NULL;

struct KeymapIFace		*IKeymap = NULL;
struct Library			*KeymapBase = NULL;

struct Locale			*_locale = NULL;
ULONG				*codeset_page = NULL;


BOOL open_lib( const char *name, int ver , const char *iname, int iver, struct Library **base, struct Interface **interface)
{
	*interface = NULL;
	*base = OpenLibrary( name , ver);
	if (*base)
	{
		 *interface = GetInterface( *base,  iname , iver, TAG_END );
		if (!*interface) printf("Unable to getInterface %s for %s %ld!\n",iname,name,ver);
	}
	else
	{
	   	printf("Unable to open the %s %ld!\n",name,ver);
	}
	return (*interface) ? TRUE : FALSE;
}

BOOL init()
{
	int i;

	for (i=0;i<32;i++)
	{
		kitty_extensions[i].base = NULL;
#ifdef amigaos4
		kitty_extensions[i].interface = NULL;
#endif
		kitty_extensions[i].lookup = NULL;
	}

	if ( ! open_lib( "asl.library", 0L , "main", 1, &AslBase, (struct Interface **) &IAsl  ) ) return FALSE;
	if ( ! open_lib( "datatypes.library", 0L , "main", 1, &DataTypesBase, (struct Interface **) &IDataTypes  ) ) return FALSE;
	if ( ! open_lib( "locale.library", 53 , "main", 1, &LocaleBase, (struct Interface **) &ILocale  ) ) return FALSE;
	if ( ! open_lib( "keymap.library", 53, "main", 1, &KeymapBase, (struct Interface **) &IKeymap) ) return FALSE;
	if ( ! open_lib( "diskfont.library", 50L, "main", 1, &DiskfontBase, (struct Interface **) &IDiskfont  ) ) return FALSE;
	if ( ! open_lib( "retromode.library", 1L , "main", 1, &RetroModeBase, (struct Interface **) &IRetroMode  ) ) return FALSE;
	if ( ! open_lib( "AmigaInput.library", 50L , "main", 1, &AIN_Base, (struct Interface **) &IAIN  ) ) return FALSE;

	_locale = (struct Locale *) OpenLocale(NULL);

	if (_locale)
	{
		codeset_page = (ULONG *) ObtainCharsetInfo(DFCS_NUMBER, (ULONG) _locale -> loc_CodeSet , DFCS_MAPTABLE);
	}

	return TRUE;
}

void closedown()
{
	int i;

	for (i=0;i<32;i++)
	{
#ifdef amigaos4
		if (kitty_extensions[i].interface) DropInterface(kitty_extensions[i].interface);
		kitty_extensions[i].interface = NULL;
#endif
		if (kitty_extensions[i].base) CloseLibrary(kitty_extensions[i].base);
		kitty_extensions[i].base = NULL;

		if (kitty_extensions[i].lookup) free(kitty_extensions[i].lookup);
		kitty_extensions[i].lookup = NULL;
	}

	if (_locale) CloseLocale(_locale); _locale = NULL;

	if (IAIN) DropInterface((struct Interface*) IAIN); IAIN = 0;
	if (AIN_Base) CloseLibrary(AIN_Base); AIN_Base = 0;

	if (IAsl) DropInterface((struct Interface*) IAsl); IAsl = 0;
	if (AslBase) CloseLibrary(AslBase); AslBase = 0;

	if (IDataTypes) DropInterface((struct Interface*) IDataTypes); IDataTypes = 0;
	if (DataTypesBase) CloseLibrary(DataTypesBase); DataTypesBase = 0;

	if (ILocale) DropInterface((struct Interface*) ILocale); ILocale = 0;
	if (LocaleBase) CloseLibrary(LocaleBase); LocaleBase = 0;

	if (IKeymap) DropInterface((struct Interface*) IKeymap); IKeymap = 0;
	if (KeymapBase) CloseLibrary(KeymapBase); KeymapBase = 0;

	if (DiskfontBase) CloseLibrary(DiskfontBase); DiskfontBase = 0;
	if (IDiskfont) DropInterface((struct Interface*) IDiskfont); IDiskfont = 0;

	if (RetroModeBase) CloseLibrary(RetroModeBase); RetroModeBase = 0;
	if (IRetroMode) DropInterface((struct Interface*) IRetroMode); IRetroMode = 0;
}

