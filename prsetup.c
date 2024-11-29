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

/* Original file date: Apr-1-1997 */

/******************************************************************************
**
**   prsetup.c
**
**   Printer setup dialog
******************************************************************************/

#define INCL_WIN
#define INCL_GPI

#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "res.h"
#include "types.h"
#include "print.h"
#include "fonts.h"
#include "prsetup.h"
#include "helpers.h"
#include "memmgr.h"


typedef struct {
    ULONG   cbSize;

} CTL;
typedef CTL* PCTL;

typedef struct {
    HAB         hab;
    PPRINTER    prnlist;
    PFONTFAMILY fontlist;
} WNDDATA;
typedef WNDDATA* PWNDDATA;


static BOOL s_Init (HWND hwnd, PCTL pCtl);


static void s_PrinterSelected (HWND hwnd, PWNDDATA pWndData);
static void s_FontSelected (HWND hwnd, PWNDDATA pWndData);
static void s_StyleSelected (HWND hwnd, PWNDDATA pWndData);
static PPRINTER s_GetSelectedPrinter (HWND hwnd, PWNDDATA pWndData);
static PFONTFAMILY s_GetSelectedFont (HWND hwnd, PWNDDATA pWndData);
static PFONTFACE s_GetSelectedStyle (HWND hwnd, PWNDDATA pWndData);
static int s_GetSelectedSize (HWND hwnd, PWNDDATA pWndData);
static void s_BuildFontList (HWND hwnd, PWNDDATA pWndData, PPRINTER pp);
static void s_BuildStyleList (HWND hwnd, PWNDDATA pWndData, PFONTFAMILY pfont);
static void s_BuildSizeList (HWND hwnd, PWNDDATA pWndData, PFONTFACE pface);
static BOOL s_OK (HWND hwnd, PWNDDATA pWndData);

MRESULT APIENTRY PrnSetupDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2);

static PRINTDEF printdef;

BOOL prs_Setup (HWND hwnd, HMODULE hRes, PPRINTDEF pdef)
{
    if (WinDlgBox (HWND_DESKTOP, hwnd, PrnSetupDlgProc, hRes, IDD_PRINTSETUP, NULL)) {
        memcpy (pdef, &printdef, sizeof printdef);
        return TRUE;
    }
    return FALSE;
}


/**********************************************
**
**  PrnSetupDlgProc
**
**********************************************/

MRESULT APIENTRY PrnSetupDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL        bDDP = FALSE;
    MRESULT     mRet = (MRESULT) 0;
    PWNDDATA    pWndData;

    switch (ulMsg)
    {
      case WM_INITDLG:
        mRet = (MRESULT) s_Init (hwnd, PVOIDFROMMP (mp2));
        break;

      case WM_COMMAND:
        pWndData = (PWNDDATA) WinQueryWindowULong (hwnd, QWL_USER);
        switch (SHORT1FROMMP (mp1))
        {
          case DID_OK:
            if (s_OK (hwnd, pWndData))
                WinDismissDlg (hwnd, TRUE);
            break;

          case DID_CANCEL:
            WinDismissDlg (hwnd, FALSE);
            break;

          default:
            bDDP = TRUE;
            break;
        }
        break;

      case WM_CONTROL:
        pWndData = (PWNDDATA) WinQueryWindowULong (hwnd, QWL_USER);
        switch (SHORT1FROMMP (mp1))
        {
          case IDD_PS_PRINTER:
            switch (SHORT2FROMMP (mp1))
            {
              case CBN_ENTER:
                s_PrinterSelected (hwnd, pWndData);
                break;

              default:
                bDDP = TRUE;
                break;
            }
            break;

          case IDD_PS_FONT:
            switch (SHORT2FROMMP (mp1))
            {
              case CBN_ENTER:
                s_FontSelected (hwnd, pWndData);
                break;

              default:
                bDDP = TRUE;
                break;
            }
            break;

          case IDD_PS_STYLE:
            switch (SHORT2FROMMP (mp1))
            {
              case CBN_ENTER:
                s_StyleSelected (hwnd, pWndData);
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
        break;

      case WM_DESTROY:
        pWndData = (PWNDDATA) WinQueryWindowULong (hwnd, QWL_USER);
        if (pWndData) {
            if (pWndData->prnlist)
                prn_FreePrinterList (pWndData->prnlist);
            if (pWndData->fontlist)
                fnt_FreeFontList (pWndData->fontlist);
            mem_HeapFree (pWndData);
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

/**********************************************
**
**  s_OK
**
**********************************************/

static BOOL s_OK (HWND hwnd, PWNDDATA pWndData)
{
    PPRINTER    pp;
    PFONTFAMILY pfam;
    PFONTFACE   pface;
    PFONTSIZE   psize = NULL;
    int         iSize, i;

    if (pp = s_GetSelectedPrinter (hwnd, pWndData)) {
        if (pfam = s_GetSelectedFont (hwnd, pWndData)) {
            if (pface = s_GetSelectedStyle (hwnd, pWndData)) {
                if (iSize = s_GetSelectedSize (hwnd, pWndData)) {
                    if (!pfam->fOutline) {
                        for (i = 0; i < pface->nSizes; i++) {
                            if (iSize == pface->fsSize[i].iSize) {
                                psize = &pface->fsSize[i];
                                break;
                            }
                        }
                        if (!psize)
                            return FALSE;
                        printdef.lMatch = psize->lMatch;
                        printdef.iFontSize = psize->iSize;
                        printdef.lMaxBaselineExt = psize->lMaxBaselineExt;
                        printdef.lAveCharWidth = psize->lAveCharWidth;
                    }
                    else {
                        printdef.lMatch = pface->fsSize[0].lMatch;
                        printdef.iFontSize = iSize;
                    }
                    memcpy (&printdef.printer, pp, sizeof (PRINTER));
                    strcpy (printdef.szFacename, pface->szFacename);
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}





/**********************************************
**
**  s_Init
**
**********************************************/

static BOOL s_Init (HWND hwnd, PCTL pCtl)
{
    PWNDDATA    pWndData;
    PPRINTER    pp;
    BOOL        bSuccess = FALSE;
    int         i = 0, idef = 0;

    hlp_DlgNicePos (hwnd);
    if (pWndData = mem_HeapAlloc (sizeof (WNDDATA))) {
        WinSetWindowULong (hwnd, QWL_USER, (ULONG) pWndData);
        pWndData->hab = WinQueryAnchorBlock (hwnd);
        if (pWndData->prnlist = prn_BuildPrinterList ()) {
            pp = pWndData->prnlist;
            WinSendDlgItemMsg (hwnd, IDD_PS_PRINTER, LM_DELETEALL, 0, 0);
            while (pp) {
                WinSendDlgItemMsg (hwnd, IDD_PS_PRINTER, LM_INSERTITEM,
                                   MPFROMSHORT (LIT_END), MPFROMP (pp->szDescr));
                if (pp->bDefault)
                    idef = i;
                pp = pp->pNext;
                i++;
            }
            hlp_ComboSelect (WinWindowFromID (hwnd, IDD_PS_PRINTER), idef);
            WinPostMsg (hwnd, WM_CONTROL, MPFROM2SHORT (IDD_PS_PRINTER, CBN_ENTER), 0);
            bSuccess = TRUE;
        }
    }
    if (!bSuccess)
        WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (DID_CANCEL, 0), 0);
    return FALSE;
}


/**********************************************
**
**  s_BuildFontList
**
**********************************************/

static void s_BuildFontList (HWND hwnd, PWNDDATA pWndData, PPRINTER pp)
{
        HPS             hps;
        PPRINTDC        pdc;
        SIZEL           sl;
        PFONTFAMILY     pfont;
        char            buff[FACESIZE + 10];
        long            width, sel = 0, l = 0;
        char            deffont[] = "Courier";

        if (pWndData->fontlist) {
                fnt_FreeFontList (pWndData->fontlist);
                pWndData->fontlist = NULL;
        }
        WinSendDlgItemMsg (hwnd, IDD_PS_FONT, LM_DELETEALL, 0, 0);
        WinSetDlgItemText (hwnd, IDD_PS_FONT, "");

        if (!(pdc = prn_OpenPrinterDC (pWndData->hab, pp->szDriverName, pp->szQueueName,
                                 pp->szDriverPrinter, FALSE, TRUE)))
                return;

        width = prn_GetPrintableWidth (pdc->hdc);
        sl.cx = sl.cy = 0;
        if (hps = GpiCreatePS (pWndData->hab, pdc->hdc, &sl, PU_ARBITRARY | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC)) {
               if (pWndData->fontlist = fnt_BuildFontList (hps, BFL_NOVARIABLE)) {
                        pfont = pWndData->fontlist;
                        while (pfont) {
                                sprintf (buff, "%s (%c,%c)", pfont->szName, pfont->fOutline ? 'V' : 'B',
                                                 pfont->fDevice ? 'D' : 'G');
                                WinSendDlgItemMsg (hwnd, IDD_PS_FONT, LM_INSERTITEM, MPFROMSHORT (LIT_END),
                                                MPFROMP (buff));
                                if (!strcmp (deffont, pfont->szName))
                                        sel = l;
                                l++;
                                pfont = pfont->pNext;
                        }
                        hlp_ComboSelect (WinWindowFromID (hwnd, IDD_PS_FONT), sel);
                        WinPostMsg (hwnd, WM_CONTROL, MPFROM2SHORT (IDD_PS_FONT, CBN_ENTER), 0);
                }
                GpiDestroyPS (hps);
        }
        prn_ClosePrinterDC (pdc);
}

/**********************************************
**
**  s_BuildStyleList
**
**********************************************/

static void s_BuildStyleList (HWND hwnd, PWNDDATA pWndData, PFONTFAMILY pfont)
{
    PFONTFACE   pface;
    long        sel = 0, l = 0;

    WinSendDlgItemMsg (hwnd, IDD_PS_STYLE, LM_DELETEALL, 0, 0);
    WinSetDlgItemText (hwnd, IDD_PS_STYLE, "");

    pface = pfont->pfirstface;
    while (pface) {
        WinSendDlgItemMsg (hwnd, IDD_PS_STYLE, LM_INSERTITEM, MPFROMSHORT (LIT_END),
                           MPFROMP (*pface->szStyle ? pface->szStyle : "<normal>"));
        if (!*pface->szStyle || !strcmp ("regular", pface->szStyle))
                sel = l;
        l++;
        pface = pface->pNext;
    }
    hlp_ComboSelect (WinWindowFromID (hwnd, IDD_PS_STYLE), sel);
    WinPostMsg (hwnd, WM_CONTROL, MPFROM2SHORT (IDD_PS_STYLE, CBN_ENTER), 0);
}


/**********************************************
**
**  s_BuildSizeList
**
**********************************************/

static void s_BuildSizeList (HWND hwnd, PWNDDATA pWndData, PFONTFACE pface)
{
    int         i;
    int         iOutlineSizes[] = {6,8,10,11,12,14,18};
    char        buff[10];

    WinSendDlgItemMsg (hwnd, IDD_PS_SIZE, LM_DELETEALL, 0, 0);
    WinSetDlgItemText (hwnd, IDD_PS_SIZE, "");

    if (pface->nSizes == 2 && (pface->fsSize[1].iSize - pface->fsSize[0].iSize > 1000)) {
        for (i = 0; i < sizeof iOutlineSizes / sizeof iOutlineSizes[0]; i++) {
            sprintf (buff, "%d", iOutlineSizes[i]);
            WinSendDlgItemMsg (hwnd, IDD_PS_SIZE, LM_INSERTITEM, MPFROMSHORT (LIT_END),
                          MPFROMP (buff));
        }
        hlp_ComboSelect (WinWindowFromID (hwnd, IDD_PS_SIZE), 2);
    }
    else {
        for (i = 0; i < pface->nSizes; i++) {
            sprintf (buff, "%.1f", (float)pface->fsSize[i].iSize / 10.);
            WinSendDlgItemMsg (hwnd, IDD_PS_SIZE, LM_INSERTITEM, MPFROMSHORT (LIT_END),
                          MPFROMP (buff));
        }
        hlp_ComboSelect (WinWindowFromID (hwnd, IDD_PS_SIZE), 0);
    }
    WinPostMsg (hwnd, WM_CONTROL, MPFROM2SHORT (IDD_PS_SIZE, CBN_ENTER), 0);
}


/**********************************************
**
**  s_PrinterSelected
**
**********************************************/

static void s_PrinterSelected (HWND hwnd, PWNDDATA pWndData)
{
        PPRINTER    pp;

        if (pp = s_GetSelectedPrinter (hwnd, pWndData)) {
                s_BuildFontList (hwnd, pWndData, pp);
        }
}

/**********************************************
**
**  s_FontSelected
**
**********************************************/

static void s_FontSelected (HWND hwnd, PWNDDATA pWndData)
{
    PFONTFAMILY    pfam;

    if (pfam = s_GetSelectedFont (hwnd, pWndData)) {
        s_BuildStyleList (hwnd, pWndData, pfam);
    }
}


/**********************************************
**
**  s_StyleSelected
**
**********************************************/

static void s_StyleSelected (HWND hwnd, PWNDDATA pWndData)
{
    PFONTFACE    pface;

    if (pface = s_GetSelectedStyle (hwnd, pWndData)) {
        s_BuildSizeList (hwnd, pWndData, pface);
    }
}


/**********************************************
**
**  s_GetSelectedPrinter
**
**********************************************/

static PPRINTER s_GetSelectedPrinter (HWND hwnd, PWNDDATA pWndData)
{
    PPRINTER    pp;
    LONG        lSel, i;

    lSel = (LONG) WinSendDlgItemMsg (hwnd, IDD_PS_PRINTER, LM_QUERYSELECTION, MPFROMSHORT (LIT_FIRST), 0);
    if (LIT_NONE != lSel) {
        pp = pWndData->prnlist;
        for (i = 0; i < lSel; i++) {
            pp = pp->pNext;
            if (!pp) break;
        }
        if (i == lSel)
            return pp;
    }
    return NULL;
}

/**********************************************
**
**  s_GetSelectedFont
**
**********************************************/

static PFONTFAMILY s_GetSelectedFont (HWND hwnd, PWNDDATA pWndData)
{
    PFONTFAMILY pfam;
    LONG        lSel, i;


    lSel = (LONG) WinSendDlgItemMsg (hwnd, IDD_PS_FONT, LM_QUERYSELECTION,
                                    MPFROMSHORT (LIT_FIRST), 0);
    if (LIT_NONE != lSel) {
        pfam = pWndData->fontlist;
        for (i = 0; i < lSel; i++) {
            pfam = pfam->pNext;
            if (!pfam) break;
        }
        if (i == lSel)
            return pfam;
    }
    return NULL;
}


/**********************************************
**
**  s_GetSelectedStyle
**
**********************************************/

static PFONTFACE s_GetSelectedStyle (HWND hwnd, PWNDDATA pWndData)
{
    PFONTFACE   pface;
    PFONTFAMILY pfam;
    LONG        lSel, i;


    lSel = (LONG) WinSendDlgItemMsg (hwnd, IDD_PS_STYLE, LM_QUERYSELECTION,
                                    MPFROMSHORT (LIT_FIRST), 0);
    if (LIT_NONE != lSel && (pfam = s_GetSelectedFont (hwnd, pWndData))) {
        pface = pfam->pfirstface;
        for (i = 0; i < lSel; i++) {
            pface = pface->pNext;
            if (!pface) break;
        }
        if (i == lSel)
            return pface;
    }
    return NULL;
}


/**********************************************
**
**  s_GetSelectedSize
**
**********************************************/

static int s_GetSelectedSize (HWND hwnd, PWNDDATA pWndData)
{
    char    buff[10];

    *buff = 0;
    WinQueryDlgItemText (hwnd, IDD_PS_SIZE, sizeof buff, buff);
    return (int) (atof (buff) * 10.);
}
