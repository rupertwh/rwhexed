/* Original file date: Apr-1-1997 */

#ifndef PRINT_H
#define PRINT_H

#ifndef FONTS_H
    #include "fonts.h"
#endif

#define TWIPS_FROM_MM(mm)   ((long) (((double)(mm) * 1440.0 / 25.4)) + 0.5)

typedef struct {
    HDC     hdc;
    PSZ     pDriver;
    PSZ     pQueue;
    PDRIVDATA     pdriv;
} PRINTDC;
typedef PRINTDC* PPRINTDC;

typedef struct tag_Printer {
    struct tag_Printer  *pNext;
    char    szDriverName[CCHMAXPATH];
    char    szDriverPrinter[CCHMAXPATH];
    char    szQueueName[CCHMAXPATH];
    char    szDescr[512];
    BOOL    bDefault;
} PRINTER;
typedef PRINTER* PPRINTER;

typedef struct {
    PRINTER     printer;
    char        docname[CCHMAXPATH];
    char        szFacename[FACESIZE];
    LONG        lMatch;
    int         iFontSize;
    LONG        lMaxBaselineExt;
    LONG        lAveCharWidth;
} PRINTDEF;
typedef PRINTDEF* PPRINTDEF;

#ifdef __cplusplus
    extern "C" {
#endif

TID prn_Print (HWND hwnd, HAB hab, PBYTE pData, ULONG ulLen, PPRINTDEF pprintdef);
PPRINTER prn_BuildPrinterList (void);
void prn_FreePrinterList (PPRINTER plist);
PPRINTDC prn_OpenPrinterDC (HAB hab, PSZ pszDriver, PSZ pszQueue, PSZ pszPrinter,
                            BOOL fRaw, BOOL fInfo);
void prn_ClosePrinterDC (PPRINTDC ppdc);
long prn_GetPrintableWidth (HDC hdc);

#ifdef __cplusplus
    }
#endif

#endif
