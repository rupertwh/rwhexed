/* Original file date: Sep-13-1995 */

/******************************************************************************
*                                                                             *
*   askval.c                                                                    *
*                                                                             *
*                                                                             *
******************************************************************************/




#define INCL_WIN

#include <os2.h>
#include <stdlib.h>

#include "res.h"
#include "askval.h"
#include "helpers.h"
#include "memmgr.h"
#include "globals.h"

typedef struct {
    int     cbSize;
    HWND    hwndParent;
    LONG    lVal;
    PSZ     pszTitle;
    PSZ     pszPre;     /* text in front of entry */
    PSZ     pszAfter;   /* text after entry */
} ASKVALCTL;
typedef ASKVALCTL* PASKVALCTL;


MRESULT APIENTRY AskValDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2);


BOOL av_QueryLong (HWND hwndParent, PLONG plVal, PSZ pszTitle, PSZ pszPre, PSZ pszAfter)
{
    PASKVALCTL  pCtl;
    BOOL        bRet = FALSE;

    pCtl = mem_HeapAlloc (sizeof (ASKVALCTL));

    pCtl->cbSize = (int) sizeof (ASKVALCTL);
    pCtl->hwndParent = hwndParent;
    pCtl->lVal = *plVal;
    pCtl->pszTitle = pszTitle;
    pCtl->pszPre = pszPre;
    pCtl->pszAfter = pszAfter;

    if (WinDlgBox (HWND_DESKTOP, hwndParent, AskValDlgProc, g.hResource, IDD_ASKVAL, pCtl)) {
        *plVal = pCtl->lVal;
        bRet = TRUE;
    }
    mem_HeapFree (pCtl);

    return bRet;
}


MRESULT APIENTRY AskValDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bDDP = FALSE;
    MRESULT mRet = (MRESULT) 0;
    char    szBuff[20];
    PASKVALCTL  pCtl;

    switch (ulMsg)
    {
      case WM_INITDLG:
        hlp_DlgNicePos (hwnd);
        pCtl = (PASKVALCTL) PVOIDFROMMP (mp2);
        WinSetDlgItemText (hwnd, IDD_AV_PRE, pCtl->pszPre);
        WinSetDlgItemText (hwnd, IDD_AV_AFTER, pCtl->pszAfter);
        WinSetWindowText (hwnd, pCtl->pszTitle);
        WinSetWindowULong (hwnd, QWL_USER, (ULONG)(PVOID)pCtl);
        bDDP = TRUE;
        break;

      case WM_COMMAND:
        switch (SHORT1FROMMP (mp1))
        {
          case DID_OK:
            pCtl = (PASKVALCTL)(PVOID)WinQueryWindowULong (hwnd, QWL_USER);
            WinQueryDlgItemText (hwnd, IDD_GTO_OFF, sizeof szBuff, szBuff);
            szBuff[(sizeof szBuff) - 1] = 0;
            if (hlp_AscToInt (szBuff, (int*)(PVOID)&pCtl->lVal))
                WinDismissDlg (hwnd, TRUE);
            else {
                WinAlarm (HWND_DESKTOP, WA_WARNING);
                WinSetDlgItemText (hwnd, IDD_GTO_OFF, szBuff);
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
