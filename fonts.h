/* Original file date: Dez-16-1996 */

#ifndef FONTS_H
#define FONTS_H

#ifndef TYPES_H
    #include "types.h"
#endif

#define BFL_NOFIXED     0x00000001
#define BFL_NOVARIABLE  0x00000002
#define BFL_NOGENERIC   0x00000004
#define BFL_NODEVICE    0x00000008
#define BFL_NOBITMAP    0x00000010
#define BFL_NOVECTOR    0x00000020

typedef struct {
    int     iSize;
    LONG    lMatch;
    LONG    lMaxBaselineExt;
    LONG    lAveCharWidth;
/*    struct tag_Fontface *pface; */
} FONTSIZE;
typedef FONTSIZE* PFONTSIZE;

typedef struct tag_Fontface {
    struct tag_Fontface *pNext;
    char        szStyle[FACESIZE];
    char        szFacename[FACESIZE];
    struct tag_Fontfamily   *pfamily;
    int         nSizes;
    FONTSIZE    fsSize[1];
} FONTFACE;
typedef FONTFACE* PFONTFACE;

typedef struct tag_Fontfamily {
    struct tag_Fontfamily   *pNext;
    char    szName[FACESIZE];
    BOOL    fFixed;
    BOOL    fOutline;
    BOOL    fDevice;
    LONG    lHRes;
    LONG    lVRes;
    PFONTFACE   pfirstface;
} FONTFAMILY;
typedef FONTFAMILY* PFONTFAMILY;

#ifdef __cplusplus
    extern "C" {
#endif

BOOL fnt_BuildMonoList (HWND hwnd);
PFONTMETRICS fnt_QueryMetrics (LONG lFont);
LONG fnt_QueryNumFromName (PSZ pszName, int iPoint);
LONG fnt_BuildIntList (PFONT *ppFont);
BOOL fnt_QueryWinFontMetrics (HWND hwnd, PFONTMETRICS pfm);
LONG fnt_CreateFont (HPS hps, LONG lFont, LONG lcid);
PFONTFAMILY fnt_BuildFontList (HPS hps, ULONG ulFlags);
void fnt_FreeFontList (PFONTFAMILY pfam);

#ifdef __cplusplus
    }
#endif


#endif
