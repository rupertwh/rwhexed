/* Original file date: Apr-5-1997 */

#define INCL_DOS
#define INCL_DOSERRORS

#include <os2.h>
#include <stdio.h>
#include <string.h>

#include "memmgr.h"
#ifdef DEBUG
   #include "debug.h"
#endif

#define MAXOBJS 500

#define TABLESIZE       (1 << 8)

typedef struct tag_heapobj {
    PVOID               pObj;
    ULONG               ulSize;
    struct tag_heapobj  *pnext;
} HEAPOBJ;
typedef HEAPOBJ*    PHEAPOBJ;

#ifdef DEBUG
static int      nHeapCount = 0;
static int      nMaxAlloc = 0;
#endif
static HEAPOBJ  aHeapObj[MAXOBJS];
static PHEAPOBJ heaptable[TABLESIZE];

static PVOID    pHeap;
static BOOL     bInit;
static HMTX     hmtxAccess;

static void s_MsgMutexErr (APIRET rc);
static void s_MsgCritical (PSZ pszMsg, APIRET rc);
static void s_MsgWarning (PSZ pszMsg, APIRET rc);
int _Inline s_hash8 (unsigned long val);

BOOL mem_HeapInit (ULONG ulHeapSize)
{
        APIRET  rc;
        int     i;

        bInit = FALSE;

        rc = DosCreateMutexSem (NULL, &hmtxAccess, 0, FALSE);
        if (NO_ERROR != rc) {
                s_MsgCritical ("Could not create Mutex-Semaphore!", rc);
                return bInit;
        }

        rc = DosAllocMem (&pHeap, ulHeapSize, PAG_WRITE | PAG_READ | PAG_GUARD);
        if (NO_ERROR != rc) {
                s_MsgCritical ("Could not allocate local Heap!", rc);
                return bInit;
        }

        rc = DosSubSetMem (pHeap, DOSSUB_INIT | DOSSUB_SPARSE_OBJ, ulHeapSize);
        if (NO_ERROR == rc)
                bInit = TRUE;
        else
                s_MsgCritical ("Could not initialize local Heap!", rc);

        for (i = 0; i < TABLESIZE; i++)
                heaptable[i] = NULL;

        return bInit;
}

PVOID mem_HeapAlloc (ULONG ulSize)
{
        APIRET  rc;
        PHEAPOBJ   pObject;
        char    *pret;
        int     hash;
        PHEAPOBJ        pheap, pprev = NULL;

        if (!bInit)
                return NULL;

        rc = DosSubAllocMem (pHeap, (PVOID) &pObject, ulSize + sizeof (HEAPOBJ));
        if (NO_ERROR != rc) {
                s_MsgCritical ("Could not allocate object on local Heap!", rc);
                return NULL;
        }
        hash = s_hash8 ((unsigned long) &pObject[1]);

        rc = DosRequestMutexSem (hmtxAccess, (ULONG) SEM_INDEFINITE_WAIT);
        if (NO_ERROR != rc) {
                DosSubFreeMem (pHeap, pObject, ulSize + sizeof (HEAPOBJ));
                s_MsgMutexErr (rc);
                return NULL;
        }

        for (pheap = heaptable[hash]; pheap; pprev = pheap, pheap = pheap->pnext)
                ;
        pheap = pObject;

        if (pprev)
                pprev->pnext = pheap;
        else
                heaptable[hash] = pheap;

        pheap->pObj = (PVOID) &pObject[1];
        pheap->ulSize = ulSize;
        pheap->pnext = NULL;

        DosReleaseMutexSem (hmtxAccess);

        memset (pheap->pObj, 0, ulSize);
        return pheap->pObj;




#ifdef NEVER
    rc = DosRequestMutexSem (hmtxAccess, (ULONG) SEM_INDEFINITE_WAIT);
    if (NO_ERROR != rc) {
        s_MsgMutexErr (rc);
        return NULL;
    }

    rc = DosSubAllocMem (pHeap, &pObject, ulSize);
    if (NO_ERROR != rc) {
        pObject = NULL;
        s_MsgCritical ("Could not allocate object on local Heap!", rc);
    }
    else {
        i = 0;
        while (aHeapObj[i].pObj && i < MAXOBJS)
            i++;

        if (MAXOBJS == i) {
            s_MsgCritical ("Too many Heap Objects!", 0);
            DosSubFreeMem (pHeap, pObject, ulSize);
            pObject = NULL;
        }
        else { /* mem alloc OK */
            #ifdef DEBUG
                nHeapCount++;
                if (nHeapCount > nMaxAlloc)
                    nMaxAlloc = nHeapCount;
            #endif
            aHeapObj[i].pObj = pObject;
            aHeapObj[i].ulSize = ulSize;
            memset (pObject, 0, ulSize);

        }
    }

    rc = DosReleaseMutexSem (hmtxAccess);
    if (NO_ERROR != rc) {
        s_MsgMutexErr (rc);
        return NULL;
    }
    return pObject;
#endif

}

void mem_HeapFree (PVOID pObject)
{
        APIRET          rc;
        int             hash;
        PHEAPOBJ        pheap, pprev = NULL;

        if (!(bInit && pObject))
                return;

        hash = s_hash8 ((unsigned long) pObject);

        rc = DosRequestMutexSem (hmtxAccess, (ULONG) SEM_INDEFINITE_WAIT);
        if (NO_ERROR != rc) {
                s_MsgMutexErr (rc);
                return;
        }

        for (pheap = heaptable[hash]; pheap && (pheap->pObj != pObject);
                               pprev = pheap, pheap = pheap->pnext)
                ;
        if (!pheap)
                return;

        if (pprev) {
                if (pheap->pnext)
                        pprev->pnext = pheap->pnext;
                else
                        pprev->pnext = NULL;
        }
        else
                heaptable[hash] = NULL;

        DosReleaseMutexSem (hmtxAccess);

        DosSubFreeMem (pHeap, pheap, pheap->ulSize + sizeof (HEAPOBJ));




#ifdef NEVER
    rc = DosRequestMutexSem (hmtxAccess, (ULONG) SEM_INDEFINITE_WAIT);
    if (NO_ERROR != rc) {
        s_MsgMutexErr (rc);
        return;
    }

    i = 0;
    while (pObject != aHeapObj[i].pObj && i < MAXOBJS)
        i++;

    if (MAXOBJS == i)
        s_MsgWarning ("Trying to free unregistered Heap Object!", 0);
    else {
        rc = DosSubFreeMem (pHeap, pObject, aHeapObj[i].ulSize);
        if (NO_ERROR != rc)
            s_MsgWarning ("Error freeing object on local Heap!", rc);
        else {
            aHeapObj[i].pObj = NULL;
            #ifdef DEBUG
                nHeapCount--;
            #endif
        }
    }

    rc = DosReleaseMutexSem (hmtxAccess);
    if (NO_ERROR != rc)
        s_MsgMutexErr (rc);
#endif

}

void mem_HeapDestroy (void)
{
    APIRET  rc;

    if (!bInit)
        return;

    rc = DosRequestMutexSem (hmtxAccess, (ULONG) SEM_INDEFINITE_WAIT);
    if (NO_ERROR != rc) {
        s_MsgMutexErr (rc);
        return;
    }

#ifdef DEBUG
{
    char    sz[300];

    sprintf (sz, "Maximum ever were %d allocated objects at a time.", nMaxAlloc);
    dbg_Msg (sz);
    if (nHeapCount) {
        int i;
        PSZ     psz;
        ULONG   len;
        sprintf (sz, "There are still %d local objects not freed!", nHeapCount);
        dbg_Msg (sz);
        for (i = 0; i < MAXOBJS; i++) {
            if (aHeapObj[i].pObj) {
                psz = (PSZ) aHeapObj[i].pObj;
                len = aHeapObj[i].ulSize;
                psz[len - 1] = (char) 0;
                sprintf (sz, "Size: %lu\n  Value: %s", len, psz);
                dbg_Msg (sz);
            }
        }
    }
}
#endif

    rc = DosSubUnsetMem (pHeap);
    if (NO_ERROR != rc)
        s_MsgWarning ("Error deinitializing local Heap!", rc);

    rc = DosFreeMem (pHeap);
    if (NO_ERROR != rc)
        s_MsgWarning ("Error freeing local Heap!", rc);


    DosReleaseMutexSem (hmtxAccess);
    rc = DosCloseMutexSem (hmtxAccess);
    if (NO_ERROR != rc)
        s_MsgMutexErr (rc);


}


int _Inline s_hash8 (unsigned long val)
{
        int    hash;

        hash = (long) ((val & 0x0000FFFF) ^ (val >> 16));
        hash = (hash & 0xFF) ^ ((hash >> 8) & 0xFF);

        return hash;
}




static void s_MsgMutexErr (APIRET rc)
{
    s_MsgCritical ("Mutex Semaphore Error", rc);
}


static void s_MsgCritical (PSZ pszMsg, APIRET rc)
{
    char    szMsg[200];

    sprintf (szMsg, "Critical Error: (%d)\n%s", (int) rc, pszMsg);
    WinAlarm (HWND_DESKTOP, WA_ERROR);
    WinMessageBox (HWND_DESKTOP, NULLHANDLE, szMsg, "Error", 0, MB_OK | MB_ERROR |
                                    MB_SYSTEMMODAL);
}

static void s_MsgWarning (PSZ pszMsg, APIRET rc)
{
    char    szMsg[200];

    sprintf (szMsg, "Warning Message: (%d)\n%s", (int) rc, pszMsg);
    WinAlarm (HWND_DESKTOP, WA_WARNING);
    WinMessageBox (HWND_DESKTOP, NULLHANDLE, szMsg, "Error", 0, MB_OK | MB_ERROR |
                                    MB_SYSTEMMODAL);

}


APIRET mem_SetSize (PVOID pBase, ULONG ulOld, ULONG ulNew)
{
    APIRET  rc;

    ulOld = (ulOld + 0x00000FFFUL) & 0xFFFFF000UL;
    ulNew = (ulNew + 0x00000FFFUL) & 0xFFFFF000UL;

    if (ulNew > ulOld)   /* commit pages  */
        rc = DosSetMem ((PVOID)((ULONG)pBase + ulOld), ulNew - ulOld,
                        PAG_COMMIT | PAG_READ | PAG_WRITE);
    else if (ulNew < ulOld)  /* decommit pages */
        rc = DosSetMem ((PVOID)((ULONG)pBase + ulNew), ulOld - ulNew, PAG_DECOMMIT);
    else rc = NO_ERROR;

    return rc;
}
