
typedef struct {
        USHORT  usSize;
        char    szDocname[CCHMAXPATH];
        char    szPrinter[32];
        BOOL    *pAbort;
} PRNSTATINIT;
typedef PRNSTATINIT* PPRNSTATINIT;

typedef struct {
        int     page;
        int     totalpages;
} PRNSTAT;
typedef PRNSTAT* PPRNSTAT;

MRESULT APIENTRY PrnStatDlgProc (HWND hwnd, ULONG ulMsg, MPARAM mp1, MPARAM mp2);


