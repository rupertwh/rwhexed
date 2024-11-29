/******************************************************************************
*                                                                             *
*   prnstat.c                                                                 *
*                                                                             *
*                                                                             *
******************************************************************************/




#define INCL_WIN

#include <os2.h>
#include <stdio.h>

#include "wmdefs.h"
#include "res.h"
#include "helpers.h"
#include "memmgr.h"
#include "prnstat.h"



static void s_init (HWND hwnd, PPRNSTATINIT pi);
static void s_cancel (HWND hwnd);
static void s_stat (HWND hwnd, PPRNSTAT ps, BOOL bClose);


MRESULT APIENTRY PrnStatDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
        BOOL    bDDP = FALSE;
        MRESULT mRet = (MRESULT) 0;

        switch (ulMsg)
        {
            case WM_INITDLG:
                s_init (hwnd, PVOIDFROMMP (mp2));
                hlp_DlgNicePos (hwnd);
                bDDP = TRUE;
                break;

            case WM_USR_PRINTSTAT:
                s_stat (hwnd, PVOIDFROMMP (mp1), LONGFROMMP (mp2));
                break;

            case WM_COMMAND:
                switch (SHORT1FROMMP (mp1))
                {
                    case DID_CANCEL:
                        s_cancel (hwnd);
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


static void s_init (HWND hwnd, PPRNSTATINIT pi)
{
        if (!pi) {
                WinPostMsg (hwnd, WM_USR_PRINTSTAT, 0, MPFROMLONG (TRUE));
                return;
        }
        WinSetDlgItemText (hwnd, IDD_PST_PAGE, "Printing page ?/?");
        WinSetDlgItemText (hwnd, IDD_PST_NAME, pi->szDocname);
        WinSetDlgItemText (hwnd, IDD_PST_PRINTER, pi->szPrinter);
        WinSetWindowULong (hwnd, QWL_USER, (ULONG) pi->pAbort);
        mem_HeapFree (pi);
}

static void s_cancel (HWND hwnd)
{
        BOOL    *pAbort;

        pAbort = (BOOL*) WinQueryWindowULong (hwnd, QWL_USER);
        if (!pAbort)
                WinDismissDlg (hwnd, FALSE);
        else
                *pAbort = TRUE;
}


static void s_stat (HWND hwnd, PPRNSTAT ps, BOOL bClose)
{
        char    buff[80];

        if (bClose)
                WinDismissDlg (hwnd, FALSE);
        else if (ps) {
                sprintf (buff, "Printing page %d/%d", ps->page, ps->totalpages);
                WinSetDlgItemText (hwnd, IDD_PST_PAGE, buff);
                mem_HeapFree (ps);
        }
}


