/* Minimal C translation of the public RED4ext SDK API v1 declarations.
 * Source: https://github.com/WopsS/RED4ext.SDK/tree/master/include/RED4ext/Api/v1
 * Copyright (c) 2020 - present Octavian Dima. MIT; see ../third_party/RED4ext.SDK.LICENSE.txt.
 * No engine structures or C++ runtime are used by this plugin. */
#ifndef TMAF_RED4EXT_API_H
#define TMAF_RED4EXT_API_H
#include <stddef.h>
#include <stdint.h>
typedef void *PluginHandle;
typedef struct { uint32_t type,number; } PrereleaseInfo;
typedef struct { uint8_t major; uint16_t minor; uint32_t patch; PrereleaseInfo prerelease; } SemVer;
typedef struct { uint16_t major,minor,build,revision; } FileVer;
typedef struct { SemVer sdk; const wchar_t *name,*author; SemVer version; FileVer runtime; } PluginInfo;
typedef void (*LogText)(PluginHandle,const char *);
typedef void (*LogFormat)(PluginHandle,const char *,...);
typedef void (*LogWide)(PluginHandle,const wchar_t *);
typedef void (*LogWideFormat)(PluginHandle,const wchar_t *,...);
#define LOG_LEVEL(n) LogText n; LogFormat n##F; LogWide n##W; LogWideFormat n##WF;
typedef struct { LOG_LEVEL(Trace) LOG_LEVEL(Debug) LOG_LEVEL(Info) LOG_LEVEL(Warn) LOG_LEVEL(Error) LOG_LEVEL(Critical) } Logger;
#undef LOG_LEVEL
typedef struct { SemVer *runtime; Logger *logger; void *hooking,*gameStates,*scripts; } Sdk;
_Static_assert(sizeof(void*)==8,"This plugin requires x64");
_Static_assert(sizeof(wchar_t)==2,"Windows UTF-16 wchar_t required");
_Static_assert(sizeof(SemVer)==16 && sizeof(FileVer)==8,"RED4ext version ABI");
_Static_assert(sizeof(PluginInfo)==56 && offsetof(PluginInfo,name)==16 && offsetof(PluginInfo,version)==32 && offsetof(PluginInfo,runtime)==48,"RED4ext PluginInfo ABI");
_Static_assert(offsetof(Sdk,logger)==8 && offsetof(Logger,Info)==64 && offsetof(Logger,Error)==128,"RED4ext logger ABI");
#endif
