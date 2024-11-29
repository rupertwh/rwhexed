/*
 *  This file is part of RW Hex Editor (RWHEXED).
 *
 *  Copyright (c) 1994-1997, Rupert Weber.
 *
 *  RWHEXED is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  RWHEXED is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with RWHEXED. If not, see <https://www.gnu.org/licenses/>.
 */

/* Original file date: Aug-28-1995 */

#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS

#include <os2.h>
#include <stdlib.h>

#include "wmdefs.h"
#include "res.h"
#include "helpers.h"
#include "thread2.h"
#include "memmgr.h"
#include "globals.h"


void _Optlink t2_ThreadCall (PVOID pArg);
void _Optlink t2_SepThread (PVOID pArg);

typedef struct {
    ULONG   ulArg;
    PTHREADFUNC pFunc;
} THREADINFO;
typedef THREADINFO* PTHREADINFO;


/*********************************************
*
*   t2_InitThread
*
*********************************************/

BOOL t2_InitThread (HAB hab)
{
    APIRET  rc;

    rc = DosCreateEventSem (NULL, &g.hevT2Ready, 0, FALSE);
    if (NO_ERROR != rc) {
        hlp_MsgResDos (hab, IDS_ERR_DOS_CANTCREATESEM, rc, HLP_MSGLEVEL_ERR);
        return FALSE;
    }

    g.tidT2 = (TID) _beginthread (t2_ThreadCall, NULL, 16384, NULL);
    if (-1 == g.tidT2) {
        hlp_MsgResDos (hab, IDS_ERR_DOS_CANTCREATET2, rc, HLP_MSGLEVEL_ERR);
        DosCloseEventSem (g.hevT2Ready);
        return FALSE;
    }
    return TRUE;
}


/*********************************************
*
*   t2_ThreadCall
*
*********************************************/

void _Optlink t2_ThreadCall (PVOID pArg)
{
    QMSG        qmsg;
    BOOL        bReadyPosted = FALSE;
    PTHREADINFO pti;
    HAB         hab;

    if (NULLHANDLE != (hab = WinInitialize (0)))
        g.hmqT2 = WinCreateMsgQueue (hab, 0);

    if (NULLHANDLE != g.hmqT2) {
        DosPostEventSem (g.hevT2Ready);
        bReadyPosted = TRUE;
        while (WinGetMsg (hab, &qmsg, NULLHANDLE, 0,0)) {
            switch (qmsg.msg)
            {
              case WM_USR_T2CALL:
                (*((void (*)(HAB,ULONG)) PVOIDFROMMP (qmsg.mp1))) (hab, LONGFROMMP (qmsg.mp2));
                break;

              case WM_USR_T2OWNTHREAD:
                pti = (PTHREADINFO) mem_HeapAlloc (sizeof (THREADINFO));
                pti->ulArg = LONGFROMMP (qmsg.mp2);
                pti->pFunc = (PTHREADFUNC) PVOIDFROMMP (qmsg.mp1);
                _beginthread (t2_SepThread, NULL, 16384, pti);
                break;

            }
        }
        DosEnterCritSec ();
        WinDestroyMsgQueue (g.hmqT2);
        g.hmqT2 = NULLHANDLE;
    }

    if (hab)
        WinTerminate (hab);

    if (!bReadyPosted)
        DosPostEventSem (g.hevT2Ready);

}


void _Optlink t2_SepThread (PVOID pArg)
{
    HAB         hab = NULLHANDLE;
    PTHREADINFO pti;

    pti = (PTHREADINFO) pArg;
    hab = WinInitialize (0);

    (*pti->pFunc) (hab, pti->ulArg);

    mem_HeapFree (pti);
    WinTerminate (hab);
}
