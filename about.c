/******************************************************************************
*                                                                             *
*   about.c                                                                   *
*                                                                             *
*                                                                             *
******************************************************************************/
 



#define INCL_WIN

#include <os2.h>

#include "res.h"
#include "about.h"
#include "helpers.h"

MRESULT APIENTRY AboutDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bDDP = FALSE;
    MRESULT mRet = (MRESULT) 0;

    switch (ulMsg) 
    {
      case WM_INITDLG:
        hlp_DlgNicePos (hwnd);
        bDDP = TRUE;
        break;

      case WM_COMMAND:
        switch (SHORT1FROMMP (mp1))
        {
          case DID_OK:
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






