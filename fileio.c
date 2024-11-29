#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN

#include <os2.h>
#include <string.h>
#include <stdio.h>

#include "types.h"
#include "wmdefs.h"
#include "errs.h"
#include "res.h"
#include "memmgr.h"
#include "helpers.h"
#include "fileio.h"
#include "globals.h"
#include "fonts.h"
#include "hewnd.h"
#ifdef DEBUG
    #include "debug.h"
#endif

#define CONDFREE(x) if (x) {mem_HeapFree (x); x = NULL;}

typedef struct {
    HWND    hwnd;
    char    szFilespec[CCHMAXPATH];
    PFILEDATAPHYS   pfdp;
} SAVET2INFO;
typedef SAVET2INFO* PSAVET2INFO;


typedef struct {
    HWND    hwnd;
    char    szFilespec[CCHMAXPATH];
} OPENT2INFO;
typedef OPENT2INFO* POPENT2INFO;


static BOOL s_GetIniName (HAB hab);
void fio_OpenFileT2 (HAB hab, ULONG ulArg);
void fio_SaveFileT2 (HAB hab, ULONG ulArg);
static int s_LoadEAs (HFILE hFile, PSECDATA *ppSecData, int *piEAs, APIRET *prc);
static int s_WriteEAs (HFILE hFile, PSECDATA pSecData, int nEAs, APIRET *prc);
static BOOL s_LongFromEA (PSECDATA pSecData, int iEAs, PSZ pszBuf, int nBufSize);
static int s_LongToEA (PSECDATA pSecData, int *piEAs, PSZ pszLongname, APIRET *prc);
static int s_NumValid (PSZ pszName);
static int s_ReadWholeFile (PSZ pszFilespec, PFILEDATAPHYS pfdp, APIRET *prc);
static int s_SaveWholeFile (PSZ pszFile, PFILEDATAPHYS pfdp, APIRET *prc);
static PSZ s_NameFromSpec (PSZ pszSpec);
static void s_CheckLongMatch (PFILEDATAPHYS pfdp);



static char szIniFileSpec[CCHMAXPATH];





/************************************************
*   s_GetIniName
************************************************/

static BOOL s_GetIniName (HAB hab)
{
    PSZ     pDefName = NULL, pSysApp = NULL, pSysKey = NULL;
    HINI    hProfile = NULLHANDLE;
    PTIB    ptib = NULL;
    PPIB    ppib = NULL;

    if (NULL == (pDefName = hlp_GetString (hab, IDS_DEFININAME))) goto Abort;
    if (NULL == (pSysApp = hlp_GetString (hab, IDS_INISYSAPP))) goto Abort;
    if (NULL == (pSysKey = hlp_GetString (hab, IDS_INISYSKEY))) goto Abort;

    if (1 != PrfQueryProfileString (HINI_USERPROFILE, pSysApp, pSysKey, "",
                            szIniFileSpec, sizeof szIniFileSpec)) {
        hProfile = PrfOpenProfile (hab, szIniFileSpec);
    }
    if (NULLHANDLE == hProfile) {
        if (NO_ERROR != DosGetInfoBlocks (&ptib, &ppib)) goto Abort;
        if (NO_ERROR != DosQueryModuleName (ppib->pib_hmte, sizeof szIniFileSpec, szIniFileSpec)) goto Abort;
        strcpy (strrchr (szIniFileSpec, '\\') + 1, pDefName);
        hProfile = PrfOpenProfile (hab, szIniFileSpec);
        if (NULLHANDLE == hProfile)
            goto Abort;
        PrfWriteProfileString (HINI_USERPROFILE, pSysApp, pSysKey, szIniFileSpec);
    }

    PrfCloseProfile (hProfile);

    CONDFREE (pSysKey);
    CONDFREE (pSysApp);
    CONDFREE (pDefName);

    return TRUE;

Abort:
    if (hProfile)
        PrfCloseProfile (hProfile);
    CONDFREE (pSysKey);
    CONDFREE (pSysApp);
    CONDFREE (pDefName);

    return FALSE;
}



/************************************************
*   fio_QueryIniName
************************************************/

const char* fio_QueryIniName (void)
{
    return szIniFileSpec;
}


/************************************************
*   fio_SetIniName
************************************************/

void fio_SetIniName (HAB hab, PSZ pszIniName)
{
    PSZ         pSysApp = NULL, pSysKey = NULL;

    if (NULL == (pSysApp = hlp_GetString (hab, IDS_INISYSAPP))) goto Abort;
    if (NULL == (pSysKey = hlp_GetString (hab, IDS_INISYSKEY))) goto Abort;

    DosQueryPathInfo (pszIniName, FIL_QUERYFULLNAME, szIniFileSpec, sizeof szIniFileSpec);
    PrfWriteProfileString (HINI_USERPROFILE, pSysApp, pSysKey, szIniFileSpec);

    /* fall through to Abort */

Abort:
    CONDFREE (pSysKey);
    CONDFREE (pSysApp);
}





/************************************************
*   fio_ReadWindowPos
************************************************/

BOOL fio_ReadWindowPos (HAB hab, PWINPOS pWinpos)
{
    BOOL    bRet = FALSE;
    HINI    hProfile = NULLHANDLE;
    PSZ     pApp = NULL, pKey = NULL;
    ULONG   ulSize;

    if (NULL == (pApp = hlp_GetString (hab, IDS_INIAPPWIN))) goto Abort;
    if (NULL == (pKey = hlp_GetString (hab, IDS_INIKEYWINPOS))) goto Abort;

    if (0 == *szIniFileSpec)
        if (FALSE == s_GetIniName (hab))
            goto Abort;

    hProfile = PrfOpenProfile (hab, szIniFileSpec);
    if (NULLHANDLE == hProfile) goto Abort;

    ulSize = sizeof (WINPOS);
    bRet = PrfQueryProfileData (hProfile, pApp, pKey, (PVOID) pWinpos, &ulSize);
    if (sizeof (WINPOS) != ulSize)
        bRet = FALSE;

    /* fall through to Abort */

Abort:
    if (NULLHANDLE != hProfile)
        PrfCloseProfile (hProfile);
    CONDFREE (pKey);
    CONDFREE (pApp);

    return bRet;
}



/************************************************
*   fio_WriteWindowPos
************************************************/

BOOL fio_WriteWindowPos (HAB hab, PWINPOS pWinpos)
{
    BOOL    bRet = FALSE;
    HINI    hProfile = NULLHANDLE;
    PSZ     pApp = NULL, pKey = NULL;

    if (NULL == (pApp = hlp_GetString (hab, IDS_INIAPPWIN))) goto Abort;
    if (NULL == (pKey = hlp_GetString (hab, IDS_INIKEYWINPOS))) goto Abort;

    if (0 == *szIniFileSpec)
        if (FALSE == s_GetIniName (hab))
            goto Abort;

    hProfile = PrfOpenProfile (hab, szIniFileSpec);
    if (NULLHANDLE == hProfile) {
        hlp_MsgRes (hab, IDS_ERR_CANTWRITEPRF, HLP_MSGLEVEL_ERR);
        goto Abort;
    }
    bRet = PrfWriteProfileData (hProfile, pApp, pKey, (PVOID) pWinpos, sizeof (WINPOS));

    /* fall through to Abort */

Abort:
    if (NULLHANDLE != hProfile)
        PrfCloseProfile (hProfile);
    CONDFREE (pApp);
    CONDFREE (pKey);

    return bRet;
}



/************************************************
*   fio_WriteGlobalSettings
************************************************/

BOOL fio_WriteGlobalSettings (HAB hab, HWND hwndHex)
{
    BOOL    bRet = FALSE;
    HINI    hProfile = NULLHANDLE;
    PSZ     pApp = NULL, pKeyName = NULL, pKeySize = NULL, pKeyColor = NULL;
    char    szBuff[20];
    int     i;
    LONG    lColors[NUMCOLS], lScheme, lFont;
    PFONTMETRICS    pfm;

    if (NULL == (pApp = hlp_GetString (hab, IDS_INIAPPFONT))) goto Abort;
    if (NULL == (pKeyName = hlp_GetString (hab, IDS_INIKEYFNAME))) goto Abort;
    if (NULL == (pKeySize = hlp_GetString (hab, IDS_INIKEYFSIZE))) goto Abort;

    if (0 == *szIniFileSpec)
        if (FALSE == s_GetIniName (hab))
            goto Abort;

    hProfile = PrfOpenProfile (hab, szIniFileSpec);
    if (NULLHANDLE == hProfile) {
        hlp_MsgRes (hab, IDS_ERR_CANTWRITEPRF, HLP_MSGLEVEL_ERR);
        goto Abort;
    }

    lFont = (LONG) WinSendMsg (hwndHex, WMUSR_HE_QUERYFONT, 0, 0);
    pfm = fnt_QueryMetrics (lFont);
    bRet = PrfWriteProfileString (hProfile, pApp, pKeyName, pfm->szFacename);
    if (FALSE == bRet) goto Abort;

    sprintf (szBuff, "%d", (int) pfm->sNominalPointSize / 10);
    bRet = PrfWriteProfileString (hProfile, pApp, pKeySize, szBuff);
    if (FALSE == bRet) goto Abort;

    CONDFREE (pApp);
    if (NULL == (pApp = hlp_GetString (hab, IDS_INIAPPCOLOR))) goto Abort;

    lScheme = (LONG) WinSendMsg (hwndHex, WMUSR_HE_QUERYCOLORS, MPFROMP (lColors), MPFROMLONG (2));
    for (i = 0; i < NUMCOLS; i++) {
        if (NULL == (pKeyColor = hlp_GetString (hab, (ULONG) (IDS_COLNAME+i)))) goto Abort;
        bRet = PrfWriteProfileData (hProfile, pApp, pKeyColor, &lColors[i], sizeof (LONG));
        if (FALSE == bRet) goto Abort;
        CONDFREE (pKeyColor);
    }
    if (NULL == (pKeyColor = hlp_GetString (hab, IDS_INIKEYSCHEME))) goto Abort;
    bRet = PrfWriteProfileData (hProfile, pApp, pKeyColor, &lScheme, sizeof (LONG));
    if (FALSE == bRet) goto Abort;
    CONDFREE (pKeyColor);


    bRet = TRUE;

    /* fall through to Abort */

Abort:
    if (NULLHANDLE != hProfile)
        PrfCloseProfile (hProfile);
    CONDFREE (pApp);
    CONDFREE (pKeyName);
    CONDFREE (pKeySize);
    CONDFREE (pKeyColor);

    return bRet;
}



/************************************************
*   fio_ReadFont
************************************************/

BOOL fio_ReadFont (HAB hab, PSZ pszFacename, int *pSize)
{
    BOOL    bRet = FALSE;
    HINI    hProfile = NULLHANDLE;
    PSZ     pApp = NULL, pKeyName = NULL, pKeySize = NULL;

    if (NULL == (pApp = hlp_GetString (hab, IDS_INIAPPFONT))) goto Abort;
    if (NULL == (pKeyName = hlp_GetString (hab, IDS_INIKEYFNAME))) goto Abort;
    if (NULL == (pKeySize = hlp_GetString (hab, IDS_INIKEYFSIZE))) goto Abort;

    if (0 == *szIniFileSpec)
        if (FALSE == s_GetIniName (hab))
            goto Abort;

    hProfile = PrfOpenProfile (hab, szIniFileSpec);
    if (NULLHANDLE == hProfile) {
        hlp_MsgRes (hab, IDS_ERR_CANTWRITEPRF, HLP_MSGLEVEL_ERR);
        goto Abort;
    }

    *pSize = PrfQueryProfileInt (hProfile, pApp, pKeySize, 11);
    PrfQueryProfileString (hProfile, pApp, pKeyName, "System VIO", pszFacename, FACESIZE);

    bRet = TRUE;

    /* fall through to Abort */

Abort:
    if (NULLHANDLE != hProfile)
        PrfCloseProfile (hProfile);
    CONDFREE (pApp);
    CONDFREE (pKeyName);
    CONDFREE (pKeySize);

    return bRet;
}


/************************************************
*   fio_ReadColors
************************************************/

BOOL fio_ReadColors (HAB hab, PLONG alColors, PLONG plScheme)
{
    BOOL    bRet = FALSE;
    HINI    hProfile = NULLHANDLE;
    PSZ     pApp = NULL, pKeyColor = NULL;
    int     i;
    ULONG   ulSize;

    if (0 == *szIniFileSpec)
        if (FALSE == s_GetIniName (hab))
            goto Abort;

    hProfile = PrfOpenProfile (hab, szIniFileSpec);
    if (NULLHANDLE == hProfile) {
        hlp_MsgRes (hab, IDS_ERR_CANTWRITEPRF, HLP_MSGLEVEL_ERR);
        goto Abort;
    }

    if (NULL == (pApp = hlp_GetString (hab, IDS_INIAPPCOLOR))) goto Abort;

    for (i = 0; i < NUMCOLS; i++) {
        ulSize = sizeof (LONG);
        if (NULL == (pKeyColor = hlp_GetString (hab, (ULONG) (IDS_COLNAME+i)))) goto Abort;
        PrfQueryProfileData (hProfile, pApp, pKeyColor, &alColors[i], &ulSize);
        CONDFREE (pKeyColor);
    }
    ulSize = sizeof (LONG);
    if (NULL == (pKeyColor = hlp_GetString (hab, IDS_INIKEYSCHEME))) goto Abort;
    PrfQueryProfileData (hProfile, pApp, pKeyColor, plScheme, &ulSize);
    CONDFREE (pKeyColor);

    bRet = TRUE;

    /* fall through to Abort */

Abort:
    if (NULLHANDLE != hProfile)
        PrfCloseProfile (hProfile);
    CONDFREE (pApp);
    CONDFREE (pKeyColor);

    return bRet;
}


/************************************************
*   fio_OpenFile
************************************************/

void fio_OpenFile (HWND hwnd, PSZ pszFile)
{
    FILEDLG fd;
    POPENT2INFO poi2;
    int     iErr = ERR_OPEN;
    char    aszType[4][20] = { "C Code", "Icon", "Plain Text", "Bitmap" };
    PSZ     aTypes[5];

    if (NULL == (poi2 = (POPENT2INFO) mem_HeapAlloc (sizeof (OPENT2INFO))))
        { iErr |= ERR_OUTOFHEAP; goto Abort; }

    if (NULL == pszFile) {
        memset (&fd, 0, sizeof fd);

        fd.cbSize = sizeof fd;
        fd.fl = FDS_CENTER | FDS_OPEN_DIALOG;
        aTypes[0] = aszType[0];
        aTypes[1] = aszType[1];
        aTypes[2] = aszType[2];
        aTypes[3] = aszType[3];
        aTypes[4] = NULL;

        fd.papszITypeList = (PAPSZ)(PVOID)aTypes;


        WinFileDlg (HWND_DESKTOP, hwnd, &fd);
        if (DID_OK != fd.lReturn) { iErr = 0; goto Abort; }

        pszFile = fd.szFullFile;
    }

    DosQueryPathInfo (pszFile, FIL_QUERYFULLNAME, poi2->szFilespec, sizeof poi2->szFilespec);
    poi2->hwnd = hwnd;
    WinPostQueueMsg (g.hmqT2, WM_USR_T2CALL, MPFROMP (fio_OpenFileT2), MPFROMP (poi2));

    return;

Abort:
    if (NULL != poi2)
        mem_HeapFree (poi2);

    WinPostMsg (hwnd, WM_USR_OPEN_FAILED, MPFROMLONG (iErr), 0);
}



/************************************************
*   fio_OpenFileT2
************************************************/

void fio_OpenFileT2 (HAB hab, ULONG ulArg)
{
    APIRET          rc = NO_ERROR;
    PFILEDATAPHYS   pfdp = NULL;
    POPENT2INFO     poi2;
    int             iErr = ERR_OPEN;

    poi2 = (POPENT2INFO)(PVOID)ulArg;
    if (NULL == (pfdp = mem_HeapAlloc (sizeof (FILEDATAPHYS)))) { iErr |= ERR_OUTOFHEAP; goto Abort; }
    memset (pfdp, 0, sizeof (FILEDATAPHYS));

    iErr = s_ReadWholeFile (poi2->szFilespec, pfdp, &rc);
    if (0 != iErr) goto Abort;

    WinPostMsg (poi2->hwnd, WM_USR_NEWFILE, MPFROMP (pfdp), 0);
    mem_HeapFree (poi2);
    return;

Abort:
    if (NULL != pfdp)
        mem_HeapFree (pfdp);
    WinPostMsg (poi2->hwnd, WM_USR_OPEN_FAILED, MPFROMLONG (iErr), MPFROMLONG(rc));
    mem_HeapFree (poi2);

}


/************************************************
*   s_ReadWholeFile
************************************************/

static int s_ReadWholeFile (PSZ pszFilespec, PFILEDATAPHYS pfdp, APIRET *prc)
{
    ULONG       ulBytes = 0;
    HFILE       hFile = NULLHANDLE;
    FILESTATUS3 fs;
    PBYTE       pBuff = NULL;
    int         iErr = ERR_OPEN;
    PSZ         p;

    *prc = NO_ERROR;
    strcpy (pfdp->szFilespec, pszFilespec);

    *prc = DosOpen (pfdp->szFilespec, &hFile, &ulBytes, 0, 0, OPEN_ACTION_FAIL_IF_NEW |
                    OPEN_ACTION_OPEN_IF_EXISTS, OPEN_FLAGS_FAIL_ON_ERROR |
                    OPEN_FLAGS_SEQUENTIAL | OPEN_SHARE_DENYWRITE | OPEN_ACCESS_READONLY,
                    NULL);
    if (NO_ERROR != *prc) { iErr |= ERR_OPENFILE; goto Abort; }

    p = strrchr (pszFilespec, '\\');
    if (!p) p = pszFilespec + strlen (pszFilespec);
    else *p = 0;
    if (2 == strlen (pszFilespec))
        strcat (pszFilespec, "\\");

    if (*pszFilespec > 'Z') *pszFilespec = (char) (*pszFilespec - 'a' + 'A');
    DosSetDefaultDisk ((ULONG) (*pszFilespec - 'A' + 1));
    DosSetCurrentDir (pszFilespec);

    *prc = DosQueryFileInfo (hFile, FIL_STANDARD, &fs, sizeof fs);
    if (NO_ERROR != *prc) { iErr |= ERR_FILEINFO; goto Abort; }

    *prc = DosAllocMem ((PVOID)&pBuff, MAXMEMSIZE, PAG_READ | PAG_WRITE);
    if (NO_ERROR != *prc) { iErr |= ERR_OUTOFADR; goto Abort; }

    if (fs.cbFile > 0UL) {
        *prc = DosSetMem (pBuff, fs.cbFile, PAG_COMMIT | PAG_DEFAULT);
        if (NO_ERROR != *prc) { iErr |= ERR_OUTOFMEM; goto Abort; }

        *prc = DosRead (hFile, pBuff, fs.cbFile, &ulBytes);
        if (NO_ERROR != *prc || fs.cbFile != ulBytes) { iErr |= ERR_READFILE; goto Abort; }
    }

    if (iErr = s_LoadEAs (hFile, &pfdp->pSecData, &pfdp->iEAs, prc)) goto Abort;

    DosClose (hFile); hFile = NULLHANDLE;

    s_LongFromEA (&pfdp->pSecData[1], pfdp->iEAs, pfdp->szLongname, sizeof pfdp->szLongname);
    s_CheckLongMatch (pfdp);

    pfdp->pSecData[0].ulSize = fs.cbFile;
    pfdp->pSecData[0].ulMaxSize = MAXMEMSIZE;
    pfdp->pSecData[0].pbyData = pBuff; pBuff = NULL;

    return 0;



Abort:
    if (NULL != pBuff)
        DosFreeMem (pBuff);
    if (NULLHANDLE != hFile)
        DosClose (hFile);

    return iErr;
}



/************************************************
*   fio_SaveFile
************************************************/

void fio_SaveFile (HAB hab, HWND hwnd, PSZ pszFile, PFILEDATAPHYS pfdp)
{
    PFILEDLG    pfd = NULL;
    char        szBuff[CCHMAXPATH + 10];
    PSAVET2INFO psi = NULL;
    int         iErr = ERR_SAVE;

    if (NULL == pszFile) {
        pfd = mem_HeapAlloc (sizeof (FILEDLG));
        if (NULL == pfd) { iErr |= ERR_OUTOFHEAP; goto Abort; }
        memset (pfd, 0, sizeof (FILEDLG));

        pfd->cbSize = sizeof (FILEDLG);
        pfd->fl = FDS_CENTER | FDS_SAVEAS_DIALOG;

        while (1) {
            WinFileDlg (HWND_DESKTOP, hwnd, pfd);
            if (DID_OK != pfd->lReturn) { iErr = 0; goto Abort; }
            if (1 == fio_IsFAT (pfd->szFullFile)) {
                if (TRUE == fio_IsValid83 (strrchr (pfd->szFullFile, '\\') + 1))
                    break;
                else
                    hlp_Msg (hab, "Not a valid filename for non-HPFS", HLP_MSGLEVEL_WRN);
            }
            else
               break;
        }
        pszFile = pfd->szFullFile;
    }

    DosQueryPathInfo (pszFile, FIL_QUERYFULLNAME, szBuff, sizeof szBuff);
/*
    if (0 == strcmp (g.fLongMatch) {
        char    szBuff2[CCHMAXPATH];
        PSZ     pszName;

        *szBuff2 = 0;
        s_LongFromEA (&pData[1], *pnEAs, szBuff2, CCHMAXPATH);
        pszName = strrchr (pszFile, '\\');
        if (NULL == pszName)
            pszName = pszFile;
        else
            pszName++;
        if (strcmp (pszName, szBuff2)) {
            if (!WinSendMsg (hwnd, WM_USR_LONGPROMPT, MPFROMP (pszName), MPFROMP (szBuff2)))
                goto Abort;
            s_LongToEA (&pData[1], pnEAs, szBuff2);
        }
    }
*/

    psi = mem_HeapAlloc (sizeof (SAVET2INFO));
    if (NULL == psi) { iErr |= ERR_OUTOFHEAP; goto Abort; }
    strcpy (psi->szFilespec, szBuff);
    psi->pfdp = pfdp;
    psi->hwnd = hwnd;
    mem_HeapFree (pfd);

    WinPostQueueMsg (g.hmqT2, WM_USR_T2CALL, MPFROMP (fio_SaveFileT2), MPFROMP (psi));
    return;


Abort:
    WinPostMsg (hwnd, WM_USR_SAVE_FAILED, MPFROMLONG (iErr), 0);

    if (NULL != psi)
        mem_HeapFree (psi);
    if (NULL != pfd)
        mem_HeapFree (pfd);
}


/**********************************
*   fio_SaveFileT2
***********************************/

void fio_SaveFileT2 (HAB hab, ULONG ulArg)
{
    PSAVET2INFO psi;
    APIRET      rc = NO_ERROR;
    int         iErr;

    psi = (PSAVET2INFO) (PVOID) ulArg;

    iErr = s_SaveWholeFile (psi->szFilespec, psi->pfdp, &rc);
    if (0 != iErr) goto Abort;

    WinPostMsg (psi->hwnd, WM_USR_FILESAVED, MPFROMP (psi->pfdp), 0);

    mem_HeapFree (psi); psi = NULL;

    return;


Abort:
    WinPostMsg (psi->hwnd, WM_USR_SAVE_FAILED, MPFROMLONG (iErr), MPFROMLONG (rc));
    if (NULL != psi)
        mem_HeapFree (psi);

}

/**********************************
*   s_SaveWholeFile
***********************************/

static int s_SaveWholeFile (PSZ pszFilespec, PFILEDATAPHYS pfdp, APIRET *prc)
{
    HFILE       hFile = NULLHANDLE;
    ULONG       ulBytes = 0;
    int         iErr = ERR_SAVE, i;

    *prc = DosOpen (pszFilespec, &hFile, &ulBytes, pfdp->pSecData[0].ulSize, FILE_NORMAL,
                    OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS,
                    OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_SEQUENTIAL |
                    OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_WRITEONLY,
                    NULL);
    if (NO_ERROR != *prc) { iErr |= ERR_OPENFILE; goto Abort; }

    *prc = DosWrite (hFile, pfdp->pSecData[0].pbyData, pfdp->pSecData[0].ulSize, &ulBytes);
    if (NO_ERROR != *prc || pfdp->pSecData[0].ulSize != ulBytes) { iErr |= ERR_WRITEFILE; goto Abort; }

    iErr = s_WriteEAs (hFile, &pfdp->pSecData[1], pfdp->iEAs, prc);
    if (0 != iErr) goto Abort;

    for (i = 1; i <= pfdp->iEAs; i++) {
        if (0 == pfdp->pSecData[i].ulSize) {  /* EA has been deleted */
            pfdp->iEAs = fio_RemoveEA (pfdp->pSecData, pfdp->iEAs, i);
            i--;
        }
    }
    DosClose (hFile); hFile = NULLHANDLE;
    strcpy (pfdp->szFilespec, pszFilespec);
    s_LongFromEA (&pfdp->pSecData[1], pfdp->iEAs, pfdp->szLongname, sizeof pfdp->szLongname);

    return 0;


Abort:
    if (NULLHANDLE != hFile)
        DosClose (hFile);

    return iErr;
}


/************************************************
*   fio_RemoveEA
************************************************/

int fio_RemoveEA (PSECDATA pSecData, int iEAs, int iEARemove)
{
    int i;

    DosFreeMem (pSecData[iEARemove].pbyData);
    mem_HeapFree (pSecData[iEARemove].pszName);

    for (i = iEARemove; i < iEAs; i++)
        memcpy (&pSecData[i], &pSecData[i+1], sizeof (SECDATA));

    return --iEAs;
}


/************************************************
*   s_LoadEAs
************************************************/

static int s_LoadEAs (HFILE hFile, PSECDATA *ppSecData, int *piEAs, APIRET *prc)
{
    PEAOP2  peaop2 = NULL;
    PBYTE   pBuffer = NULL;
    PFEA2   pfea2 = NULL;
    PGEA2   pgea2 = NULL;
    FILESTATUS4 fs4;
    ULONG   i, ulCount = (ULONG) -1, ulGSize = 0UL, ulFSize = 0UL, ulSize;
    int     iErr = ERR_OPEN;

    *ppSecData = NULL;
    *piEAs = 0;
    *prc = NO_ERROR;

    memset (&fs4, 0, sizeof (FILESTATUS4));
    *prc = DosQueryFileInfo (hFile, FIL_QUERYEASIZE, &fs4, sizeof fs4);
    if (NO_ERROR != *prc) { iErr |= ERR_EAINFO; goto Abort; }

    *prc = DosAllocMem ((PVOID)&pBuffer, fs4.cbList, PAG_READ|PAG_WRITE|PAG_COMMIT);
    if (NO_ERROR != *prc) { iErr |= ERR_OUTOFMEM; goto Abort; }

    *prc = DosEnumAttribute (ENUMEA_REFTYPE_FHANDLE, &hFile, 1, pBuffer, fs4.cbList, &ulCount, 1);
    if (NO_ERROR != *prc || (ULONG)-1 == ulCount) { iErr |= ERR_EANAMES; goto Abort; }

    pfea2 = (PFEA2)(PVOID)pBuffer;
    for (i = 0; i < ulCount; i++) {
        ulGSize += 0xFFFFFFFCUL & (sizeof (GEA2) + (ULONG) pfea2->cbName + 3UL);
        ulFSize += 0xFFFFFFFCUL & (sizeof (FEA2) + (ULONG) pfea2->cbName + (ULONG) pfea2->cbValue + 3UL);
        if (0UL == pfea2->oNextEntryOffset) {
            ulCount = i + 1;
            break;
        }
        pfea2 = (PFEA2)(PVOID) (((PBYTE)(PVOID)pfea2) + pfea2->oNextEntryOffset);
    }

    ulSize = ulGSize + ulFSize + sizeof (EAOP2) + 2 * sizeof (ULONG);
    *prc = DosAllocMem ((PVOID)&peaop2, ulSize, PAG_READ|PAG_WRITE|PAG_COMMIT);
    if (NO_ERROR != *prc) { iErr |= ERR_OUTOFMEM; goto Abort; }

    peaop2->fpGEA2List = (PGEA2LIST)(PVOID) (((PBYTE)(PVOID)peaop2) + sizeof (EAOP2));
    peaop2->fpFEA2List = (PFEA2LIST)(PVOID) (((PBYTE)(PVOID)peaop2->fpGEA2List) + sizeof (ULONG)
                                                                + ulGSize);
    peaop2->fpGEA2List->cbList = ulGSize;
    peaop2->fpFEA2List->cbList = ulFSize;

    pfea2 = (PFEA2)(PVOID)pBuffer;
    pgea2 = peaop2->fpGEA2List->list;
    for (i = 0; i < ulCount; i++) {
        if (i == ulCount - 1)
            pgea2->oNextEntryOffset = 0;
        else
            pgea2->oNextEntryOffset = 0xFFFFFFFC & (sizeof (GEA2) + (ULONG) pfea2->cbName + 3UL);
        pgea2->cbName = pfea2->cbName;
        strcpy (pgea2->szName, pfea2->szName);
        pgea2 = (PGEA2)(PVOID) (((PBYTE)(PVOID)pgea2) + pgea2->oNextEntryOffset);
        pfea2 = (PFEA2)(PVOID) (((PBYTE)(PVOID)pfea2) + pfea2->oNextEntryOffset);
    }

    DosFreeMem (pBuffer); pBuffer = NULL; pfea2 = NULL;

    if (0 != ulCount) {
        *prc = DosQueryFileInfo (hFile, FIL_QUERYEASFROMLIST, peaop2, sizeof (EAOP2));
        if (NO_ERROR != *prc) {
            #ifdef DEBUG
                printf ("oError: 0x%lx\n", peaop2->oError);
            #endif
            iErr |= ERR_READEAS;
            goto Abort;
        }

    }

    pfea2 = peaop2->fpFEA2List->list;

    *prc = DosAllocMem ((PVOID) ppSecData, (ulCount + 10) * sizeof (SECDATA), PAG_READ|PAG_WRITE|PAG_COMMIT);
    /* added 1 for Data and 1 for possible LONGNAME to be added + 10 more that can be added*/
    if (NO_ERROR != *prc) { iErr |= ERR_OUTOFMEM; goto Abort; }

    for (i = 1; i < ulCount + 1; i++) { /* reserve i=0 for actual file data  */
        *prc = DosAllocMem ((PVOID)&(*ppSecData)[i].pbyData, 0x0000FFFF, PAG_READ|PAG_WRITE);
        if (NO_ERROR != *prc) { iErr |= ERR_OUTOFADR; goto Abort; }
        if (!((*ppSecData)[i].pszName = mem_HeapAlloc (CCHMAXPATH)))
            { iErr |= ERR_OUTOFMEM; goto Abort; }

        (*ppSecData)[i].ulMaxSize = 0x0000FFFFL;
        ulGSize = (ULONG) pfea2->cbName + 1UL;
        ulSize = (ULONG) pfea2->cbValue;
        *prc = DosSetMem ((PVOID)(*ppSecData)[i].pbyData, ulSize, PAG_DEFAULT | PAG_COMMIT);
        if (NO_ERROR != *prc) { iErr |= ERR_OUTOFMEM; goto Abort; };
        strcpy ((*ppSecData)[i].pszName, pfea2->szName);
        memcpy ((*ppSecData)[i].pbyData, pfea2->szName + pfea2->cbName + 1, pfea2->cbValue);
        (*ppSecData)[i].ulFlags = (ULONG) pfea2->fEA;
        (*ppSecData)[i].ulSize = (ULONG) pfea2->cbValue;
        pfea2 = (PFEA2)(PVOID) (((PBYTE)(PVOID)pfea2) + pfea2->oNextEntryOffset);
    }

    DosFreeMem (peaop2); peaop2 = NULL;
    *piEAs = (int) ulCount;

    return 0;

Abort:
    if (NULL != pBuffer)
        DosFreeMem (pBuffer);
    if (NULL != peaop2)
        DosFreeMem (peaop2);
    if (NULL != *ppSecData)
        DosFreeMem (*ppSecData);
    return iErr;
}




/************************************************
*   s_WriteEAs
************************************************/


static int s_WriteEAs (HFILE hFile, PSECDATA pSecData, int nEAs, APIRET *prc)
{
    PEAOP2  peaop2 = NULL;
    PFEA2   pfea2;
    int     i;
    ULONG   ulSize;
    int     iErr = ERR_SAVE;

    ulSize = sizeof (EAOP2) + sizeof (ULONG);

    for (i = 0; i < nEAs; i++)
        ulSize += sizeof (FEA2) + (0xFFFFFFFC & (strlen (pSecData[i].pszName)
                                                + pSecData[i].ulSize + 3UL));

    *prc = DosAllocMem ((PVOID)&peaop2, ulSize, PAG_READ|PAG_WRITE|PAG_COMMIT);
    if (NO_ERROR != *prc) {iErr |= ERR_OUTOFMEM; goto Abort; }

    peaop2->fpGEA2List = NULL;
    peaop2->fpFEA2List = (PFEA2LIST)(PVOID) (((PBYTE)(PVOID)peaop2) + sizeof (EAOP2));
    peaop2->oError = (ULONG) -1;

    peaop2->fpFEA2List->cbList = ulSize - sizeof (EAOP2);
    pfea2 = peaop2->fpFEA2List->list;
    pfea2->oNextEntryOffset = 0;

    for (i = 0; i < nEAs; i++) {
        pfea2 = (PFEA2)(PVOID) (((PBYTE)(PVOID)pfea2) + pfea2->oNextEntryOffset);
        pfea2->oNextEntryOffset = sizeof (FEA2) + strlen (pSecData[i].pszName) + pSecData[i].ulSize;
        pfea2->oNextEntryOffset = 0xFFFFFFFC & (pfea2->oNextEntryOffset + 3UL);
        pfea2->fEA = (BYTE) pSecData[i].ulFlags;
        pfea2->cbName = (BYTE) strlen (pSecData[i].pszName);
        pfea2->cbValue = (USHORT) pSecData[i].ulSize;
        strcpy (pfea2->szName, pSecData[i].pszName);
        memcpy (pfea2->szName + pfea2->cbName + 1, pSecData[i].pbyData, pSecData[i].ulSize);
    }
    pfea2->oNextEntryOffset = 0UL;

    *prc = DosSetFileInfo (hFile, 2, peaop2, sizeof (EAOP2));
    if (NO_ERROR != *prc) { iErr |= ERR_WRITEEAS; goto Abort; }

    DosFreeMem (peaop2);

    return 0;


Abort:
    if (NULL != peaop2)
        DosFreeMem (peaop2);

    return iErr;
}



/************************************************
*   s_LongFromEA
************************************************/

static BOOL s_LongFromEA (PSECDATA pSecData, int iEAs, PSZ pszBuf, int nBufSize)
{
    int     i, nSize;

    *pszBuf = 0;
    for (i = 0; i < iEAs; i++) {
        if (pSecData[i].pszName && 0 == strcmp (pSecData[i].pszName, ".LONGNAME"))
            break;
    }
    if (i == iEAs || pSecData[i].ulSize < 5)
        return FALSE;

    nSize = (int) *(PUSHORT)(PVOID)(pSecData[i].pbyData + 2);
    if (nSize + 1 > nBufSize) return FALSE;

    memcpy (pszBuf, pSecData[i].pbyData + 4, (ULONG) nSize);
    pszBuf[nSize] = 0;

    return TRUE;
}



/************************************************
*   s_LongToEA
************************************************/

static int s_LongToEA (PSECDATA pSecData, int *piEAs, PSZ pszLongname, APIRET *prc)
{
    int     i, iLen;
    PBYTE   pBuff1 = NULL, pBuff2 = NULL;
    PUSHORT pus;
    int     iErr;

    for (i = 0; i < *piEAs; i++) {
        if (pSecData[i].pszName && 0 == strcmp (pSecData[i].pszName, ".LONGNAME"))
            break;
    }
    iLen = (int) strlen (pszLongname);
    if (i == *piEAs) {
        *prc = DosAllocMem ((PVOID)&pBuff1, 0x0000FFFF, PAG_READ | PAG_WRITE);
        if (NO_ERROR != *prc) { iErr = ERR_OUTOFADR; goto Abort; }
        *prc = DosSetMem (pBuff1, (ULONG) (iLen + 4), PAG_DEFAULT | PAG_COMMIT);
        if (NO_ERROR != *prc) { iErr = ERR_OUTOFMEM; goto Abort; }
        *prc = DosAllocMem ((PVOID)&pBuff2, 0x00001000, PAG_READ|PAG_WRITE|PAG_COMMIT);
        if (NO_ERROR != *prc) { iErr = ERR_OUTOFMEM; goto Abort; }
        *piEAs++;
        pSecData[i].ulMaxSize = 0x0000FFFF;
        pSecData[i].pszName = pBuff2;
        strcpy (pSecData[i].pszName, ".LONGNAME");
        pSecData[i].pbyData = pBuff1;
        pSecData[i].ulFlags = 0;
    }
    pus = (PUSHORT) pSecData[i].pbyData;
    *pus++ = (USHORT) 0xFFFD; /* ASCII */
    *pus = (USHORT) iLen;
    memcpy (pSecData[i].pbyData + 4, pszLongname, (ULONG) iLen);
    pSecData[i].ulSize = (ULONG) iLen + 4;

    return 0;

Abort:
    if (NULL != pBuff1)
        DosFreeMem (pBuff1);
    if (NULL != pBuff2)
        DosFreeMem (pBuff2);
    return iErr;
}



/************************************************
*   fio_IsValid83
************************************************/

BOOL fio_IsValid83 (PSZ pszName)
{
    register int i;

    if (NULL == pszName) return FALSE;

    i = s_NumValid (pszName);
    if (0 == i || i > 8) return FALSE;              /* too long or no legal char */

    pszName += i;
    if (*pszName && *pszName != '.') return FALSE;  /* base followed by something else than '.' */

    i = s_NumValid (++pszName);
    if (i > 3) return FALSE;                        /* ext too long */

    pszName += i;
    if (*pszName) return FALSE;                     /* inv character follows ext. */

    return TRUE;                                    /* legal 8.3 filename */
}




/************************************************
*   s_NumValid
************************************************/

static int s_NumValid (PSZ p)
{
    int i = 0;

    while (*p) {
        if (NULL == strchr ("<>|+=:;,.\"/\\[]", *p) && *p > 0x1F)
            i++;
        else
            break;
        p++;
    }

    return i;
}


/************************************************
*   fio_IsFAT
************************************************/

int fio_IsFAT (PSZ pszPath)
{
    char        szDrive[3];
    char        szBuff[100];
    PFSQBUFFER2 p;
    APIRET      rc;
    ULONG       ulSize = sizeof szBuff;

    if (NULL == pszPath) return -1;

    memcpy (szDrive, pszPath, 2);
    szDrive[2] = (char) 0;

    p = (PFSQBUFFER2)(PVOID) szBuff;
    rc = DosQueryFSAttach (szDrive, 0UL, 1UL, p, &ulSize);
    if (NO_ERROR != rc) return -1;

    if (0 == strcmp ("HPFS", p->szName + p->cbName + 1))
        return 0;
    else
        return 1;
}


/************************************************
*   fio_QueryFS
************************************************/

BOOL fio_QueryFS (int iDrive, PSZ pszName, ULONG ulBuffSize)
{
    char        szBuff[100];
    PFSQBUFFER2 p;
    APIRET      rc;
    ULONG       ulSize = sizeof szBuff;

    p = (PFSQBUFFER2) (PVOID) szBuff;

    rc = DosQueryFSAttach (NULL, (ULONG) iDrive, FSAIL_DRVNUMBER, p, &ulSize);
    if (NO_ERROR != rc) return FALSE;

    strcpy (pszName, p->szName + p->cbName + 1);
    return TRUE;

}



/************************************************
*   fio_AllocEmptyBuffer
************************************************/

PFILEDATAPHYS fio_AllocEmptyBuffer (void)
{
    PFILEDATAPHYS   pfdp;

    if (!(pfdp = mem_HeapAlloc (sizeof (FILEDATAPHYS))))
        return NULL;

    if (!(pfdp->pSecData = (PSECDATA) mem_HeapAlloc (10 * sizeof (SECDATA)))) {
        mem_HeapFree (pfdp);
        return NULL;
    }

    if (NO_ERROR != DosAllocMem ((PVOID)&pfdp->pSecData[0].pbyData, MAXMEMSIZE, PAG_READ|PAG_WRITE)) {
        mem_HeapFree (pfdp->pSecData);
        mem_HeapFree (pfdp);
        return NULL;
    }

    *pfdp->szFilespec = 0;
    *pfdp->szLongname = 0;
    pfdp->fLongNoMatch = FALSE;
    pfdp->fLongShouldMatch = TRUE;
    pfdp->fLongCouldMatch = TRUE;
    pfdp->iEAs = 0;

    pfdp->pSecData[0].pszName = NULL;
    pfdp->pSecData[0].ulSize = 0;
    pfdp->pSecData[0].ulMaxSize = MAXMEMSIZE;
    pfdp->pSecData[0].ulFlags = 0;
    pfdp->pSecData[0].ulPageOffset = 0;
    pfdp->pSecData[0].ulOffset = 0;
    pfdp->pSecData[0].ulSelEnd = 0;

    return pfdp;
}


/************************************************
*   fio_FreeBuffer
************************************************/

void fio_FreeBuffer (PFILEDATAPHYS pfdp)
{
    register int i;

    if (NULL == pfdp) return;
    if (NULL != pfdp->pSecData) {
        for (i = 0; i <= pfdp->iEAs; i++) {
            if (NULL != pfdp->pSecData[i].pszName)
                mem_HeapFree (pfdp->pSecData[i].pszName);
            if (NULL != pfdp->pSecData[i].pbyData)
                DosFreeMem (pfdp->pSecData[i].pbyData);
        }
        DosFreeMem (pfdp->pSecData);
    }
    mem_HeapFree (pfdp);
}


/************************************************
*   s_CheckLongMatch
************************************************/

static void s_CheckLongMatch (PFILEDATAPHYS pfdp)
{
    PSZ     pszPhysname, pszLongname;

    pszPhysname = s_NameFromSpec (pfdp->szFilespec);
    pszLongname = pfdp->szLongname;

    pfdp->fLongNoMatch = pfdp->fLongShouldMatch = pfdp->fLongCouldMatch = FALSE;


    if (0 == strcmp (pszPhysname, pszLongname)) return; /* names match */
    pfdp->fLongNoMatch = TRUE;

    if (!fio_IsValid83 (pszPhysname))  /*  physical name is a Longname */
        pfdp->fLongShouldMatch = TRUE;
    else {  /* phys. is FAT8   */
        if (fio_IsValid83 (pszLongname))  /* LONGNAME is FAT8 */
            pfdp->fLongShouldMatch = TRUE;
        else {    /* LONGNAME is Long */
            if (!fio_IsFAT (pfdp->szFilespec))  /* we're on HPFS */
                pfdp->fLongCouldMatch = TRUE;
            /* otherwise they can't match */
        }
    }
}


/************************************************
*   s_NameFromSpec
************************************************/

static PSZ s_NameFromSpec (PSZ pszSpec)
{
    PSZ     pszName, p;

    if (strlen (pszSpec) < 2UL) return pszSpec;
    if (':' == pszSpec[1])
        pszSpec += 2;

    p = strrchr (pszSpec, '\\');
    if (NULL == p)
        pszName = pszSpec;
    else
        pszName = p + 1;

    return pszName;
}


/************************************************
*   fio_BuildDriveList
************************************************/

int fio_BuildDriveList (PDRIVESPEC pSpec)
{
    int         i;
    APIRET      rc;
    ULONG       ulDriveMap = 0;
    int         iDrives = 0;
    PFSQBUFFER2 pfs;
    char        achBuff[100];
    ULONG       ulBufflen;

    pfs = (PFSQBUFFER2)(PVOID)achBuff;
    rc = DosQueryCurrentDisk ((PVOID)&iDrives, &ulDriveMap); /* iDrives is dummy here */

    iDrives = 0;
    DosError (FERR_DISABLEHARDERR);

    for (i = 0; i < 32; i++) {
        if (ulDriveMap & 1) {
            /* store drive letter as int */
            pSpec->iLetter = i + 'A';

            *pSpec->szName = 0;
            *pSpec->szFileSys = 0;
            *pSpec->szFileSys = 0;

            if (i > 1) {  /* don't get info for floppies. Too slow */
                /* get file system */
                ulBufflen = sizeof achBuff;
                rc = DosQueryFSAttach (NULL, (ULONG) (i+1), FSAIL_DRVNUMBER, pfs, &ulBufflen);
                if (NO_ERROR == rc) {
                    strcpy (pSpec->szName, pfs->szName);
                    strcpy (pSpec->szFileSys, pfs->szName + pfs->cbName + 1);

                    /* get volume name */
                    rc = DosQueryFSInfo ((ULONG) (i+1), FSIL_VOLSER, achBuff, sizeof achBuff);
                    if (NO_ERROR == rc)
                        strcpy (pSpec->szLabel, achBuff + sizeof (ULONG) + sizeof (BYTE));
                }
            }
            iDrives++;
            pSpec++;
        }
        ulDriveMap >>= 1;
    }
    return iDrives;
}


/************************************************
*   fio_QueryDriveInfo
************************************************/

int fio_QueryDriveInfo (int iQDrive, PDRIVESPEC pSpec)
{
    int         i;
    APIRET      rc;
    ULONG       ulDriveMap = 0;
    int         iDrive = 0;
    PFSQBUFFER2 pfs;
    char        achBuff[100];
    ULONG       ulBufflen;

    DosError (FERR_DISABLEHARDERR);
    pfs = (PFSQBUFFER2)(PVOID)achBuff;

    if (iQDrive == 0) {
        rc = DosQueryCurrentDisk ((PVOID)&iDrive, &ulDriveMap);
        if (NO_ERROR != rc) return 0;
    }
    else
        iDrive = iQDrive;

    pSpec->iLetter = iDrive + 'A' - 1;
    *pSpec->szName = 0;
    *pSpec->szFileSys = 0;
    *pSpec->szFileSys = 0;

    /* get file system */
    ulBufflen = sizeof achBuff;
    rc = DosQueryFSAttach (NULL, (ULONG) iDrive, FSAIL_DRVNUMBER, pfs, &ulBufflen);
    if (NO_ERROR == rc) {
        strcpy (pSpec->szName, pfs->szName);
        strcpy (pSpec->szFileSys, pfs->szName + pfs->cbName + 1);

        /* get volume name */
        rc = DosQueryFSInfo ((ULONG) iDrive, FSIL_VOLSER, achBuff, sizeof achBuff);
        if (NO_ERROR == rc)
            strcpy (pSpec->szLabel, achBuff + sizeof (ULONG) + sizeof (BYTE));
    }
    return iDrive;
}


