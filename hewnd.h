#ifndef __HEWND_H__
#define __HEWND_H__

#define WMUSR_HE_SETDATA        (WM_USER + 100)
#define WMUSR_HE_QUERYDATA      (WM_USER + 101)
#define WMUSR_HE_QUERYSOMEDATA  (WM_USER + 102)
#define WMUSR_HE_INSERTDATA     (WM_USER + 103)
#define WMUSR_HE_INSERTEMPTY    (WM_USER + 104)
#define WMUSR_HE_DELETEDATA     (WM_USER + 105)
#define WMUSR_HE_SETFONT        (WM_USER + 106)
#define WMUSR_HE_QUERYFONT      (WM_USER + 107)
#define WMUSR_HE_SETCOLORS      (WM_USER + 108)
#define WMUSR_HE_QUERYCOLORS    (WM_USER + 109)
#define WMUSR_HE_SETPOS         (WM_USER + 110)
#define WMUSR_HE_QUERYPOS       (WM_USER + 111)
#define WMUSR_HE_SEARCH         (WM_USER + 112)
#define WMUSR_HE_SETHSCROLLHWND (WM_USER + 113)
#define WMUSR_HE_SETVSCROLLHWND (WM_USER + 114)
#define WMUSR_HE_SETSTATUSHWND  (WM_USER + 115)
#define WMUSR_HE_COPY           (WM_USER + 116)
#define WMUSR_HE_PASTE          (WM_USER + 117)
#define WMUSR_HE_QUERYCANPASTE  (WM_USER + 118)
#define WMUSR_HE_DEL            (WM_USER + 119)
#define WMUSR_HE_CUT            (WM_USER + 120)


#define HEN_SIZE        0x0001
#define HEN_OFFSET      0x0002
#define HEN_CHANGED     0x0003
#define HEN_SELEND      0x0004


#define SRCH_BACKWARD   0x0010
#define SRCH_WHOLEFILE  0x0020


typedef struct {
    PBYTE   pbData;
    ULONG   ulSize;
    ULONG   ulMaxAlloc;
    ULONG   ulOffset;
    ULONG   ulSelEnd;
    ULONG   ulPageOffset;
} HESETDATA;
typedef HESETDATA* PHESETDATA;


typedef struct {
    LONG    lrgbTextFore;
    LONG    lrgbTextBack;
    LONG    lrgbCursorFore;
    LONG    lrgbCursorBack;
    LONG    lrgbHiliteFore;
    LONG    lrgbHiliteBack;
} HECOLORS;
typedef HECOLORS* PHECOLORS;


BOOL hex_RegisterClass (HAB hab, PSZ pszName);

#endif

