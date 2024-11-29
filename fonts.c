#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_GPI
#define INCL_WIN

#include <os2.h>
#include <string.h>
#include <stdlib.h>
#ifdef DEBUG
    #include <stdio.h>
#endif

#include "types.h"
#include "fonts.h"
#include "memmgr.h"
#include "globals.h"


static BOOL s_IsSameFamily (PFONTFAMILY pff, PFONTMETRICS pfm);
static LONG s_CountFaces (PFONTFAMILY pff, PFONTMETRICS pfm, LONG lFonts);
PFONTFAMILY s_MakeFontList (PFONTMETRICS pfmlist, LONG lFonts,
                            LONG lHRes, LONG lVRes, ULONG ulFlags);
static void s_FontToFace (PFONTFACE pface, PFONTMETRICS pfm);

void s_SortSizes (PFONTFACE pface);
void s_SwapSizes (PFONTSIZE pa, PFONTSIZE pb);

static PFONTMETRICS afmMono;
static LONG         lMonoFonts;
static PFONTMETRICS pfmHelv8;



/********************************************
*
*   fnt_BuildMonoList
*
********************************************/

BOOL fnt_BuildMonoList (HWND hwnd)
{
    HPS             hps;
    HDC             hdc;
    PFONTMETRICS    pfm = NULL;
    LONG            lReq = 0, lNum, lHorzDpi = 0, lVertDpi = 0, i;
    ULONG           ulSize, ulFinalSize;

    if (NULL != afmMono)
        DosFreeMem (afmMono);

    afmMono = NULL;
    lMonoFonts = 0;

    hps = WinGetPS (hwnd);
    hdc = GpiQueryDevice (hps);

    DevQueryCaps (hdc, CAPS_HORIZONTAL_FONT_RES, 1, &lHorzDpi);
    DevQueryCaps (hdc, CAPS_VERTICAL_FONT_RES, 1, &lVertDpi);

    lNum = GpiQueryFonts (hps, QF_PUBLIC, NULL, &lReq, 0, NULL);

    ulSize = (ULONG) lNum * sizeof (FONTMETRICS);
    if (NO_ERROR != DosAllocMem ((PVOID)&pfm, ulSize, PAG_COMMIT | PAG_READ | PAG_WRITE)) goto Abort;

    if (NO_ERROR != DosAllocMem ((PVOID)&afmMono, ulSize, PAG_COMMIT | PAG_READ | PAG_WRITE)) goto Abort;

    lReq = lNum;
    lNum = GpiQueryFonts (hps, QF_PUBLIC, NULL, &lReq, sizeof (FONTMETRICS), pfm);

    WinReleasePS (hps);

    for (i = 0; i < lReq; i++) {
        if ((lHorzDpi != (LONG) pfm[i].sXDeviceRes) ||
                (lVertDpi != (LONG) pfm[i].sYDeviceRes) ||
                (FM_DEFN_OUTLINE & pfm[i].fsDefn) ||
                (pfm[i].lAveCharWidth != pfm[i].lMaxCharInc))
            continue;

        /* got fixed bitmap font with correct dpi (96 vs 120) */
        memcpy (&afmMono[lMonoFonts++], &pfm[i], sizeof (FONTMETRICS));
    }

    /* get Helv8 font */
    for (i = 0; i < lReq; i++) {
        if ((lHorzDpi != (LONG) pfm[i].sXDeviceRes) ||
                (lVertDpi != (LONG) pfm[i].sYDeviceRes) ||
                (80 !=  pfm[i].sNominalPointSize) ||
                (strcmp ("Helv", pfm[i].szFacename)))
            continue;
        pfmHelv8 = &afmMono[lMonoFonts];
        memcpy (pfmHelv8, &pfm[i], sizeof (FONTMETRICS));
        break;
    }

    DosFreeMem (pfm); pfm = NULL;
    if (0 == lMonoFonts) goto Abort;

    ulFinalSize = (ULONG) (lMonoFonts + 1) * sizeof (FONTMETRICS);
    ulSize = (ulSize + 0x00000FFF) & 0xFFFFF000;
    ulFinalSize = (ulFinalSize + 0x00000FFF) & 0xFFFFF000;
    DosSetMem (afmMono + ulFinalSize, ulSize - ulFinalSize, PAG_DECOMMIT);
    return TRUE;

Abort:
    if (NULL != afmMono) {
        DosFreeMem (afmMono); afmMono = NULL;
    }
    if (NULL != pfm)
        DosFreeMem (pfm);
    pfmHelv8 = NULL;
    lMonoFonts = 0;

    return FALSE;

}


/********************************************
*
*   fnt_QueryMetrics
*
********************************************/

PFONTMETRICS fnt_QueryMetrics (LONG lFont)
{
    if (NULL == afmMono || lFont < -1 || lFont >= lMonoFonts)
        return NULL;

    if (-1 == lFont) /* Helv 8 */
        return pfmHelv8;

    return &afmMono[lFont];
}


/********************************************
*
*   fnt_QueryNumFromName
*
********************************************/

LONG fnt_QueryNumFromName (PSZ pszName, int iPoint)
{
    LONG    l;

    iPoint *= 10;

    for (l = 0; l < lMonoFonts; l++) {
        if ((int)afmMono[l].sNominalPointSize == iPoint &&
                    0 == strcmp (pszName, afmMono[l].szFacename))
            return l;
    }
    return -2;
}



/********************************************
*
*   fnt_BuildIntList
*
********************************************/

LONG fnt_BuildIntList (PFONT *ppFont)
{
    char            szCurrFace[FACESIZE];
    LONG            i, nFirstIndex;

    if (0 == lMonoFonts) return 0;

    if (NULL != *ppFont)
        DosFreeMem (*ppFont);
    *ppFont = NULL;

    if (NO_ERROR != DosAllocMem ((PVOID)ppFont, (ULONG) lMonoFonts * sizeof (FONT),
        PAG_COMMIT|PAG_READ|PAG_WRITE)) goto Abort;

    *szCurrFace = 0;
    for (i = 0; i < lMonoFonts; i++) {
        if (strcmp (szCurrFace, afmMono[i].szFacename)) {  /* new font */
            strcpy (szCurrFace, afmMono[i].szFacename);
            nFirstIndex = i;
        }

        strcpy ((*ppFont)[i].szName, afmMono[i].szFacename);
        (*ppFont)[i].nPoint = (int) afmMono[i].sNominalPointSize / 10;
        (*ppFont)[i].nFirstType = nFirstIndex;
    }
    return lMonoFonts;

Abort:
    if (NULL != *ppFont) {
        DosFreeMem (*ppFont);
        *ppFont = NULL;
    }
    return 0L;
}



/********************************************
*
*   fnt_QueryWinFontMetrics
*
********************************************/

BOOL fnt_QueryWinFontMetrics (HWND hwnd, PFONTMETRICS pfm)
{
    char    szBuff[FACESIZE + 4];
    char    *pFace;
    ULONG   ulFound = 0;
    HPS     hps = NULLHANDLE;
    LONG    lReq, lNum = 0;
    int     i, iPoint;
    PFONTMETRICS    afm = NULL;

    WinQueryPresParam (hwnd, PP_FONTNAMESIZE, 0, &ulFound, sizeof szBuff, (PVOID) &szBuff, 0);
    if (PP_FONTNAMESIZE != ulFound) {
        hps = WinGetPS (hwnd);
        GpiQueryFontMetrics (hps, sizeof (FONTMETRICS), pfm);
        WinReleasePS (hps);
        return TRUE;
    }

    pFace = strchr (szBuff, '.') + 1;
    if ((char*)1 == pFace) goto Abort;
    iPoint = atoi (szBuff);

    hps = WinGetPS (hwnd);
    lReq = GpiQueryFonts (hps, QF_PUBLIC, pFace, &lNum, 0, NULL);
    if (0 == lReq || GPI_ALTERROR == lReq) goto Abort;

    afm = (PFONTMETRICS) mem_HeapAlloc ((ULONG)lReq * sizeof (FONTMETRICS));
    if (NULL == afm) goto Abort;

    GpiQueryFonts (hps, QF_PUBLIC, pFace, &lReq, sizeof (FONTMETRICS), afm);
    WinReleasePS (hps); hps = NULLHANDLE;
    iPoint *= 10;
    for (i = 0; i < lReq; i++) {
        if (iPoint == (int) afm[i].sNominalPointSize) {
            memcpy (pfm, &afm[i], sizeof (FONTMETRICS));
            break;
        }
    }
    if (i == lReq) goto Abort;

    mem_HeapFree (afm);
    return TRUE;


Abort:
    if (NULL != afm)
        mem_HeapFree (afm);
    if (NULLHANDLE != hps)
        WinReleasePS (hps);

    return FALSE;
}




/********************************************
*
*   fnt_CreateFont
*
********************************************/

LONG fnt_CreateFont (HPS hps, LONG lFont, LONG lcid)
{
    FATTRS          fat;
    PFONTMETRICS    pfm;

    if (lFont < -1 || lFont >= lMonoFonts) return GPI_ERROR;

    if (-1 == lFont)
        pfm = pfmHelv8;
    else
        pfm = &afmMono[lFont];

    memset (&fat, 0, sizeof fat);

    fat.usRecordLength = sizeof fat;
    fat.lMatch = pfm->lMatch;
    fat.idRegistry = pfm->idRegistry;
    fat.lMaxBaselineExt = pfm->lMaxBaselineExt;
    fat.lAveCharWidth = pfm->lAveCharWidth;
    fat.fsFontUse = FATTR_FONTUSE_NOMIX;
    strcpy (fat.szFacename, pfm->szFacename);

    return GpiCreateLogFont (hps, NULL, lcid, &fat);
}




/**********************************************
**
**  fnt_BuildFontList
**
**********************************************/

PFONTFAMILY fnt_BuildFontList (HPS hps, ULONG ulFlags)
{
    PFONTFAMILY pfamlist = NULL;
    LONG        lFonts, i = 0;
    PFONTMETRICS    pfm = NULL;
    HDC         hdc;
    LONG        lHorzDpi = 0, lVertDpi = 0;

    lFonts = GpiQueryFonts (hps, QF_PUBLIC, NULL, &i, sizeof (FONTMETRICS), NULL);
    if (NO_ERROR == DosAllocMem ((PVOID)&pfm, lFonts * sizeof (FONTMETRICS),
                                      PAG_COMMIT|PAG_READ|PAG_WRITE)) {
        GpiQueryFonts (hps, QF_PUBLIC, NULL, &lFonts, sizeof (FONTMETRICS), pfm);
        hdc = GpiQueryDevice (hps);
        if (hdc != HDC_ERROR && hdc) {
            DevQueryCaps (hdc, CAPS_HORIZONTAL_FONT_RES, 1, &lHorzDpi);
            DevQueryCaps (hdc, CAPS_VERTICAL_FONT_RES, 1, &lVertDpi);
        }
        pfamlist = s_MakeFontList (pfm, lFonts, lHorzDpi, lVertDpi, ulFlags);
        DosFreeMem (pfm);
    }
    return pfamlist;
}



/**********************************************
**
**  fnt_FreeFontList
**
**********************************************/

void fnt_FreeFontList (PFONTFAMILY pfam)
{
    PFONTFAMILY pff;

    pff = pfam;
    while (pff) {
        if (pff->pfirstface)
            mem_HeapFree (pff->pfirstface);
        pff = pff->pNext;
    }
    DosFreeMem (pfam);
}



/**********************************************
**
**  s_MakeFontList
**
**********************************************/

PFONTFAMILY s_MakeFontList (PFONTMETRICS pfmlist, LONG lFonts,
                            LONG lHRes, LONG lVRes, ULONG ulFlags)
{
    LONG        i, j, lFaces;
    PFONTFAMILY pfam, pfirstfam = NULL;
    PFONTFACE   pface, pfirstface;
    PFONTMETRICS    pfm;
    BOOL        *afDone;
    FONTFACE    face;

    afDone = mem_HeapAlloc (lFonts * sizeof (BOOL));
    memset (afDone, 0, lFonts * sizeof (BOOL));

    /* exclude bitmap fonts that don't fit resolution as well as those exluded
    ** through ulFlags:
    */
    if (lHRes && lVRes) {
        for (i = 0; i < lFonts; i++) {
            if (!(FM_DEFN_OUTLINE & pfmlist[i].fsDefn) &&
                (lHRes != pfmlist[i].sXDeviceRes || lVRes != pfmlist[i].sYDeviceRes))
                { afDone[i] = TRUE; continue; }
            if ((BFL_NOFIXED & ulFlags) && (FM_TYPE_FIXED & pfmlist[i].fsType))
                { afDone[i] = TRUE; continue; }
            if ((BFL_NOVARIABLE & ulFlags) && !(FM_TYPE_FIXED & pfmlist[i].fsType))
                { afDone[i] = TRUE; continue; }
            if ((BFL_NOGENERIC & ulFlags) && (FM_DEFN_GENERIC & pfmlist[i].fsDefn))
                { afDone[i] = TRUE; continue; }
            if ((BFL_NODEVICE & ulFlags) && !(FM_DEFN_GENERIC & pfmlist[i].fsDefn))
                { afDone[i] = TRUE; continue; }
            if ((BFL_NOBITMAP & ulFlags) && !(FM_DEFN_OUTLINE & pfmlist[i].fsDefn))
                { afDone[i] = TRUE; continue; }
            if ((BFL_NOVECTOR & ulFlags) && (FM_DEFN_OUTLINE & pfmlist[i].fsDefn))
                { afDone[i] = TRUE; continue; }
        }
    }

    /* Build linked list of FONTFAMILY structs, one for every font.
    ** Each FONTFAMILY points to a linked list of FONTFACEs, one for every style.
    ** If it's a bitmap font, FONTFACE contains an array of FONTSIZEs, one for every available size.
    ** If it's an outline font, the array has two elements: the minimum and maximum point size.
    */
    if (!DosAllocMem ((PVOID)&pfirstfam, lFonts * sizeof (FONTFAMILY), PAG_COMMIT|PAG_READ|PAG_WRITE)) {
        pfam = pfirstfam;
        i = 0;
        while (i < lFonts && afDone[i]) i++; /* look for next font we haven'T dealt with */
        while (i < lFonts) {
            /* Copy data for a new font to FONTFAMILY:
            */
            pfm = &pfmlist[i];
            strcpy (pfam->szName, pfm->szFamilyname);
            if (FM_TYPE_FIXED & pfm->fsType)
                pfam->fFixed = TRUE;
            else
                pfam->fFixed = FALSE;
            if (FM_DEFN_OUTLINE & pfm->fsDefn)
                pfam->fOutline = TRUE;
            else
                pfam->fOutline = FALSE;
            if (FM_DEFN_GENERIC & pfm->fsDefn)
                pfam->fDevice = FALSE;
            else
                pfam->fDevice = TRUE;
            pfam->lHRes = pfm->sXDeviceRes;
            pfam->lVRes = pfm->sYDeviceRes;
            /* Enumerate all styles for that font into FONTFACEs:
            */
            lFaces = s_CountFaces (pfam, pfmlist, lFonts);
            pfam->pfirstface = mem_HeapAlloc (lFaces * (0xFFFFFFFC & (sizeof (FONTFACE) + sizeof (FONTSIZE) + 3)));
            pface = pfam->pfirstface;
            while (1) {
                afDone[i] = TRUE;
                s_FontToFace (pface, pfm);
                pface->pfamily = pfam;
                strcpy (pface->szFacename, pfm->szFacename);
                pface->nSizes = 0;
                if (!pfam->fOutline) {
                    /* It's a bitmap font, so we have to look for all available sizes:
                    */
                    pface->fsSize[0].iSize = pfm->sNominalPointSize;
                    pface->fsSize[0].lMatch = pfm->lMatch;
                    pface->nSizes++;
                    j = i;
                    while (1) {
                        j++;
                        while (afDone[j] && j < lFonts) j++;
                        if (j == lFonts) break;
                        if (s_IsSameFamily (pfam, &pfmlist[j])) {
                            s_FontToFace (&face, &pfmlist[j]);
                            if (!strcmp (pface->szStyle, face.szStyle)) {
                                pface->fsSize[pface->nSizes].iSize = pfmlist[j].sNominalPointSize;
                                pface->fsSize[pface->nSizes].lMatch = pfmlist[j].lMatch;
                                pface->fsSize[pface->nSizes].lMaxBaselineExt = pfmlist[j].lMaxBaselineExt;
                                pface->fsSize[pface->nSizes].lAveCharWidth = pfmlist[j].lAveCharWidth;
                                pface->nSizes++;
                                afDone[j] = TRUE;
                            }
                        }
                    }
                    s_SortSizes (pface);
                }
                else {
                    /* It's an outline font, so we just store min and max size:
                    */
                    pface->nSizes = 2;
                    pface->fsSize[0].iSize = pfm->sMinimumPointSize;
                    pface->fsSize[1].iSize = pfm->sMaximumPointSize;
                    pface->fsSize[0].lMatch = pface->fsSize[1].lMatch = pfm->lMatch;
                }

                pface->pNext = (PFONTFACE)
                                (0xFFFFFFFC & ((ULONG)pface + sizeof (FONTFACE) +
                                 (pface->nSizes - 1) * sizeof pface->fsSize[0] + 0x00000003));
                i++;
                /* look for next font of same family we haven't dealt with yet:
                */
                while (1) {
                    while (afDone[i] && i < lFonts ) i++;
                    if (i == lFonts) break;
                    if (s_IsSameFamily (pfam, &pfmlist[i]))
                        break;
                    i++;
                }
                if (i == lFonts) {
                    pface->pNext = NULL;
                    break; /* we're done with this font */
                }
                pface = pface->pNext;
                pfm = &pfmlist[i];
            }
            i = 0;
            while (i < lFonts && afDone[i]) i++; /* look for next font we haven'T dealt with */
            if (i == lFonts) {
                pfam->pNext = NULL;
                break; /* we're done */
            }
            pfam->pNext = (PFONTFAMILY) (0xFFFFFFFC & ((ULONG)pfam + sizeof (FONTFAMILY) + 0x00000003));
            pfam = pfam->pNext;
        }
    }
#ifdef DEBUG
    else
        printf ("Can't allocate mem!\n");
#endif

    mem_HeapFree (afDone);
    return pfirstfam;
}



/**********************************************
**
**  s_CountFaces
**
**********************************************/

LONG s_CountFaces (PFONTFAMILY pff, PFONTMETRICS pfm, LONG lFonts)
{
    LONG    l;
    LONG    lFaces = 0;

    for (l = 0; l < lFonts; l++) {
        if (s_IsSameFamily (pff, &pfm[l]))
            lFaces++;
    }
    return lFaces;
}



/**********************************************
**
**  s_IsSameFamily
**
**********************************************/

BOOL s_IsSameFamily (PFONTFAMILY pff, PFONTMETRICS pfm)
{
    if (strcmp (pff->szName, pfm->szFamilyname))
        return FALSE;
    if ((pff->fFixed && !(FM_TYPE_FIXED & pfm->fsType)) ||
        (!pff->fFixed && (FM_TYPE_FIXED & pfm->fsType)))
        return FALSE;

    if ((pff->fOutline && !(FM_DEFN_OUTLINE & pfm->fsDefn)) ||
        (!pff->fOutline && (FM_DEFN_OUTLINE & pfm->fsDefn)))
        return FALSE;

    if ((pff->fDevice && (FM_DEFN_GENERIC & pfm->fsDefn)) ||
        (!pff->fDevice && !(FM_DEFN_GENERIC & pfm->fsDefn)))
        return FALSE;

    if (!pff->fOutline && ((pff->lHRes != pfm->sXDeviceRes) || (pff->lVRes != pfm->sYDeviceRes)))
        return FALSE;

    return TRUE;
}



/**********************************************
**
**  s_FontToFace
**
**********************************************/

void s_FontToFace (PFONTFACE pface, PFONTMETRICS pfm)
{
    char    *p, *p1;

    p = pfm->szFacename;
    p1 = pfm->szFamilyname;

    while (*p && *p1 && *p == *p1) {
        p++; p1++;
    }

    while (*p == ' ' || *p == '-' || *p == '.')
        p++;
    strcpy (pface->szStyle, p);
}



/**********************************************
**
**  s_SortSizes
**
**********************************************/

void s_SortSizes (PFONTFACE pface)
{
    int i,j;

    i = 0;
    while (i < pface->nSizes - 1) {
        j = i;
        while ((j < pface->nSizes - 1) &&
               (pface->fsSize[j].iSize > pface->fsSize[j+1].iSize)) {
            s_SwapSizes (&pface->fsSize[j], &pface->fsSize[j+1]);
            j++;
        }
        if (j == i)
            i++;
    }
}



/**********************************************
**
**  s_SwapSizes
**
**********************************************/

void s_SwapSizes (PFONTSIZE pa, PFONTSIZE pb)
{
    FONTSIZE    fs;

    memcpy (&fs, pa, sizeof fs);
    memcpy (pa, pb, sizeof fs);
    memcpy (pb, &fs, sizeof fs);
}





