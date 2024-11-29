/* Original file date: Mär-25-1995 */

/******************************************************************************
*                                                                             *
*   colors.c                                                                   *
*                                                                             *
*                                                                             *
******************************************************************************/




#define INCL_WIN
#define INCL_GPI

#include <os2.h>
#include <string.h>

#include "res.h"
#include "types.h"
#include "colors.h"
#include "helpers.h"
#include "memmgr.h"
#include "fonts.h"
#include "globals.h"
#include "hewnd.h"

#define COLMAXNAME  30

#define LONGARR(h)  ((PLONG)(PVOID)&h)

MRESULT APIENTRY ColorSubProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2);
MRESULT APIENTRY SampleSubProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2);
static void s_MakeList (HWND hwndDlg);
static void s_SetSpinVals (HWND hwnd, PHECOLORS phec);

static PFNWP    pColorOrgProc, pSampleOrgProc;
static BOOL     bListOK;
static HWND     hwndHex;
static HECOLORS hecSystem, hecDefault, hecCustom, *phecCurr;
static int      iColorTable;
static int      cxChar, cyChar, cyDesc;
static LONG     lFont;

MRESULT APIENTRY ColorsDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL        bDDP = FALSE;
    MRESULT     mRet = (MRESULT) 0;
    static HWND hwndRect, hwndSample;
    int         i;
    PFONTMETRICS    pfm;
    LONG        lVal;

    switch (ulMsg)
    {
      case WM_INITDLG:
        hwndHex = *((HWND*)PVOIDFROMMP (mp2));
        if (!hwndHex) WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (DID_CANCEL, 0), 0);

        WinSendDlgItemMsg (hwnd, IDD_C_RED, SPBM_SETLIMITS, MPFROMLONG (255), MPFROMLONG (0));
        WinSendDlgItemMsg (hwnd, IDD_C_GREEN, SPBM_SETLIMITS, MPFROMLONG (255), MPFROMLONG (0));
        WinSendDlgItemMsg (hwnd, IDD_C_BLUE, SPBM_SETLIMITS, MPFROMLONG (255), MPFROMLONG (0));

        hwndRect = WinWindowFromID (hwnd, IDD_C_RECT);
        hwndSample = WinWindowFromID (hwnd, IDD_C_SAMPLE);
        pColorOrgProc = WinSubclassWindow (hwndRect, ColorSubProc);
        pSampleOrgProc = WinSubclassWindow (hwndSample, SampleSubProc);

        WinSendMsg (hwndHex, WMUSR_HE_QUERYCOLORS, MPFROMP (&hecCustom), MPFROMLONG (2));
        WinSendMsg (hwndHex, WMUSR_HE_QUERYCOLORS, MPFROMP (&hecSystem), MPFROMLONG (1));
        iColorTable = (int) WinSendMsg (hwndHex, WMUSR_HE_QUERYCOLORS, MPFROMP (&hecDefault), MPFROMLONG (0));
        if (0 == iColorTable) {
            phecCurr = &hecDefault;
            WinCheckButton (hwnd, IDD_C_DEFAULT, TRUE);
            WinEnableWindow (WinWindowFromID (hwnd, IDD_C_RED), FALSE);
            WinEnableWindow (WinWindowFromID (hwnd, IDD_C_GREEN), FALSE);
            WinEnableWindow (WinWindowFromID (hwnd, IDD_C_BLUE), FALSE);
        }
        else if (1 == iColorTable) {
            phecCurr = &hecSystem;
            WinCheckButton (hwnd, IDD_C_SYSTEM, TRUE);
            WinEnableWindow (WinWindowFromID (hwnd, IDD_C_RED), FALSE);
            WinEnableWindow (WinWindowFromID (hwnd, IDD_C_GREEN), FALSE);
            WinEnableWindow (WinWindowFromID (hwnd, IDD_C_BLUE), FALSE);
        }
        else {
            phecCurr = &hecCustom;
            WinCheckButton (hwnd, IDD_C_CUSTOM, TRUE);
        }

        lFont = (LONG) WinSendMsg (hwndHex, WMUSR_HE_QUERYFONT, 0, 0);
        pfm = fnt_QueryMetrics (lFont);
        cxChar = pfm->lAveCharWidth;
        cyChar = pfm->lMaxBaselineExt;
        cyDesc = pfm->lMaxDescender;

        bListOK = FALSE;
        s_MakeList (hwnd);
        bListOK = TRUE;
        WinSendDlgItemMsg (hwnd, IDD_C_LIST, LM_SELECTITEM, MPFROMSHORT (OFF_TEXT), MPFROMSHORT (TRUE));
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
            WinSubclassWindow (hwndRect, pColorOrgProc);
            WinSubclassWindow (hwndSample, pSampleOrgProc);
            WinSendMsg (hwndHex, WMUSR_HE_SETCOLORS, MPFROMP (&hecCustom), MPFROMLONG (iColorTable));
            WinDismissDlg (hwnd, TRUE);
            break;

          case ID_NBCANCEL:
            WinSubclassWindow (hwndRect, pColorOrgProc);
            WinSubclassWindow (hwndSample, pSampleOrgProc);
            WinDismissDlg (hwnd, FALSE);
            break;

        }
        break;

      case WM_CONTROL:
        switch (SHORT1FROMMP (mp1))
        {
          case IDD_C_RED:
          case IDD_C_GREEN:
          case IDD_C_BLUE:
            if (SPBN_CHANGE == SHORT2FROMMP (mp1)) {
                if (!bListOK) break;
                i = (int) WinSendDlgItemMsg (hwnd, IDD_C_LIST, LM_QUERYSELECTION,
                            MPFROMSHORT (LIT_FIRST), 0);
                if (0 <= i && i < NUMCOLS) {
                    lVal = 0;
                    WinSendDlgItemMsg (hwnd, IDD_C_RED, SPBM_QUERYVALUE, MPFROMP (&lVal),
                                        MPFROM2SHORT (0, SPBQ_ALWAYSUPDATE));
                    LONGARR(hecCustom)[i] = RGBFROMVALS (lVal, GREEN (LONGARR(*phecCurr)[i]), BLUE (LONGARR(hecCustom)[i]));
                    WinSendDlgItemMsg (hwnd, IDD_C_GREEN, SPBM_QUERYVALUE, MPFROMP (&lVal),
                                        MPFROM2SHORT (0, SPBQ_ALWAYSUPDATE));
                    LONGARR(hecCustom)[i] = RGBFROMVALS (RED (LONGARR(*phecCurr)[i]), lVal, BLUE (LONGARR(hecCustom)[i]));
                    WinSendDlgItemMsg (hwnd, IDD_C_BLUE, SPBM_QUERYVALUE, MPFROMP (&lVal),
                                        MPFROM2SHORT (0, SPBQ_ALWAYSUPDATE));
                    LONGARR(hecCustom)[i] = RGBFROMVALS (RED (LONGARR(*phecCurr)[i]), GREEN (LONGARR(hecCustom)[i]), lVal);
                    WinInvalidateRect (hwndRect, NULL, FALSE);
                    WinInvalidateRect (hwndSample, NULL, FALSE);
                }
            }
            else
                bDDP = TRUE;
            break;

          case IDD_C_LIST:
            if (LN_SELECT == SHORT2FROMMP (mp1)) {
                if (!bListOK) break;
                i = (int) WinSendDlgItemMsg (hwnd, IDD_C_LIST, LM_QUERYSELECTION,
                            MPFROMSHORT (LIT_FIRST), 0);
                if (0 <= i && i < NUMCOLS) {
                    bListOK = FALSE;
                    WinSendDlgItemMsg (hwnd, IDD_C_RED, SPBM_SETCURRENTVALUE,
                                MPFROMLONG (RED(LONGARR(*phecCurr)[i])), 0);
                    WinSendDlgItemMsg (hwnd, IDD_C_GREEN, SPBM_SETCURRENTVALUE,
                                MPFROMLONG (GREEN(LONGARR(*phecCurr)[i])), 0);
                    WinSendDlgItemMsg (hwnd, IDD_C_BLUE, SPBM_SETCURRENTVALUE,
                                MPFROMLONG (BLUE(LONGARR(*phecCurr)[i])), 0);
                    WinInvalidateRect (hwndRect, NULL, FALSE);
                    WinInvalidateRect (hwndSample, NULL, FALSE);
                    bListOK = TRUE;
                }
            }
            else
                bDDP = TRUE;
            break;

          case IDD_C_DEFAULT:
            if (BN_CLICKED == SHORT2FROMMP (mp1)) {
                if (WinSendDlgItemMsg (hwnd, IDD_C_DEFAULT, BM_QUERYCHECK, 0, 0)) {
                    phecCurr = &hecDefault;
                    iColorTable = 0;
                    s_SetSpinVals (hwnd, phecCurr);
                    WinInvalidateRect (hwndRect, NULL, FALSE);
                    WinInvalidateRect (hwndSample, NULL, FALSE);
                    WinEnableWindow (WinWindowFromID (hwnd, IDD_C_RED), FALSE);
                    WinEnableWindow (WinWindowFromID (hwnd, IDD_C_GREEN), FALSE);
                    WinEnableWindow (WinWindowFromID (hwnd, IDD_C_BLUE), FALSE);
                }
            }
            else
                bDDP = TRUE;
            break;

          case IDD_C_SYSTEM:
            if (BN_CLICKED == SHORT2FROMMP (mp1)) {
                if (WinSendDlgItemMsg (hwnd, IDD_C_SYSTEM, BM_QUERYCHECK, 0, 0)) {
                    phecCurr = &hecSystem;
                    iColorTable = 1;
                    s_SetSpinVals (hwnd, phecCurr);
                    WinInvalidateRect (hwndRect, NULL, FALSE);
                    WinInvalidateRect (hwndSample, NULL, FALSE);
                    WinEnableWindow (WinWindowFromID (hwnd, IDD_C_RED), FALSE);
                    WinEnableWindow (WinWindowFromID (hwnd, IDD_C_GREEN), FALSE);
                    WinEnableWindow (WinWindowFromID (hwnd, IDD_C_BLUE), FALSE);
                }
            }
            else
                bDDP = TRUE;
            break;

          case IDD_C_CUSTOM:
            if (BN_CLICKED == SHORT2FROMMP (mp1)) {
                if (WinSendDlgItemMsg (hwnd, IDD_C_CUSTOM, BM_QUERYCHECK, 0, 0)) {
                    phecCurr = &hecCustom;
                    iColorTable = 2;
                    s_SetSpinVals (hwnd, phecCurr);
                    WinInvalidateRect (hwndRect, NULL, FALSE);
                    WinInvalidateRect (hwndSample, NULL, FALSE);
                    WinEnableWindow (WinWindowFromID (hwnd, IDD_C_RED), TRUE);
                    WinEnableWindow (WinWindowFromID (hwnd, IDD_C_GREEN), TRUE);
                    WinEnableWindow (WinWindowFromID (hwnd, IDD_C_BLUE), TRUE);
                }
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


/**************************************************
*
*   s_SetSpinVals
*
**************************************************/

static void s_SetSpinVals (HWND hwnd, PHECOLORS phec)
{
    int i;
    BOOL    bList = bListOK;

    bListOK = FALSE;

    i = (int) WinSendDlgItemMsg (hwnd, IDD_C_LIST, LM_QUERYSELECTION,
                    MPFROMSHORT (LIT_FIRST), 0);

    if (0 <= i && i < NUMCOLS) {
        WinSendDlgItemMsg (hwnd, IDD_C_RED, SPBM_SETCURRENTVALUE,
                    MPFROMLONG (RED(LONGARR(*phec)[i])), 0);
        WinSendDlgItemMsg (hwnd, IDD_C_GREEN, SPBM_SETCURRENTVALUE,
                    MPFROMLONG (GREEN(LONGARR(*phec)[i])), 0);
        WinSendDlgItemMsg (hwnd, IDD_C_BLUE, SPBM_SETCURRENTVALUE,
                    MPFROMLONG (BLUE(LONGARR(*phec)[i])), 0);
    }
    bListOK = bList;
}

/**************************************************
*
*   s_MakeList
*
**************************************************/

static void s_MakeList (HWND hwndDlg)
{
    int     i;
    HWND    hwndList;
    char    szBuff[COLMAXNAME];
    HAB     hab;

    hab = WinQueryAnchorBlock (hwndDlg);
    hwndList = WinWindowFromID (hwndDlg, IDD_C_LIST);
    WinEnableWindow (hwndList, FALSE);
    for (i = 0; i < NUMCOLS; i++) {
        WinLoadString (hab, g.hResource, (ULONG) (IDS_COLNAME + i), COLMAXNAME, szBuff);
        WinSendMsg (hwndList, LM_INSERTITEM, MPFROMSHORT (LIT_END), MPFROMP (szBuff));
    }
    WinEnableWindow (hwndList, TRUE);
}



/**************************************************
*
*   ColorSubProc
*
**************************************************/

MRESULT APIENTRY ColorSubProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bDWP = TRUE;
    MRESULT mRes = MRFROMLONG (0);
    RECTL   rectl;
    HWND    hwndDlg;
    HPS     hps;
    int     i;

    switch (ulMsg)
    {
      case WM_PAINT:
        hwndDlg = WinQueryWindow (hwnd, QW_PARENT);
        hps = WinBeginPaint (hwnd, NULLHANDLE, &rectl);
        GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL);
        i = (int) WinSendDlgItemMsg (hwndDlg, IDD_C_LIST, LM_QUERYSELECTION,
                        MPFROMSHORT (LIT_FIRST), 0);
        if (i < 0) i = 0;

        WinQueryWindowRect (hwnd, &rectl);
        WinFillRect (hps, &rectl, LONGARR(*phecCurr)[i]);
        WinEndPaint (hps);

        bDWP = FALSE;
        break;

    }

    if (TRUE == bDWP)
        mRes = (*pColorOrgProc) (hwnd, ulMsg, mp1, mp2);
    return mRes;
}




/**************************************************
*
*   SampleSubProc
*
**************************************************/

MRESULT APIENTRY SampleSubProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bDWP = TRUE;
    MRESULT mRes = MRFROMLONG (0);
    RECTL   rectl, rectlSel;
    HPS     hps;
    POINTL  ptl;
    PSZ     psz;

    switch (ulMsg)
    {
      case WM_PAINT:
        hps = WinBeginPaint (hwnd, NULLHANDLE, &rectl);
        fnt_CreateFont (hps, lFont, 1);
        GpiSetCharSet (hps, 1);
        GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL);


        WinQueryWindowRect (hwnd, &rectl);
        WinFillRect (hps, &rectl, phecCurr->lrgbTextBack);

        ptl.x = cxChar;
        ptl.y = rectl.yTop - cyChar;
        GpiSetColor (hps, phecCurr->lrgbTextFore);
        psz = "This line contains a selection";
        GpiCharStringAt (hps, &ptl, (LONG) strlen (psz), psz);

        rectlSel.xLeft = ptl.x + 13 * cxChar;
        rectlSel.xRight = rectlSel.xLeft + cxChar;
        rectlSel.yBottom = ptl.y - cyDesc;
        rectlSel.yTop = rectlSel.yBottom + cyChar;
        WinFillRect (hps, &rectlSel, phecCurr->lrgbCursorBack);
        GpiSetColor (hps, phecCurr->lrgbCursorFore);
        ptl.x += 13 * cxChar;
        GpiCharStringAt (hps, &ptl, 1, psz + 13);

        ptl.x = cxChar;
        ptl.y = rectl.yTop - 2 * cyChar;
        rectlSel.xLeft = ptl.x + 13 * cxChar;
        rectlSel.xRight = rectlSel.xLeft + cxChar;
        rectlSel.yBottom = ptl.y - cyDesc;
        rectlSel.yTop = rectlSel.yBottom + cyChar;
        WinFillRect (hps, &rectlSel, phecCurr->lrgbHiliteBack);
        GpiSetColor (hps, phecCurr->lrgbTextFore);
        psz = "This line contains a mark";
        GpiCharStringAt (hps, &ptl, (LONG) strlen (psz), psz);
        GpiSetColor (hps, phecCurr->lrgbHiliteFore);
        ptl.x += 13 * cxChar;
        GpiCharStringAt (hps, &ptl, 1, psz + 13);

        GpiDeleteSetId (hps, 1);
        WinEndPaint (hps);

        bDWP = FALSE;
        break;

    }

    if (TRUE == bDWP)
        mRes = (*pSampleOrgProc) (hwnd, ulMsg, mp1, mp2);
    return mRes;
}
