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

/* Original file date: Jan-15-1995 */

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
