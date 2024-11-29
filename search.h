
#define BACKWARD    0x00000010
#define WHOLEFILE   0x00000020

BOOL srch_CanRepeat (void);
BOOL srch_GetString (PBYTE *ppPattern, int *pnSize, PULONG pulOptions, BOOL bPrompt);

