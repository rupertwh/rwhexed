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

/* Original file date: Dez-7-1996 */

#define INCL_WIN

#include <os2.h>
#include <stdlib.h>

#include "res.h"
#include "helpers.h"
#include "memmgr.h"
#include "globals.h"
#include "addeawnd.h"

typedef struct {
    int     cbSize;
    HWND    hwndParent;
    PSZ     pszName;
    ULONG   ulMaxSize;
} CTL;
typedef CTL* PCTL;


MRESULT APIENTRY AddEADlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2);


BOOL ae_QueryName (HWND hwndParent, PSZ pszName, ULONG ulMaxSize)
{
    PCTL    pCtl;
    BOOL    bRet = FALSE;

    pCtl = mem_HeapAlloc (sizeof (CTL));

    pCtl->cbSize = (int) sizeof (CTL);
    pCtl->hwndParent = hwndParent;
    pCtl->pszName = pszName;
    pCtl->ulMaxSize = ulMaxSize;

    if (WinDlgBox (HWND_DESKTOP, hwndParent, AddEADlgProc, g.hResource, IDD_ADDEA, pCtl))
        bRet = TRUE;
    mem_HeapFree (pCtl);

    return bRet;
}


MRESULT APIENTRY AddEADlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bDDP = FALSE;
    MRESULT mRet = (MRESULT) 0;
    PCTL    pCtl;

    switch (ulMsg)
    {
      case WM_INITDLG:
        hlp_DlgNicePos (hwnd);
        pCtl = (PCTL) PVOIDFROMMP (mp2);
        WinSetWindowULong (hwnd, QWL_USER, (ULONG)(PVOID)pCtl);
        WinSendDlgItemMsg (hwnd, IDD_EAA_NAME, EM_SETTEXTLIMIT, MPFROMLONG (pCtl->ulMaxSize - 1), 0);
        bDDP = TRUE;
        break;

      case WM_COMMAND:
        switch (SHORT1FROMMP (mp1))
        {
          case DID_OK:
            pCtl = (PCTL)(PVOID)WinQueryWindowULong (hwnd, QWL_USER);
            if (!WinQueryDlgItemTextLength (hwnd, IDD_EAA_NAME)) {
                WinAlarm (HWND_DESKTOP, WA_WARNING);
            }
            else {
                WinQueryDlgItemText (hwnd, IDD_EAA_NAME, pCtl->ulMaxSize - 1, pCtl->pszName);
                WinDismissDlg (hwnd, TRUE);
            }
            break;

          case DID_CANCEL:
            WinDismissDlg (hwnd, FALSE);
            break;

          default:
            bDDP = TRUE;
            break;
        }
        break;

      default:
        bDDP = TRUE;
        break;
    }

    if (bDDP)
        mRet = WinDefDlgProc (hwnd, ulMsg, mp1, mp2);
    return mRet;
}
