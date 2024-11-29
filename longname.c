/* Original file date: Feb-26-1995 */

/******************************************************************************
*                                                                             *
*   longname.c                                                                *
*                                                                             *
*                                                                             *
******************************************************************************/




#define INCL_WIN

#include <os2.h>

#include "res.h"
#include "longname.h"
#include "helpers.h"
#include "globals.h"

MRESULT APIENTRY LNameDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2);



static PSZ  pszFilename, pszLongname;

BOOL ln_MatchNames (HWND hwnd, PSZ pszFile, PSZ pszLong)
{
    pszFilename = pszFile;
    pszLongname = pszLong;

    return (BOOL) WinDlgBox (HWND_DESKTOP, hwnd, LNameDlgProc, g.hResource, IDD_LONGNAME, NULL);
}


MRESULT APIENTRY LNameDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bDDP = FALSE;
    MRESULT mRet = (MRESULT) 0;

    switch (ulMsg)
    {
      case WM_INITDLG:
        WinSendDlgItemMsg (hwnd, IDD_L_FILE, EM_SETTEXTLIMIT, MPFROMSHORT (CCHMAXPATH), 0);
        WinSendDlgItemMsg (hwnd, IDD_L_EA, EM_SETTEXTLIMIT, MPFROMSHORT (CCHMAXPATH), 0);
        WinSetDlgItemText (hwnd, IDD_L_FILE, pszFilename);
        WinSetDlgItemText (hwnd, IDD_L_EA, pszLongname);
        hlp_DlgNicePos (hwnd);
        bDDP = TRUE;
        break;

      case WM_COMMAND:
        switch (SHORT1FROMMP (mp1))
        {
          case DID_OK:
            WinQueryDlgItemText (hwnd, IDD_L_FILE, CCHMAXPATH, pszFilename);
            WinQueryDlgItemText (hwnd, IDD_L_EA, CCHMAXPATH, pszLongname);
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

      default:
        bDDP = TRUE;
        break;
    }

    if (bDDP)
        mRet = WinDefDlgProc (hwnd, ulMsg, mp1, mp2);
    return mRet;
}
