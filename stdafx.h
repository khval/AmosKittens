// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _CRT_NONSTDC_NO_DEPRECATE

#ifdef _MSC_VER
#include "targetver.h"
#endif

#include <stdio.h>

#ifdef _MSC_VER
#include <tchar.h>
#endif

#if defined(__amigaos4__)
#define sys_public_alloc(size) AllocVecTags( size, AVT_Type, MEMF_SHARED, TAG_END )
#define sys_public_alloc_clear(size) AllocVecTags( size, AVT_Type, MEMF_SHARED, AVT_ClearWithValue, 0, TAG_END )
#define sys_priv_alloc(size) AllocVecTags( size, AVT_Type, MEMF_PRIVATE, TAG_END )
#define sys_priv_alloc_clear(size) AllocVecTags( size, AVT_Type, MEMF_PRIVATE, AVT_ClearWithValue, 0, TAG_END )
#define sys_memavail_gfxmem()
#define sys_memavail_sysmem()
#define sys_free FreeVec
#endif

#if defined(__linux__)
#define BOOL bool
#define FALSE false
#define TRUE true
#define sys_public_alloc(size) malloc(size)
#define sys_public_alloc_clearsize(size) calloc(1,size)
#define sys_priv_alloc(size) malloc(size)
#define sys_priv_alloc_clear(size) calloc(1,size)
#define sys_memavail_gfxmem() linux_memavail_gfxmem()
#define sys_memavail_sysmem() linux_memavail_sysmem()
#define sys_free free
#define Delay(n) sleep(n)
#define ULONG uint16_t
typedef void * APTR;
typedef void * BPTR;
#define TAG_END 0
#endif
// TODO: reference additional headers your program requires here
