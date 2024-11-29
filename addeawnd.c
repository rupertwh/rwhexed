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
