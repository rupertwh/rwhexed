#ifndef __MEMMGR_H__
#define __MEMMGR_H__


BOOL mem_HeapInit (ULONG ulHeapSize);
PVOID mem_HeapAlloc (ULONG ulSize);
void mem_HeapFree (PVOID pObject);
void mem_HeapDestroy (void);
APIRET mem_SetSize (PVOID pBase, ULONG ulOld, ULONG ulNew);


#endif

