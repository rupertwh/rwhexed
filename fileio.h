#ifndef __FILEIO_H__
#define __FILEIO_H__


BOOL fio_WriteWindowPos (HAB hab, PWINPOS pWinpos);
BOOL fio_ReadWindowPos (HAB hab, PWINPOS pWinpos);
BOOL fio_WriteGlobalSettings (HAB hab, HWND hwndHex);
BOOL fio_ReadFont (HAB hab, PSZ pszFacename, int *pSize);
BOOL fio_ReadColors (HAB hab, PLONG alColors, PLONG plScheme);
void fio_OpenFile (HWND hwnd, PSZ pszFile);
void fio_SaveFile (HAB hab, HWND hwnd, PSZ pszFile, PFILEDATAPHYS pfdp);
void fio_SetIniName (HAB hab, PSZ pszIniName);
const char* fio_QueryIniName (void);
BOOL fio_IsValid83 (PSZ pszName);
int fio_IsFAT (PSZ pszPath);
PFILEDATAPHYS fio_AllocEmptyBuffer (void);
void fio_FreeBuffer (PFILEDATAPHYS pfdp);
int fio_BuildDriveList (PDRIVESPEC pSpec);
int fio_QueryDriveInfo (int iQDrive, PDRIVESPEC pSpec);
int fio_RemoveEA (PSECDATA pSecData, int iEAs, int iEARemove);

#endif

