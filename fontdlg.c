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

/* Original file date: Mär-17-1995 */

/******************************************************************************
*                                                                             *
*   fontdlg.c                                                                 *
*                                                                             *
*                                                                             *
******************************************************************************/




#define INCL_WIN
#define INCL_GPI

#include <os2.h>
#include <string.h>
#include <stdio.h>

#include "res.h"
#include "types.h"
#include "helpers.h"
#include "memmgr.h"
#include "fonts.h"
#include "fontdlg.h"
#include "globals.h"


static void s_SetupList (HWND hwnd);
static void s_SetupSizeList (HWND hwnd, int nFace);
static int s_FontFromSel (HWND hwnd);
static void s_SelectFromFont (HWND hwnd, int iFont);


MRESULT APIENTRY FontSubProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2);

static PFNWP    pSampleOrgProc;
static int      nCurrFont, nFaces;
static int      nIndex[20]; /* assume not more than 20 fonts */

MRESULT APIENTRY FontDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL        bDDP = FALSE;
    MRESULT     mRet = (MRESULT) 0;
    static HWND hwndSample;

    switch (ulMsg)
    {
      case WM_INITDLG:
        hwndSample = WinWindowFromID (hwnd, IDD_F_SAMPLE);
        pSampleOrgProc = WinSubclassWindow (hwndSample, FontSubProc);
        s_SetupList (hwnd);
        nCurrFont = g.nCurrFont;
        s_SelectFromFont (hwnd, nCurrFont);
        bDDP = TRUE;
        break;

      case WM_COMMAND:
        switch (SHORT1FROMMP (mp1))
        {
          case DID_OK:
            WinPostMsg (WinQueryWindow (WinQueryWindow (WinQueryWindow (hwnd, QW_PARENT),
                            QW_PARENT), QW_PARENT),
                            WM_COMMAND, MPFROM2SHORT (DID_OK, 0), 0);
            break;

          case DID_CANCEL:
            WinPostMsg (WinQueryWindow (WinQueryWindow (WinQueryWindow (hwnd, QW_PARENT),
                            QW_PARENT), QW_PARENT),
                            WM_COMMAND, MPFROM2SHORT (DID_CANCEL, 0), 0);
            break;

          case ID_NBOK:
            WinSubclassWindow (hwndSample, pSampleOrgProc);
            g.nCurrFont = nCurrFont;
            WinDismissDlg (hwnd, TRUE);
            break;

          case ID_NBCANCEL:
            WinSubclassWindow (hwndSample, pSampleOrgProc);
            WinDismissDlg (hwnd, FALSE);
            break;

          default:
            bDDP = TRUE;
            break;
        }
        break;

      case WM_CONTROL:
        switch (SHORT1FROMMP (mp1))
        {
          case IDD_F_FONT:
          case IDD_F_SIZE:
            if (LN_SELECT == SHORT2FROMMP (mp1)) {
                if (IDD_F_FONT == SHORT1FROMMP (mp1)) {
                    int nSel;
                    nSel = (int) WinSendDlgItemMsg (hwnd, IDD_F_FONT, LM_QUERYSELECTION, 0, 0);
                    s_SetupSizeList (hwnd, nSel);
                    WinSendDlgItemMsg (hwnd, IDD_F_SIZE, LM_SELECTITEM, 0, MPFROMSHORT (TRUE));
                }
                nCurrFont = s_FontFromSel (hwnd);
                WinInvalidateRect (hwndSample, NULL, FALSE);
            }
            else
                bDDP = TRUE;
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


static void s_SelectFromFont (HWND hwnd, int iFont)
{
    int i, iFace, iSizeIndex, iFaceIndex;

    iFace = g.pFont[iFont].nFirstType;
    for (i = 0; i < nFaces; i++) {
        if (nIndex[i] == iFace) {
            iFaceIndex = i;
            iSizeIndex = iFont - iFace;
            break;
        }
    }
    if (nFaces == i) return;

    WinSendDlgItemMsg (hwnd, IDD_F_FONT, LM_SELECTITEM, MPFROMSHORT (iFaceIndex), MPFROMSHORT (TRUE));
    WinSendDlgItemMsg (hwnd, IDD_F_SIZE, LM_SELECTITEM, MPFROMSHORT (iSizeIndex), MPFROMSHORT (TRUE));
}



static void s_SetupList (HWND hwnd)
{
    int     i;
    HWND    hwndFaceList;

    nFaces = 0;
    hwndFaceList = WinWindowFromID (hwnd, IDD_F_FONT);
    for (i = 0; i < g.nFonts; i++) {
        if (i == g.pFont[i].nFirstType) {
            nIndex[nFaces++] = i;
            WinSendMsg (hwndFaceList, LM_INSERTITEM, MPFROMSHORT (LIT_END), MPFROMP (g.pFont[i].szName));
        }
    }
}


static void s_SetupSizeList (HWND hwnd, int nFace)
{
    int     i;
    HWND    hwndSizeList;
    char    szBuf[20];

    hwndSizeList = WinWindowFromID (hwnd, IDD_F_SIZE);

    WinSendMsg (hwndSizeList, LM_DELETEALL, 0, 0);
    for (i = nIndex[nFace]; i < g.nFonts; i++) {
        if (g.pFont[i].nFirstType != nIndex[nFace])
            break;
        sprintf (szBuf, "%d", g.pFont[i].nPoint);
        WinSendMsg (hwndSizeList, LM_INSERTITEM, MPFROMSHORT (LIT_END), MPFROMP (szBuf));
    }
}

static int s_FontFromSel (HWND hwnd)
{
    int nFace, nSize;

    nFace = (int) WinSendDlgItemMsg (hwnd, IDD_F_FONT, LM_QUERYSELECTION, 0, 0);
    nSize = (int) WinSendDlgItemMsg (hwnd, IDD_F_SIZE, LM_QUERYSELECTION, 0, 0);
    return (nIndex[nFace] + nSize);
}


MRESULT APIENTRY FontSubProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bDWP = TRUE;
    MRESULT mRes = MRFROMLONG (0);
    RECTL   rectl;
    HPS     hps;
    LONG    lBack;
    POINTL  ptl;
    PSZ     psz;
    PFONTMETRICS    pfm;

    switch (ulMsg)
    {
      case WM_PAINT:
        hps = WinBeginPaint (hwnd, NULLHANDLE, &rectl);
        fnt_CreateFont (hps, nCurrFont, 1);
        GpiSetCharSet (hps, 1);


        lBack = WinQuerySysColor (HWND_DESKTOP, SYSCLR_DIALOGBACKGROUND, 0);
        WinQueryWindowRect (hwnd, &rectl);
        GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL); /* set RGB mode */
        WinFillRect (hps, &rectl, lBack);

        pfm = fnt_QueryMetrics (nCurrFont);
        ptl.x = pfm->lAveCharWidth;
        ptl.y = (rectl.yTop >> 1) - (pfm->lMaxBaselineExt >> 1);
        psz = "This is some sample text";
        GpiCharStringAt (hps, &ptl, (LONG) strlen (psz), psz);

        GpiDeleteSetId (hps, 1);
        WinEndPaint (hps);

        bDWP = FALSE;
        break;

    }

    if (TRUE == bDWP)
        mRes = (*pSampleOrgProc) (hwnd, ulMsg, mp1, mp2);
    return mRes;
}
