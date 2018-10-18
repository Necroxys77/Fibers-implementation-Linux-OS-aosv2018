#pragma once

#define KERNEL

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
#include "UserspaceLibrary/fiberlib.h"
// TODO:
// Here you should point to the invocation of your code!
// See README.md for further details.

#define ConvertThreadToFiber() convertThreadToFiber()
#define CreateFiber(dwStackSize, lpStartAddress, lpParameter) createFiber(dwStackSize, lpStartAddress, lpParameter)
#define SwitchToFiber(lpFiber) switchToFiber(lpFiber)
#define FlsAlloc(lpCallback) flsAlloc(lpCallback)
#define FlsFree(dwFlsIndex) flsFree(dwFlsIndex)
#define FlsGetValue(dwFlsIndex) flsGet(dwFlsIndex)
#define FlsSetValue(dwFlsIndex, lpFlsData) flsSet((dwFlsIndex), (long long)(lpFlsData))

#endif /* USERSPACE */
