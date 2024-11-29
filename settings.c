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

/* Original file date: Mär-16-1995 */

/******************************************************************************
*                                                                             *
*   settings.c                                                                *
*                                                                             *
*                                                                             *
******************************************************************************/




#define INCL_WIN
#define INCL_GPI

#include <os2.h>
#include <string.h>

#include "res.h"
#include "types.h"
#include "memmgr.h"
#include "helpers.h"
#include "fonts.h"
#include "settings.h"
#include "colors.h"
#include "fontdlg.h"
#include "inidlg.h"
#include "globals.h"

#define NUMPAGES 3

typedef struct {
    HWND    hwndDlg;
    ULONG   ulID;
} PAGE;
typedef PAGE*   PPAGE;


static int s_PageFromID (ULONG ulID);


static PAGE page[NUMPAGES];


MRESULT APIENTRY SettingsDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL        bDDP = FALSE;
    MRESULT     mRet = (MRESULT) 0;
    static HWND hwndBook, hwndHex;
    int         i;
    LONG        lRGB;
    static      bInit;
    PPAGESELECTNOTIFY   pSel;
    PFONTMETRICS pfm;

    switch (ulMsg)
    {
      case WM_INITDLG:
        bInit = FALSE;
        hlp_DlgNicePos (hwnd);
        hwndHex = *((HWND*)PVOIDFROMMP (mp2));
        hwndBook = WinWindowFromID (hwnd, IDD_S_NOTEBOOK);
        lRGB = WinQuerySysColor (HWND_DESKTOP, SYSCLR_DIALOGBACKGROUND, 0);
        WinSendMsg (hwndBook, BKM_SETNOTEBOOKCOLORS, MPFROMLONG (lRGB),
                                MPFROMSHORT (BKA_BACKGROUNDPAGECOLOR));
        WinSendMsg (hwndBook, BKM_SETNOTEBOOKCOLORS, MPFROMLONG (lRGB),
                                MPFROMSHORT (BKA_BACKGROUNDMAJORCOLOR));
        WinSendMsg (hwndBook, BKM_SETNOTEBOOKCOLORS, MPFROMLONG (lRGB),
                                MPFROMSHORT (BKA_BACKGROUNDMINORCOLOR));

        page[2].ulID = (ULONG)WinSendMsg (hwndBook, BKM_INSERTPAGE, MPFROMLONG(0),
                        MPFROM2SHORT (BKA_MAJOR, BKA_FIRST | BKA_AUTOPAGESIZE));
        WinSendMsg (hwndBook, BKM_SETTABTEXT, MPFROMLONG (page[2].ulID), MPFROMP ("INI File"));
        page[2].hwndDlg = NULLHANDLE;

        page[1].ulID = (ULONG)WinSendMsg (hwndBook, BKM_INSERTPAGE, MPFROMLONG(0),
                        MPFROM2SHORT (BKA_MAJOR, BKA_FIRST | BKA_AUTOPAGESIZE));
        WinSendMsg (hwndBook, BKM_SETTABTEXT, MPFROMLONG (page[1].ulID), MPFROMP ("Fonts"));
        page[1].hwndDlg = NULLHANDLE;

        page[0].ulID = (ULONG)WinSendMsg (hwndBook, BKM_INSERTPAGE, MPFROMLONG(0),
                        MPFROM2SHORT (BKA_MAJOR, BKA_FIRST | BKA_AUTOPAGESIZE));
        WinSendMsg (hwndBook, BKM_SETTABTEXT, MPFROMLONG (page[0].ulID), MPFROMP ("Colors"));
        page[0].hwndDlg = NULLHANDLE;

        pfm = (PFONTMETRICS) mem_HeapAlloc (sizeof (FONTMETRICS));
        fnt_QueryWinFontMetrics (hwnd, pfm);

        WinSendMsg (hwndBook, BKM_SETDIMENSIONS, MPFROM2SHORT (10 * pfm->lAveCharWidth,
                                (pfm->lMaxBaselineExt * 7) / 4),
                                MPFROMSHORT (BKA_MAJORTAB));
        mem_HeapFree (pfm);
        bInit = TRUE;
        WinSendMsg (hwndBook, BKM_TURNTOPAGE, MPFROMLONG (page[0].ulID), 0);

        bDDP = TRUE;
        break;

      case WM_CONTROL:
        switch (SHORT2FROMMP (mp1))
        {
          case BKN_PAGESELECTED:
            if (bInit) {
                pSel = (PPAGESELECTNOTIFY) PVOIDFROMMP (mp2);
                i = s_PageFromID (pSel->ulPageIdNew);
                if (-1 == i)
                    break;
                if (NULLHANDLE == page[i].hwndDlg) {
                    switch (i)
                    {
                      case 0:
                        page[0].hwndDlg = WinLoadDlg (hwndBook, hwnd, ColorsDlgProc, g.hResource,
                                            IDD_COLORS, &hwndHex);
                        break;

                      case 1:
                        page[1].hwndDlg = WinLoadDlg (hwndBook, hwnd, FontDlgProc, g.hResource,
                                            IDD_FONTS, NULL);
                        break;


                      case 2:
                        page[2].hwndDlg = WinLoadDlg (hwndBook, hwnd, IniDlgProc, g.hResource,
                                            IDD_INISPEC, NULL);
                        break;
                    }
                    WinSendMsg (hwndBook, BKM_SETPAGEWINDOWHWND, MPFROMLONG (page[i].ulID),
                                                MPFROMHWND (page[i].hwndDlg));
                }
            }
            break;

          default:
            bDDP = TRUE;
            break;
        }
        break;

      case WM_COMMAND:
        switch (SHORT1FROMMP (mp1))
        {
          case DID_OK:
            for (i = 0; i < NUMPAGES; i++) {
                if (NULLHANDLE != page[i].hwndDlg)
                    WinSendMsg (page[i].hwndDlg, WM_COMMAND, MPFROM2SHORT (ID_NBOK, 0), MPFROMLONG (0));
            }
            WinDismissDlg (hwnd, TRUE);
            break;

          case DID_CANCEL:
            for (i = 0; i < NUMPAGES; i++) {
                if (NULLHANDLE != page[i].hwndDlg)
                    WinSendMsg (page[i].hwndDlg, WM_COMMAND, MPFROM2SHORT (ID_NBCANCEL, 0), MPFROMLONG (0));
            }
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



static int s_PageFromID (ULONG ulID)
{
    int i;

    for (i = 0; i < NUMPAGES; i++) {
        if (page[i].ulID == ulID)
            return i;
    }
    return -1;
}
