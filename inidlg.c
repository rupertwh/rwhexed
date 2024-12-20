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
*   inidlg.c                                                                  *
*                                                                             *
*                                                                             *
******************************************************************************/




#define INCL_WIN

#include <os2.h>
#include <string.h>

#include "res.h"
#include "types.h"
#include "inidlg.h"
#include "fileio.h"
#include "helpers.h"


static BOOL s_Prompt (HWND hwnd);


static char szIniName[CCHMAXPATH];


MRESULT APIENTRY IniDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL    bDDP = FALSE;
    MRESULT mRet = (MRESULT) 0;

    switch (ulMsg)
    {
      case WM_INITDLG:
        strcpy (szIniName, fio_QueryIniName ());
        WinSetDlgItemText (hwnd, IDD_I_FILESPEC, szIniName);
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

          case IDD_I_CHANGE:
            if (s_Prompt (hwnd))
                WinSetDlgItemText (hwnd, IDD_I_FILESPEC, szIniName);
            break;

          case ID_NBOK:
            fio_SetIniName (WinQueryAnchorBlock (hwnd), szIniName);
            WinDismissDlg (hwnd, TRUE);
            break;

          case ID_NBCANCEL:
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



static BOOL s_Prompt (HWND hwnd)
{
    FILEDLG fd;

    memset (&fd, 0, sizeof fd);

    fd.cbSize = sizeof fd;
    fd.fl = FDS_ENABLEFILELB | FDS_SAVEAS_DIALOG;
    fd.pszTitle = "INI File Location";
    strcpy (fd.szFullFile, szIniName);

    WinFileDlg (HWND_DESKTOP, hwnd, &fd);
    if (DID_OK != fd.lReturn) return FALSE;

    strcpy (szIniName, fd.szFullFile);
    return TRUE;
}
