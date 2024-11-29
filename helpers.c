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

#define HELPERS_C

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN

#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define STRSIZE 128

#include "types.h"
#include "res.h"
#include "memmgr.h"
#include "helpers.h"
#include "globals.h"

void s_MsgNoMsg (void);

HMODULE hResource;
int     hextable[256];

void hlp_init_hextable (void)
{
        int     i;

        for (i = 0; i < 256; i++)
                hextable[i] = -1;

        for (i = '0'; i <= '9'; i++)
                hextable[i] = i - '0';

        hextable['a'] = 10;
        hextable['b'] = 11;
        hextable['c'] = 12;
        hextable['d'] = 13;
        hextable['e'] = 14;
        hextable['f'] = 15;

        hextable['A'] = 10;
        hextable['B'] = 11;
        hextable['C'] = 12;
        hextable['D'] = 13;
        hextable['E'] = 14;
        hextable['F'] = 15;
}

TID hlp_gettid (void)
{
        PTIB    ptib = NULL;
        PPIB    ppib = NULL;

        if ((NO_ERROR == DosGetInfoBlocks (&ptib, &ppib) && ptib))
                return (TID) ptib->tib_ptib2->tib2_ultid;
        else
                return (TID) 0;
}


void hlp_Init (HMODULE hResourcex)
{
    hResource = hResourcex;
}

PSZ hlp_GetString (HAB hab, ULONG ulID)
{
    PSZ pszStr;

    pszStr = (PSZ) mem_HeapAlloc (STRSIZE);
    if (pszStr)
        WinLoadString (hab, hResource, ulID, STRSIZE, pszStr);
    return pszStr;
}

void hlp_FreeString (PSZ pszString)
{
    mem_HeapFree (pszString);
}


void hlp_MsgRes (HAB hab, ULONG ulID, int nLevel)
{
    PSZ pszMsg;

    pszMsg = hlp_GetString (hab, ulID);
    hlp_Msg (hab, pszMsg, nLevel);
    if (pszMsg)
        mem_HeapFree (pszMsg);
}


void hlp_Msg (HAB hab, PSZ pszMsg, int nLevel)
{
    ULONG   ulBeep, ulTitle, ulIcon;
    PSZ     pszTitle;

    switch (nLevel)
    {
      case HLP_MSGLEVEL_INF:
        ulBeep = WA_NOTE;
        ulTitle = IDS_INFO;
        ulIcon = 0;
        break;

      case HLP_MSGLEVEL_WRN:
        ulBeep = WA_WARNING;
        ulTitle = IDS_WARNING;
        ulIcon = MB_WARNING;
        break;

      case HLP_MSGLEVEL_ERR: /* fall through */
      default:
        ulBeep = WA_ERROR;
        ulTitle = IDS_ERROR;
        ulIcon = MB_ERROR;
        break;
    }

    pszTitle = hlp_GetString (hab, ulTitle);
    if (pszMsg && *pszMsg) {
        WinAlarm (HWND_DESKTOP, ulBeep);
        WinMessageBox (HWND_DESKTOP, g.hwndFrame, pszMsg, pszTitle, 0, MB_OK | ulIcon);
    }
    else
        s_MsgNoMsg ();

    if (pszTitle)
        mem_HeapFree (pszTitle);
}

void hlp_MsgResDos (HAB hab, ULONG ulID, APIRET rc, int nLevel)
{
    PSZ pszMsg;

    pszMsg = hlp_GetString (hab, ulID);
    hlp_MsgDos (hab, pszMsg, rc, nLevel);
    if (pszMsg)
        mem_HeapFree (pszMsg);
}

void hlp_MsgDos (HAB hab, PSZ pszMsg, APIRET rc, int nLevel)
{
    PSZ pszBuff = NULL;

    if (pszMsg)
        pszBuff = (PSZ) mem_HeapAlloc (strlen (pszMsg) + 20);
    if (pszBuff) {
        sprintf (pszBuff, pszMsg, (int) rc);
        hlp_Msg (hab, pszBuff, nLevel);
        mem_HeapFree (pszBuff);
    }
    else
        s_MsgNoMsg ();

}


void s_MsgNoMsg (void)
{
    static char szMsg[] = "An error occurred while trying to display the correct "
                          "error message!";
    static char szTitle[] = "HexEdit - Critical Error!";

    WinAlarm (HWND_DESKTOP, WA_ERROR);
    WinMessageBox (HWND_DESKTOP, NULLHANDLE, szMsg, szTitle, 0, MB_OK | MB_ERROR | MB_SYSTEMMODAL);
}


void hlp_DlgNicePos (HWND hwndDlg)
{
    HWND    hwndOwner;
    LONG    cxFrame, cyFrame, xFrame, yFrame, lMargin, lTopBorder;
    LONG    cxScreen, cyScreen;
    SWP     swp;

    hwndOwner = WinQueryWindow (hwndDlg, QW_OWNER);
    if (!hwndOwner) return;

    WinQueryWindowPos (hwndOwner, &swp);
    cxFrame = swp.cx;
    cyFrame = swp.cy;
    xFrame = swp.x;
    yFrame = swp.y;

    WinQueryWindowPos (hwndDlg, &swp);

    lTopBorder = WinQuerySysValue (HWND_DESKTOP, SV_CYSIZEBORDER) +
                 WinQuerySysValue (HWND_DESKTOP, SV_CYTITLEBAR) +
                 WinQuerySysValue (HWND_DESKTOP, SV_CYMENU);
    cxScreen = WinQuerySysValue (HWND_DESKTOP, SV_CXSCREEN);
    cyScreen = WinQuerySysValue (HWND_DESKTOP, SV_CYSCREEN);
    lMargin = WinQuerySysValue (HWND_DESKTOP, SV_CYMENU);

    if (swp.cx < cxFrame)
        swp.x = xFrame + (cxFrame >> 1) - (swp.cx >> 1); /* center in frame */
    else
        swp.x = xFrame + lMargin; /* left align in frame */

    if (swp.x + swp.cx >= cxScreen) /* off the screen? */
        swp.x = cxScreen - swp.cx - WinQuerySysValue (HWND_DESKTOP, SV_CXDLGFRAME);
    else if (swp.x < 0) swp.x = 0;


    if (swp.cy < cyFrame - lTopBorder)
        swp.y = yFrame + ((cyFrame - lTopBorder) >> 1) - (swp.cy >> 1); /* center */
    else
        swp.y = yFrame + cyFrame - lTopBorder - lMargin - swp.cy; /* top align */

    if (swp.y < 0 || swp.y + swp.cy >= cyScreen) /* off the screen? */
        swp.y = WinQuerySysValue (HWND_DESKTOP, SV_CYDLGFRAME);


    swp.fl = SWP_MOVE;

    WinSetWindowPos (hwndDlg, NULLHANDLE, swp.x, swp.y, swp.cx, swp.cy, swp.fl);
}


BOOL hlp_AscToInt (char *pStr, int *pInt)
{
    int     iVal;
    char    *p;
    int     iBase = 10;
    BOOL    bRet = TRUE;
    int     dig;

    iVal = 0;
    p = pStr;

    if (strlen (pStr) > 1 && pStr[0] == '0' && pStr[1] == 'x') {   /* hex  */
        iBase = 16;
        p += 2;
    }

    while (*p) {
        if (*p < '0' || (*p > '9' && *p < 'A') || (*p > 'F' && *p < 'a') || *p > 'f') {
            goto Abort;
        }

        if (*p <= '9')
            dig = *p - '0';
        else
            dig = toupper (*p) - 'A' + 10;

        if (dig >= iBase) {
            goto Abort;
        }

        iVal *= iBase;
        iVal += dig;

        p++;
    }

    *pInt = iVal;
    return TRUE;

Abort:
    if (16 == iBase)
        sprintf (pStr, "0x%0x", iVal);
    else
        sprintf (pStr, "%d", iVal);

    *pInt = iVal;
    return FALSE;

}


HWND hlp_RealOwner (HWND hwndKid)
{
    HWND    hwndOwner, hwnd = NULLHANDLE;

    hwndOwner = hwndKid;

    do {
        hwnd = WinQueryWindow (hwndOwner, QW_OWNER);
        if (hwnd)
            hwndOwner = hwnd;
    } while (hwnd);

    return hwndOwner;
}


void hlp_ComboSelect (HWND hwnd, LONG lSel)
{
    char    *p;
    ULONG   ulSize;

    WinSendMsg (hwnd, LM_SELECTITEM, MPFROMSHORT (lSel), MPFROMSHORT (TRUE));
    ulSize = (ULONG) WinSendMsg (hwnd, LM_QUERYITEMTEXTLENGTH, MPFROMSHORT (lSel), 0);
    if (p = mem_HeapAlloc (1 + ulSize)) {
        WinSendMsg (hwnd, LM_QUERYITEMTEXT, MPFROM2SHORT (lSel, ulSize), MPFROMP (p));
        WinSetWindowText (hwnd, p);
        mem_HeapFree (p);
    }
}

int hlp_printPMError (HAB hab)
{
    ULONG   error, sev, code;


    error = WinGetLastError (hab);
    sev = 0x0000FFFF & (error >> 16);
    code = 0x0000FFFF & error;
    return printf ("PM Error: %s: %04x\n", SEVERITY_NOERROR == sev ? "No Error" :
                                    SEVERITY_WARNING == sev ? "Warning" :
                                    SEVERITY_ERROR == sev ? "Error" :
                                    SEVERITY_SEVERE == sev ? "Severe" :
                                    SEVERITY_UNRECOVERABLE == sev ? "Unrecoverable" :
                                    "unkown error severity", code);
}
