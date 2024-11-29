/* Original file date: Dez-16-1996 */

#ifndef TYPES_H
#define TYPES_H


typedef unsigned long   ulong;
typedef unsigned short  ushort;
typedef unsigned char   uchar;
typedef unsigned int    uint;
typedef signed char     schar;

typedef struct {
    LONG    x;
    LONG    y;
    LONG    cx;
    LONG    cy;
    BOOL    bMax;
} WINPOS;
typedef WINPOS* PWINPOS;


typedef struct {
    char    szName[FACESIZE];
    int     nPoint;
    int     nFirstType; /* index of first size of this face */
} FONT;
typedef FONT* PFONT;


typedef struct {
    PSZ     pszName;
    ULONG   ulSize;
    ULONG   ulMaxSize;
    PBYTE   pbyData;
    ULONG   ulFlags;
    ULONG   ulPageOffset;
    ULONG   ulOffset;
    ULONG   ulSelEnd;
} SECDATA;
typedef SECDATA* PSECDATA;

typedef struct {
    char        szFilespec[CCHMAXPATH];
    char        szLongname[CCHMAXPATHCOMP];
    BOOL        fLongNoMatch;
    BOOL        fLongShouldMatch;
    BOOL        fLongCouldMatch;
    int         iEAs;
    PSECDATA    pSecData;
} FILEDATAPHYS;
typedef FILEDATAPHYS* PFILEDATAPHYS;

typedef struct {
    USHORT  usMenuID;
    LONG    lLock;
    ULONG   ulMenuAction;
} MENULOCK;
typedef MENULOCK* PMENULOCK;

typedef struct {
    USHORT  usMenuID;
    BOOL    fEnable;
} MENUENABLE;
typedef MENUENABLE* PMENUENABLE;

typedef struct {
    int     iLetter;
    char    szName[30];
    char    szFileSys[20];
    char    szLabel[20];
} DRIVESPEC;
typedef DRIVESPEC* PDRIVESPEC;



typedef void (*PTHREADFUNC) (HAB hab, ULONG ulArg);

#endif
