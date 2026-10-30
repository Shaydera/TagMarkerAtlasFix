#ifndef TMAF_WIN_API_H
#define TMAF_WIN_API_H
#include <stddef.h>
#include <stdint.h>
/* Win64 ABI declarations; intentionally avoid compiler-specific Windows SDK/CRT headers. */
__declspec(dllimport) void *GetModuleHandleW(const wchar_t *name);
__declspec(dllimport) void *GetCurrentProcess(void);
__declspec(dllimport) int VirtualProtect(void *address,size_t size,uint32_t protection,uint32_t *oldProtection);
__declspec(dllimport) int FlushInstructionCache(void *process,const void *address,size_t size);
#define PAGE_EXECUTE_READWRITE 0x40u
#endif
