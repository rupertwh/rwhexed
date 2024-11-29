/******************************************************************************
*                                                                             *
*   search.c                                                                   *
*                                                                             *
*                                                                             *
******************************************************************************/




#define INCL_WIN

#include <os2.h>
#include <string.h>
#include <stdio.h>

#include "res.h"
#include "search.h"
#include "helpers.h"
#include "globals.h"

#define MAXSEARCH   30

#define TYPEMASK    0x00000003
#define TYPESTR     0x00000001
#define TYPEHEX     0x00000002


static unsigned char    szSearchStr[MAXSEARCH + 1];
static unsigned char    aSearchHex[MAXSEARCH];
static int              nHexLen;
static ULONG            ulOptions = TYPEHEX;
static BOOL             can_search_again;

static void s_SetHexDisplay (HWND hwnd);
static void s_ReadHexEntry (HWND hwnd);

MRESULT APIENTRY SearchDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2);


BOOL srch_CanRepeat (void)
{
        return can_search_again;
}

BOOL srch_GetString (PBYTE *ppPattern, int *pnSize, PULONG pulOptions, BOOL bPrompt)
{
        ulOptions &= ~WHOLEFILE;

        if (bPrompt) {
                if (!WinDlgBox (HWND_DESKTOP, g.hwndClient, SearchDlgProc, g.hResource,
                                                 IDD_SEARCH, NULL))
                        return FALSE;
                can_search_again = TRUE;
        }
        else if (!can_search_again)
                return FALSE;

        if (TYPESTR == (TYPEMASK & ulOptions)) {
                *ppPattern = szSearchStr;
                *pnSize = (int) strlen (szSearchStr);
        }
        else {
                *ppPattern = aSearchHex;
                *pnSize = nHexLen;
        }
        *pulOptions = ~TYPEMASK & ulOptions;

        return TRUE;
}

MRESULT APIENTRY SearchDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2)
{
        BOOL    bDDP = FALSE;
        MRESULT mRet = (MRESULT) 0;
        static  bIgnoreHex, bIgnoreStr;

        switch (ulMsg)
        {
            case WM_INITDLG:
                bIgnoreHex = bIgnoreStr = TRUE;
                hlp_DlgNicePos (hwnd);
                WinSetFocus (HWND_DESKTOP,
                            WinWindowFromID (hwnd,
                                            (ULONG) (TYPEHEX & ulOptions ?
                                                IDD_S_HEXENTRY : IDD_S_STRENTRY)));
                WinCheckButton (hwnd, IDD_S_WHOLE, ulOptions & WHOLEFILE ? 1 : 0);
                WinCheckButton (hwnd, IDD_S_BACK, ulOptions & BACKWARD ? 1 : 0);
                WinSendDlgItemMsg (hwnd, IDD_S_STRENTRY, EM_SETTEXTLIMIT,
                                    MPFROMSHORT (MAXSEARCH), 0);
                WinSendDlgItemMsg (hwnd, IDD_S_HEXENTRY, EM_SETTEXTLIMIT,
                                    MPFROMSHORT (3 * MAXSEARCH), 0);
                WinSetDlgItemText (hwnd, IDD_S_STRENTRY, szSearchStr);
                s_SetHexDisplay (hwnd);
                WinCheckButton (hwnd, (ULONG) (TYPEHEX & ulOptions ?
                                             IDD_S_RBHEX : IDD_S_RBSTR), TRUE);
                bIgnoreHex = bIgnoreStr = FALSE;
                mRet = MRFROMSHORT (TRUE);
                break;

            case WM_COMMAND:
                switch (SHORT1FROMMP (mp1))
                {
                    case DID_OK:
                        WinQueryDlgItemText (hwnd, IDD_S_STRENTRY, sizeof szSearchStr, szSearchStr);
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

            case WM_CONTROL:
                switch (SHORT1FROMMP (mp1))
                {
                    case IDD_S_HEXENTRY:
                        switch (SHORT2FROMMP (mp1))
                        {
                            case EN_KILLFOCUS:
                                s_ReadHexEntry (hwnd);
                                bIgnoreHex = TRUE;
                                s_SetHexDisplay (hwnd);
                                bIgnoreHex = FALSE;
                                break;

                            case EN_CHANGE:
                                if (!bIgnoreHex && (ulOptions & TYPESTR)) {
                                        ulOptions = (ulOptions & ~TYPEMASK) | TYPEHEX;
                                        WinCheckButton (hwnd, IDD_S_RBHEX, 1);
                                }
                                break;

                            default:
                                bDDP = TRUE;
                                break;
                        }
                        break;

                    case IDD_S_STRENTRY:
                        switch (SHORT2FROMMP (mp1))
                        {
                            case EN_CHANGE:
                                if (!bIgnoreStr && (ulOptions & TYPEHEX)) {
                                        ulOptions = (ulOptions & ~TYPEMASK) | TYPESTR;
                                        WinCheckButton (hwnd, IDD_S_RBSTR, 1);
                                }
                                break;

                            default:
                                bDDP = TRUE;
                                break;

                        }
                        break;

                    case IDD_S_RBSTR:
                        if (BN_CLICKED == SHORT2FROMMP (mp1))
                                ulOptions = (~TYPEMASK & ulOptions) | TYPESTR;
                        break;

                    case IDD_S_RBHEX:
                        if (BN_CLICKED == SHORT2FROMMP (mp1))
                                ulOptions = (~TYPEMASK & ulOptions) | TYPEHEX;
                        break;

                    case IDD_S_WHOLE:
                        if (BN_CLICKED == SHORT2FROMMP (mp1))
                                ulOptions ^= WHOLEFILE;
                        break;

                    case IDD_S_BACK:
                        if (BN_CLICKED == SHORT2FROMMP (mp1))
                                ulOptions ^= BACKWARD;
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



static void s_SetHexDisplay (HWND hwnd)
{
        char    disp[3 * MAXSEARCH + 1];
        char    buff[4];
        int     i;

        for (*disp = 0, i = 0; i < nHexLen; i++) {
                sprintf (buff, "%02X ", (int) aSearchHex[i]);
                strcat (disp, buff);
        }
        WinSetDlgItemText (hwnd, IDD_S_HEXENTRY, disp);
}


static void s_ReadHexEntry (HWND hwnd)
{
        char    buff[3 * MAXSEARCH + 1], *p;
        int     digit, val;

        WinQueryDlgItemText (hwnd, IDD_S_HEXENTRY, sizeof buff, buff);

        for (nHexLen = 0, p = buff; *p; ) {

                digit = hextable[*p++];

                if (-1 == digit)
                        continue;

                val = digit;

                if (*p && (-1 != (digit = hextable[*p++]))) {
                        val <<= 4;
                        val += digit;
                }

                aSearchHex[nHexLen++] = (unsigned char) val;
        }
}




