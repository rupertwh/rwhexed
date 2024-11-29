#define INCL_WIN
#define INCL_GPI


#include <os2.h>
#include <string.h>
#include <stdio.h>

#include "res.h"
#include "memmgr.h"
#include "helpers.h"
#include "fonts.h"
#include "status.h"

typedef struct {
    HAB     hab;
    HPS     hps;
    LONG    lSize;
    LONG    lOffset;
    LONG    lSelEnd;
    LONG    lSel;
    ULONG   ulHexSel;
    char    szLongname[CCHMAXPATH];
    char    szBuff[CCHMAXPATH + 20];
    PSZ     pszSize;
    PSZ     pszOffset;
    PSZ     pszSel;
    PSZ     pszLongname;
    PSZ     pszActive;
    RECTL   rectlSize;
    RECTL   rectlOffset;
    RECTL   rectlSel;
    RECTL   rectlLongname;
    RECTL   rectlActiveLED;
    RECTL   rectlWin;
    LONG    lOptHight;
    int     cxChar;
    int     cyChar;
    int     cyDesc;
    LONG    lrgbBackground;
    LONG    lrgbLEDRedOn;
    LONG    lrgbLEDRedOff;
    LONG    lcxScroll;
    LONG    lcyScroll;
    BOOL    fActive;
} WNDDATA;
typedef WNDDATA* PWNDDATA;

#define min(x,y) ((x)<(y)?(x):(y))

static MRESULT s_Create (HWND hwnd);
static void s_Paint (HWND hwnd, PWNDDATA pWndData);
static void s_Size (HWND hwnd, PWNDDATA pWndData, int cx, int cy);
static void s_Draw3DRect (HPS hps, PRECTL prectl, BOOL bInset);

MRESULT EXPENTRY StatusWndProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2);





/***********************************************
*
*   st_RegisterClass
*
************************************************/

BOOL st_RegisterClass (HAB hab, PSZ pszName)
{
   return WinRegisterClass (hab, pszName, StatusWndProc, 0, sizeof (ULONG));
}



/***********************************************
*
*   StatusWndProc
*
************************************************/

MRESULT EXPENTRY StatusWndProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL        bDWP = FALSE;
    MRESULT     mRet = MRFROMLONG (0);
    PWNDDATA    pWndData = NULL;

    switch (ulMsg)
    {
      case WM_PAINT:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        s_Paint (hwnd, pWndData);
        break;

      case WM_CREATE:
        mRet = s_Create (hwnd);
        break;

      case WM_SIZE:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        s_Size (hwnd, pWndData, SHORT1FROMMP (mp2), SHORT2FROMMP (mp2));
        break;

      case WMUSR_ST_SETSIZE:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        pWndData->lSize = (LONG) LONGFROMMP (mp1);
        WinInvalidateRect (hwnd, &pWndData->rectlSize, FALSE);
        break;

      case WMUSR_ST_SETOFFSET:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        pWndData->lOffset = (LONG) LONGFROMMP (mp1);
        WinInvalidateRect (hwnd, &pWndData->rectlOffset, FALSE);
        break;

      case WMUSR_ST_SETSEL:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        pWndData->lSelEnd = (LONG) LONGFROMMP (mp1);
        if (pWndData->lSelEnd >= pWndData->lOffset) {
            pWndData->lSel = pWndData->lSelEnd - pWndData->lOffset + 1;
            pWndData->ulHexSel = (ULONG) pWndData->lSel;
        }
        else {
            pWndData->lSel = pWndData->lSelEnd - pWndData->lOffset - 1;
            pWndData->ulHexSel = (ULONG) -pWndData->lSel;
        }

        WinInvalidateRect (hwnd, &pWndData->rectlSel, FALSE);
        break;

      case WMUSR_ST_SETLONGNAME:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        strcpy (pWndData->szLongname, (PSZ)PVOIDFROMMP (mp1));
        WinInvalidateRect (hwnd, &pWndData->rectlLongname, FALSE);
        break;

      case WMUSR_ST_SETACTIVE:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        pWndData->fActive = (BOOL) LONGFROMMP (mp1);
        WinInvalidateRect (hwnd, &pWndData->rectlActiveLED, FALSE);
        break;

      case WMUSR_ST_QUERYOPTHIGHT:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        mRet = MRFROMLONG (pWndData->lOptHight);
        break;

      case WM_DESTROY:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        hlp_FreeString (pWndData->pszSize);
        hlp_FreeString (pWndData->pszOffset);
        hlp_FreeString (pWndData->pszSel);
        hlp_FreeString (pWndData->pszLongname);
        hlp_FreeString (pWndData->pszActive);
        mem_HeapFree (pWndData);
        break;

      default:
        bDWP = TRUE;
        break;
    }

    if (bDWP)
        mRet = WinDefWindowProc (hwnd, ulMsg, mp1, mp2);

    return mRet;
}




/***********************************************
*
*   s_Create
*
************************************************/

static MRESULT s_Create (HWND hwnd)
{
    PWNDDATA        pWndData;
    HDC             hdc;
    SIZEL           sl;
    PFONTMETRICS    pfm = NULL;

    pWndData = (PWNDDATA) mem_HeapAlloc (sizeof (WNDDATA));
    if (NULL == pWndData) return MRFROMLONG (TRUE);

    WinSetWindowULong (hwnd, 0, (ULONG)(PVOID)pWndData);
    pWndData->lSize = pWndData->lOffset = 0;
    *pWndData->szLongname = (char) 0;

    pWndData->hab = WinQueryAnchorBlock (hwnd);
    pWndData->pszSize = hlp_GetString (pWndData->hab, IDS_STAT_SIZE);
    pWndData->pszOffset = hlp_GetString (pWndData->hab, IDS_STAT_OFFSET);
    pWndData->pszSel = hlp_GetString (pWndData->hab, IDS_STAT_SELEND);
    pWndData->pszLongname = hlp_GetString (pWndData->hab, IDS_STAT_LONGNAME);
    pWndData->pszActive = hlp_GetString (pWndData->hab, IDS_STAT_ACTIVE);

    if (!(pWndData->pszSize && pWndData->pszOffset && pWndData->pszLongname))
        return MRFROMLONG (TRUE);


    hdc = WinOpenWindowDC (hwnd);
    sl.cx = sl.cy = 0;
    pWndData->hps = GpiCreatePS (pWndData->hab, hdc, &sl, PU_PELS|GPIT_MICRO|GPIA_ASSOC);
    if (NULLHANDLE == pWndData->hps) return MRFROMLONG (TRUE);

    fnt_CreateFont (pWndData->hps, -1, 1); /* select Helv 8 */
    GpiSetCharSet (pWndData->hps, 1);


    pfm = fnt_QueryMetrics (-1);
    if (NULL == pfm) return MRFROMLONG (TRUE);

    pWndData->cxChar = (int) pfm->lAveCharWidth;
    pWndData->cyChar = (int) pfm->lMaxBaselineExt;
    pWndData->cyDesc = (int) pfm->lMaxDescender;

    pWndData->lOptHight = 2 * (pWndData->cyChar) + 6 * pWndData->cyDesc;

    pWndData->lrgbBackground = WinQuerySysColor (HWND_DESKTOP, SYSCLR_DIALOGBACKGROUND, 0);
    pWndData->lrgbLEDRedOn = CLR_RED;
    pWndData->lrgbLEDRedOff = CLR_DARKRED;

    pWndData->lcxScroll = WinQuerySysValue (HWND_DESKTOP, SV_CXVSCROLL);
    pWndData->lcyScroll = WinQuerySysValue (HWND_DESKTOP, SV_CYHSCROLL);

    return MRFROMLONG (0);
}



/***********************************************
*
*   s_Size
*
************************************************/

static void s_Size (HWND hwnd, PWNDDATA pWndData, int cx, int cy)
{
    pWndData->rectlWin.xLeft = 1;
    pWndData->rectlWin.xRight = cx - 1;
    pWndData->rectlWin.yBottom = 1;
    pWndData->rectlWin.yTop = cy - 1;

    pWndData->rectlSize.xLeft = pWndData->cxChar;
    pWndData->rectlSize.xRight = pWndData->rectlSize.xLeft + (LONG)(strlen (pWndData->pszSize) + 24) * pWndData->cxChar;
    pWndData->rectlSize.yBottom = cy - pWndData->cyChar - (pWndData->cyDesc << 1)/* - pWndData->cyDesc*/;
    pWndData->rectlSize.yTop = pWndData->rectlSize.yBottom + pWndData->cyChar;

    pWndData->rectlOffset.xLeft = pWndData->rectlSize.xRight + 4 * pWndData->cxChar;
    pWndData->rectlOffset.xRight = pWndData->rectlOffset.xLeft +
                                (LONG)(strlen (pWndData->pszOffset) + 24) * pWndData->cxChar;
    pWndData->rectlOffset.yBottom = pWndData->rectlSize.yBottom;
    pWndData->rectlOffset.yTop = pWndData->rectlOffset.yBottom + pWndData->cyChar;

    pWndData->rectlSel.xLeft = pWndData->rectlOffset.xRight + 4 * pWndData->cxChar;
    pWndData->rectlSel.xRight = pWndData->rectlSel.xLeft +
                                (LONG)(strlen (pWndData->pszSel) + 24) * pWndData->cxChar;
    pWndData->rectlSel.yBottom = pWndData->rectlSize.yBottom;
    pWndData->rectlSel.yTop = pWndData->rectlSel.yBottom + pWndData->cyChar;

    pWndData->rectlLongname.xLeft = pWndData->cxChar;
/*    pWndData->rectlLongname.xRight = pWndData->rectlLongname.xLeft +
                                        min (cx / pWndData->cxChar - 3,
                                           (LONG) strlen (pWndData->pszLongname) + CCHMAXPATH)
                                        * pWndData->cxChar; */
    pWndData->rectlLongname.xRight = pWndData->rectlSel.xRight;
    pWndData->rectlLongname.yBottom = pWndData->rectlSize.yBottom -
                                      pWndData->cyChar - (pWndData->cyDesc << 1);
    pWndData->rectlLongname.yTop = pWndData->rectlLongname.yBottom + pWndData->cyChar;

    pWndData->rectlActiveLED.yTop = pWndData->rectlSize.yTop;
    pWndData->rectlActiveLED.yBottom = pWndData->rectlActiveLED.yTop - (pWndData->lcyScroll * 5) / 12;
    pWndData->rectlActiveLED.xRight = cx - pWndData->lcxScroll;
    pWndData->rectlActiveLED.xLeft = pWndData->rectlActiveLED.xRight - pWndData->lcxScroll;

}




/***********************************************
*
*   s_Paint
*
************************************************/

static void s_Paint (HWND hwnd, PWNDDATA pWndData)
{
    RECTL   rectl;
    POINTL  pointl;
    HPS     hps;
    LONG    lrgb;

    hps = WinBeginPaint (hwnd, pWndData->hps, &rectl);
/*    GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL); /* set to RGB mode */
    GpiSetColor (hps, CLR_DEFAULT);
    WinFillRect (hps, &rectl, CLR_PALEGRAY);

    s_Draw3DRect (hps, &pWndData->rectlWin, FALSE);
    s_Draw3DRect (hps, &pWndData->rectlSize, TRUE);
    s_Draw3DRect (hps, &pWndData->rectlOffset, TRUE);
    s_Draw3DRect (hps, &pWndData->rectlSel, TRUE);
    s_Draw3DRect (hps, &pWndData->rectlLongname, TRUE);
    s_Draw3DRect (hps, &pWndData->rectlActiveLED, TRUE);

    sprintf (pWndData->szBuff, "%s 0x%08lX (%ld)", pWndData->pszSize, pWndData->lSize, pWndData->lSize);
    pointl.x = pWndData->rectlSize.xLeft;
    pointl.y = pWndData->rectlSize.yBottom + pWndData->cyDesc;
    GpiCharStringAt (hps, &pointl, (LONG) strlen (pWndData->szBuff), pWndData->szBuff);

    sprintf (pWndData->szBuff, "%s 0x%08lX (%ld)", pWndData->pszOffset, pWndData->lOffset, pWndData->lOffset);
    pointl.x = pWndData->rectlOffset.xLeft;
    pointl.y = pWndData->rectlOffset.yBottom + pWndData->cyDesc;
    GpiCharStringAt (hps, &pointl, (LONG) strlen (pWndData->szBuff), pWndData->szBuff);

    sprintf (pWndData->szBuff, "%s 0x%08lX (%ld)", pWndData->pszSel, pWndData->ulHexSel, pWndData->lSel);
    pointl.x = pWndData->rectlSel.xLeft;
    pointl.y = pWndData->rectlSel.yBottom + pWndData->cyDesc;
    GpiCharStringAt (hps, &pointl, (LONG) strlen (pWndData->szBuff), pWndData->szBuff);

    sprintf (pWndData->szBuff, "%s %s", pWndData->pszLongname, pWndData->szLongname);
    pointl.x = pWndData->rectlLongname.xLeft;
    pointl.y = pWndData->rectlLongname.yBottom + pWndData->cyDesc;
    GpiCharStringAt (hps, &pointl, (LONG) strlen (pWndData->szBuff), pWndData->szBuff);

    if (pWndData->fActive)
        lrgb = pWndData->lrgbLEDRedOn;
    else
        lrgb = pWndData->lrgbLEDRedOff;


    WinFillRect (hps, &pWndData->rectlActiveLED, lrgb);

    WinEndPaint (hps);

}


static void s_Draw3DRect (HPS hps, PRECTL prectl, BOOL bInset)
{
    POINTL  pointl;
    LONG    lColor;

    lColor = GpiQueryColor (hps);

    pointl.x = prectl->xLeft - 1;
    pointl.y = prectl->yBottom - 1;
    GpiMove (hps, &pointl);

    GpiSetColor (hps, bInset ? CLR_DARKGRAY : CLR_WHITE);
    pointl.y = prectl->yTop;
    GpiLine (hps, &pointl);
    pointl.x = prectl->xRight;
    GpiLine (hps, &pointl);

    GpiSetColor (hps, bInset ? CLR_WHITE : CLR_DARKGRAY);
    pointl.y = prectl->yBottom - 1;
    GpiLine (hps, &pointl);
    pointl.x = prectl->xLeft - 1;
    GpiLine (hps, &pointl);

    GpiSetColor (hps, lColor);
}















