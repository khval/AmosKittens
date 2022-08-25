
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/locale.h>
#include <proto/diskfont.h>
#include <diskfont/diskfonttag.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/Amigainput.h>
#include <proto/icon.h>
#include <proto/wb.h>
#include <proto/intuition.h>
#include <intuition/pointerclass.h>
#include <proto/retroMode.h>
#include <proto/kittyCompact.h>
#include "kittyAudio.h"

#include "joysticks.h"
#include "amoskittens.h"
#include "debug.h"

struct TextFont *topaz8_font = NULL;
struct TextFont *gfx_font = NULL;

extern struct TextFont *open_font( char const *filename, int size );

struct Process *main_task = NULL;

struct Library 				*AmosExtensionBase = NULL;
struct AmosExtensionIFace	*IAmosExtension = NULL;

struct Library				*DataTypesBase = NULL;
struct DataTypesIFace		*IDataTypes = NULL;

extern struct Library		 	*DOSBase ;
extern struct DOSIFace		*IDOS ;

struct DebugIFace		*IDebug = NULL;

struct Library 			*AHIBase = NULL;
struct AHIIFace			*IAHI = NULL;

struct Library 			*AslBase = NULL;
struct AslIFace			*IAsl = NULL;

struct LocaleIFace		*ILocale  = NULL;
struct Library			*LocaleBase = NULL;

struct Library			*DiskfontBase = NULL;
struct DiskfontIFace		*IDiskfont = NULL;

struct Library			*GadToolsBase = NULL;
struct DiskfontIFace		*IGadTools = NULL;

struct KeymapIFace		*IKeymap = NULL;
struct Library			*KeymapBase = NULL;

struct Locale			*_locale = NULL;
ULONG				*codeset_page = NULL;

struct Library 			* RetroModeBase = NULL;
struct RetroModeIFace 	*IRetroMode = NULL;

struct WorkbenchIFace	*IWorkbench = NULL;
struct Library			*WorkbenchBase = NULL;

struct IconIFace		*IIcon = NULL;
struct Library			*IconBase = NULL;

struct Library			*IntuitionBase = NULL;
struct IntuitionIFace		*IIntuition = NULL;

struct Library			*GraphicsBase = NULL;
struct GraphicsIFace		*IGraphics = NULL;

struct Library			*LayersBase = NULL;
struct LayersIFace		*ILayers = NULL;

struct Library			*kittyCompactBase = NULL;
struct kittyCompactIFace	*IkittyCompact = NULL;

struct Library 			*RequesterBase = NULL;
struct RequesterIFace	*IRequester= NULL;

APTR engine_mx = 0;
APTR channel_mx[4] = { 0,0,0,0 };

bool remove_words(char *name,const char **list);

UWORD *EmptyPointer = NULL;

#ifdef __amigaos3__
UWORD *ImagePointer = NULL;
#endif

#ifdef __amigaos4__
uint32 *ImagePointer = NULL;
Object *objectPointer = NULL;
#endif

const char *prefixList[] = 
{
	"_",
	"AMOSPro",
	".lib",
	NULL
};

struct BitMap *pointerBitmap = NULL;

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

const char *newprefix = "kitty";
const char *newsuffix  = ".library";

#ifdef __amigaos4__
// can't use inline for extensions
#undef makeLookupTable
#undef makeContext
#undef FreeLookupTable
#undef FreeContext
#endif

void open_extension( const char *name, int id )
{
	int l;
	bool new_format = false;
	char					*newName = NULL;
	struct Library			*extBase = NULL;
	struct kittyCompactIFace	*Iext = NULL;

	if (strlen(name)>8)
	{
		if (strcmp((name + strlen(name) - 8),".library")==0) new_format = true;
	}

	if (new_format)
	{
		char *c;
		newName = strdup( name );

		for (c=newName;*c;c++)
		{
			if ((*c>='A')&&(*c<='Z')) *c = *c-'A'+'a';
		}		
	}
	else
	{
		char	*ext = strdup(name);
		if (ext == NULL) goto cleanup;

		if (remove_words( ext, prefixList ))
		{
			l = strlen(newprefix) + strlen( ext ) + strlen( newsuffix) + 1;
			newName = (char *) malloc(l);
			if (newName == NULL)
			{
				free(ext);
				goto cleanup;
			}
			sprintf(newName,"%s%s%s",newprefix,ext,newsuffix);
		}
		else newName = strdup( ext );
		free(ext);
	}

	if (newName == NULL) goto cleanup;

	extension_printf("%s\n",newName);

	if ( open_lib( newName, 1L , "main", 1, &extBase, (struct Interface **) &Iext  ) )
	{
		extension_printf("Extension is now open\n");

		kitty_extensions[ id ].base = extBase;
		kitty_extensions[ id ].interface = (struct Interface *) Iext;
		kitty_extensions[ id ].lookup = (char *) Iext -> makeLookupTable();

		instance.extensions_context[ id ] = (char *) Iext -> makeContext();

		extension_printf("kitty_extensions[ %d ].lookup = %08x\n", id, kitty_extensions[ id ].lookup);
		extension_printf("instance.extensions_context[ %d ] = %08x\n", id, instance.extensions_context[ id ]);

		return;
	}

	extension_printf("iext %08x, extBase %08x\n",Iext,extBase);

	if (Iext) DropInterface((struct Interface*) Iext); Iext = 0;
	if (extBase) CloseLibrary(extBase); extBase = 0;

cleanup:
	if (newName) free(newName);

}


BOOL init()
{
	int i;

	main_task = (struct Process *) FindTask(NULL);

	for (i=0;i<32;i++)
	{
		kitty_extensions[i].base = NULL;
#ifdef amigaos4
		kitty_extensions[i].interface = NULL;
#endif
		kitty_extensions[i].lookup = NULL;
	}

	IDebug = (DebugIFace*) GetInterface( SysBase,"debug",1,TAG_END);
	if ( IDebug == NULL ) return FALSE;

	if ( ! open_lib( "asl.library", 0L , "main", 1, &AslBase, (struct Interface **) &IAsl  ) ) return FALSE;
	if ( ! open_lib( "datatypes.library", 0L , "main", 1, &DataTypesBase, (struct Interface **) &IDataTypes  ) ) return FALSE;
	if ( ! open_lib( "locale.library", 53 , "main", 1, &LocaleBase, (struct Interface **) &ILocale  ) ) return FALSE;
	if ( ! open_lib( "keymap.library", 53, "main", 1, &KeymapBase, (struct Interface **) &IKeymap) ) return FALSE;
	if ( ! open_lib( "diskfont.library", 50L, "main", 1, &DiskfontBase, (struct Interface **) &IDiskfont  ) ) return FALSE;
	if ( ! open_lib( "retromode.library", 1L , "main", 1, &RetroModeBase, (struct Interface **) &IRetroMode  ) ) return FALSE;
	if ( ! open_lib( "AmigaInput.library", 50L , "main", 1, &AIN_Base, (struct Interface **) &IAIN  ) ) return FALSE;
	if ( ! open_lib( "gadtools.library", 53L , "main", 1, &GadToolsBase, (struct Interface **) &IGadTools  ) ) return FALSE;
	if ( ! open_lib( "intuition.library", 51L , "main", 1, &IntuitionBase, (struct Interface **) &IIntuition  ) ) return FALSE;
	if ( ! open_lib( "graphics.library", 54L , "main", 1, &GraphicsBase, (struct Interface **) &IGraphics  ) ) return FALSE;
	if ( ! open_lib( "layers.library", 54L , "main", 1, &LayersBase, (struct Interface **) &ILayers  ) ) return FALSE;
	if ( ! open_lib( "workbench.library", 53 , "main", 1, &WorkbenchBase, (struct Interface **) &IWorkbench ) ) return FALSE;
	if ( ! open_lib( "icon.library", 53, "main", 1, &IconBase, (struct Interface **) &IIcon) ) return FALSE;
	if ( ! open_lib( "kittycompact.library", 53, "main", 1, &kittyCompactBase, (struct Interface **) &IkittyCompact) ) return FALSE;
	if ( ! open_lib( "requester.class", 53, "main", 1, &RequesterBase, (struct Interface **) &IRequester) ) return FALSE;

	_locale = (struct Locale *) OpenLocale(NULL);

	if (_locale)
	{
		codeset_page = (ULONG *) ObtainCharsetInfo(DFCS_NUMBER, (ULONG) _locale -> loc_CodeSet , DFCS_MAPTABLE);
	}

	topaz8_font =  open_font( "topaz.font" ,  8);
	if ( ! topaz8_font ) return FALSE;

	engine_mx = (APTR) AllocSysObjectTags(ASOT_MUTEX, TAG_DONE);
	if ( ! engine_mx) return FALSE;

	for (i=0;i<4;i++)
	{
		channel_mx[i] = (APTR) AllocSysObjectTags(ASOT_MUTEX, TAG_DONE);
	}

	// bitmap 16 bit alighed width = 2 bytes, 8 layers = 16 bytes

	EmptyPointer = (UWORD*)  AllocVecTags( 16, 
					AVT_Type, MEMF_SHARED,
					AVT_ClearWithValue, 0,
					TAG_END );

#ifdef __amigaos4__

	ImagePointer = (uint32*)  AllocVecTags( 64 * 64 * sizeof(uint32), AVT_Type, MEMF_SHARED,
					AVT_ClearWithValue, 0, TAG_END );

	pointerBitmap = AllocBitMapTags(64,64, 32, BMATags_PixelFormat, PIXF_CLUT, TAG_END );

	if ((ImagePointer)&&(pointerBitmap))
	{
		struct RastPort rp;
		int x,y;

		InitRastPort(&rp);
		rp.BitMap = pointerBitmap;

		for (y=0;y<64;y++)
		{
			for (x=0;x<64;x++)
			{
				WritePixel( &rp, x, y );
				ImagePointer[ y*64+x ] = (x*255/63)*0x01010101;
			}
		}

		objectPointer = NewObject( NULL, POINTERCLASS,
					POINTERA_BitMap, pointerBitmap, 
					POINTERA_ImageData, ImagePointer,
					POINTERA_Width, 64,
					POINTERA_Height, 64,
 					TAG_END );
	}

#endif

#ifdef __amigsos3__
	ImagePointer = (UWORD*)  AllocVecTags( 32 * 32, 
					AVT_Type, MEMF_SHARED,
					AVT_ClearWithValue, 0,
					TAG_END );

	if ( ! ImagePointer ) return FALSE;
#endif

	if ( ! EmptyPointer ) return FALSE;

	return TRUE;
}


void closedown()
{
	int i;

	cleanup_printf("%s:%s:%d\n",__FILE__,__FUNCTION__,__LINE__);

	for (i=0;i<32;i++)
	{
		cleanup_printf("kitty_extensions %d\n",i);

		if (kitty_extensions[i].interface) 
		{
			struct kittyCompactIFace *iext = (struct kittyCompactIFace *)  kitty_extensions[i].interface;

			if (instance.extensions_context[i]) iext -> FreeContext(instance.extensions_context[i]);
			instance.extensions_context[i] = NULL;

			if (kitty_extensions[i].lookup) iext -> FreeLookupTable(kitty_extensions[i].lookup);
			kitty_extensions[i].lookup = NULL;

			DropInterface(kitty_extensions[i].interface);
			kitty_extensions[i].interface = NULL;
		}

		if (kitty_extensions[i].base) CloseLibrary(kitty_extensions[i].base);
		kitty_extensions[i].base = NULL;

	}

	cleanup_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (topaz8_font) CloseFont(topaz8_font); topaz8_font= NULL;

	if (_locale) CloseLocale(_locale); _locale = NULL;

	cleanup_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ( EmptyPointer ) 
	{
		FreeVec( EmptyPointer );
		EmptyPointer = NULL;
	}

	cleanup_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if ( ImagePointer ) 
	{
		FreeVec( ImagePointer );
		ImagePointer = NULL;
	}

	cleanup_printf("%s:%d\n",__FUNCTION__,__LINE__);

#ifdef __amigaos4__
	if ( objectPointer )
	{
		DisposeObject( objectPointer );
		objectPointer = NULL;
	}
#endif

	cleanup_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (IDebug) DropInterface((struct Interface*) IDebug); IDebug = 0;

	if (IIcon) DropInterface((struct Interface*) IIcon); IIcon = 0;
	if (IconBase) CloseLibrary(IconBase); IconBase = 0;

	if (IWorkbench) DropInterface((struct Interface*) IWorkbench); IWorkbench = 0;
	if (WorkbenchBase) CloseLibrary(WorkbenchBase); WorkbenchBase = 0;

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

	if (IntuitionBase) CloseLibrary(IntuitionBase); IntuitionBase = 0;
	if (IIntuition) DropInterface((struct Interface*) IIntuition); IIntuition = 0;

	if (LayersBase) CloseLibrary(LayersBase); LayersBase = 0;
	if (ILayers) DropInterface((struct Interface*) ILayers); ILayers = 0;

	if (GraphicsBase) CloseLibrary(GraphicsBase); GraphicsBase = 0;
	if (IGraphics) DropInterface((struct Interface*) IGraphics); IGraphics = 0;

	if (kittyCompactBase) CloseLibrary(kittyCompactBase); kittyCompactBase = 0;
	if (IkittyCompact) DropInterface((struct Interface*) IkittyCompact); IkittyCompact = 0;

	if (RequesterBase) CloseLibrary(RequesterBase); RequesterBase = 0;
	if (IRequester) DropInterface((struct Interface*) IRequester); IRequester = 0;

	cleanup_printf("%s:%d\n",__FUNCTION__,__LINE__);

	if (engine_mx) 
	{
		FreeSysObject(ASOT_MUTEX, engine_mx); 
		engine_mx = NULL;
	}

	cleanup_printf("%s:%d\n",__FUNCTION__,__LINE__);

	for (i=0;i<4;i++)
	{
		if (channel_mx[i]) 
		{
			FreeSysObject(ASOT_MUTEX, channel_mx[i]); 
			channel_mx[i] = NULL;
		}
	}
}

bool remove_words(char *name,const char **list)
{
	const char **i;
	char *src;
	char *dest = name;
	bool found;
	bool word_removed = false;
	int sym;

	for (src = name;*src;src++)
	{

		found = false;

		for (i=list;*i;i++)
		{
			if (strncasecmp( src, *i , strlen(*i) ) == 0 )
			{
				src += strlen(*i);
				src--;
				found = true;
				word_removed = true;
				continue;
			}
		}

		if (found) continue;

		sym = *src;

		if ((sym>='A')&&(sym<='Z')) sym=sym-'A'+'a';

		*dest = sym;
		dest++;
	}
	*dest = 0;

	if (strcmp(name,"") == 0)
	{
		sprintf(name,"cmd");
	}
	return word_removed;
}


