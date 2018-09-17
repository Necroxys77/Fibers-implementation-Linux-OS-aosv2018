#pragma once

#define USERSPACE

#ifdef USERSPACE

#include "src/ult.h"

#define ConvertThreadToFiber() ult_convert()
#define CreateFiber(dwStackSize, lpStartAddress, lpParameter) ult_creat(dwStackSize, lpStartAddress, lpParameter)
#define SwitchToFiber(lpFiber) ult_switch_to(lpFiber)
#define FlsAlloc(lpCallback) fls_alloc()
#define FlsFree(dwFlsIndex)	fls_free(dwFlsIndex)
#define FlsGetValue(dwFlsIndex) fls_get(dwFlsIndex)
#define FlsSetValue(dwFlsIndex, lpFlsData) fls_set((dwFlsIndex), (long long)(lpFlsData))

#else


// TODO:
// Here you should point to the invocation of your code!
// See README.md for further details.

#define ConvertThreadToFiber() {}
#define CreateFiber(dwStackSize, lpStartAddress, lpParameter) {}
#define SwitchToFiber(lpFiber) {}
#define FlsAlloc(lpCallback) {}
#define FlsFree(dwFlsIndex) {}
#define FlsGetValue(dwFlsIndex) {}
#define FlsSetValue(dwFlsIndex, lpFlsData) {}

#endif /* USERSPACE */
