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

#if defined(__linux__)
#define BOOL bool
#define FALSE false
#define TRUE true
#define sys_alloc(size) malloc(size)
#define sys_alloc_clear(size) calloc(1,size)
#define sys_free free
#define Delay(n) sleep(n)
#define ULONG uint16_t
typedef void * APTR;
#define TAG_END 0
#endif
// TODO: reference additional headers your program requires here
