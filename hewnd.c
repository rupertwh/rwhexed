#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS

#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hewnd.h"
#include "fonts.h"
#include "helpers.h"
#include "res.h"
#include "memmgr.h"

#ifdef DEBUG
    #include "debug.h"
#endif

typedef struct {
    USHORT      usId;           /* window ID */
    HAB         hab;
    HPS         hps;
    PBYTE       pData;          /* pointer to displayed data */
    LONG        lSize;          /* length of data in bytes */
    LONG        lLines;         /* # of lines */
    LONG        lMaxSize;       /* allcoated size for data */
    LONG        lOffset;        /* current cursor pos */
    LONG        lPageOffset;    /* offset of first displayed byte on page */
    LONG        lSelEnd;        /* offset of last selected byte */
    BOOL        fMulSel;        /* more than one byte is selected */
    HWND        hwndParent;
    HWND        hwndHScroll;
    HWND        hwndVScroll;
    int         iColorTable; /* 0 = hecColors, 1 = Defaut, 2 = System */
    HECOLORS    hecColors;
    int         cx;             /* width of window in pixels */
    int         cy;             /* height */
    LONG        lFont;
    int         cxChar;         /* width of one char */
    int         cyChar;         /* height of one char */
    int         cyDesc;         /* height of descent */
    int         cyLines;        /* # of lines that fit into display */
    int         cxText;         /* width in pixels of complete display */
    int         cxCols;         /* # of columns that fit into display */
    SHORT       sVScrollRes;
    double      dVScrollScale;
    int         iHScrollPos;
    int         iHScrollMax;
    int         iVScrollPos;
    BOOL        fHex;           /* cursor is in hex part of window */
    BOOL        fLow;           /* cursor is on low nibble */
    BOOL        fEnabled;       /* window is enabled */
    BOOL        fInMotion;      /* mouse selection */
    BOOL        fShift;         /* shift is being held down */
    BOOL        fAlt;           /* alt ... */
    BOOL        fCtrl;          /* alt ... */
    BOOL        fFocus;         /* window has focus */
    ATOM        atomClipbrd;    /* Clipbrd format ID for hex-data */
} WNDDATA;
typedef WNDDATA* PWNDDATA;


static void s_HorzScroll (HWND hwnd, PWNDDATA pWndData, int iCmd, int iSlider);
static void s_VertScroll (HWND hwnd, PWNDDATA pWndData, int iCmd, int iSlider);
static LONG s_Search (PWNDDATA pWndData, PBYTE pPat, LONG nSize, int ulOptions);
static void s_ScrollSet (HWND hwnd, PWNDDATA pWndData);
static int s_VertSlider (PWNDDATA pWndData, LONG lOff);
static void s_ScrollPos (PWNDDATA pWndData);
static void s_Paint (HWND hwnd, PWNDDATA pWndData);
static int s_Visible (PWNDDATA pWndData, LONG lOff);
static int s_HorzOffHex (PWNDDATA pWndData, LONG lOff);
static int s_HorzOffChar (PWNDDATA pWndData, LONG lOff);
static int s_VertOff (PWNDDATA pWndData, LONG lOff);
static LONG s_OffFromPoint (PWNDDATA pWndData, POINTS pts, PLONG plPos);
static void s_InvLine (HWND hwnd, PWNDDATA pWndData, LONG lOff);
static void s_InvArea (HWND hwnd, PWNDDATA pWndData, LONG lOffStart, LONG lOffEnd);
static void s_InvNewSel (HWND hwnd, PWNDDATA pWndData, LONG lNewOff, LONG lNewEnd, BOOL fNewHex);
static BOOL s_CharProc (HWND hwnd, PWNDDATA pWndData, ULONG mp1, ULONG mp2);
static void s_IntoView (HWND hwnd, PWNDDATA pWndData, LONG lNewPos, int iDir, ULONG mp1, ULONG mp2);
static APIRET s_SetBufferSize (PBYTE pBuffer, LONG lOldSize, LONG lNewSize);
static void s_Bttn1Click (HWND hwnd, PWNDDATA pWndData, POINTS pts);
static BOOL s_DeleteData (HWND hwnd, PWNDDATA pWndData, LONG lDelLen);
static BOOL s_InsertEmpty (HWND hwnd, PWNDDATA pWndData, LONG lLen, int iChar);
static BOOL s_InsertData (HWND hwnd, PWNDDATA pWndData, LONG lLen, PBYTE pBuff);
static BOOL s_QuerySomeData (PWNDDATA pWndData, PBYTE pBuff, LONG lLen);
static BOOL s_SetFont (HWND hwnd, PWNDDATA pWndData, LONG lFont);
static MRESULT s_Create (HWND hwnd);
static BOOL s_IsOffSelected (LONG lOff, PWNDDATA pWndData);
static void s_EndMotion (HWND hwnd, PWNDDATA pWndData, POINTS pts);
static int s_PosFromString (int iStrOff);
static void s_DragSel (HWND hwnd, PWNDDATA pWndData, POINTS pts);
static void s_DealWithFocus (HWND hwnd, PWNDDATA pWndData, HWND hFocWin, BOOL fSet);
static void s_ClipCopy (HWND hwnd, PWNDDATA pWndData);
static void s_ClipPaste (HWND hwnd, PWNDDATA pWndData);
static BOOL s_DeleteSel (HWND hwnd, PWNDDATA pWndData);
static BOOL s_QueryPaste (HWND hwnd, PWNDDATA pWndData);


MRESULT EXPENTRY HexWndProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2);


static HECOLORS hecSystem;
static HECOLORS hecDefault;
static BOOL     fInit;
static HPOINTER hPointerText;   /* the 'I-beam' cursor */


/*****************************************************
*
*   hex_RegisterClass
*
******************************************************/

BOOL hex_RegisterClass (HAB hab, PSZ pszName)
{
   return WinRegisterClass (hab, pszName, HexWndProc, 0, sizeof (ULONG));
}


/*****************************************************
*
*   HexWndProc
*
******************************************************/

MRESULT EXPENTRY HexWndProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL            bDWP = FALSE;
    MRESULT         mRet = (MRESULT) 0;
    PWNDDATA        pWndData = NULL;
    PHECOLORS       phec;
    PHESETDATA      phesd;
    LONG            lVal;

    switch (ulMsg)
    {

      case WM_MOUSEMOVE:
        WinSetPointer (HWND_DESKTOP, hPointerText);
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        if (pWndData->fInMotion)
            s_DragSel (hwnd, pWndData, *((POINTS*)(PVOID)&mp1));
        break;

      case WM_PAINT:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        s_Paint (hwnd, pWndData);
        break;

      case WM_CHAR:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        if (pWndData->fEnabled)
            bDWP = s_CharProc (hwnd, pWndData, (ULONG)LONGFROMMP (mp1), (ULONG)LONGFROMMP (mp2));
        break;

      case WM_HSCROLL:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        s_HorzScroll (hwnd, pWndData, SHORT2FROMMP (mp2), SHORT1FROMMP (mp2));
        break;

      case WM_VSCROLL:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        s_VertScroll (hwnd, pWndData, SHORT2FROMMP (mp2), SHORT1FROMMP (mp2));
        break;

      case WM_SIZE:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        pWndData->cx = (int) SHORT1FROMMP (mp2);
        pWndData->cy = (int) SHORT2FROMMP (mp2);
        pWndData->cyLines = pWndData->cy / pWndData->cyChar;
        pWndData->cxCols = (pWndData->cx - (pWndData->cxChar << 1)) /
                             pWndData->cxChar;
        s_ScrollSet (hwnd, pWndData);
        break;

      case WM_BUTTON1DOWN:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        WinSetFocus (HWND_DESKTOP, hwnd);
        s_Bttn1Click (hwnd, pWndData, *((POINTS*)(PVOID)&mp1));
        bDWP = TRUE;
        break;

      case WM_BUTTON2CLICK:
        WinPostMsg (WinQueryWindow (hwnd, QW_PARENT), ulMsg, mp1, mp2);
        break;

      case WM_BUTTON1MOTIONSTART:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        pWndData->fInMotion = TRUE;
        pWndData->fMulSel = TRUE;
        break;

      case WM_BUTTON1MOTIONEND:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        s_EndMotion (hwnd, pWndData, *((POINTS*)(PVOID)&mp1));
        break;

      case WM_SETFOCUS:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        s_DealWithFocus (hwnd, pWndData, HWNDFROMMP (mp1), (BOOL) SHORT1FROMMP (mp2));
        break;

      case WMUSR_HE_SETDATA:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        phesd = (PHESETDATA) PVOIDFROMMP (mp1);
        pWndData->pData = phesd->pbData;
        pWndData->lSize = (LONG) phesd->ulSize;
        pWndData->lMaxSize = (LONG) phesd->ulMaxAlloc;
        pWndData->lOffset = (LONG) phesd->ulOffset;
        pWndData->lSelEnd = (LONG) phesd->ulSelEnd;
        pWndData->lPageOffset = (LONG) phesd->ulPageOffset;
        pWndData->lLines = ((pWndData->lSize + 15) >> 4) + 1;
        s_ScrollSet (hwnd, pWndData);
        WinInvalidateRect (hwnd, NULL, FALSE);
        mRet = MRFROMSHORT (TRUE);
        break;

      case WMUSR_HE_QUERYDATA:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        if (NULL == pWndData->pData) {
            mRet = MRFROMLONG (FALSE);
            break;
        }
        phesd = (PHESETDATA) PVOIDFROMMP (mp1);
        phesd->pbData = pWndData->pData;
        phesd->ulSize  = (ULONG) pWndData->lSize;
        phesd->ulMaxAlloc = (ULONG) pWndData->lMaxSize;
        phesd->ulOffset = (ULONG) pWndData->lOffset;
        phesd->ulSelEnd = (ULONG) pWndData->lSelEnd;
        phesd->ulPageOffset = (ULONG) pWndData->lPageOffset;
        mRet = MRFROMLONG (TRUE);
        break;

      case WMUSR_HE_QUERYSOMEDATA:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        phesd = (PHESETDATA) PVOIDFROMMP (mp1);
        mRet = MRFROMLONG (s_QuerySomeData (pWndData, phesd->pbData, (LONG) phesd->ulSize));
        phesd->ulMaxAlloc = (ULONG) pWndData->lMaxSize;
        phesd->ulOffset = (ULONG) pWndData->lOffset;
        phesd->ulSelEnd = (ULONG) pWndData->lSelEnd;
        phesd->ulPageOffset = (ULONG) pWndData->lPageOffset;
        break;

      case WMUSR_HE_DELETEDATA:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        if (pWndData->fEnabled) {
            mRet = MRFROMLONG (s_DeleteData (hwnd, pWndData, (LONG) LONGFROMMP (mp1)));
            WinInvalidateRect (hwnd, NULL, FALSE);
            s_ScrollSet (hwnd, pWndData);
        }
        break;

      case WMUSR_HE_INSERTEMPTY:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        if (pWndData->fEnabled) {
            mRet = MRFROMLONG (s_InsertEmpty (hwnd, pWndData, (LONG) LONGFROMMP (mp1), (int) SHORT1FROMMP (mp2)));
            WinInvalidateRect (hwnd, NULL, FALSE);
            s_ScrollSet (hwnd, pWndData);
        }
        break;

      case WMUSR_HE_INSERTDATA:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        if (pWndData->fEnabled) {
            mRet = MRFROMLONG (s_InsertData (hwnd, pWndData, (LONG) LONGFROMMP (mp1), PVOIDFROMMP (mp2)));
            WinInvalidateRect (hwnd, NULL, FALSE);
            s_ScrollSet (hwnd, pWndData);
        }
        break;

      case WMUSR_HE_SETPOS:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        lVal = (LONG) LONGFROMMP (mp1);
        if (lVal > pWndData->lSize - 1)
            mRet = MRFROMSHORT (FALSE);
        else {
            mRet = MRFROMSHORT (TRUE);
            s_IntoView (hwnd, pWndData, lVal, 1, 0, 0);
            s_InvLine (hwnd, pWndData, lVal);
            s_InvArea (hwnd, pWndData, pWndData->lOffset, pWndData->lSelEnd);
            pWndData->lOffset = pWndData->lSelEnd = lVal;
        }
        WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_OFFSET),
                                                  MPFROMLONG (pWndData->lOffset));
        WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SELEND),
                                                  MPFROMLONG (pWndData->lSelEnd));
        break;

      case WMUSR_HE_QUERYPOS:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        mRet = MRFROMLONG (pWndData->lOffset);
        break;

      case WMUSR_HE_SETCOLORS:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        phec = (PHECOLORS) PVOIDFROMMP (mp1);
        pWndData->iColorTable = (int) LONGFROMMP (mp2);
        if (NULL != phec)
            memcpy (&pWndData->hecColors, phec, sizeof (HECOLORS));
        WinInvalidateRect (hwnd, NULL, FALSE);
        mRet = MRFROMSHORT (TRUE);
        break;

      case WMUSR_HE_SEARCH:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        mRet = MRFROMLONG (s_Search (pWndData, PVOIDFROMMP (mp1), (LONG)SHORT1FROMMP (mp2),
                                     (int) SHORT2FROMMP (mp2)));
        break;

      case WMUSR_HE_QUERYCOLORS:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        switch (LONGFROMMP (mp2)) {
          case 1: phec = &hecSystem; break;
          case 2: phec = &pWndData->hecColors; break;
          default: phec = &hecDefault; break;
        }
        if (mp1)
            memcpy (PVOIDFROMMP (mp1), phec, sizeof (HECOLORS));
        mRet = MRFROMLONG (pWndData->iColorTable);
        break;

      case WMUSR_HE_SETFONT:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        mRet = MRFROMLONG (s_SetFont (hwnd, pWndData, (LONG)LONGFROMMP (mp1)));
        break;

      case WMUSR_HE_QUERYFONT:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        mRet = MRFROMLONG (pWndData->lFont);
        break;

      case WMUSR_HE_SETHSCROLLHWND:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        pWndData->hwndHScroll = HWNDFROMMP (mp1);
        break;

      case WMUSR_HE_SETVSCROLLHWND:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        pWndData->hwndVScroll = HWNDFROMMP (mp1);
        break;

      case WMUSR_HE_COPY:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        if (pWndData->pData)
            s_ClipCopy (hwnd, pWndData);
        break;

      case WMUSR_HE_PASTE:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        if (pWndData->pData)
            s_ClipPaste (hwnd, pWndData);
        break;

      case WMUSR_HE_DEL:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        if (pWndData->pData) {
            s_DeleteSel (hwnd, pWndData);
            WinInvalidateRect (hwnd, NULL, FALSE);
            s_ScrollSet (hwnd, pWndData);
            WinUpdateWindow (hwnd);
        }
        break;

      case WMUSR_HE_QUERYCANPASTE:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        if (pWndData->pData)
            mRet = (MRFROMLONG (s_QueryPaste (hwnd, pWndData)));
        else
            mRet = MRFROMLONG (FALSE);
        break;

      case WM_ENABLE:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        pWndData->fEnabled = (BOOL) SHORT1FROMMP (mp1);
        break;

      case WM_CREATE:
        mRet = s_Create (hwnd);
        break;

      case WM_DESTROY:
        pWndData = (PWNDDATA)(PVOID) WinQueryWindowULong (hwnd, 0);
        if (hPointerText) {
            WinDestroyPointer (hPointerText);
            hPointerText = NULLHANDLE;
        }
        WinDeleteAtom (WinQuerySystemAtomTable (), pWndData->atomClipbrd);
        mem_HeapFree (pWndData);
        bDWP = TRUE;
        break;

      default:
        bDWP = TRUE;
        break;
    }

    if (bDWP)
        mRet = WinDefWindowProc (hwnd, ulMsg, mp1, mp2);

    return mRet;
}


/******************************************************
*
*   s_Create
*
******************************************************/

static MRESULT s_Create (HWND hwnd)
{
    PWNDDATA    pWndData = NULL;
    SIZEL       sl;
    HDC         hdc;

    if (!(pWndData = (PWNDDATA) mem_HeapAlloc (sizeof (WNDDATA))))
        return MRFROMLONG (TRUE);

    WinSetWindowULong (hwnd, 0, (ULONG)(PVOID)pWndData);

    if (FALSE == fInit) {
        hecDefault.lrgbTextFore = 0x00008000;
        hecDefault.lrgbTextBack = 0x00000000;
        hecDefault.lrgbCursorFore = 0x00000000;
        hecDefault.lrgbCursorBack = 0x00FF0000;
        hecDefault.lrgbHiliteFore = 0x0000FF00;
        hecDefault.lrgbHiliteBack = 0x00000000;

        hecSystem.lrgbTextFore = WinQuerySysColor (HWND_DESKTOP, SYSCLR_WINDOWTEXT, 0);
        hecSystem.lrgbTextBack = WinQuerySysColor (HWND_DESKTOP, SYSCLR_ENTRYFIELD, 0);
        hecSystem.lrgbCursorFore = WinQuerySysColor (HWND_DESKTOP, SYSCLR_HILITEFOREGROUND, 0);
        hecSystem.lrgbCursorBack = WinQuerySysColor (HWND_DESKTOP, SYSCLR_HILITEBACKGROUND, 0);
        hecSystem.lrgbHiliteFore = 0x00FF0000;
        hecSystem.lrgbHiliteBack = hecSystem.lrgbTextBack;

        fInit = TRUE;
    }

    pWndData->usId = WinQueryWindowUShort (hwnd, QWS_ID);
    pWndData->cxChar = pWndData->cyChar = 1; /* avoid division by zero */
    pWndData->hab = WinQueryAnchorBlock (hwnd);
    pWndData->hwndParent = WinQueryWindow (hwnd, QW_PARENT);

    hdc = WinOpenWindowDC (hwnd);
    sl.cx = sl.cy = 0;
    pWndData->hps = GpiCreatePS (pWndData->hab, hdc, &sl, PU_PELS|GPIT_MICRO|GPIA_ASSOC);
    if (NULLHANDLE == pWndData->hps) return MRFROMLONG (TRUE);

    pWndData->fEnabled = TRUE;
    if (!hPointerText)
        hPointerText = WinLoadPointer (HWND_DESKTOP, NULLHANDLE, ID_CARET);

    pWndData->atomClipbrd = WinAddAtom (WinQuerySystemAtomTable (), "RW_Binary");

    return MRFROMLONG (FALSE);
}



/******************************************************
*
*   s_DealWithFocus
*
******************************************************/

static void s_DealWithFocus (HWND hwnd, PWNDDATA pWndData, HWND hFocWin, BOOL fSet)
{
        HWND    hwndOwner;
        BOOL    fInv;

        /* when another window (e.g. a menu) of our app gets the focus, */
        /* the hex window should still look like it has focus */

        if (fSet) {  /* we're getting focus */
                if (!pWndData->fFocus) {
                        fInv = TRUE;
                        pWndData->fFocus = TRUE;
                }
                else
                        fInv = FALSE;
        }
        else {  /* we're losing focus */
                hwndOwner = hlp_RealOwner (hwnd);
                if (hFocWin == hwndOwner || hwndOwner == hlp_RealOwner (hFocWin))
                        fInv = FALSE;
                else {   /* lost focus to another application */
                        fInv = TRUE;
                        pWndData->fFocus = FALSE;
                }
        }

        if (fInv)
                s_InvArea (hwnd, pWndData, pWndData->lOffset, pWndData->lSelEnd);
}




/******************************************************
*
*   s_SetFont
*
******************************************************/

static BOOL s_SetFont (HWND hwnd, PWNDDATA pWndData, LONG lFont)
{
        PFONTMETRICS    pfm;

        pfm = fnt_QueryMetrics (lFont);
        if (NULL == pfm)
                return FALSE;

        GpiDeleteSetId (pWndData->hps, 1);
        if (GPI_ERROR == fnt_CreateFont (pWndData->hps, lFont, 1))
                return FALSE;
        GpiSetCharSet (pWndData->hps, 1);

        pWndData->lFont = lFont;
        pWndData->cxChar = (int) pfm->lAveCharWidth;
        pWndData->cyChar = (int) pfm->lMaxBaselineExt;
        pWndData->cyDesc = (int) pfm->lMaxDescender;

        pWndData->cyLines = pWndData->cy / pWndData->cyChar;
        pWndData->cxCols = (pWndData->cx - (pWndData->cxChar << 1)) / pWndData->cxChar;
        pWndData->cxText = 80 * pWndData->cxChar;

        WinInvalidateRect (hwnd, NULL, FALSE);
        return TRUE;
}


/******************************************************
*
*   s_QuerySomeData
*
******************************************************/

static BOOL s_QuerySomeData (PWNDDATA pWndData, PBYTE pBuff, LONG lLen)
{
        if (pWndData->lOffset + lLen > pWndData->lSize)
                return FALSE;
        memcpy (pBuff, pWndData->pData + pWndData->lOffset, lLen);
        return TRUE;
}


/******************************************************
*
*   s_ClipCopy
*
******************************************************/

static void s_ClipCopy (HWND hwnd, PWNDDATA pWndData)
{
        PBYTE   pData = NULL, pText = NULL;
        ULONG   ulLen, ulStart, ulEnd;
        APIRET  rc;

        if (pWndData->lOffset > pWndData->lSelEnd) {
                ulStart = (ULONG) pWndData->lSelEnd;
                ulEnd = (ULONG) pWndData->lOffset;
        }
        else {
                ulStart = (ULONG) pWndData->lOffset;
                ulEnd = (ULONG) pWndData->lSelEnd;
        }
        ulLen = ulEnd - ulStart + 1;

        rc = DosAllocSharedMem ((PVOID)&pData, NULL, ulLen + sizeof (ULONG),
                            PAG_COMMIT | PAG_READ | PAG_WRITE | OBJ_GIVEABLE);
        if (NO_ERROR != rc) {
#ifdef DEBUG
                dbg_Msg ("Cannot allocate shared mem.\n");
#endif
                WinAlarm (HWND_DESKTOP, WA_WARNING);
                return;
        }
        rc = DosAllocSharedMem ((PVOID)&pText, NULL, 4096, PAG_COMMIT | PAG_READ | PAG_WRITE
                                                        | OBJ_GIVEABLE);

        memcpy (pData + sizeof (ULONG), pWndData->pData + ulStart, ulLen);
        *((PULONG)pData) = ulLen;
        strcpy (pText, "Clipboard contains binary data from RWHexEd!\r\n"
                       "   DWORD   Length\r\n"
                       "   BYTE[]  Data\r\n");

        if (!WinOpenClipbrd (pWndData->hab))
                goto Abort;
        WinEmptyClipbrd (pWndData->hab);
        WinSetClipbrdData (pWndData->hab, (ULONG)(PVOID)pText, CF_DSPTEXT, CFI_POINTER); pText = NULL;
        if (WinSetClipbrdData (pWndData->hab, (ULONG)(PVOID)pData, (ULONG) pWndData->atomClipbrd, CFI_POINTER))
                pData = NULL;

        WinCloseClipbrd (pWndData->hab);
        return;

Abort:
        WinCloseClipbrd (pWndData->hab);
        if (NULL != pData)
                DosFreeMem (pData);
        if (NULL != pText)
                DosFreeMem (pText);
}



/******************************************************
*
*   s_ClipPaste
*
******************************************************/

static void s_ClipPaste (HWND hwnd, PWNDDATA pWndData)
{
    ULONG   ulMaxLen, ulLen, ulPasteLen;
    LONG    lInvFrom;
    PBYTE   pData;

    lInvFrom = min (pWndData->lOffset, pWndData->lSelEnd);
    ulMaxLen = (ULONG) pWndData->lSize - (ULONG) pWndData->lOffset;

    WinOpenClipbrd (pWndData->hab);

    if (pData = (PBYTE)(PVOID)WinQueryClipbrdData (pWndData->hab, pWndData->atomClipbrd)) {
        ulLen = *((PULONG)(PVOID)pData);
        if (ulLen > ulMaxLen) {
            s_InsertEmpty (hwnd, pWndData, ulLen - ulMaxLen, 0);
            s_ScrollSet (hwnd, pWndData);
        }
       /* ulPasteLen = min (ulLen, ulMaxLen); */
        ulPasteLen = ulLen;
        memcpy (pWndData->pData + pWndData->lOffset, pData + sizeof (ULONG), ulPasteLen);
        s_InvArea (hwnd, pWndData, lInvFrom, pWndData->lOffset + (LONG) ulPasteLen);
        pWndData->lOffset += (LONG) ulPasteLen;
        pWndData->lOffset = min (pWndData->lOffset, pWndData->lSize - 1);
        pWndData->lSelEnd = pWndData->lOffset;
        pWndData->fMulSel = FALSE;
        pWndData->fLow = FALSE;
    }
    else if (pData = (PBYTE)(PVOID)WinQueryClipbrdData (pWndData->hab, CF_TEXT)) {
        ulLen = strlen (pData);
        if (ulLen > ulMaxLen) {
            s_InsertEmpty (hwnd, pWndData, ulLen - ulMaxLen, 0);
            s_ScrollSet (hwnd, pWndData);
        }
       /* ulPasteLen = min (ulLen, ulMaxLen); */
        ulPasteLen = ulLen;
        memcpy (pWndData->pData + pWndData->lOffset, pData, ulPasteLen);
        s_InvArea (hwnd, pWndData, lInvFrom, pWndData->lOffset + (LONG) ulPasteLen);
        pWndData->lOffset += (LONG) ulPasteLen;
        pWndData->lOffset = min (pWndData->lOffset, pWndData->lSize - 1);
        pWndData->lSelEnd = pWndData->lOffset;
        pWndData->fMulSel = FALSE;
        pWndData->fLow = FALSE;
    }
    else
        WinAlarm (HWND_DESKTOP, WA_WARNING);

    WinCloseClipbrd (pWndData->hab);
}


/******************************************************
*
*   s_QueryPaste
*
******************************************************/

static BOOL s_QueryPaste (HWND hwnd, PWNDDATA pWndData)
{
    ULONG   ulFmtInfo = 0;
    BOOL    bRet;

    WinOpenClipbrd (pWndData->hab);

    if (WinQueryClipbrdFmtInfo (pWndData->hab, pWndData->atomClipbrd, &ulFmtInfo) ||
        WinQueryClipbrdFmtInfo (pWndData->hab, CF_TEXT, &ulFmtInfo))
        bRet = TRUE;
    else
        bRet = FALSE;

    WinCloseClipbrd (pWndData->hab);
    return bRet;
}




/******************************************************
*
*   s_DeleteData
*
******************************************************/

static BOOL s_DeleteData (HWND hwnd, PWNDDATA pWndData, LONG lDelLen)
{
    register LONG   i, iTop;
    LONG            lNewSize;
    APIRET          rc;

    if (NULL == pWndData->pData) return FALSE;
    lDelLen = min (lDelLen, pWndData->lSize - pWndData->lOffset);
    iTop = pWndData->lSize - lDelLen;
    for (i = pWndData->lOffset; i < iTop; i++)
        pWndData->pData[i] = pWndData->pData[i + lDelLen];
    lNewSize = pWndData->lSize - lDelLen;
    rc = s_SetBufferSize (pWndData->pData, pWndData->lSize, lNewSize);
    if (NO_ERROR != rc) return FALSE;

    pWndData->lSize = lNewSize;
    pWndData->lLines = ((pWndData->lSize + 15) >> 4) + 1;
    if (pWndData->lOffset >= pWndData->lSize)
        pWndData->lOffset = max (0, pWndData->lSize - 1L);
    pWndData->lSelEnd = pWndData->lOffset;

    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SIZE),
                                                  MPFROMLONG (pWndData->lSize));
    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_OFFSET),
                                                  MPFROMLONG (pWndData->lOffset));
    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SELEND),
                                                  MPFROMLONG (pWndData->lSelEnd));


    return TRUE;

}


/******************************************************
*
*   s_DeleteSel
*
******************************************************/

static BOOL s_DeleteSel (HWND hwnd, PWNDDATA pWndData)
{
    register LONG   i, iTop;
    LONG            lNewSize;
    APIRET          rc;
    LONG            lDelStart, lDelEnd, lDelLen;

    if (NULL == pWndData->pData) return FALSE;

    if (pWndData->lSelEnd >= pWndData->lOffset) {
        lDelStart = pWndData->lOffset;
        lDelEnd = pWndData->lSelEnd;
    }
    else {
        lDelStart = pWndData->lSelEnd;
        lDelEnd = pWndData->lOffset;
    }

    lDelLen = lDelEnd - lDelStart + 1;

    iTop = pWndData->lSize - lDelLen;
    for (i = lDelStart; i < iTop; i++)
        pWndData->pData[i] = pWndData->pData[i + lDelLen];
    lNewSize = pWndData->lSize - lDelLen;
    rc = s_SetBufferSize (pWndData->pData, pWndData->lSize, lNewSize);
    if (NO_ERROR != rc) return FALSE;

    pWndData->lSize = lNewSize;
    pWndData->lLines = ((pWndData->lSize + 15) >> 4) + 1;
    if (pWndData->lOffset >= pWndData->lSize)
        pWndData->lOffset = max (0, pWndData->lSize - 1L);
    pWndData->lSelEnd = pWndData->lOffset;

    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SIZE),
                                                  MPFROMLONG (pWndData->lSize));
    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_OFFSET),
                                                  MPFROMLONG (pWndData->lOffset));
    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SELEND),
                                                  MPFROMLONG (pWndData->lSelEnd));


    return TRUE;

}


/******************************************************
*
*   s_InsertEmpty
*
******************************************************/

static BOOL s_InsertEmpty (HWND hwnd, PWNDDATA pWndData, LONG lLen, int iChar)
{
    register LONG   i, iBottom;
    LONG            lNewSize;
    APIRET          rc;

    if (lLen < 1 || pWndData->lSize + lLen > pWndData->lMaxSize) return FALSE;

    lNewSize = pWndData->lSize + lLen;

    rc = s_SetBufferSize (pWndData->pData, pWndData->lSize, lNewSize);
    if (NO_ERROR != rc) return FALSE;
    iBottom = pWndData->lOffset;
    for (i = pWndData->lSize - 1; i >= iBottom; i--)
        pWndData->pData[i+lLen] = pWndData->pData[i];
    for (i = pWndData->lOffset; i < pWndData->lOffset + lLen; i++)
        pWndData->pData[i] = (BYTE) iChar;

    pWndData->lSize = lNewSize;
    pWndData->lLines = ((pWndData->lSize + 15) >> 4) + 1;
    pWndData->lSelEnd = pWndData->lOffset;

    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SIZE),
                                                  MPFROMLONG (pWndData->lSize));
    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SELEND),
                                                  MPFROMLONG (pWndData->lSelEnd));

    return TRUE;
}



/******************************************************
*
*   s_InsertData
*
******************************************************/

static BOOL s_InsertData (HWND hwnd, PWNDDATA pWndData, LONG lLen, PBYTE pBuff)
{
    register LONG   i, iBottom;
    LONG            lNewSize;
    APIRET          rc;

    if (lLen < 1 || pWndData->lSize + lLen > pWndData->lMaxSize) return FALSE;

    lNewSize = pWndData->lSize + lLen;

    rc = s_SetBufferSize (pWndData->pData, pWndData->lSize, lNewSize);
    if (NO_ERROR != rc) return FALSE;
    iBottom = pWndData->lOffset;
    for (i = pWndData->lSize - 1; i >= iBottom; i--)
        pWndData->pData[i+lLen] = pWndData->pData[i];
    for (i = pWndData->lOffset; i < pWndData->lOffset + lLen; i++)
        pWndData->pData[i] = pBuff[i - pWndData->lOffset];

    pWndData->lSize = lNewSize;
    pWndData->lLines = ((pWndData->lSize + 15) >> 4) + 1;
    pWndData->lSelEnd = pWndData->lOffset;

    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SIZE),
                                                  MPFROMLONG (pWndData->lSize));
    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SELEND),
                                                  MPFROMLONG (pWndData->lSelEnd));

    return TRUE;
}





/******************************************************
*
*   s_Bttn1Click
*
******************************************************/

static void s_Bttn1Click (HWND hwnd, PWNDDATA pWndData, POINTS pts)
{
    int     iRet;
    LONG    lOff;
    BOOL    fNewHex;

    if (NULL == pWndData->pData) return;
    lOff = 0;

    iRet = s_OffFromPoint (pWndData, pts, &lOff);
    if (0 == iRet) return;
    if (-1 == iRet)
        fNewHex = TRUE;
    else
        fNewHex = FALSE;

    if (!pWndData->fShift) {
        s_InvNewSel (hwnd, pWndData, lOff, lOff, fNewHex);
        pWndData->lOffset = pWndData->lSelEnd = lOff;
        pWndData->fMulSel = FALSE;
    }
    else {
        s_InvNewSel (hwnd, pWndData, pWndData->lOffset, lOff, fNewHex);
        pWndData->lSelEnd = lOff;
        pWndData->fMulSel = TRUE;
    }

    pWndData->fHex = fNewHex;
    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_OFFSET),
                                                  MPFROMLONG (pWndData->lOffset));
    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SELEND),
                                                  MPFROMLONG (pWndData->lSelEnd));

    WinUpdateWindow (hwnd);
}




/******************************************************
*
*   s_DragSel
*
******************************************************/

static void s_DragSel (HWND hwnd, PWNDDATA pWndData, POINTS pts)
{
    LONG    lSelStart, lSelEnd, lSelNow = 0;

    lSelStart = pWndData->lOffset;
    lSelEnd = pWndData->lSelEnd;

    s_OffFromPoint (pWndData, pts, &lSelNow);

    if (lSelNow == lSelEnd)
        return;

    if (lSelNow < lSelEnd)
        lSelStart = lSelNow;
    else if (lSelNow > lSelEnd) {
        lSelStart = lSelEnd;
        lSelEnd = lSelNow;
    }

    s_InvNewSel (hwnd, pWndData, pWndData->lOffset, lSelNow, pWndData->fHex);
    pWndData->lSelEnd = lSelNow;
    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SELEND),
                                                  MPFROMLONG (pWndData->lSelEnd));
    WinUpdateWindow (hwnd);
}




/******************************************************
*
*   s_EndMotion
*
******************************************************/

static void s_EndMotion (HWND hwnd, PWNDDATA pWndData, POINTS pts)
{
    int     iRet;
    LONG    lOff;

    pWndData->fInMotion = FALSE;
    if (NULL == pWndData->pData) return;



    lOff = 0;
    iRet = s_OffFromPoint (pWndData, pts, &lOff);
    if (0 == iRet) return;

    s_InvNewSel (hwnd, pWndData, pWndData->lOffset, lOff, pWndData->fHex);
    pWndData->lSelEnd = lOff;
    pWndData->fMulSel = TRUE;
    pWndData->fLow = FALSE;
    WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SELEND),
                                                  MPFROMLONG (pWndData->lSelEnd));
}



/******************************************************
*
*   s_VertScroll
*
******************************************************/


static void s_VertScroll (HWND hwnd, PWNDDATA pWndData, int iCmd, int iSlider)
{
    LONG    lInc = 0;
    BOOL    fUpdate = TRUE;

    switch (iCmd) {
      case SB_LINEUP:
        if (pWndData->lPageOffset >= 16)
            lInc = -1;
        break;

      case SB_LINEDOWN:
        lInc = 1;
        break;

      case SB_PAGEUP:
        if ((pWndData->lPageOffset >> 4) >= pWndData->cyLines )
            lInc = -pWndData->cyLines + 1;
        else
            lInc = - (pWndData->lPageOffset >> 4);
        break;

      case SB_PAGEDOWN:
        lInc = pWndData->cyLines - 1;
        break;

      case SB_SLIDERTRACK:
#pragma info (none)
        fUpdate = FALSE;
        /* fall through */
      case SB_SLIDERPOSITION:
        lInc = (LONG) ((double) iSlider *
                       (double) (pWndData->lLines - pWndData->cyLines) /
                       (double) pWndData->sVScrollRes)
                          - (pWndData->lPageOffset>>4);
#pragma info (restore)
        break;

      default:
        return;
    }

    if (0 != pWndData->lSize) {
        lInc = max (-(pWndData->lPageOffset>>4),
               min (lInc, max (0, pWndData->lLines - pWndData->cyLines) - (pWndData->lPageOffset>>4)));

        pWndData->lPageOffset += (lInc<<4);
        WinScrollWindow (hwnd, 0, pWndData->cyChar * lInc,
                        NULL, NULL, NULLHANDLE, NULL, SW_INVALIDATERGN);
        WinUpdateWindow (hwnd);
        if (fUpdate)
            s_ScrollPos (pWndData);
    }

}




/******************************************************
*
*   s_HorzScroll
*
******************************************************/

static void s_HorzScroll (HWND hwnd, PWNDDATA pWndData, int iCmd, int iSlider)
{
    BOOL    fUpdate = TRUE;
    LONG    lInc;

    switch (iCmd) {
      case SB_LINELEFT:
        lInc = - pWndData->cxChar;
        break;

      case SB_LINERIGHT:
        lInc = pWndData->cxChar;
        break;

      case SB_PAGELEFT:
        lInc = - (pWndData->cxChar << 3);
        break;

      case SB_PAGERIGHT:
        lInc = pWndData->cxChar << 3;
        break;

      case SB_SLIDERTRACK:
        fUpdate = FALSE;
        /* fall through */

#pragma info (none)
      case SB_SLIDERPOSITION:
        lInc = iSlider - pWndData->iHScrollPos;
#pragma info (restore)
        break;

      default:
        lInc = 0;
        break;

    }

    lInc = max (-pWndData->iHScrollPos, min (lInc, pWndData->iHScrollMax - pWndData->iHScrollPos));

    if (0 != lInc) {
        pWndData->iHScrollPos += lInc;
        WinScrollWindow (hwnd, -lInc, 0, NULL, NULL, NULLHANDLE, NULL, SW_INVALIDATERGN);
        WinUpdateWindow (hwnd);
        if (fUpdate)
            WinSendMsg (pWndData->hwndHScroll, SBM_SETPOS, MPFROM2SHORT (pWndData->iHScrollPos, 0), 0);
    }

}



/******************************************************
*
*   s_Search
*
******************************************************/

static LONG s_Search (PWNDDATA pWndData, PBYTE pPat, LONG nSize, int iOptions)
{
    PBYTE   p, pMax, pMin;
    LONG    i, nInc;
    BOOL    bMatch = FALSE;

    if (iOptions & SRCH_BACKWARD) {
        pMin = pWndData->pData;
        if (iOptions & SRCH_WHOLEFILE)
            pMax = pWndData->pData + (pWndData->lSize - nSize);
        else
            pMax = min (pWndData->pData + pWndData->lOffset, pWndData->pData + (pWndData->lSize - nSize));
        p = pMax - 1;
        nInc = -1;
    }
    else {
        pMax = pWndData->pData + (pWndData->lSize - nSize);
        if (iOptions & SRCH_WHOLEFILE)
            pMin = pWndData->pData;
        else
            pMin = min (pWndData->pData + pWndData->lOffset + 1, pWndData->pData + pWndData->lSize);
        p = pMin;
        nInc = 1;
    }

    while (p <= pMax && p >= pMin) {
        if (*p == *pPat) {
            for (i = 1; i < nSize; i++) {
                if (p[i] != pPat[i])
                    break;
            }
            if (nSize == i) {
                bMatch = TRUE;
                break;
            }
        }
        p += nInc;
    }
    if (!bMatch)
        return -1;

    return (LONG) (p - pWndData->pData);
}


/******************************************************
*
*   s_ScrollSet
*
******************************************************/

static void s_ScrollSet (HWND hwnd, PWNDDATA pWndData)
{
    SHORT   sSlider;

    if (NULL == pWndData->pData) {
        WinEnableWindow (pWndData->hwndVScroll, FALSE);
        WinEnableWindow (pWndData->hwndHScroll, FALSE);
        return;
    }

    if (0 == pWndData->cyLines || pWndData->lLines <= pWndData->cyLines) {
        if (pWndData->lLines < pWndData->cyLines - 1) {
            pWndData->lPageOffset = 0;
            WinInvalidateRect (hwnd, NULLHANDLE, FALSE);
        }
        WinEnableWindow (pWndData->hwndVScroll, FALSE);
    }
    else {
        WinEnableWindow (pWndData->hwndVScroll, TRUE);
        pWndData->sVScrollRes = (SHORT) min (0x7FFF, pWndData->lLines - 1);
        pWndData->dVScrollScale = (double) pWndData->sVScrollRes / (double) (pWndData->lLines - 1);

        sSlider = (SHORT) s_VertSlider (pWndData, pWndData->lPageOffset);
        WinSendMsg (pWndData->hwndVScroll, SBM_SETSCROLLBAR,
                    MPFROM2SHORT (sSlider, 0),
                    MPFROM2SHORT (0, pWndData->sVScrollRes));
        WinSendMsg (pWndData->hwndVScroll, SBM_SETTHUMBSIZE,
                    MPFROM2SHORT (pWndData->cyLines, pWndData->sVScrollRes), NULL);

    }

    pWndData->iHScrollMax = max (0, pWndData->cxText - pWndData->cx);
    pWndData->iHScrollPos = min (pWndData->iHScrollPos, pWndData->iHScrollMax);
    if (0 == pWndData->iHScrollMax) {
        pWndData->iHScrollPos = 0;
        WinInvalidateRect (hwnd, NULLHANDLE, FALSE);
        WinEnableWindow (pWndData->hwndHScroll, FALSE);
    }
    else {
        WinEnableWindow (pWndData->hwndHScroll, TRUE);
        WinSendMsg (pWndData->hwndHScroll, SBM_SETSCROLLBAR,
                    MPFROM2SHORT (pWndData->iHScrollPos, 0),
                    MPFROM2SHORT (0, pWndData->iHScrollMax));
        WinSendMsg (pWndData->hwndHScroll, SBM_SETTHUMBSIZE,
                    MPFROM2SHORT (pWndData->cx, pWndData->cxText), 0);
    }

}


/******************************************************
*
*   s_VertSlider
*
******************************************************/

static int s_VertSlider (PWNDDATA pWndData, LONG lOff)
{
    double  dPag, dRes, dLin;

    dPag = (double) (pWndData->lPageOffset >> 4);
    dRes = (double) pWndData->sVScrollRes;
    dLin = (double) (pWndData->lLines - pWndData->cyLines);
    return min ((int) (dPag * dRes / dLin), (int) pWndData->sVScrollRes);

}




/******************************************************
*
*   s_ScrollPos
*
******************************************************/

static void s_ScrollPos (PWNDDATA pWndData)
{
    SHORT   sSlider;

    if (NULL == pWndData->pData) return;
    if (pWndData->lLines <= pWndData->cyLines) return;

    sSlider = (SHORT) s_VertSlider (pWndData, pWndData->lPageOffset);
    WinSendMsg (pWndData->hwndVScroll, SBM_SETPOS, MPFROMSHORT (sSlider), NULL);
}



/******************************************************
*
*   s_Paint
*
******************************************************/

static void s_Paint (HWND hwnd, PWNDDATA pWndData)
{
    int     i, j, k, iBegin, iEnd;
    char    szBuff[100], szBuff2[20];
    HPS     hps;
    POINTL  ptl;
    PBYTE   pBase, p, pMax;
    RECTL   rectl, rectlText;
    LONG    lCurrBack;
    PHECOLORS   phec;
    LONG    lSelStart, lSelEnd, lPStart, lPEnd;

    switch (pWndData->iColorTable) {
      case 1: phec = &hecSystem; break;
      case 2: phec = &pWndData->hecColors; break;
      default: phec = &hecDefault; break;
    }

    memset (&rectl, 0, sizeof rectl);
    hps = WinBeginPaint (hwnd, pWndData->hps, &rectl);
    GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL); /* set to RGB mode */
    WinFillRect (hps, &rectl, phec->lrgbTextBack); /* paint background */


    if (pWndData->pData) {
        pBase = pWndData->pData;
        pMax = pBase + pWndData->lSize - 1;
        if (pWndData->lOffset <= pWndData->lSelEnd) {
            lSelStart = pWndData->lOffset;
            lSelEnd = pWndData->lSelEnd;
        }
        else {
            lSelStart = pWndData->lSelEnd;
            lSelEnd = pWndData->lOffset;
        }

        ptl.x = pWndData->cxChar - pWndData->iHScrollPos;

        iBegin = (pWndData->cy - rectl.yTop) / pWndData->cyChar;
        iEnd = (pWndData->cy - rectl.yBottom) / pWndData->cyChar + 1;

        /* print unslected data */
        GpiSetColor (hps, phec->lrgbTextFore);
        for (i = iBegin; i < iEnd; i++) {
            p = pBase + pWndData->lPageOffset + (i << 4);
            *szBuff = *szBuff2 = 0;
            if (p > pMax)
                break;

            sprintf (szBuff, "%08lX: ", (LONG) (p - pBase));
            if (lSelStart >= (p - pBase) || lSelEnd <= (p - pBase + 15)) {
                for (j = 0; j < 4; j++) {
                    for (k = 0; k < 4; k++) {
                        if (p <= pMax) {
                            sprintf (szBuff2, "%02X ", (int)*p);
                            strcat (szBuff, szBuff2);
                        }
                        else
                            strcat (szBuff, "   ");
                        p++;
                    }
                    strcat (szBuff, " ");
                }
                p -= 16;

                for (j = 0; j < 16; j++) {
                    if (p <= pMax) {
                        if (*p > 0 && *p < 255)
                            szBuff2[j] = *p;
                        else
                            szBuff2[j] = '.';
                    }
                    else
                       szBuff2[j] = 0;
                    p++;

                }
                szBuff2[16] = 0;
                strcat (szBuff, szBuff2);
            }

            ptl.y = pWndData->cy - pWndData->cyChar * (i+1) +
                    pWndData->cyDesc;
            GpiCharStringAt (hps, &ptl, (LONG) strlen (szBuff), szBuff);
        }

        /* print selected data */
        for (i = iBegin; i < iEnd; i++) {
            p = pBase + pWndData->lPageOffset + (i << 4);
            if (lSelStart > (p - pBase + 15) || lSelEnd < (p - pBase))
                continue; /* dont print completely unselected lines */
            *szBuff = *szBuff2 = 0;
            if (p > pMax)
                break;

            for (j = 0; j < 4; j++) {
                for (k = 0; k < 4; k++) {
                    if (p <= pMax) {
                        sprintf (szBuff2, "%02X ", (int)*p);
                        strcat (szBuff, szBuff2);
                    }
                    else
                        strcat (szBuff, "   ");
                    p++;
                }
                strcat (szBuff, " ");
            }
            p -= 16;

            while (*szBuff && szBuff[strlen (szBuff) - 1] == ' ')
                szBuff[strlen (szBuff) - 1] = 0;

            ptl.x = pWndData->cxChar - pWndData->iHScrollPos;

            if (lSelStart > p - pBase)    /* line not selected from start */
                lPStart = lSelStart - (p - pBase);
            else
                lPStart = 0;

            if (lSelEnd < (p - pBase + 15)) /* line not selected to end */
                lPEnd = lSelEnd - (p - pBase);
            else
                lPEnd = 15;

            if (lPEnd < 15) /* cut off unselected from String */
                szBuff[s_PosFromString (lPEnd) + 2] = 0;
            if (lPStart > 0) /* skip unselected */
                strcpy (szBuff, szBuff + s_PosFromString (lPStart));

            ptl.y = pWndData->cy - pWndData->cyChar * (i+1) +
                    pWndData->cyDesc;

            ptl.x += pWndData->cxChar * (10 + s_PosFromString (lPStart));
            rectlText.xLeft = ptl.x;
            rectlText.xRight = ptl.x + (LONG) strlen (szBuff) * pWndData->cxChar;
            rectlText.yBottom = ptl.y - pWndData->cyDesc;
            rectlText.yTop = rectlText.yBottom + pWndData->cyChar;

            if (pWndData->fHex && pWndData->fFocus) {
                GpiSetColor (hps, phec->lrgbCursorFore);
                lCurrBack = phec->lrgbCursorBack;
                if (pWndData->lOffset == pWndData->lSelEnd) {
                    if (pWndData->fLow) {
                        szBuff[0] = szBuff[1];
                        szBuff[1] = 0;
                        rectlText.xLeft += pWndData->cxChar;
                        rectlText.xRight = rectlText.xLeft + pWndData->cxChar;
                        ptl.x += pWndData->cxChar;
                    }
                    else {
                        szBuff[1] = 0;
                        rectlText.xRight = rectlText.xLeft + pWndData->cxChar;
                    }
                }

            }
            else {
                GpiSetColor (hps, phec->lrgbHiliteFore);
                lCurrBack = phec->lrgbHiliteBack;
            }

            WinFillRect (hps, &rectlText, lCurrBack);
            GpiCharStringAt (hps, &ptl, (LONG) strlen (szBuff), szBuff);



            for (j = 0; j < 16; j++) {
                if (p <= pMax) {
                    if (*p > 0 && *p < 255)
                        szBuff[j] = *p;
                    else
                        szBuff[j] = '.';
                }
                else
                   szBuff[j] = 0;
                p++;

            }
            ptl.x = 63 * pWndData->cxChar - pWndData->iHScrollPos;

            szBuff[lPEnd + 1] = 0;
            if (lPStart > 0) /* skip unselected */
                strcpy (szBuff, szBuff + lPStart);

            if (pWndData->fHex || !pWndData->fFocus) {
                GpiSetColor (hps, phec->lrgbHiliteFore);
                lCurrBack = phec->lrgbHiliteBack;
            }
            else {
                GpiSetColor (hps, phec->lrgbCursorFore);
                lCurrBack = phec->lrgbCursorBack;
            }

            ptl.x += lPStart * pWndData->cxChar;
            rectlText.xLeft = ptl.x;
            rectlText.xRight = ptl.x + (LONG) strlen (szBuff) * pWndData->cxChar;
            WinFillRect (hps, &rectlText, lCurrBack);

            GpiCharStringAt (hps, &ptl, (LONG) strlen (szBuff), szBuff);

        }
        GpiDeleteSetId (hps, 1);
    }

    WinEndPaint (hps);
}


/******************************************************
*
*   s_PosFromString
*
******************************************************/

static int s_PosFromString (int iStrOff)
{
    int     aiPos[] = {0, 3, 6, 9, 13, 16, 19, 22, 26, 29, 32, 35, 39, 42, 45, 48, 50};

    if (iStrOff < 0 || iStrOff >= (sizeof aiPos / sizeof aiPos[0]))
        return 0;

    return aiPos[iStrOff];
}




/******************************************************
*
*   s_Visible
*
******************************************************/

static int s_Visible (PWNDDATA pWndData, LONG lOff)
{
    int     nRet = 0;
    LONG    lHorzOff;

    if (NULL == pWndData->pData) return 0;
    if (lOff < pWndData->lPageOffset) return 0;
    if (lOff > pWndData->lPageOffset + (pWndData->lLines<<4) + 16) return 0;

    lHorzOff = s_HorzOffHex (pWndData, lOff);
    if (lHorzOff >= 0L && lHorzOff < pWndData->cx)
        nRet |= 1;

    lHorzOff = s_HorzOffChar (pWndData, lOff);
    if (lHorzOff >= 0L && lHorzOff < pWndData->cx)
        nRet |= 2;

    return nRet;
}



/******************************************************
*
*   s_HorzOffHex
*
******************************************************/

static int s_HorzOffHex (PWNDDATA pWndData, LONG lOff)
{
    int     nOff, nPos;

    nPos = lOff - ((lOff >> 4) << 4);

    nOff = 11 * pWndData->cxChar;
    nOff += nPos * (pWndData->cxChar * 3);
    nOff += (nPos / 4) * pWndData->cxChar;

    nOff -= pWndData->iHScrollPos;

    return nOff;
}



/******************************************************
*
*   s_HorzOffChar
*
******************************************************/

static int s_HorzOffChar (PWNDDATA pWndData, LONG lOff)
{
    int     nOff, nPos;

    nPos = lOff - ((lOff >> 4) << 4);

    nOff = (63 + nPos ) * pWndData->cxChar;
    nOff -= pWndData->iHScrollPos;

    return nOff;
}



/******************************************************
*
*   s_VertOff
*
******************************************************/

static int s_VertOff (PWNDDATA pWndData, LONG lOff)
{
    int     nOff;
    LONG    lPos;

    lPos = (lOff - pWndData->lPageOffset) >> 4;

    nOff = pWndData->cy - pWndData->cyChar * (lPos+1) + pWndData->cyDesc;

    return nOff;
}



/******************************************************
*
*   s_OffFromPoint
*
******************************************************/

static LONG s_OffFromPoint (PWNDDATA pWndData, POINTS pts, PLONG plPos)
{
    int     row, column, iRet, frac, rem;
    LONG    lPos;


    pts.x = (SHORT) (pts.x + pWndData->iHScrollPos - pWndData->cxChar);
    column = (int) (pts.x << 1) / pWndData->cxChar;
    column -= 18;

    pts.y = (SHORT) (pWndData->cy - (int) pts.y);
    row = pts.y / pWndData->cyChar;

    lPos = pWndData->lPageOffset + (row << 4);

    if (column < 4 * 26) {        /* hex */
        frac = column / 26;
        rem = column % 26 - 2;
        if (rem < 0) rem = 0;
        if (rem > 23) rem = 23;

        column = frac * 8 + rem / 3;

        if (column & 1)
            pWndData->fLow = TRUE;
        else
            pWndData->fLow = FALSE;

        column >>= 1;
        lPos += column;
        iRet = -1;
    }
    else {  /* char */
        column -= (4 * 26 + 2);
        if (column < 0) column = 0;
        if (column > 31) column = 31;
        column >>= 1;
        lPos += column;
        iRet = 1;
    }

    if (lPos >= pWndData->lSize)
        lPos = max (0, pWndData->lSize - 1);
    if (lPos < 0)
        lPos = 0;

    *plPos = lPos;

    return iRet;
}



/******************************************************
*
*   s_InvLine
*
******************************************************/

static void s_InvLine (HWND hwnd, PWNDDATA pWndData, LONG lOff)
{
    RECTL   rectl;

    rectl.xLeft = s_HorzOffHex (pWndData, lOff);
    rectl.xRight = rectl.xLeft + (pWndData->cxChar << 1);
    rectl.yBottom = s_VertOff (pWndData, lOff) - pWndData->cyDesc;
    rectl.yTop = rectl.yBottom + pWndData->cyChar;
    WinInvalidateRect (hwnd, &rectl, FALSE);

    rectl.xLeft = s_HorzOffChar (pWndData, lOff);
    rectl.xRight = rectl.xLeft + pWndData->cxChar;
    WinInvalidateRect (hwnd, &rectl, FALSE);
}


/******************************************************
*
*   s_InvArea
*
******************************************************/

static void s_InvArea (HWND hwnd, PWNDDATA pWndData, LONG lOffStart, LONG lOffEnd)
{
    RECTL   rectl;
    LONG    l, lLeft, lRight;

    if (lOffStart > lOffEnd) {
        l = lOffStart;
        lOffStart = lOffEnd;
        lOffEnd = l;
    }

    if ((lOffStart & 0xFFFFFFF0) < (lOffEnd & 0xFFFFFFF0)) {
        lLeft = 0;
        lRight = 15;
    }
    else {
        lLeft = min (lOffStart & 0x0000000F, lOffEnd & 0x0000000F);
        lRight = max (lOffStart & 0x0000000F, lOffEnd & 0x0000000F);
    }

    rectl.xLeft = s_HorzOffHex (pWndData, lLeft);
    rectl.xRight = s_HorzOffHex (pWndData, lRight) + (pWndData->cxChar << 1);
    rectl.yBottom = s_VertOff (pWndData, lOffEnd) - pWndData->cyDesc;
    rectl.yTop = s_VertOff (pWndData, lOffStart) + pWndData->cyChar;
    WinInvalidateRect (hwnd, &rectl, FALSE);

    rectl.xLeft = s_HorzOffChar (pWndData, lLeft);
    rectl.xRight = s_HorzOffChar (pWndData, lRight) + pWndData->cxChar;
    WinInvalidateRect (hwnd, &rectl, FALSE);
}





/******************************************************
*
*   s_InvNewSel
*
******************************************************/

static void s_InvNewSel (HWND hwnd, PWNDDATA pWndData, LONG lNewOff, LONG lNewEnd, BOOL fNewHex)
{
    LONG    l, lOffset, lSelEnd;

    if (lNewOff > lNewEnd) {
        l = lNewOff;
        lNewOff = lNewEnd;
        lNewEnd = l;
    }

    if (pWndData->lOffset > pWndData->lSelEnd) {
        lOffset = pWndData->lSelEnd;
        lSelEnd = pWndData->lOffset;
    }
    else {
        lOffset = pWndData->lOffset;
        lSelEnd = pWndData->lSelEnd;
    }

    if (fNewHex != pWndData->fHex) {
        s_InvArea (hwnd, pWndData, lOffset, lSelEnd);
        s_InvArea (hwnd, pWndData, lNewOff, lNewEnd);
        return;
    }

    if (pWndData->fHex && lNewOff == lNewEnd && lNewOff == lOffset && lOffset == lSelEnd) {
        s_InvArea (hwnd, pWndData, lNewOff, lNewOff);
        return;
    }

    if (lNewOff > lSelEnd) {    /* old and new sel don't overlap */
        s_InvArea (hwnd, pWndData, lOffset, lSelEnd);
        s_InvArea (hwnd, pWndData, lNewOff, lNewEnd);
    }
    else {  /* they overlap */
        if (lNewOff > lOffset)
            s_InvArea (hwnd, pWndData, lOffset, lNewOff);
        else if (lNewOff < lOffset)
            s_InvArea (hwnd, pWndData, lNewOff, lOffset);
        if (lNewEnd > lSelEnd)
            s_InvArea (hwnd, pWndData, lSelEnd, lNewEnd);
        else if (lNewEnd < lSelEnd)
            s_InvArea (hwnd, pWndData, lNewEnd, lSelEnd);
    }
}





/******************************************************
*
*   s_CharProc
*
******************************************************/

static BOOL s_CharProc (HWND hwnd, PWNDDATA pWndData, ULONG mp1, ULONG mp2)
{
    int     fs, vkey, chr;
/*    int     scancode, cRepeat; */
    BOOL    fInvLine = FALSE, fScroll = FALSE, fProcessed = FALSE;
    LONG    lOldOff = 0, lOldSelEnd = 0;
    LONG    lNewOff, lNewEnd, l, l1, l2;
    BOOL    bDWP = FALSE, fNewHex;

    if (NULL != pWndData->pData) {
        lOldOff = pWndData->lOffset;
        lOldSelEnd = pWndData->lSelEnd;
        lNewOff = pWndData->lOffset;
        lNewEnd = pWndData->lSelEnd;
    }
    fNewHex = pWndData->fHex;

/*    scancode = (int) ((mp1 & 0xFF000000) >> 24); */
/*    cRepeat = (int) ((mp1 & 0x00FF0000) >> 16); */
    fs = (int) (mp1 & 0x0000FFFF);
    vkey = (int) ((mp2 & 0xFFFF0000) >> 16);
    chr = (int) (mp2 & 0x0000FFFF);

    if (KC_INVALIDCHAR & fs ) return TRUE;

    if (KC_KEYUP & fs) {
        if (!(KC_VIRTUALKEY & fs))
            return TRUE;
        switch (vkey) {
          case VK_ALT:
            pWndData->fAlt = FALSE;
            break;

          case VK_SHIFT:
            pWndData->fShift = FALSE;
            break;

          case VK_CTRL:
            pWndData->fCtrl = FALSE;
            break;
        }
        return TRUE;
    }

    if (KC_VIRTUALKEY & fs && pWndData->pData) {
        fProcessed = TRUE;
        switch (vkey) {
          case VK_ALT:
            if (!(KC_PREVDOWN & fs))
                pWndData->fAlt = TRUE;
            bDWP = TRUE;
            break;

          case VK_SHIFT:
            if (!(KC_PREVDOWN & fs))
                pWndData->fShift = TRUE;
            bDWP = TRUE;
            break;

          case VK_CTRL:
            if (!(KC_PREVDOWN & fs))
                pWndData->fCtrl = TRUE;
            bDWP = TRUE;
            break;

          case VK_ESC:
            if (!(KC_ALT & fs)) {
                if (pWndData->fMulSel) {
                    lNewEnd = lNewOff;
                    pWndData->fMulSel = FALSE;
                    fInvLine = TRUE;
                }
            }
            break;

          case VK_TAB:
            if (!(KC_ALT & fs)) {
                if (FALSE == pWndData->fHex) {
                    fNewHex = TRUE;
                    pWndData->fLow = FALSE;
                }
                else
                    fNewHex = FALSE;
                fInvLine = TRUE;
            }
            break;

          case VK_LEFT:
            if (!(KC_ALT & fs)) {
                if (KC_CHAR & fs) {
                    fProcessed = FALSE;
                    break;
                }
                if (!(KC_SHIFT & fs)) {
                    if (pWndData->fHex && pWndData->fLow)
                        pWndData->fLow = FALSE;
                    else {
                        if (lNewOff > 0) {
                            lNewOff--;
                            lNewEnd = lNewOff;
                            pWndData->fMulSel = FALSE;
                            pWndData->fLow = TRUE;
                        }
                    }
                    s_IntoView (hwnd, pWndData, lNewOff, -1, mp1, mp2);
                }
                else {  /* shift is pressed */
                    if (pWndData->fHex)
                        pWndData->fLow = FALSE;
                    if (lNewEnd > 0) {
                        lNewEnd--;
                        pWndData->fMulSel = TRUE;
                    }
                    s_IntoView (hwnd, pWndData, lNewEnd, -1, mp1, mp2);
                }
                fInvLine = TRUE;
            }
            break;

          case VK_RIGHT:
            if (!(KC_ALT & fs)) {
                if (KC_CHAR & fs) {
                    fProcessed = FALSE;
                    break;
                }
                if (!(KC_SHIFT & fs)) {
                    if (pWndData->fHex && !pWndData->fLow)
                        pWndData->fLow = TRUE;
                    else {
                        if (lNewOff < (pWndData->lSize - 1)) {
                            lNewOff++;
                            lNewEnd = lNewOff;
                            pWndData->fLow = FALSE;
                            pWndData->fMulSel = FALSE;
                        }
                    }
                    s_IntoView (hwnd, pWndData, lNewOff, 1, mp1, mp2);
                }
                else {  /* shift is pressed */
                    if (pWndData->fHex)
                        pWndData->fLow = FALSE;
                    if (lNewEnd < (pWndData->lSize - 1)) {
                        lNewEnd++;
                        pWndData->fMulSel = TRUE;
                    }
                    s_IntoView (hwnd, pWndData, lNewEnd, 1, mp1, mp2);
                }
                fInvLine = TRUE;
            }
            break;


          case VK_UP:
            if (!(KC_ALT & fs)) {
                if (KC_CHAR & fs) {
                    fProcessed = FALSE;
                    break;
                }
                if (!(KC_SHIFT & fs)) {
                    if (lNewOff > 15) {
                        lNewOff -= 16;
                        lNewEnd = lNewOff;
                        pWndData->fMulSel = FALSE;
                        s_IntoView (hwnd, pWndData, lNewOff, -1, mp1, mp2);
                    }
                }
                else { /* shift */
                    if (lNewEnd > 15) {
                        lNewEnd -= 16;
                        pWndData->fMulSel = TRUE;
                        s_IntoView (hwnd, pWndData, lNewEnd, -1, mp1, mp2);
                    }
                }
                fInvLine = TRUE;
            }
            break;

          case VK_DOWN:
            if (!(KC_ALT & fs)) {
                if (KC_CHAR & fs) {
                    fProcessed = FALSE;
                    break;
                }
                if (!(KC_SHIFT & fs)) {
                    if (lNewOff < (pWndData->lSize - 16)) {
                        lNewOff += 16;
                        lNewEnd = lNewOff;
                        pWndData->fMulSel = FALSE;
                        s_IntoView (hwnd, pWndData, lNewOff, 1, mp1, mp2);
                    }
                }
                else { /* shift */
                    if (lNewEnd < (pWndData->lSize - 16)) {
                        lNewEnd += 16;
                        pWndData->fMulSel = TRUE;
                        s_IntoView (hwnd, pWndData, lNewEnd, 1, mp1, mp2);
                    }
                }
                fInvLine = TRUE;
            }
            break;

          case VK_PAGEUP:
            if (!(KC_ALT & fs)) {
                if (KC_CHAR & fs) {
                    fProcessed = FALSE;
                    break;
                }
                if (!(KC_SHIFT & fs))
                    WinSendMsg (pWndData->hwndVScroll, WM_CHAR, MPFROMLONG (mp1), MPFROMLONG (mp2));
                else {
                    lNewEnd -= (pWndData->cyLines << 4);
                    lNewEnd = max (lNewEnd, 0);
                    pWndData->fMulSel = TRUE;
                    s_IntoView (hwnd, pWndData, lNewEnd, -1, mp1, mp2);
                    fInvLine = TRUE;
                }
            }
            break;

          case VK_PAGEDOWN:
            if (!(KC_ALT & fs)) {
                if (KC_CHAR & fs) {
                    fProcessed = FALSE;
                    break;
                }
                if (!(KC_SHIFT & fs))
                    WinSendMsg (pWndData->hwndVScroll, WM_CHAR, MPFROMLONG (mp1), MPFROMLONG (mp2));
                else {
                    lNewEnd += (pWndData->cyLines << 4);
                    lNewEnd = min (lNewEnd, pWndData->lSize - 1);
                    pWndData->fMulSel = TRUE;
                    s_IntoView (hwnd, pWndData, lNewEnd, 1, mp1, mp2);
                    fInvLine = TRUE;
                }
            }
            break;

          case VK_HOME:
            if (!(KC_ALT & fs)) {
                if (KC_CHAR & fs) {
                    fProcessed = FALSE;
                    break;
                }
                if (!(KC_SHIFT & fs)) {
                    lNewOff = lNewEnd = 0;
                    pWndData->fMulSel = FALSE;
                    fInvLine = TRUE;
                    fScroll = TRUE;
                    s_IntoView (hwnd, pWndData, lNewOff, -1, mp1, mp2);
                }
                else {
                    lNewEnd = 0;
                    pWndData->fMulSel = TRUE;
                    fInvLine = TRUE;
                    s_IntoView (hwnd, pWndData, lNewEnd, -1, mp1, mp2);
                }
            }
            break;

          case VK_END:
            if (!(KC_ALT & fs)) {
                if (KC_CHAR & fs) {
                    fProcessed = FALSE;
                    break;
                }
                if (pWndData->lSize > 0) {
                    if (!(KC_SHIFT & fs)) {
                        lNewOff = lNewEnd = pWndData->lSize - 1;
                        pWndData->fMulSel = FALSE;
                        fInvLine = TRUE;
                        fScroll = TRUE;
                        s_IntoView (hwnd, pWndData, lNewOff, 1, mp1, mp2);
                    }
                    else {
                        lNewEnd = pWndData->lSize - 1;
                        pWndData->fMulSel = TRUE;
                        fInvLine = TRUE;
                        s_IntoView (hwnd, pWndData, lNewEnd, 1, mp1, mp2);
                    }
                }
            }
            break;

          case VK_NEWLINE:
            if (!(KC_ALT & fs)) {
                fProcessed = FALSE;
                chr = '\n';
            }
            break;

          case VK_ENTER:
            if (!(KC_ALT & fs)) {
                fProcessed = FALSE;
                chr = '\r';
            }
            break;

          default:
            fProcessed = FALSE;
            break;

        }
    } /* endif VIRTUALKEY  */
    if (!fProcessed && KC_CHAR & fs) {
        if (pWndData->pData && pWndData->lSize) {
            fInvLine = TRUE;
            if (FALSE == pWndData->fHex) {
                if (lNewOff == lNewEnd) {
                    pWndData->pData[lNewOff] = (BYTE) chr;
                    if (!(KC_DEADKEY & fs)) {
                        if (lNewOff < pWndData->lSize -1)
                            lNewOff++;
                    }
                    lNewEnd = lNewOff;
                    pWndData->fMulSel = FALSE;
                }
                else {
                    if (lNewOff > lNewEnd) {
                        l1 = lNewEnd; l2 = lNewOff;
                    }
                    else {
                        l1 = lNewOff; l2 = lNewEnd;
                    }
                    for (l = l1; l <= l2; l++)
                        pWndData->pData[l] = (BYTE) chr;
                    s_InvArea (hwnd, pWndData, lNewOff, lNewEnd);
                }
                s_IntoView (hwnd, pWndData, lNewOff, 1, mp1, mp2);
            }
            else {
                if (('0' <= chr && chr <= '9') ||
                    ('a' <= chr && chr <= 'f') ||
                    ('A' <= chr && chr <= 'F')) {
                    if (chr > 'F')
                        chr = chr - 'a' + 10;
                    else if (chr > '9')
                        chr = chr - 'A' + 10;
                    else
                        chr = chr - '0';
                    if (lNewOff == lNewEnd) {
                        if (TRUE == pWndData->fLow) {
                            pWndData->pData[lNewOff] = (BYTE) ((pWndData->pData[lNewOff] & 0xF0) | chr);
                            if (lNewOff < pWndData->lSize -1)
                                lNewOff++;
                            lNewEnd = lNewOff;
                            pWndData->fLow = FALSE;
                        }
                        else {
                            pWndData->pData[lNewOff] = (BYTE) ((pWndData->pData[lNewOff] & 0x0F) | (chr << 4));
                            pWndData->fLow = TRUE;
                        }
                        s_IntoView (hwnd, pWndData, lNewOff, 1, mp1, mp2);
                    }
                    else {
                        if (lNewOff > lNewEnd) {
                            l1 = lNewEnd; l2 = lNewOff;
                        }
                        else {
                            l1 = lNewOff; l2 = lNewEnd;
                        }
                        if (pWndData->fLow) {
                            for (l = l1; l <= l2; l++)
                                pWndData->pData[l] = (BYTE) ((pWndData->pData[l] & 0xF0) | chr);
                        }
                        else {
                            for (l = l1; l <= l2; l++)
                                pWndData->pData[l] = (BYTE) ((pWndData->pData[l] & 0x0F) | (chr << 4));
                        }
                        pWndData->fLow = (BOOL) !pWndData->fLow;
                        s_InvArea (hwnd, pWndData, lNewOff, lNewEnd);
                    }
                }
                else
                    fInvLine = FALSE;
            }
        }
    }


    if (fInvLine) {
        s_InvNewSel (hwnd, pWndData, lNewOff, lNewEnd, fNewHex);
        s_InvLine (hwnd, pWndData, lNewOff);
    }
    pWndData->lOffset = lNewOff;
    pWndData->lSelEnd = lNewEnd;
    pWndData->fHex = fNewHex;

    if (fScroll)
        s_ScrollPos (pWndData);

    if (pWndData->pData) {
        if (lOldOff != pWndData->lOffset)
            WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_OFFSET),
                                                  MPFROMLONG (pWndData->lOffset));
        if (lOldSelEnd != pWndData->lSelEnd)
            WinPostMsg (pWndData->hwndParent, WM_CONTROL, MPFROM2SHORT (pWndData->usId, HEN_SELEND),
                                                  MPFROMLONG (pWndData->lSelEnd));
    }
    bDWP = (BOOL) !fProcessed;
    return bDWP;
}


/******************************************************
*
*   s_IntoView
*
******************************************************/

static void s_IntoView (HWND hwnd, PWNDDATA pWndData, LONG lNewPos, int iDir, ULONG mp1, ULONG mp2)
{
   /* iDir: -1 = going left/up; 1 = going right/down */

    BOOL    fScroll = TRUE;

    if (NULL == pWndData->pData) return;
    if (lNewPos >= pWndData->lPageOffset &&
        lNewPos < pWndData->lPageOffset + (pWndData->cyLines << 4))
        return; /* already visible */

    pWndData->lPageOffset = (lNewPos & (LONG)0xFFFFFFF0) - ((pWndData->cyLines >> 1) << 4);
    pWndData->lPageOffset = min (max (0, pWndData->lPageOffset),
                                (0xFFFFFFF0 & (pWndData->lSize - 1))
                                 - ((pWndData->cyLines - 1) << 4)
                                 );
    WinInvalidateRect (hwnd, NULL, FALSE);

    if (TRUE == fScroll)
        s_ScrollPos (pWndData);
}



/******************************************************
*
*   s_SetBufferSize
*
******************************************************/

static APIRET s_SetBufferSize (PBYTE pBuffer, LONG lOldSize, LONG lNewSize)
{
    APIRET  rc;
    ULONG   ulOldAlloc, ulNewAlloc;

    ulOldAlloc = 0xFFFFF000 & ((ULONG)lOldSize + 0x00000FFF);
    ulNewAlloc = 0xFFFFF000 & ((ULONG)lNewSize + 0x00000FFF);

    if (ulNewAlloc < ulOldAlloc) /* shrink */
        rc = DosSetMem (pBuffer + ulNewAlloc, ulOldAlloc - ulNewAlloc, PAG_DECOMMIT);
    else if (ulNewAlloc > ulOldAlloc) /* grow  */
        rc = DosSetMem (pBuffer + ulOldAlloc, ulNewAlloc - ulOldAlloc, PAG_COMMIT|PAG_DEFAULT);
    else
        rc = NO_ERROR;

    return rc;
}


/******************************************************
*
*   s_IsOffSelected
*
******************************************************/

static BOOL s_IsOffSelected (LONG lOff, PWNDDATA pWndData)
{
    if (pWndData->lSelEnd >= pWndData->lOffset) {
        if (lOff >= pWndData->lOffset && lOff <= pWndData->lSelEnd)
            return TRUE;
        else
            return FALSE;
    }
    if (lOff <= pWndData->lOffset && lOff >= pWndData->lSelEnd)
        return TRUE;
    return FALSE;
}









