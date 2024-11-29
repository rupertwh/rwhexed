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

/* Original file date: Apr-4-1997 */

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN

#include <os2.h>
#include <string.h>
#include <stdio.h>

#include "wmdefs.h"
#include "types.h"
#include "res.h"
#include "memmgr.h"
#include "helpers.h"
#include "client.h"
#include "thread2.h"
#include "fileio.h"
#include "globals.h"
#include "hewnd.h"
#include "status.h"
#include "fonts.h"
#include "init.h"
#ifdef DEBUG
    #include "debug.h"
#endif
static HWND s_CreateFrame (HAB hab);
static void s_StandardSize (PSWP pSwp);

void ini_InitAppT2 (HAB hab, ULONG ulArg);


/*********************************************
*
*   InitApp
*
*********************************************/

BOOL InitApp (HAB hab)
{
    APIRET  rc;

    g.hwndFrame = NULLHANDLE;
#ifdef DEBUG
        dbg_Msg ("Seems to go well");
#endif

    hlp_Init (NULLHANDLE);

    /* Initialize working thread T2 */
    if (FALSE == t2_InitThread (hab)) goto Abort;

    /* Wait until T2 is ready */
    rc = DosWaitEventSem (g.hevT2Ready, (ULONG) SEM_INDEFINITE_WAIT);
    if (NO_ERROR != rc) {
        hlp_MsgResDos (hab, IDS_ERR_DOS_WAITSEM, rc, HLP_MSGLEVEL_ERR);
        goto Abort;
    }
    DosCloseEventSem (g.hevT2Ready);

    if (NULLHANDLE == g.hmqT2) {
        hlp_MsgRes (hab, IDS_ERR_NOT2WINDOW, HLP_MSGLEVEL_ERR);
        goto Abort;
    }
    g.hwndFrame = s_CreateFrame (hab);


    if (g.hwndFrame) {
        char        szFacename[FACESIZE];
        int         nSize = 11;

        g.hwndMenuBar = WinWindowFromID (g.hwndFrame, FID_MENU);
        if (!fnt_BuildMonoList (g.hwndClient)) goto Abort;
        if (!(g.nFonts = (int) fnt_BuildIntList (&g.pFont))) goto Abort;
        fio_ReadFont (hab, szFacename, &nSize);
        g.nCurrFont = fnt_QueryNumFromName (szFacename, nSize);
        if (-2 == g.nCurrFont) goto Abort;
        WinPostQueueMsg (g.hmqT2, WM_USR_T2CALL, MPFROMP (ini_InitAppT2), 0);
        if (!WinSendMsg (g.hwndClient, WM_USR_CREATEHEWND, 0, 0))
            goto Abort;

    }
    else
       return FALSE;

    return TRUE;

Abort:
    if (g.hmqT2)
        WinPostQueueMsg (g.hmqT2, WM_QUIT, 0, 0);
    if (g.tidT2)
        DosWaitThread (&g.tidT2, DCWW_WAIT);
    return FALSE;
}




/* multi threaded program initialization: */

/*********************************************
*
*   ini_InitAppT2
*
*********************************************/

void ini_InitAppT2 (HAB hab, ULONG ulArg)
{
    WINPOS  winpos;
    SWP     swp;

    hlp_init_hextable ();

    memset (&winpos, 0, sizeof (WINPOS));
    memset (&swp, 0, sizeof (SWP));
    s_StandardSize (&swp);
    swp.fl = SWP_MOVE | SWP_SIZE | SWP_ACTIVATE | SWP_SHOW;
    if (fio_ReadWindowPos (hab, &winpos)) {
        if (winpos.cx && winpos.cy) {
            swp.x = winpos.x;
            swp.y = winpos.y;
            swp.cx = winpos.cx;
            swp.cy = winpos.cy;
        }
        swp.fl |= (ULONG) (winpos.bMax ? SWP_MAXIMIZE : 0);
    }

    WinSetWindowPos (g.hwndFrame, NULLHANDLE, swp.x, swp.y, swp.cx, swp.cy, swp.fl);
    WinShowWindow (g.hwndFrame, TRUE);
}


/*********************************************
*
*   s_CreateFrame
*
*********************************************/

static HWND s_CreateFrame (HAB hab)
{
    PSZ     pszClass = NULL, pszTitle = NULL;
    BOOL    bSuccess;
    ULONG   ulFrameFlags;
    HWND    hwndFrame = NULLHANDLE;

    pszClass = hlp_GetString (hab, IDS_HEXCLASS);
    bSuccess = hex_RegisterClass (hab, pszClass);
    hlp_FreeString (pszClass);

    pszClass = hlp_GetString (hab, IDS_STATUSCLASS);
    bSuccess = (BOOL) (bSuccess && st_RegisterClass (hab, pszClass));
    hlp_FreeString (pszClass);

    pszTitle = hlp_GetString (hab, IDS_APPTITLE);

    pszClass = hlp_GetString (hab, IDS_CLIENTCLASS);
    if (!(pszClass && pszTitle))
        return NULLHANDLE;
    bSuccess = (BOOL) (bSuccess && WinRegisterClass (hab, pszClass, ClientWndProc,
                                CS_SIZEREDRAW, sizeof (PVOID)));

    if (bSuccess) {
        ulFrameFlags = FCF_TITLEBAR | FCF_SYSMENU | FCF_MENU | FCF_MINMAX | FCF_SIZEBORDER
                        | FCF_ICON | FCF_SHELLPOSITION | FCF_TASKLIST | FCF_AUTOICON;
        hwndFrame = WinCreateStdWindow (HWND_DESKTOP,
                                    0,
                                    &ulFrameFlags,
                                    pszClass,
                                    pszTitle,
                                    0,
                                    NULLHANDLE,
                                    ID_FRAMERES,
                                    &g.hwndClient);
        WinSetOwner (g.hwndClient, hwndFrame);

    }
    hlp_FreeString (pszTitle);
    hlp_FreeString (pszClass);

    return hwndFrame;
}


#define SYSVAL(x)   WinQuerySysValue (HWND_DESKTOP, (x))



static void s_StandardSize (PSWP pSwp)
{
    pSwp->cx = SYSVAL (SV_CXSCREEN) - 2 * SYSVAL (SV_CXSIZEBORDER);
    pSwp->cy = SYSVAL (SV_CYSCREEN) - 3 * SYSVAL (SV_CYICON);
    pSwp->x = 0;
    pSwp->y = 3 * SYSVAL (SV_CYICON) - SYSVAL (SV_CYSIZEBORDER);
}
