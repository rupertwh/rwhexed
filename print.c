#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_PM
#define INCL_DEVERRORS
#define INCL_SPLDOSPRINT

#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "wmdefs.h"
#include "res.h"
#include "types.h"
#include "memmgr.h"
#include "print.h"
#include "fonts.h"
#include "helpers.h"
#include "prnstat.h"
#include "globals.h"


static void _Optlink doprint (PVOID pArg);

struct pageinfo {
        char    *docname;
        int     page;
        int     pages;
        ULONG   firstoff;
};

typedef struct {
        HWND    hwnd;
        HWND    hwndStat;
        HAB     hab;
        PBYTE   pData;
        ULONG   ulLen;
        BOOL    bAbort;
        PRINTDEF   printdef;
} TD_DOPRINT;
typedef TD_DOPRINT* PTD_DOPRINT;

int _Inline s_printercount (PULONG pbytesneeded);
BOOL s_pagedimensions (HPS hps, PSIZEL ppagesize, PLONG prows, PLONG pcols, PFATTRS pfattrs,
                     PRECTL margins, int point, PFONTMETRICS pfm);
BOOL s_printpage (HPS hps, HDC hdc, PSIZEL ppagesize, struct pageinfo *pi,
                  PBYTE pData, ULONG ulLen,
                  PFATTRS pfattrs, PRECTL margins,
                  int point, PFONTMETRICS pfm);
int _Inline s_printchar (int c);
void _Inline s_setfattrs (PFATTRS fattrs, PPRINTDEF printfdef);
static BOOL s_setmargins (HDC hdc, PRECTL margins);
void _Inline s_updatedisplay (HWND hwnd, long page, long totalpages);



TID prn_Print (HWND hwnd, HAB hab, PBYTE pData, ULONG ulLen, PPRINTDEF pprintdef)
{
        TID     tid = (TID) 0;
        PTD_DOPRINT ptd;
        PPRNSTATINIT    ppsi;

        if (ptd = mem_HeapAlloc (sizeof (TD_DOPRINT))) {
                ptd->hwnd = hwnd;
                ptd->hab = hab;
                ptd->pData = pData;
                ptd->ulLen = ulLen;
                ptd->bAbort = FALSE;
                memcpy (&ptd->printdef, pprintdef, sizeof ptd->printdef);
                if (!(ppsi = mem_HeapAlloc (sizeof (PRNSTATINIT)))) {
                        mem_HeapFree (ptd);
                        return 0;
                }
                ppsi->usSize = (USHORT) sizeof (PRNSTATINIT);
                strcpy (ppsi->szDocname, pprintdef->docname);
                strcpy (ppsi->szPrinter, pprintdef->printer.szQueueName);
                ppsi->pAbort = &ptd->bAbort;
                ptd->hwndStat = WinLoadDlg (HWND_DESKTOP, HWND_DESKTOP, PrnStatDlgProc,
                                            g.hResource, IDD_PRINTSTAT,
                                            ppsi);
                if (!ptd->hwndStat) {
                        mem_HeapFree (ppsi);
                        mem_HeapFree (ptd);
                        return 0;
                }

                if (!(tid = _beginthread (doprint, NULL, 0xFFFF, ptd))) {
                        WinPostMsg (ptd->hwndStat, WM_USR_PRINTSTAT, 0, MPFROMLONG (TRUE));
                        mem_HeapFree (ptd);
                }
                else
                        DosSetPriority (PRTYS_THREAD, PRTYC_REGULAR, -1, tid);
        }
        if (0 == tid)
                DosFreeMem (pData);
        return tid;
}



/**********************************************
**
**  doprint
**
**********************************************/

static void _Optlink doprint (PVOID pArg)
{
        HCINFO  *phci = NULL;
        LONG    i, rows, columns, lFontSel, lOut, lpages;
        HAB     hab = NULLHANDLE;
        HPS     hps = NULLHANDLE;
        SIZEL   sl;
        PFONTFAMILY pfam = NULL, pff;
        PFONTFACE   pface;
        PTD_DOPRINT ptd;
        int     linespp, charspl;
        PPRINTDC    ppdc = NULL;
        FATTRS  fattrs;
        USHORT  usJob;
        FONTMETRICS fm;
        RECTL   margins;
        struct pageinfo pi;
        BOOL    bDocStarted = FALSE;

        if (!(hab = WinInitialize (0)))
                goto Abort;

        ptd = (PTD_DOPRINT) pArg;
        if (!(ppdc = prn_OpenPrinterDC (hab, ptd->printdef.printer.szDriverName,
                                   ptd->printdef.printer.szQueueName,
                                   ptd->printdef.printer.szDriverPrinter,
                                   FALSE, FALSE)))
                goto Abort;

        margins.xLeft = TWIPS_FROM_MM (20);
        margins.yTop = TWIPS_FROM_MM (10);
        margins.xRight = TWIPS_FROM_MM (10);
        margins.yBottom = TWIPS_FROM_MM (20);

        if (!s_setmargins (ppdc->hdc, &margins))
                goto Abort;

        sl.cx = sl.cy = 0;
        if (!(hps = GpiCreatePS (hab, ppdc->hdc, &sl,
                    PU_TWIPS | GPIF_LONG | GPIT_NORMAL | GPIA_ASSOC)))
                goto Abort;

        s_setfattrs (&fattrs, &ptd->printdef);
        if (DEV_OK != DevEscape (ppdc->hdc, DEVESC_STARTDOC,
                                 strlen (ptd->printdef.docname),
                                 ptd->printdef.docname, NULL, NULL)) {
                hlp_printPMError (hab);
                goto Abort;
        }
        bDocStarted = TRUE;
        if (!s_pagedimensions (hps, &sl, &rows, &columns, &fattrs, &margins,
                               ptd->printdef.iFontSize, &fm))
                goto Abort;

        lpages = (((ptd->ulLen + 15) >> 4) + (rows-4) - 1) / (rows-4);
        pi.docname = ptd->printdef.docname;
        pi.pages = lpages;
        for (i = 0; i < lpages; i++) {
                pi.page = i + 1;
                pi.firstoff = i * ((rows - 4) << 4);
                s_updatedisplay (ptd->hwndStat, pi.page, lpages);
                if (ptd->bAbort || !s_printpage (hps, ppdc->hdc, &sl, &pi,
                                  ptd->pData + pi.firstoff,
                                  min (ptd->ulLen - pi.firstoff, ((rows - 4) << 4)),
                                  &fattrs, &margins,
                                  ptd->printdef.iFontSize, &fm)) {
                        goto Abort;
                }
        }
        lOut = 2;
        DevEscape (ppdc->hdc, DEVESC_ENDDOC, 0, NULL, &lOut, (PVOID)&usJob);
        GpiDestroyPS (hps);

        prn_ClosePrinterDC (ppdc);
        DosFreeMem (ptd->pData);
        WinPostMsg (ptd->hwndStat, WM_USR_PRINTSTAT, 0, MPFROMLONG (TRUE));
        WinPostMsg (ptd->hwnd, WM_USR_PRINTDONE, MPFROMLONG (hlp_gettid), 0);
        return;

    Abort:
        if (bDocStarted)
                DevEscape (ppdc->hdc, DEVESC_ABORTDOC, 0, NULL, NULL, NULL);
        if (hps)
                GpiDestroyPS (hps);
        if (ppdc)
                prn_ClosePrinterDC (ppdc);
        if (hab)
                WinTerminate (hab);
        DosFreeMem (ptd->pData);
        WinPostMsg (ptd->hwndStat, WM_USR_PRINTSTAT, 0, MPFROMLONG (TRUE));
        WinPostMsg (ptd->hwnd, WM_USR_PRINTDONE, MPFROMLONG (hlp_gettid), 0);
}


void _Inline s_updatedisplay (HWND hwnd, long page, long totalpages)
{
        PPRNSTAT        ps;

        if (!(ps = mem_HeapAlloc (sizeof (PRNSTAT))))
                return;

        ps->page = page;
        ps->totalpages = totalpages;

        WinPostMsg (hwnd, WM_USR_PRINTSTAT, MPFROMP (ps), MPFROMLONG (FALSE));
}

/**********************************************
**
**  s_setmargins
**
**********************************************/

static BOOL s_setmargins (HDC hdc, PRECTL margins)
{
        LONG    formn, i;
        PHCINFO info;
        BOOL    ret = FALSE;

        formn = DevQueryHardcopyCaps (hdc, 0, 0, NULL);
        if (!(info = (PHCINFO) mem_HeapAlloc (formn * sizeof (HCINFO))))
                return FALSE;

        formn = DevQueryHardcopyCaps (hdc, 0, formn, info);
        for (i = 0; i < formn; i++) {
                if (HCAPS_CURRENT & info[i].flAttributes) {
                            margins->xLeft -= TWIPS_FROM_MM (info[i].xLeftClip);
                            margins->xLeft = max (margins->xLeft, 0);

                            margins->xRight -= TWIPS_FROM_MM (info[i].cx - info[i].xRightClip);
                            margins->xRight = max (margins->xRight, 0);

                            margins->yTop -= TWIPS_FROM_MM (info[i].cy - info[i].yTopClip);
                            margins->yTop = max (margins->yTop, 0);

                            margins->yBottom -= TWIPS_FROM_MM (info[i].yBottomClip);
                            margins->yBottom = max (margins->yBottom, 0);

                            ret = TRUE;
                            break;
                }
        }
        mem_HeapFree (info);
        return ret;
}



/**********************************************
**
**  prn_GetPrintableWidth
**
**********************************************/

long prn_GetPrintableWidth (HDC hdc)
{
        LONG    formn, i;
        PHCINFO info;
        long    width = 0;

        formn = DevQueryHardcopyCaps (hdc, 0, 0, NULL);
        if (!(info = (PHCINFO) mem_HeapAlloc (formn * sizeof (HCINFO))))
                return 0;

        formn = DevQueryHardcopyCaps (hdc, 0, formn, info);
        for (i = 0; i < formn; i++) {
                if (HCAPS_CURRENT & info[i].flAttributes) {
                        width = TWIPS_FROM_MM (info[i].cx
                                               - max (20, info[i].xLeftClip)
                                               - max (10, (info[i].cx - info[i].xRightClip)));

                        break;
                }
        }
        mem_HeapFree (info);
        return width;
}

/**********************************************
**
**  s_setfattrs
**
**********************************************/

void _Inline s_setfattrs (PFATTRS fattrs, PPRINTDEF printdef)
{
    memset (fattrs, 0, sizeof (FATTRS));
    fattrs->usRecordLength = sizeof (FATTRS);
    fattrs->fsSelection = 0;
    fattrs->lMatch = printdef->lMatch;
    strcpy (fattrs->szFacename, printdef->szFacename);
    fattrs->idRegistry = 0;
    fattrs->usCodePage = 0;
    fattrs->lMaxBaselineExt = printdef->lMaxBaselineExt;
    fattrs->lAveCharWidth = printdef->lAveCharWidth;
    fattrs->fsType = 0;
    fattrs->fsFontUse = FATTR_FONTUSE_NOMIX;
}

/**********************************************
**
**  s_printpage
**
**********************************************/

BOOL s_printpage (HPS hps, HDC hdc, PSIZEL ppagesize, struct pageinfo *pi,
                  PBYTE pData, ULONG ulLen,
                  PFATTRS pfattrs, PRECTL margins,
                  int point, PFONTMETRICS pfm)
{
    SIZEF   boxsize;
    LONG    cxChar, cyChar, cyDesc, i;
    char    buff[90], *p;
    POINTL  pos;
    LONG    savedps;

    savedps = GpiSavePS (hps);
    if (GpiCreateLogFont (hps, NULL, 1, pfattrs) == GPI_ERROR)
        return FALSE;
    GpiSetCharSet (hps, 1);
    boxsize.cx = (point << 16) / 10 * 20;
    boxsize.cy = boxsize.cx;
    GpiSetCharBox (hps, &boxsize);
    cxChar = pfm->lAveCharWidth;
    cyChar = pfm->lEmHeight * 120 / 100;
    cyDesc = pfm->lMaxDescender;

    pos.x = margins->xLeft;
    pos.y = ppagesize->cy - margins->yTop - cyChar + cyDesc;
    GpiCharStringAt (hps, &pos, strlen (pi->docname), pi->docname);
    pos.y -= 2 * cyChar;
    while (ulLen) {
        p = buff + sprintf (buff, "%08lX: ", pi->firstoff);
        pi->firstoff += 16;
        for (i = 0; i < 16; i++) {
            if (i < ulLen)
                p += sprintf (p, "%02X ", (int) pData[i]);
            else
                p += sprintf (p, "   ");
            if (3 == (3 & i))
                *p++ = ' ';
        }
        for (i = 0; i < 16; i++) {
            if (i < ulLen)
                *p++ = s_printchar (pData[i]);
        }
        *p = 0;
        pData += 16;
        ulLen -= min (16, ulLen);
        GpiCharStringAt (hps, &pos, strlen (buff), buff);
        pos.y -= cyChar;
    }
    pos.y = margins->yBottom;
    sprintf (buff, "Page %d of %d", pi->page, pi->pages);
    GpiCharStringAt (hps, &pos, strlen (buff), buff);

    GpiSetCharSet (hps, 0);
    GpiDeleteSetId (hps, 1);
    GpiRestorePS (hps, savedps);
    if (pi->page < pi->pages)
        DevEscape (hdc, DEVESC_NEWFRAME, 0, NULL, NULL, NULL);
    return TRUE;
}



/**********************************************
**
**  s_pagedimensions
**
**********************************************/

BOOL s_pagedimensions (HPS hps, PSIZEL ppagesize, PLONG prows, PLONG pcols, PFATTRS pfattrs,
                     PRECTL margins, int point, PFONTMETRICS pfm)
{
    SIZEF   boxsize;

    memset (ppagesize, 0, sizeof (SIZEL));
    GpiQueryPS (hps, ppagesize);
    if (GpiCreateLogFont (hps, NULL, 2, pfattrs) == GPI_ERROR)
        return FALSE;
    GpiSetCharSet (hps, 2);
    boxsize.cx = (point << 16) / 10 * 20;
    boxsize.cy = boxsize.cx;
    GpiSetCharBox (hps, &boxsize);

    GpiQueryFontMetrics (hps, sizeof (FONTMETRICS), pfm);

    *prows = (ppagesize->cy - (margins->yTop + margins->yBottom))/ (pfm->lEmHeight * 120 / 100);
    *pcols = (ppagesize->cx - (margins->xLeft + margins->xRight))/ pfm->lAveCharWidth;
    GpiSetCharSet (hps, 0);
    GpiDeleteSetId (hps, 2);
}







/**********************************************
**
**  prn_BuildPrinterList
**
**********************************************/

PPRINTER prn_BuildPrinterList (void)
{
    ULONG   l1, l2, l3, size = 0;
    SPLERR  splret;
    int     printers, i;
    PRQINFO3    *info;
    PPRINTER    printer;
    char        *p;

    if (-1 == (printers = s_printercount (&size)))
        return NULL;

    if (!(info = (PRQINFO3*) mem_HeapAlloc (size)))
        return NULL;

    splret = SplEnumQueue (NULL, 3, info, size, &l1, &l2, &l3, NULL);
    if (splret == ERROR_MORE_DATA || splret == NERR_BufTooSmall) {
        printf ("Hmmm. Buffer too small. A printer might have been added while"
                " I was doing the query. Try again.\n");
        mem_HeapFree (info);
        return NULL;
    }
    if (!(printer = mem_HeapAlloc (printers * sizeof (PRINTER)))) {
        printf ("Out of heap memory :-( \n");
        mem_HeapFree (info);
        return NULL;
    }

    for (i = 0; i < printers; i++) {
        if (p = strchr (info[i].pszDriverName, '.'))
            strncpy (printer[i].szDriverName, info[i].pszDriverName, p - info[i].pszDriverName);
        else
            strcpy (printer[i].szDriverName, info[i].pszDriverName);
        if (info[i].pDriverData && *info[i].pDriverData->szDeviceName)
            strcpy (printer[i].szDriverPrinter, info[i].pDriverData->szDeviceName);
        strcpy (printer[i].szQueueName, info[i].pszName);
        if (info[i].fsType & PRQ3_TYPE_APPDEFAULT)
            printer[i].bDefault = TRUE;
        strcpy (printer[i].szDescr, info[i].pszComment);
        if (!(i == printers - 1))
            printer[i].pNext = &printer[i+1];
    }
    mem_HeapFree (info);
    return printer;
}



/**********************************************
**
**  prn_FreePrinterList
**
**********************************************/

void prn_FreePrinterList (PPRINTER plist)
{
    if (plist)
       mem_HeapFree (plist);
}



/**********************************************
**
**  prn_OpenPrinterDC
**
**********************************************/

PPRINTDC prn_OpenPrinterDC (HAB hab, PSZ pszDriver, PSZ pszQueue, PSZ pszPrinter, BOOL fRaw, BOOL fInfo)
{
        PPRINTDC    ppdc = NULL;
        DEVOPENSTRUC    dop;

        if (!(ppdc = mem_HeapAlloc (sizeof (PRINTDC)))) goto Abort;

        if (!(ppdc->pDriver = mem_HeapAlloc (strlen (pszDriver) + 1))) goto Abort;
        strcpy (ppdc->pDriver, pszDriver);

        if (!(ppdc->pQueue = mem_HeapAlloc (strlen (pszQueue) + 1))) goto Abort;
        strcpy (ppdc->pQueue, pszQueue);

        if (pszPrinter && *pszPrinter) {

                if (!(ppdc->pdriv = mem_HeapAlloc (sizeof (DRIVDATA)))) goto Abort;

                dop.pdriv = ppdc->pdriv;
                strcpy (dop.pdriv->szDeviceName, pszPrinter);
                dop.pdriv->cb = sizeof (DRIVDATA);
                dop.pdriv->lVersion = 0;
                *dop.pdriv->abGeneralData = 0;
        }
        else
                dop.pdriv = NULL;

        dop.pszDriverName = ppdc->pDriver;
        dop.pszLogAddress = ppdc->pQueue;
        dop.pszDataType = fRaw ? "PM_Q_RAW" : "PM_Q_STD";
        ppdc->hdc = DevOpenDC (hab, fInfo ? OD_INFO : OD_QUEUED, "*", 4,
                               (PDEVOPENDATA) &dop, 0);

        return ppdc;

Abort:
        if (ppdc) {
                if (ppdc->pdriv) mem_HeapFree (ppdc->pdriv);
                if (ppdc->pQueue) mem_HeapFree (ppdc->pQueue);
                if (ppdc->pDriver) mem_HeapFree (ppdc->pDriver);
                mem_HeapFree (ppdc);
        }
        return NULL;
}



/**********************************************
**
**  prn_ClosePrinterDC
**
**********************************************/

void prn_ClosePrinterDC (PPRINTDC ppdc)
{
    if (ppdc) {
        DevCloseDC (ppdc->hdc);
        if (ppdc->pdriv) mem_HeapFree (ppdc->pdriv);
        if (ppdc->pQueue) mem_HeapFree (ppdc->pQueue);
        if (ppdc->pDriver) mem_HeapFree (ppdc->pDriver);
        mem_HeapFree (ppdc);
    }
}



/**********************************************************************
**
**  LOCAL FUNCTIONS
**
**********************************************************************/

/**********************************************
*
*  s_printercount
*
**********************************************/

int _Inline s_printercount (PULONG pbytesneeded)
{
    ULONG   infolevel = 3, numentries = 0, totalentries = 0;
    SPLERR  splret;

    splret = SplEnumQueue (NULL, infolevel, NULL, 0, &numentries, &totalentries,
                           pbytesneeded,  0);
    if (splret == ERROR_MORE_DATA || splret == NERR_BufTooSmall)
        return (int) totalentries;
    else
        return -1;
}

#ifdef NEVER
    char    *pbuff, *p;
    int     iRet = -1;
    ULONG   ulSize = 4096L;

    if (!DosAllocMem ((PVOID)&pbuff, ulSize, PAG_COMMIT|PAG_READ|PAG_WRITE)) {
        if (PrfQueryProfileData (HINI_PROFILE, "PM_SPOOLER_PRINTER", NULL, pbuff, &ulSize)) {
            iRet = 0;
            p = pbuff;
            while (1) {
                if (*p)
                    iRet++; /* found printer name */
                else
                    break; /* end of list */
                while (*p++); /* skip this printer name */
            }
        }
        if (ppnames)
            *ppnames = pbuff;
        else
            DosFreeMem (pbuff);
    }
    return iRet;
}
#endif


/**********************************************
*
*  s_printchar
*
**********************************************/

int _Inline s_printchar (int c)
{
    static char pc[] = ".\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
                       "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
                       " !\"#$%&'()*+,-./0123456789:;<=>?"
                       "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                       "`abcdefghijklmnopqrstuvwxyz{|}~"
                       "ÄÅÇÉÑÖÜáàâäãåçéèêëíìîïñóòôöõúùûü"
                       "†°¢£§•¶ß®©™´¨≠ÆØ∞±≤≥¥µ∂∑∏π∫ªºΩæø"
                       "¿¡¬√ƒ≈∆«»… ÀÃÕŒœ–—“”‘’÷◊ÿŸ⁄€‹›ﬁﬂ"
                       "‡·‚„‰ÂÊÁËÈÍÎÏÌÓÔÒÚÛÙıˆ˜¯˘˙˚¸˝\xfe.";
    return pc[c];
}




