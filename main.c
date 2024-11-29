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

/* Original file date: Dez-11-1996 */

#define INCL_WIN
#define INCL_DOS

#include <os2.h>

#include "wmdefs.h"
#include "memmgr.h"
#include "thread2.h"
#include "init.h"
#include "globals.h"

int main (int argc, char **parg)
{
    BOOL        bHeap = FALSE;
    QMSG        qmsg;
    HAB         hab;

    hab = WinInitialize (0);
    if (NULLHANDLE == hab) goto QuitMain;

    g.hmq = WinCreateMsgQueue (hab, 0);
    if (NULLHANDLE == g.hmq) goto QuitMain;

    bHeap = mem_HeapInit (0x10000);
    if (!bHeap)  goto QuitMain;

    if (FALSE == InitApp (hab)) goto QuitMain;

    if (argc > 1)
        WinPostMsg (g.hwndClient, WM_USR_OPENCMD, MPFROMP((PVOID)(parg[1])), 0);

    while (WinGetMsg (hab, &qmsg, NULLHANDLE, 0, 0))
        WinDispatchMsg (hab, &qmsg);

    WinDestroyWindow (g.hwndFrame);
    g.hwndClient = NULLHANDLE;

  QuitMain:
    if (g.hmqT2) {
        WinPostQueueMsg (g.hmqT2, WM_QUIT, 0, 0);
        DosWaitThread (&g.tidT2, DCWW_WAIT);
    }

    mem_HeapDestroy ();
    if (g.hmq)
        WinDestroyMsgQueue (g.hmq);
    if (hab)
        WinTerminate (hab);

    return 0;
}
