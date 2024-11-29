#ifndef GLOBALS_H
#define GLOBALS_H

#ifndef TYPES_H
#include "types.h"
#endif

#define MAXMEMSIZE  0x3200000L

#define OFF_TEXT    0
#define OFF_BACK    1
#define OFF_HITEXT  2
#define OFF_HIBACK  3
#define OFF_RED     4
#define NUMCOLS     6

#define MNUM_FILE   6
#define MNUM_EDIT   10
#define MNUM_VIEW   2
#define MNUM_SYS    1
#define MNUM_HELP   1

typedef struct {
    HMQ     hmq;
    HMQ     hmqT2;
    HMODULE hResource;
    HWND    hwndFrame;
    HWND    hwndClient;
    HWND    hwndMenuBar;
    TID     tidT2;
    HEV     hevT2Ready;
    PFONT   pFont;
    int     nFonts;
    int     nCurrFont;
} GLOBALSTRUCT;
typedef GLOBALSTRUCT* PGLOBALSTRUCT;


#define MA_OPEN     0x00000001
#define MA_SAVE     0x00000002
#define MA_SAVEAS   0x00000004
#define MA_CLOSE    0x00000008
#define MA_EXIT     0x00000010
#define MA_NEW      0x00000020

#define MA_CUT      0x00000040
#define MA_COPY     0x00000080
#define MA_PASTE    0x00000100
#define MA_CLEAR    0x00000200
#define MA_INSERT   0x00000400
#define MA_DELETE   0x00000800
#define MA_SEARCH   0x00001000
#define MA_GOTO     0x00002000
#define MA_REPEAT   0x00004000
#define MA_ADDEA    0x00008000

#define MA_DATA     0x00010000
#define MA_EA       0x00020000

#define MA_ABOUT    0x00040000

#define MA_SYSCLOSE 0x00080000



#ifdef __GLOBALS_C__
    GLOBALSTRUCT g;
#else
    extern GLOBALSTRUCT g;
#endif

PGLOBALSTRUCT glb_GetPointer (void);


#endif
