#define INCL_WIN
#define INCL_GPI
#define INCL_DOS
#define INCL_DOSERRORS

#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "wmdefs.h"
#include "types.h"
#include "res.h"
#include "memmgr.h"
#include "helpers.h"
#include "about.h"
#include "fileio.h"
#include "client.h"
#include "fonts.h"
/* #include "delete.h" */
/* #include "insert.h" */
 #include "search.h"
#include "globals.h"
#include "longname.h"
#include "settings.h"
#include "hewnd.h"
#include "status.h"
/* #include "goto.h" */
#include "askval.h"
#include "addeawnd.h"
#include "print.h"
#include "prsetup.h"
#ifdef DEBUG
    #include "debug.h"
#endif


#define ID_HSCROLL  1
#define ID_VSCROLL  2
#define ID_HEX      3
#define ID_STATUS   4


typedef struct tag_job {
        TID     tid;
        int     type;
        struct tag_job  *pprev;
        struct tag_job  *pnext;
} JOB;
typedef JOB* PJOB;

typedef struct {
    HWND    hwndEntry;
    HWND    hwndStatus;
    HAB     hab;
    int     cx;
    int     cy;
    int     cxChar;
    int     cyChar;
    int     cyDesc;
    PFILEDATAPHYS   pfdp;
    int     iSecs;
    int     iCurrSec;
    BOOL    fModified;
    MENULOCK    mlFile[MNUM_FILE];
    MENULOCK    mlEdit[MNUM_EDIT];
    MENULOCK    mlView[MNUM_VIEW];
    MENULOCK    mlHelp[MNUM_HELP];
    MENULOCK    mlSys[MNUM_SYS];
    ULONG       flMenuAction;
    LONG        lcxScroll;
    LONG        lcyScroll;
} WNDDATA;
typedef WNDDATA* PWNDDATA;


static void s_SaveWinPos (PWNDDATA pWndData);
static BOOL s_CharProc (HWND hwnd, PWNDDATA pWndData, ULONG mp1, ULONG mp2);
static void s_InitMenu (PWNDDATA pWndData, HWND hwndMenu, int nID);
static void s_ClearFile (PWNDDATA pWndData);
static void s_CreateEAMenu (PWNDDATA pWndData);
static void s_ClearEAMenu (HWND hwndEA);
static void s_LockMenu (PWNDDATA pWndData, USHORT usMenuID, USHORT usItemID, int iBias);
static void s_SetSection (HWND hwnd, PWNDDATA pWndData, int iSec, BOOL bNoUpdate);
static MRESULT s_Create (HWND hwnd, PCREATESTRUCT pcs);
static void s_New (HWND hwnd, PWNDDATA pWndData);
static void s_NewFile (HWND hwnd, PWNDDATA pWndData, PFILEDATAPHYS pfdp);
static void s_Close (HWND hwnd, PWNDDATA pWndData);
static void s_Search (HWND hwnd, PWNDDATA pWndData, BOOL bPrompt);
static void s_Size (HWND hwnd, PWNDDATA pWndData, int iHorz, int iVert);
static BOOL s_CreateHexWindow (HWND hwnd, PWNDDATA pWndData);
static void s_Update (HWND hwnd, PWNDDATA pWndData);
static void s_SetMenuStates (PWNDDATA pWndData);
static BOOL s_IsMenuLocked (PWNDDATA pWndData, USHORT usMenuID, USHORT usItemID);
static void s_Insert (HWND hwnd, PWNDDATA pWndData);
static void s_Delete (HWND hwnd, PWNDDATA pWndData);
static void s_Goto (HWND hwnd, PWNDDATA pWndData);
static void s_AddEA (HWND hwnd, PWNDDATA pWndData);
static void s_Print (HWND hwnd, PWNDDATA pWndData);
static void s_printdone (TID tid);

static JOB     job = {0, 0, NULL, NULL};


/******************************************************
*
*   ClientWndProc
*
******************************************************/

MRESULT EXPENTRY ClientWndProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
    BOOL            bDWP = FALSE;
    MRESULT         mRet = MRFROMLONG (0);
    PWNDDATA        pWndData = NULL;
    PSECDATA        ps;
    int             i;
    PRINTDEF        printdef;

    switch (ulMsg)
    {
      case WM_CREATE:
        mRet = s_Create (hwnd, (PCREATESTRUCT) PVOIDFROMMP (mp2));
        break;

      case WM_ERASEBACKGROUND:
        mRet = MRFROMLONG (TRUE);
        break;

      case WM_USR_CREATEHEWND:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        mRet = MRFROMLONG (s_CreateHexWindow (hwnd, pWndData));
        break;

      case WM_USR_NEWFILE:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        s_NewFile (hwnd, pWndData, (PFILEDATAPHYS) PVOIDFROMMP (mp1));
        break;

      case WM_USR_SAVE_FAILED:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        if (mp1) {
            char    szBuf[100];
            sprintf (szBuf, "Save failed: err=%d, rc=%d", LONGFROMMP (mp1), LONGFROMMP (mp2));
            hlp_Msg (pWndData->hab, szBuf, HLP_MSGLEVEL_ERR);
        }
            s_LockMenu (pWndData, M_FILE, MI_NEW, -1);
            s_LockMenu (pWndData, M_FILE, MI_OPEN, -1);
            s_LockMenu (pWndData, M_FILE, MI_SAVE, -1);
            s_LockMenu (pWndData, M_FILE, MI_SAVEAS, -1);
            s_LockMenu (pWndData, M_FILE, MI_CLOSE, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_CUT, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_COPY, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_PASTE, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_CLEAR, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_INSERT, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_DELETE, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_SEARCH, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_REPEAT, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_GOTO, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_ADDEA, -1);
        s_SetMenuStates (pWndData);
        WinEnableWindow (pWndData->hwndEntry, TRUE);
        WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETACTIVE, MPFROMLONG (FALSE), 0);
        break;

      case WM_USR_OPEN_FAILED:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        if (mp1) {
            char    szBuf[100];
            sprintf (szBuf, "Open failed: err=%d, rc=%d", LONGFROMMP (mp1), LONGFROMMP (mp2));
            hlp_Msg (pWndData->hab, szBuf, HLP_MSGLEVEL_ERR);
        }
            s_LockMenu (pWndData, M_FILE, MI_NEW, -1);
            s_LockMenu (pWndData, M_FILE, MI_OPEN, -1);
            s_LockMenu (pWndData, M_FILE, MI_SAVE, -1);
            s_LockMenu (pWndData, M_FILE, MI_SAVEAS, -1);
            s_LockMenu (pWndData, M_FILE, MI_CLOSE, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_CUT, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_COPY, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_PASTE, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_CLEAR, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_INSERT, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_DELETE, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_SEARCH, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_REPEAT, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_GOTO, -1);
            s_LockMenu (pWndData, M_MODIFY, MI_ADDEA, -1);
        s_SetMenuStates (pWndData);
        WinEnableWindow (pWndData->hwndEntry, TRUE);
        WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETACTIVE, MPFROMLONG (FALSE), 0);
        break;

      case WM_USR_FILESAVED:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        pWndData->fModified = FALSE;
        pWndData->iSecs = pWndData->pfdp->iEAs + 1;
        #ifdef DEBUG
            for (i = 1; i <= pWndData->pfdp->iEAs; i++)
                ps = &pWndData->pfdp->pSecData[i];
        #endif
        if (pWndData->pfdp->iEAs > 0)
            FLAGON (pWndData->flMenuAction, MA_EA);
        else
            FLAGOFF (pWndData->flMenuAction, MA_EA);
        pWndData->iCurrSec = min (pWndData->iCurrSec, pWndData->iSecs - 1);
        s_CreateEAMenu (pWndData);

        s_LockMenu (pWndData, M_FILE, MI_NEW, -1);
        s_LockMenu (pWndData, M_FILE, MI_OPEN, -1);
        s_LockMenu (pWndData, M_FILE, MI_SAVE, -1);
        s_LockMenu (pWndData, M_FILE, MI_SAVEAS, -1);
        s_LockMenu (pWndData, M_FILE, MI_CLOSE, -1);
        s_LockMenu (pWndData, M_MODIFY, MI_CUT, -1);
        s_LockMenu (pWndData, M_MODIFY, MI_COPY, -1);
        s_LockMenu (pWndData, M_MODIFY, MI_PASTE, -1);
        s_LockMenu (pWndData, M_MODIFY, MI_CLEAR, -1);
        s_LockMenu (pWndData, M_MODIFY, MI_INSERT, -1);
        s_LockMenu (pWndData, M_MODIFY, MI_DELETE, -1);
        s_LockMenu (pWndData, M_MODIFY, MI_SEARCH, -1);
        s_LockMenu (pWndData, M_MODIFY, MI_REPEAT, -1);
        s_LockMenu (pWndData, M_MODIFY, MI_GOTO, -1);
        s_LockMenu (pWndData, M_MODIFY, MI_ADDEA, -1);
        s_SetMenuStates (pWndData);
        WinEnableWindow (pWndData->hwndEntry, TRUE);
        WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETACTIVE, MPFROMLONG (FALSE), 0);
        WinSendMsg (hwnd, WM_USR_SETSECTION, MPFROMLONG (pWndData->iCurrSec), MPFROMLONG (1)); /* don't update */
        WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETLONGNAME, MPFROMP (pWndData->pfdp->szLongname), 0);
        #ifdef DEBUG
            for (i = 1; i <= pWndData->pfdp->iEAs; i++)
                ps = &pWndData->pfdp->pSecData[i];
        #endif
        break;

      case WM_USR_LONGPROMPT:
        mRet = MRFROMSHORT (ln_MatchNames (hwnd, PVOIDFROMMP (mp1), PVOIDFROMMP (mp2)));
        break;

      case WM_USR_REFRESHEAMENU:
        s_CreateEAMenu (NULLHANDLE);
        break;

      case WM_USR_LOCKMENU:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        s_LockMenu (pWndData, SHORT1FROMMP (mp1), SHORT2FROMMP (mp1), 1);
        break;

      case WM_USR_UNLOCKMENU:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        s_LockMenu (pWndData, SHORT1FROMMP (mp1), SHORT2FROMMP (mp1), -1);
        break;

      case WM_USR_PRINTDONE:
          s_printdone (LONGFROMMP (mp1));
          break;

      case WM_VSCROLL:
      case WM_HSCROLL:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        mRet = WinSendMsg (pWndData->hwndEntry, ulMsg, mp1, mp2);
        break;

      case WM_COMMAND:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        switch (SHORT1FROMMP (mp1))
        {
          case MI_ABOUT:
            WinDlgBox (HWND_DESKTOP, hwnd, AboutDlgProc, g.hResource, IDD_ABOUT, NULL);
            break;

          case MI_PREFS:
            if (WinDlgBox (HWND_DESKTOP, hwnd, SettingsDlgProc, g.hResource, IDD_SETTINGS, &pWndData->hwndEntry))
                WinSendMsg (pWndData->hwndEntry, WMUSR_HE_SETFONT,
                        MPFROMLONG (g.nCurrFont), 0);
            break;

          case MI_NEW:
            if (s_IsMenuLocked (pWndData, M_FILE, MI_NEW)) break;
            s_New (hwnd, pWndData);
            break;

          case MI_CLOSE:
            if (s_IsMenuLocked (pWndData, M_FILE, MI_CLOSE)) break;
            s_Close (hwnd, pWndData);
            break;

          case MI_OPEN:
            if (s_IsMenuLocked (pWndData, M_FILE, MI_OPEN)) break;
            s_LockMenu (pWndData, M_FILE, MI_NEW, 1);
            s_LockMenu (pWndData, M_FILE, MI_OPEN, 1);
            s_LockMenu (pWndData, M_FILE, MI_SAVE, 1);
            s_LockMenu (pWndData, M_FILE, MI_SAVEAS, 1);
            s_LockMenu (pWndData, M_FILE, MI_CLOSE, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_CUT, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_COPY, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_PASTE, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_CLEAR, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_INSERT, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_DELETE, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_SEARCH, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_REPEAT, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_GOTO, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_ADDEA, 1);
            s_SetMenuStates (pWndData);
            WinEnableWindow (pWndData->hwndEntry, FALSE);
            WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETACTIVE, MPFROMLONG (TRUE), 0);
            fio_OpenFile (hwnd, NULL);
            break;

          case MI_SAVE:
            if (s_IsMenuLocked (pWndData, M_FILE, MI_SAVE)) break;
            if (NULL == pWndData->pfdp) break;
            if (!*pWndData->pfdp->szFilespec) {
                WinSendMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_SAVEAS, 0), 0);
                break;
            }
            s_LockMenu (pWndData, M_FILE, MI_NEW, 1);
            s_LockMenu (pWndData, M_FILE, MI_OPEN, 1);
            s_LockMenu (pWndData, M_FILE, MI_SAVE, 1);
            s_LockMenu (pWndData, M_FILE, MI_SAVEAS, 1);
            s_LockMenu (pWndData, M_FILE, MI_CLOSE, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_CUT, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_COPY, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_PASTE, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_CLEAR, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_INSERT, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_DELETE, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_SEARCH, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_REPEAT, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_GOTO, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_ADDEA, 1);
            s_SetMenuStates (pWndData);
            WinEnableWindow (pWndData->hwndEntry, FALSE);
            s_Update (hwnd, pWndData);
            WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETACTIVE, MPFROMLONG (TRUE), 0);
            fio_SaveFile (pWndData->hab, hwnd, pWndData->pfdp->szFilespec, pWndData->pfdp);
            break;

          case MI_SAVEAS:
            if (s_IsMenuLocked (pWndData, M_FILE, MI_SAVEAS)) break;
            if (NULL == pWndData->pfdp) break;
            s_LockMenu (pWndData, M_FILE, MI_NEW, 1);
            s_LockMenu (pWndData, M_FILE, MI_OPEN, 1);
            s_LockMenu (pWndData, M_FILE, MI_SAVE, 1);
            s_LockMenu (pWndData, M_FILE, MI_SAVEAS, 1);
            s_LockMenu (pWndData, M_FILE, MI_CLOSE, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_CUT, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_COPY, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_PASTE, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_CLEAR, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_INSERT, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_DELETE, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_SEARCH, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_REPEAT, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_GOTO, 1);
            s_LockMenu (pWndData, M_MODIFY, MI_ADDEA, 1);
            s_SetMenuStates (pWndData);
            WinEnableWindow (pWndData->hwndEntry, FALSE);
            s_Update (hwnd, pWndData);
            WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETACTIVE, MPFROMLONG (TRUE), 0);
            fio_SaveFile (pWndData->hab, hwnd, NULL, pWndData->pfdp);
            break;

          case MI_PRINT:
            if (NULL == pWndData->pfdp) break;
            s_Print (hwnd, pWndData);
            break;

          case MI_COPY:
            if (s_IsMenuLocked (pWndData, M_MODIFY, MI_COPY)) break;
            if (NULL == pWndData->pfdp) break;
            WinSendMsg (pWndData->hwndEntry, WMUSR_HE_COPY, 0, 0);
            break;

          case MI_PASTE:
            if (s_IsMenuLocked (pWndData, M_MODIFY, MI_PASTE)) break;
            if (NULL == pWndData->pfdp) break;
            WinSendMsg (pWndData->hwndEntry, WMUSR_HE_PASTE, 0, 0);
            break;

          case MI_CLEAR:
            if (s_IsMenuLocked (pWndData, M_MODIFY, MI_CLEAR)) break;
            if (NULL == pWndData->pfdp) break;
            WinSendMsg (pWndData->hwndEntry, WMUSR_HE_DEL, 0, 0);
            break;

          case MI_DELETE:
            if (s_IsMenuLocked (pWndData, M_MODIFY, MI_DELETE)) break;
            if (NULL == pWndData->pfdp) break;
                s_Delete (hwnd, pWndData);
            break;

          case MI_INSERT:
            if (s_IsMenuLocked (pWndData, M_MODIFY, MI_INSERT)) break;
            if (NULL == pWndData->pfdp) break;
                s_Insert (hwnd, pWndData);
            break;

          case MI_SEARCH:
            if (s_IsMenuLocked (pWndData, M_MODIFY, MI_SEARCH)) break;
            if (NULL == pWndData->pfdp) break;
            s_Search (hwnd, pWndData, TRUE);
            break;

          case MI_REPEAT:
            if (s_IsMenuLocked (pWndData, M_MODIFY, MI_REPEAT)) break;
            if (NULL == pWndData->pfdp) break;
            s_Search (hwnd, pWndData, FALSE);
            break;

          case MI_GOTO:
            if (s_IsMenuLocked (pWndData, M_MODIFY, MI_GOTO)) break;
            if (NULL == pWndData->pfdp) break;
            s_Goto (hwnd, pWndData);
            break;

          case MI_ADDEA:
            if (s_IsMenuLocked (pWndData, M_MODIFY, MI_ADDEA)) break;
            if (NULL == pWndData->pfdp) break;
            s_AddEA (hwnd, pWndData);
            break;

          case MI_DATA:
            if (s_IsMenuLocked (pWndData, M_VIEW,  MI_DATA)) break;
            WinSendMsg (hwnd, WM_USR_SETSECTION, 0, 0);
            break;

          case MI_EXIT:
            WinPostMsg (hwnd, WM_CLOSE, 0, 0);
            break;

          default:
            if (MI_FIRSTEA <= SHORT1FROMMP (mp1) && SHORT1FROMMP (mp1) <= MI_FIRSTEA + pWndData->pfdp->iEAs) {
            if (s_IsMenuLocked (pWndData, M_VIEW,  M_EAS)) break;
                WinSendMsg (hwnd, WM_USR_SETSECTION, MPFROMLONG (SHORT1FROMMP (mp1) - MI_FIRSTEA + 1), 0);
            }
            else
                bDWP = TRUE;
            break;
        }
        break;

      case WM_CONTROL:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        switch (SHORT1FROMMP (mp1))
        {
          case ID_HEX:
            switch (SHORT2FROMMP (mp1))
            {
              case HEN_SIZE:
                WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETSIZE, mp2, 0);
                break;

              case HEN_OFFSET:
                WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETOFFSET, mp2, 0);
                break;

              case HEN_SELEND:
                WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETSEL, mp2, 0);
                break;

              case HEN_CHANGED:
                pWndData->fModified = TRUE;
                break;

              default:
                bDWP =TRUE;
                break;

            }
            break;

          default:
            bDWP = TRUE;
            break;
        }
        break;

      case WM_SETFOCUS:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        if (LONGFROMMP (mp2))
            WinSetFocus (HWND_DESKTOP, pWndData->hwndEntry);
        break;

      case WM_USR_OPENCMD:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        s_LockMenu (pWndData, M_FILE, MI_NEW, 1);
        s_LockMenu (pWndData, M_FILE, MI_OPEN, 1);
        s_LockMenu (pWndData, M_FILE, MI_SAVE, 1);
        s_LockMenu (pWndData, M_FILE, MI_SAVEAS, 1);
        s_LockMenu (pWndData, M_FILE, MI_CLOSE, 1);
        s_LockMenu (pWndData, M_MODIFY, MI_CUT, 1);
        s_LockMenu (pWndData, M_MODIFY, MI_COPY, 1);
        s_LockMenu (pWndData, M_MODIFY, MI_PASTE, 1);
        s_LockMenu (pWndData, M_MODIFY, MI_CLEAR, 1);
        s_LockMenu (pWndData, M_MODIFY, MI_INSERT, 1);
        s_LockMenu (pWndData, M_MODIFY, MI_DELETE, 1);
        s_LockMenu (pWndData, M_MODIFY, MI_SEARCH, 1);
        s_LockMenu (pWndData, M_MODIFY, MI_REPEAT, 1);
        s_LockMenu (pWndData, M_MODIFY, MI_GOTO, 1);
        s_LockMenu (pWndData, M_MODIFY, MI_ADDEA, 1);
        s_SetMenuStates (pWndData);
        WinEnableWindow (pWndData->hwndEntry, FALSE);
        WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETACTIVE, MPFROMLONG (TRUE), 0);
        fio_OpenFile (hwnd, (PSZ)PVOIDFROMMP (mp1));
        break;

      case WM_USR_SETSECTION:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        s_SetSection (hwnd, pWndData, (int) LONGFROMMP (mp1), LONGFROMMP (mp2));
        break;

      case WM_SIZE:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        s_Size (hwnd, pWndData, SHORT1FROMMP (mp2), SHORT2FROMMP (mp2));
        break;

      case WM_INITMENU:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        s_InitMenu (pWndData, HWNDFROMMP (mp2), (int)SHORT1FROMMP (mp1));
        break;

      case WM_CHAR:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        bDWP = s_CharProc (hwnd, pWndData, LONGFROMMP (mp1), LONGFROMMP (mp2));
        break;

      case WM_BUTTON2CLICK:
        WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_PREFS, 0), 0);
        break;


      case WM_SYSCOMMAND:
        switch (SHORT1FROMMP (mp1)) {
          case SC_CLOSE:
            break;

          default:
            bDWP = TRUE;
            break;
        }
        break;


      case WM_CLOSE:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        if (NULL != pWndData->pfdp)
            WinSendMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_CLOSE, 0), 0);
        bDWP = TRUE;
        break;

      case WM_DESTROY:
        pWndData = (PWNDDATA)(PVOID)WinQueryWindowULong (hwnd, 0);
        s_SaveWinPos (pWndData);
        fio_WriteGlobalSettings (pWndData->hab, pWndData->hwndEntry);
        mem_HeapFree (pWndData);
        WinPostQueueMsg (g.hmqT2, WM_QUIT, 0, 0);
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
*   s_Print
*
******************************************************/

static void s_Print (HWND hwnd, PWNDDATA pWndData)
{
        PRINTDEF        printdef;
        PBYTE           data = NULL;
        PSECDATA        psec;
        TID             tid;
        PJOB            pj;

        if (!prs_Setup (hwnd, g.hResource, &printdef))
                return;

        WinQueryWindowText (g.hwndFrame, sizeof printdef.docname, printdef.docname);
        s_Update (hwnd, pWndData);
        psec = &pWndData->pfdp->pSecData[pWndData->iCurrSec];

        if (DosAllocMem ((PVOID)&data, psec->ulSize, PAG_COMMIT|PAG_READ|PAG_WRITE)) {
                WinAlarm (HWND_DESKTOP, WA_ERROR);
                return;
        }
        memcpy (data, psec->pbyData, psec->ulSize);
        if (tid = prn_Print (hwnd, pWndData->hab, data, psec->ulSize, &printdef)) {
                pj = &job;
                while (pj->pnext)
                        pj = pj->pnext;
                if (!(pj->pnext = mem_HeapAlloc (sizeof (JOB))))
                        return;
                pj->pnext->pprev = pj;
                pj = pj->pnext;
                pj->tid = tid;
                pj->type = 1;
                pj->pnext = NULL;
        }

}

static void s_printdone (TID tid)
{
        PJOB    pj = &job;

        while (pj->pnext) {
                pj = pj->pnext;
                if (tid == pj->tid) {
                        pj->pprev->pnext = pj->pnext;
                        if (pj->pnext)
                                pj->pnext->pprev = pj->pprev;
                        mem_HeapFree (pj);
                }
        }
}



/******************************************************
*
*   s_Update
*
******************************************************/

static void s_Update (HWND hwnd, PWNDDATA pWndData)
{
    HESETDATA   hesd;

    if (WinSendMsg (pWndData->hwndEntry, WMUSR_HE_QUERYDATA, MPFROMP (&hesd), 0)) {
        pWndData->pfdp->pSecData[pWndData->iCurrSec].ulSize = hesd.ulSize;
        pWndData->pfdp->pSecData[pWndData->iCurrSec].ulOffset = hesd.ulOffset;
        pWndData->pfdp->pSecData[pWndData->iCurrSec].ulSelEnd = hesd.ulSelEnd;
        pWndData->pfdp->pSecData[pWndData->iCurrSec].ulPageOffset = hesd.ulPageOffset;
    }
}

/******************************************************
*
*   s_Create
*
******************************************************/

static MRESULT s_Create (HWND hwnd, PCREATESTRUCT pcs)
{
    PFONTMETRICS    pfm = NULL;
    HPS             hps = NULLHANDLE;
    PWNDDATA        pWndData;

    pWndData = mem_HeapAlloc (sizeof (WNDDATA));
    if (NULL == pWndData) goto Abort;
    WinSetWindowULong (hwnd, 0, (ULONG)(PVOID)pWndData);
    pWndData->hab = WinQueryAnchorBlock (hwnd);

    if (NULLHANDLE == (hps = WinGetPS (hwnd))) goto Abort;
    if (NULL == (pfm = (PFONTMETRICS) mem_HeapAlloc (sizeof (FONTMETRICS)))) goto Abort;
    if (!GpiQueryFontMetrics (hps, sizeof (FONTMETRICS), pfm)) goto Abort;
    pWndData->cxChar = (int) pfm->lAveCharWidth;
    pWndData->cyChar = (int) pfm->lMaxBaselineExt;
    pWndData->cyDesc = (int) pfm->lMaxDescender;
    mem_HeapFree (pfm); pfm = NULL;
    WinReleasePS (hps); hps = NULLHANDLE;

    pWndData->cx = pcs->cx;
    pWndData->cy = pcs->cy;

    WinSetWindowULong (hwnd, QWL_STYLE, WS_CLIPCHILDREN | WinQueryWindowULong (hwnd, QWL_STYLE));

    pWndData->lcxScroll = WinQuerySysValue (HWND_DESKTOP, SV_CXVSCROLL);
    pWndData->lcyScroll = WinQuerySysValue (HWND_DESKTOP, SV_CYHSCROLL);

    pWndData->mlFile[0].usMenuID = MI_OPEN;
    pWndData->mlFile[0].ulMenuAction = MA_OPEN;
    pWndData->mlFile[1].usMenuID = MI_SAVE;
    pWndData->mlFile[1].ulMenuAction = MA_SAVE;
    pWndData->mlFile[2].usMenuID = MI_SAVEAS;
    pWndData->mlFile[2].ulMenuAction = MA_SAVEAS;
    pWndData->mlFile[3].usMenuID = MI_CLOSE;
    pWndData->mlFile[3].ulMenuAction = MA_CLOSE;
    pWndData->mlFile[4].usMenuID = MI_EXIT;
    pWndData->mlFile[4].ulMenuAction = MA_EXIT;
    pWndData->mlFile[5].usMenuID = MI_NEW;
    pWndData->mlFile[5].ulMenuAction = MA_NEW;

    pWndData->mlEdit[0].usMenuID = MI_INSERT;
    pWndData->mlEdit[0].ulMenuAction = MA_INSERT;
    pWndData->mlEdit[1].usMenuID = MI_DELETE;
    pWndData->mlEdit[1].ulMenuAction = MA_DELETE;
    pWndData->mlEdit[2].usMenuID = MI_SEARCH;
    pWndData->mlEdit[2].ulMenuAction = MA_SEARCH;
    pWndData->mlEdit[3].usMenuID = MI_GOTO;
    pWndData->mlEdit[3].ulMenuAction = MA_GOTO;
    pWndData->mlEdit[4].usMenuID = MI_COPY;
    pWndData->mlEdit[4].ulMenuAction = MA_COPY;
    pWndData->mlEdit[5].usMenuID = MI_PASTE;
    pWndData->mlEdit[5].ulMenuAction = MA_PASTE;
    pWndData->mlEdit[6].usMenuID = MI_REPEAT;
    pWndData->mlEdit[6].ulMenuAction = MA_REPEAT;
    pWndData->mlEdit[7].usMenuID = MI_CUT;
    pWndData->mlEdit[7].ulMenuAction = MA_CUT;
    pWndData->mlEdit[8].usMenuID = MI_CLEAR;
    pWndData->mlEdit[8].ulMenuAction = MA_CLEAR;
    pWndData->mlEdit[9].usMenuID = MI_ADDEA;
    pWndData->mlEdit[9].ulMenuAction = MA_ADDEA;

    pWndData->mlView[0].usMenuID = MI_DATA;
    pWndData->mlView[0].ulMenuAction = MA_DATA;
    pWndData->mlView[1].usMenuID = M_EAS;
    pWndData->mlView[1].ulMenuAction = MA_EA;

    pWndData->mlHelp[0].usMenuID = MI_ABOUT;
    pWndData->mlHelp[0].ulMenuAction = MA_ABOUT;

    pWndData->mlSys[0].usMenuID = 0;    /*******   !!!!!!!!!!!  *************/
    pWndData->mlSys[0].ulMenuAction = MA_SYSCLOSE;

    pWndData->flMenuAction = MA_NEW | MA_OPEN | MA_ABOUT | MA_SYSCLOSE | MA_EXIT;
    g.hwndMenuBar = WinWindowFromID (WinQueryWindow (hwnd, QW_PARENT), FID_MENU);
    s_SetMenuStates (pWndData);

    return MRFROMLONG (FALSE);

Abort:
    if (NULLHANDLE != hps)
        WinReleasePS (hps);
    if (NULL != pfm)
        mem_HeapFree (pfm);
    return MRFROMLONG (TRUE);
}

/******************************************************
*
*   s_Size
*
******************************************************/

static void s_Size (HWND hwnd, PWNDDATA pWndData, int iHorz, int iVert)
{
    HWND    hwndScroll;
    LONG    lStatusHight;

    pWndData->cx = iHorz;
    pWndData->cy = iVert;

    lStatusHight = (LONG) WinSendMsg (pWndData->hwndStatus, WMUSR_ST_QUERYOPTHIGHT, 0, 0);

    WinSetWindowPos (pWndData->hwndEntry, NULLHANDLE, 0, pWndData->lcyScroll,
                            pWndData->cx - pWndData->lcxScroll,
                            pWndData->cy - pWndData->lcyScroll - lStatusHight,
                            SWP_SIZE | SWP_MOVE | SWP_SHOW);
    hwndScroll = WinWindowFromID (hwnd, ID_HSCROLL);
    WinSetWindowPos (hwndScroll, NULLHANDLE, 0, 0, pWndData->cx - pWndData->lcxScroll,
                                pWndData->lcyScroll, SWP_SIZE | SWP_MOVE | SWP_SHOW);
    hwndScroll = WinWindowFromID (hwnd, ID_VSCROLL);
    WinSetWindowPos (hwndScroll, NULLHANDLE, pWndData->cx - pWndData->lcxScroll, pWndData->lcyScroll,
                                pWndData->lcxScroll, pWndData->cy - pWndData->lcyScroll - lStatusHight,
                                SWP_SIZE | SWP_MOVE | SWP_SHOW);

    WinSetWindowPos (pWndData->hwndStatus, NULLHANDLE, 0, pWndData->cy - lStatusHight,
                                        pWndData->cx, lStatusHight,
                                        SWP_SIZE | SWP_MOVE | SWP_SHOW);
}



/******************************************************
*
*   s_CreateHexWindow
*
******************************************************/

static BOOL s_CreateHexWindow (HWND hwnd, PWNDDATA pWndData)
{
    PSZ     psz;
    HWND    hwndScroll;
    LONG    alColors[NUMCOLS], lScheme;

    psz = hlp_GetString (pWndData->hab, IDS_HEXCLASS);
    pWndData->hwndEntry = WinCreateWindow (hwnd, psz, "", 0, 0,0, 100,100, hwnd,
                                             HWND_BOTTOM, ID_HEX, NULL, NULL);
    hlp_FreeString (psz);
    if (!pWndData->hwndEntry) return FALSE;

    psz = hlp_GetString (pWndData->hab, IDS_STATUSCLASS);
    pWndData->hwndStatus = WinCreateWindow (hwnd, psz, "", 0, 0,0, 100,100, hwnd,
                                             HWND_BOTTOM, ID_STATUS, NULL, NULL);
    hlp_FreeString (psz);
    if (!pWndData->hwndStatus) return FALSE;

    hwndScroll = WinCreateWindow (hwnd, WC_SCROLLBAR, "", SBS_HORZ, 0,0, 100,100, hwnd,
                            HWND_BOTTOM, ID_HSCROLL, NULL, NULL);
    WinSendMsg (pWndData->hwndEntry, WMUSR_HE_SETHSCROLLHWND, MPFROMHWND (hwndScroll), 0);

    hwndScroll = WinCreateWindow (hwnd, WC_SCROLLBAR, "", SBS_VERT, 0,0, 100,100, hwnd,
                            HWND_BOTTOM, ID_VSCROLL, NULL, NULL);
    WinSendMsg (pWndData->hwndEntry, WMUSR_HE_SETVSCROLLHWND, MPFROMHWND (hwndScroll), 0);

    WinSendMsg (pWndData->hwndEntry, WMUSR_HE_SETFONT, MPFROMLONG (g.nCurrFont), 0);

    lScheme = 0;
    fio_ReadColors (pWndData->hab, alColors, &lScheme);
    WinSendMsg (pWndData->hwndEntry, WMUSR_HE_SETCOLORS, MPFROMP (alColors), MPFROMLONG (lScheme));
/*    WinSendMsg (pWndData->hwndEntry, WMUSR_HE_SETSTATUSHWND, MPFROMHWND (hwnd), 0);*/
    return TRUE;
}

/******************************************************
*
*   s_AddEA
*
******************************************************/

static void s_AddEA (HWND hwnd, PWNDDATA pWndData)
{
    char    szName[CCHMAXPATH];
    PSECDATA    ps;
    APIRET  rc;

    s_Update (hwnd, pWndData);
    if (ae_QueryName (hwnd, szName, sizeof szName)) {
        ps = &pWndData->pfdp->pSecData[pWndData->iSecs];
        ps->pszName = mem_HeapAlloc (strlen (szName) + 1);
        if (!ps->pszName) {
            #ifdef DEBUG
                dbg_Msg ("Can't allocate Heap mem for EA Name");
            #endif
            WinAlarm (HWND_DESKTOP, WA_ERROR);
            return;
        }
        rc = DosAllocMem ((PVOID)&ps->pbyData, 0x0000FFFF, PAG_READ|PAG_WRITE);
        if (NO_ERROR != rc) {
            mem_HeapFree (ps->pszName);
            #ifdef DEBUG
            {
                char    szBuff[100];

                sprintf (szBuff, "Out of address space for EA Data: rc=%lu", rc);
                dbg_Msg (szBuff);
            }
            #endif
            WinAlarm (HWND_DESKTOP, WA_ERROR);
            return;
        }
        ps->ulMaxSize = 0x0000FFFF;
        ps->ulSize = ps->ulFlags = ps->ulPageOffset = ps->ulOffset = ps->ulSelEnd = 0L;
        strcpy (ps->pszName, szName);
        pWndData->iSecs++;
        pWndData->pfdp->iEAs++;
        pWndData->iCurrSec = pWndData->iSecs - 1;
        s_CreateEAMenu (pWndData);
        pWndData->flMenuAction |= MA_EA;
        s_SetMenuStates (pWndData);
        WinSendMsg (hwnd, WM_USR_SETSECTION, MPFROMLONG (pWndData->iCurrSec), MPFROMLONG (TRUE));
    }
}


/******************************************************
*
*   s_New
*
******************************************************/

static void s_New (HWND hwnd, PWNDDATA pWndData)
{
    PFILEDATAPHYS   pfdp;

    WinSendMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_CLOSE, 0), 0);

    if (pfdp = fio_AllocEmptyBuffer ()) {
        WinPostMsg (hwnd, WM_USR_NEWFILE, MPFROMP (pfdp), 0);
    }
    else
        WinAlarm (HWND_DESKTOP, WA_ERROR);
}

/******************************************************
*
*   s_NewFile
*
******************************************************/

static void s_NewFile (HWND hwnd, PWNDDATA pWndData, PFILEDATAPHYS pfdp)
{
    if (NULL != pWndData->pfdp)
        s_ClearFile (pWndData);

    pWndData->iSecs = pfdp->iEAs + 1;
    pWndData->pfdp = pfdp;
    pWndData->iCurrSec = 0;

    s_CreateEAMenu (pWndData);

    s_LockMenu (pWndData, M_FILE, MI_NEW, -1);
    s_LockMenu (pWndData, M_FILE, MI_OPEN, -1);
    s_LockMenu (pWndData, M_FILE, MI_SAVE, -1);
    s_LockMenu (pWndData, M_FILE, MI_SAVEAS, -1);
    s_LockMenu (pWndData, M_FILE, MI_CLOSE, -1);
    s_LockMenu (pWndData, M_MODIFY, MI_CUT, -1);
    s_LockMenu (pWndData, M_MODIFY, MI_COPY, -1);
    s_LockMenu (pWndData, M_MODIFY, MI_PASTE, -1);
    s_LockMenu (pWndData, M_MODIFY, MI_CLEAR, -1);
    s_LockMenu (pWndData, M_MODIFY, MI_INSERT, -1);
    s_LockMenu (pWndData, M_MODIFY, MI_DELETE, -1);
    s_LockMenu (pWndData, M_MODIFY, MI_SEARCH, -1);
    s_LockMenu (pWndData, M_MODIFY, MI_REPEAT, -1);
    s_LockMenu (pWndData, M_MODIFY, MI_GOTO, -1);
    s_LockMenu (pWndData, M_MODIFY, MI_ADDEA, -1);
    pWndData->flMenuAction |= (MA_NEW | MA_OPEN|MA_SAVE|MA_SAVEAS|MA_CLOSE|MA_INSERT|MA_COPY|MA_PASTE|
                                MA_CUT|MA_CLEAR|MA_DELETE|MA_SEARCH|MA_REPEAT|MA_DATA|MA_GOTO|MA_ADDEA);
    if (pfdp->iEAs > 0)
        pWndData->flMenuAction |= MA_EA;
    else
        pWndData->flMenuAction = (pWndData->flMenuAction & ~MA_EA);
    s_SetMenuStates (pWndData);
    WinEnableWindow (pWndData->hwndEntry, TRUE);
    WinSetFocus (HWND_DESKTOP, pWndData->hwndEntry);
    WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETACTIVE, MPFROMLONG (FALSE), 0);

    WinSendMsg (hwnd, WM_USR_SETSECTION, MPFROMLONG (0), 0);
    WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETLONGNAME, MPFROMP (pfdp->szLongname), 0);
}

/******************************************************
*
*   s_Close
*
******************************************************/

static void s_Close (HWND hwnd, PWNDDATA pWndData)
{
    PSZ pszTitle;
    HESETDATA   hesd;

    if (NULL == pWndData->pfdp) return;

    memset (&hesd, 0, sizeof (HESETDATA));
    WinSendMsg (pWndData->hwndEntry, WMUSR_HE_SETDATA, MPFROMP (&hesd), 0);

    s_ClearFile (pWndData);
    s_ClearEAMenu (NULLHANDLE);
    pszTitle = hlp_GetString (pWndData->hab, IDS_APPTITLE);
    WinSetWindowText (g.hwndFrame, pszTitle);
    hlp_FreeString (pszTitle);
    WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETLONGNAME, MPFROMP (""), 0);
    WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETSIZE, 0, 0);
    WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETOFFSET, 0, 0);
    WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETSEL, 0, 0);
    pWndData->flMenuAction |= (MA_NEW | MA_OPEN);
    pWndData->flMenuAction &= ~(MA_SAVE|MA_SAVEAS|MA_CLOSE|MA_COPY|MA_PASTE|MA_CUT|MA_CLEAR|
                                MA_INSERT|MA_DELETE|MA_SEARCH|MA_REPEAT|MA_GOTO|MA_ADDEA|MA_DATA|MA_EA);
    s_SetMenuStates (pWndData);
}

/******************************************************
*
*   s_SetSection
*
******************************************************/

static void s_SetSection (HWND hwnd, PWNDDATA pWndData, int iSec, BOOL bNoUpdate)
{
    char        szFileBuff[CCHMAXPATH];
    char        szBuf[CCHMAXPATH + 270];
    HESETDATA   hesd;
    PSZ         pszTitle, pszUntitled;

    if (iSec < 0 || iSec >= pWndData->iSecs) return;
    if (NULL == pWndData->pfdp) return;

    if (*pWndData->pfdp->szFilespec) {
        if (strlen (pWndData->pfdp->szFilespec) > 40) {
            memcpy (szFileBuff, pWndData->pfdp->szFilespec, 3UL);
            strcpy (szFileBuff + 3, "...");
            strcat (szFileBuff, pWndData->pfdp->szFilespec + strlen (pWndData->pfdp->szFilespec) - 35);
        }
        else
            strcpy (szFileBuff, pWndData->pfdp->szFilespec);
        if (0 == iSec)
            WinSetWindowText (g.hwndFrame, szFileBuff);
        else {
             sprintf (szBuf, "%s  -  EA: %s", szFileBuff, pWndData->pfdp->pSecData[iSec].pszName);
             WinSetWindowText (g.hwndFrame, szBuf);
        }
    }
    else {
        pszTitle = hlp_GetString (pWndData->hab, IDS_APPTITLE);
        pszUntitled = hlp_GetString (pWndData->hab, IDS_UNTITLED);
        if (0 == iSec)
            sprintf (szBuf, "%s - %s", pszTitle, pszUntitled);
        else
            sprintf (szBuf, "%s - EA: %s", pszUntitled, pWndData->pfdp->pSecData[iSec].pszName);
        WinSetWindowText (g.hwndFrame, szBuf);
        hlp_FreeString (pszUntitled);
        hlp_FreeString (pszTitle);
    }

    if (!bNoUpdate)
        s_Update (hwnd, pWndData);

    memset (&hesd, 0, sizeof hesd);
    hesd.pbData = pWndData->pfdp->pSecData[iSec].pbyData;
    hesd.ulSize = pWndData->pfdp->pSecData[iSec].ulSize;
    hesd.ulMaxAlloc = pWndData->pfdp->pSecData[iSec].ulMaxSize;
    hesd.ulOffset = pWndData->pfdp->pSecData[iSec].ulOffset;
    hesd.ulSelEnd = pWndData->pfdp->pSecData[iSec].ulSelEnd;
    hesd.ulPageOffset = pWndData->pfdp->pSecData[iSec].ulPageOffset;

    pWndData->iCurrSec = iSec;

    WinSendMsg (pWndData->hwndEntry, WMUSR_HE_SETDATA, MPFROMP (&hesd), 0);
    WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETSIZE, MPFROMLONG (hesd.ulSize), 0);
    WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETOFFSET, MPFROMLONG (hesd.ulOffset), 0);
    WinSendMsg (pWndData->hwndStatus, WMUSR_ST_SETSEL, MPFROMLONG (hesd.ulSelEnd), 0);

}

/******************************************************
*
*   s_Search
*
******************************************************/

static void s_Search (HWND hwnd, PWNDDATA pWndData, BOOL bPrompt)
{
    ULONG   ulOptions = 0;
    PBYTE   pPat = NULL;
    int     iSize = 0;
    LONG    lResult;

    if (!srch_GetString (&pPat, &iSize, &ulOptions, bPrompt))
        return;

    lResult = (LONG) WinSendMsg (pWndData->hwndEntry, WMUSR_HE_SEARCH, MPFROMP (pPat),
                              MPFROM2SHORT (iSize, ulOptions));
    if (-1 == lResult) {
        WinAlarm (HWND_DESKTOP, WA_WARNING);
        return;
    }

    WinSendMsg (pWndData->hwndEntry, WMUSR_HE_SETPOS, MPFROMLONG (lResult), 0);
}


/******************************************************
*
*   s_Insert
*
******************************************************/

static void s_Insert (HWND hwnd, PWNDDATA pWndData)
{
    LONG    lBytes = 0;

    if (av_QueryLong (hwnd, &lBytes, "Insert...", "Insert", "bytes(s)"))
        WinSendMsg (pWndData->hwndEntry, WMUSR_HE_INSERTEMPTY, MPFROMLONG (lBytes), 0);
}




/******************************************************
*
*   s_Delete
*
******************************************************/

static void s_Delete (HWND hwnd, PWNDDATA pWndData)
{
    LONG    lBytes = 0;

    if (av_QueryLong (hwnd, &lBytes, "Delete...", "Delete", "bytes(s)"))
        WinSendMsg (pWndData->hwndEntry, WMUSR_HE_DELETEDATA, MPFROMLONG (lBytes), 0);
}


/******************************************************
*
*   s_Goto
*
******************************************************/

static void s_Goto (HWND hwnd, PWNDDATA pWndData)
{
    LONG    lOff = 0;

    if (av_QueryLong (hwnd, &lOff, "Goto...", "Offset:", "")) {
        if (lOff >= 0) {
            if (!WinSendMsg (pWndData->hwndEntry, WMUSR_HE_SETPOS, MPFROMLONG (lOff), 0))
                WinAlarm (HWND_DESKTOP, WA_WARNING);
        }
    }
}




/******************************************************
*
*   s_LockMenu
*
******************************************************/


static void s_LockMenu (PWNDDATA pWndData, USHORT usMenuID, USHORT usItemID, int iBias)
{
    int     i;

    switch (usMenuID) {
      case M_FILE:
        for (i = 0; i < MNUM_FILE; i++) {
            if (usItemID == 0xFFFF || usItemID == pWndData->mlFile[i].usMenuID)
                pWndData->mlFile[i].lLock = max (0, pWndData->mlFile[i].lLock + iBias);
        }
        break;

      case M_MODIFY:
        for (i = 0; i < MNUM_EDIT; i++) {
            if (usItemID == 0xFFFF || usItemID == pWndData->mlEdit[i].usMenuID)
                pWndData->mlEdit[i].lLock = max (0, pWndData->mlEdit[i].lLock + iBias);
        }
        break;

      case M_VIEW:
        for (i = 0; i < MNUM_VIEW; i++) {
            if (usItemID == 0xFFFF || usItemID == pWndData->mlView[i].usMenuID)
                pWndData->mlView[i].lLock = max (0, pWndData->mlView[i].lLock + iBias);
        }
        break;

      case M_HELP:
        for (i = 0; i < MNUM_HELP; i++) {
            if (usItemID == 0xFFFF || usItemID == pWndData->mlHelp[i].usMenuID)
                pWndData->mlHelp[i].lLock = max (0, pWndData->mlHelp[i].lLock + iBias);
        }
        break;

    }
}

/******************************************************
*
*   s_IsMenuLocked
*
******************************************************/

static BOOL s_IsMenuLocked (PWNDDATA pWndData, USHORT usMenuID, USHORT usItemID)
{
    int     i;

    switch (usMenuID) {
      case M_FILE:
        for (i = 0; i < MNUM_FILE; i++) {
            if (usItemID == pWndData->mlFile[i].usMenuID)
                return (BOOL) ((pWndData->mlFile[i].lLock > 0)
                                || !(pWndData->mlFile[i].ulMenuAction & pWndData->flMenuAction) ?
                                TRUE: FALSE);
        }
        break;

      case M_MODIFY:
        for (i = 0; i < MNUM_EDIT; i++) {
            if (usItemID == pWndData->mlEdit[i].usMenuID)
                return (BOOL) ((pWndData->mlEdit[i].lLock > 0)
                                || !(pWndData->mlEdit[i].ulMenuAction & pWndData->flMenuAction) ?
                                TRUE: FALSE);
        }
        break;

      case M_VIEW:
        for (i = 0; i < MNUM_VIEW; i++) {
            if (usItemID == pWndData->mlView[i].usMenuID)
                return (BOOL) ((pWndData->mlView[i].lLock > 0)
                                || !(pWndData->mlView[i].ulMenuAction & pWndData->flMenuAction) ?
                                TRUE: FALSE);
        }
        break;

      case M_HELP:
        for (i = 0; i < MNUM_HELP; i++) {
            if (usItemID == pWndData->mlHelp[i].usMenuID)
                return (BOOL) ((pWndData->mlHelp[i].lLock > 0)
                                || !(pWndData->mlHelp[i].ulMenuAction & pWndData->flMenuAction) ?
                                TRUE: FALSE);
        }
        break;

    }
    return FALSE;
}


/******************************************************
*
*   s_InitMenu
*
******************************************************/

static void s_InitMenu (PWNDDATA pWndData, HWND hwndMenu, int nID)
{
    LONG    i;
    BOOL    bEnable;

    switch (nID)
    {
      case M_FILE:
        for (i = 0; i < MNUM_FILE; i++)
            WinEnableMenuItem (hwndMenu, pWndData->mlFile[i].usMenuID,
                     (BOOL) ((pWndData->mlFile[i].ulMenuAction & pWndData->flMenuAction) &&
                             (pWndData->mlFile[i].lLock == 0) ? TRUE : FALSE));
        break;

      case M_MODIFY:
        for (i = 0; i < MNUM_EDIT; i++) {
            if (MI_PASTE == pWndData->mlEdit[i].usMenuID)
                bEnable = (BOOL) WinSendMsg (pWndData->hwndEntry, WMUSR_HE_QUERYCANPASTE, 0, 0);
            else if (MI_REPEAT == pWndData->mlEdit[i].usMenuID)
                bEnable = srch_CanRepeat ();
            else
                bEnable = TRUE;
            WinEnableMenuItem (hwndMenu, pWndData->mlEdit[i].usMenuID,
                     (BOOL) ((pWndData->mlEdit[i].ulMenuAction & pWndData->flMenuAction) &&
                             (pWndData->mlEdit[i].lLock == 0) ? bEnable : FALSE));
        }
        break;

      case M_VIEW:
        for (i = 0; i < MNUM_VIEW; i++)
            WinEnableMenuItem (hwndMenu, pWndData->mlView[i].usMenuID,
                     (BOOL) ((pWndData->mlView[i].ulMenuAction & pWndData->flMenuAction) &&
                             (pWndData->mlView[i].lLock == 0) ? TRUE : FALSE));
        break;

      case M_HELP:
        for (i = 0; i < MNUM_HELP; i++)
            WinEnableMenuItem (hwndMenu, pWndData->mlHelp[i].usMenuID,
                     (BOOL) ((pWndData->mlHelp[i].ulMenuAction & pWndData->flMenuAction) &&
                             (pWndData->mlHelp[i].lLock == 0) ? TRUE : FALSE));
        break;


    }
}


/******************************************************
*
*   s_SetMenuStates
*
******************************************************/

static void s_SetMenuStates (PWNDDATA pWndData)
{
    int     i;
    BOOL    fEnable;

#ifdef NEVER   /*  file menu always valid */
    fEnable = FALSE;
    for (i = 0; i < MNUM_FILE; i++) {
        if ((pWndData->mlFile[i].ulMenuAction & pWndData->flMenuAction) &&
             pWndData->mlFile[i].lLock == 0)
            fEnable = TRUE;
    }
    WinEnableMenuItem (g.hwndMenuBar, M_FILE, fEnable);
#endif

    fEnable = FALSE;
    for (i = 0; i < MNUM_EDIT; i++) {
        if ((pWndData->mlEdit[i].ulMenuAction & pWndData->flMenuAction) &&
             pWndData->mlEdit[i].lLock == 0)
            fEnable = TRUE;
    }
    WinEnableMenuItem (g.hwndMenuBar, M_MODIFY, fEnable);

    fEnable = FALSE;
    for (i = 0; i < MNUM_VIEW; i++) {
        if ((pWndData->mlView[i].ulMenuAction & pWndData->flMenuAction) &&
             pWndData->mlView[i].lLock == 0)
            fEnable = TRUE;
    }
    WinEnableMenuItem (g.hwndMenuBar, M_VIEW, fEnable);
}


/******************************************************
*
*   s_SaveWinPos
*
******************************************************/

static void s_SaveWinPos (PWNDDATA pWndData)
{
    WINPOS  winpos;
    SWP     swp;

    memset (&swp, 0, sizeof (SWP));
    memset (&winpos, 0, sizeof (WINPOS));

    WinQueryWindowPos (g.hwndFrame, &swp);
    if (swp.fl & (SWP_MAXIMIZE | SWP_MINIMIZE)) {
        swp.x = (LONG) WinQueryWindowUShort (g.hwndFrame, QWS_XRESTORE);
        swp.y = (LONG) WinQueryWindowUShort (g.hwndFrame, QWS_YRESTORE);
        swp.cx = (LONG) WinQueryWindowUShort (g.hwndFrame, QWS_CXRESTORE);
        swp.cy = (LONG) WinQueryWindowUShort (g.hwndFrame, QWS_CYRESTORE);
        if (swp.fl & SWP_MAXIMIZE)
            winpos.bMax = TRUE;
    }

    winpos.x = swp.x;
    winpos.y = swp.y;
    winpos.cx = swp.cx;
    winpos.cy = swp.cy;

    fio_WriteWindowPos (pWndData->hab, &winpos);
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
    BOOL    fProcessed = FALSE;

/*    scancode = (int) ((mp1 & 0xFF000000) >> 24);*/
/*    cRepeat = (int) ((mp1 & 0x00FF0000) >> 16);*/
    fs = (int) (mp1 & 0x0000FFFF);
    vkey = (int) ((mp2 & 0xFFFF0000) >> 16);
    chr = (int) (mp2 & 0x0000FFFF);

    if ((KC_KEYUP | KC_INVALIDCHAR) & fs ) return TRUE;

    if ((KC_CTRL & fs) && !((KC_ALT|KC_SHIFT) & fs)) {
        fProcessed = TRUE;
        if (vkey == VK_F4 ) {
            WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_CLOSE, 0), 0);
            return FALSE;
        }

        switch (chr) {
          case 'O':
          case 'o':
            WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_OPEN, 0), 0);
            break;

          case 'S':
          case 's':
            WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_SAVE, 0), 0);
            break;

          case 'D':
          case 'd':
            WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_DELETE, 0), 0);
            break;

          case 'I':
          case 'i':
            WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_INSERT, 0), 0);
            break;

          case 'G':
          case 'g':
            WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_GOTO, 0), 0);
            break;

          case 'E':
          case 'e':
            WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_ADDEA, 0), 0);
            break;

          case 'N':
          case 'n':
            WinSendMsg (hwnd, WM_USR_SETSECTION, MPFROMLONG (pWndData->iCurrSec + 1), 0);
            fProcessed = TRUE;
            break;

          case 'P':
          case 'p':
            WinSendMsg (hwnd, WM_USR_SETSECTION, MPFROMLONG (pWndData->iCurrSec - 1), 0);
            fProcessed = TRUE;
            break;

          case 'C':
          case 'c':
            WinSendMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_COPY, 0), 0);
            fProcessed = TRUE;
            break;

          case 'V':
          case 'v':
            WinSendMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_PASTE, 0), 0);
            fProcessed = TRUE;
            break;

          case 'F':
          case 'f':
            WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_SEARCH, 0), 0);
            fProcessed = TRUE;
            break;

          case 'R':
          case 'r':
            WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_REPEAT, 0), 0);
            fProcessed = TRUE;
            break;

          default:
            fProcessed = FALSE;
            break;
        }
    } /* endif CTRL */
    else if (KC_VIRTUALKEY & fs && pWndData->pfdp) {
        fProcessed = TRUE;
        switch (vkey) {
          case VK_DELETE:
            if (!(fs & (KC_ALT|KC_CTRL))) {
                if (fs & KC_SHIFT)
                    WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_CUT, 0), 0);
                else
                    WinPostMsg (hwnd, WM_COMMAND, MPFROM2SHORT (MI_CLEAR, 0), 0);
            }
            else
                fProcessed = FALSE;
            break;

          default:
            fProcessed = FALSE;
            break;

        }
    } /* endif VIRTUALKEY  */

    return (BOOL) !fProcessed;

}


/******************************************************
*
*   s_ClearFile
*
******************************************************/

static void s_ClearFile (PWNDDATA pWndData)
{
    HESETDATA   hesd;

    if (NULL == pWndData->pfdp) return;

    memset (&hesd, 0, sizeof hesd);
    fio_FreeBuffer (pWndData->pfdp); pWndData->pfdp = NULL;
    pWndData->iSecs = pWndData->iCurrSec = 0;
    WinSendMsg (pWndData->hwndEntry, WMUSR_HE_SETDATA, MPFROMP (&hesd), 0);

}


/******************************************************
*
*   s_ClearEAMenu
*
******************************************************/

static void s_ClearEAMenu (HWND hwndEA)
{
    MENUITEM    mi;
    int         i, nCount;

    if (NULLHANDLE == hwndEA) {
        memset (&mi, 0, sizeof mi);
        WinSendMsg (g.hwndMenuBar, MM_QUERYITEM, MPFROM2SHORT (M_EAS, TRUE), MPFROMP (&mi));
        hwndEA = mi.hwndSubMenu;
    }

    nCount = (int) WinSendMsg (hwndEA, MM_QUERYITEMCOUNT, 0, 0);
    for (i = 0; i < nCount; i++)
        WinSendMsg (hwndEA, MM_DELETEITEM, MPFROM2SHORT (MI_FIRSTEA + i, FALSE), 0);

}



/******************************************************
*
*   s_CreateEAMenu
*
******************************************************/


static void s_CreateEAMenu (PWNDDATA pWndData)
{
    MENUITEM    mi;
    int         i, nEAs;
    PSECDATA    pSecData;
    HWND        hwndEA;

    memset (&mi, 0, sizeof mi);
    WinSendMsg (g.hwndMenuBar, MM_QUERYITEM, MPFROM2SHORT (M_EAS, TRUE), MPFROMP (&mi));
    hwndEA = mi.hwndSubMenu;

    s_ClearEAMenu (hwndEA);

    if (NULL == pWndData->pfdp) return;
    pSecData = &pWndData->pfdp->pSecData[1];
    nEAs = pWndData->pfdp->iEAs;

    for (i = 0; i < nEAs; i++) {
        memset (&mi, 0, sizeof mi);
        mi.iPosition = MIT_END;
        mi.afStyle = MIS_TEXT;
        mi.id = (USHORT) (MI_FIRSTEA + i);
        WinSendMsg (hwndEA, MM_INSERTITEM, MPFROMP (&mi), MPFROMP (pSecData[i].pszName));
    }
}





