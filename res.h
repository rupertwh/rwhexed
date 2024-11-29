/* Original file date: Dez-15-1996 */

#ifndef __RES_H__
#define __RES_H__

#include "dialog.h"

#define ID_FRAMERES 1
#define ID_CARET    10

#define M_FILE      0x0010
#define MI_OPEN     0x0011
#define MI_SAVE     0x0012
#define MI_SAVEAS   0x0013
#define MI_CLOSE    0x0014
#define MI_NEW      0x0015
#define MI_PRINT    0x0016
#define MI_EXIT     0x001D

#define M_MODIFY    0x0020
#define MI_INSERT   0x0021
#define MI_DELETE   0x0022
#define MI_SEARCH   0x0023
#define MI_GOTO     0x0024
#define MI_COPY     0x0025
#define MI_PASTE    0x0026
#define MI_REPEAT   0x0027
#define MI_CUT      0x0028
#define MI_CLEAR    0x0029
#define MI_ADDEA    0x002A

#define M_SETTINGS  0x0030
#define MI_PREFS    0x0031

#define M_VIEW      0x0040
#define MI_DATA     0x0041
#define M_EAS       0x0042
#define MI_FIRSTEA  0x1000

#define M_HELP      0x00F0
#define MI_ABOUT    0x00F1


#define MI_ALL      0xFFFF



#define M_SYS       0x0100

#define IDS_APPTITLE    0x00001000
#define IDS_MODULENAME  0x00001001
#define IDS_CLIENTCLASS 0x00001002
#define IDS_HEXCLASS    0x00001003
#define IDS_STATUSCLASS 0x00001004
#define IDS_UNTITLED    0x00001005

#define IDS_INISYSAPP       0x00001010
#define IDS_INISYSKEY       0x00001011
#define IDS_INIAPPWIN       0x00001012
#define IDS_INIKEYWINPOS    0x00001013
#define IDS_DEFININAME      0x00001014
#define IDS_INIAPPFONT      0x00001015
#define IDS_INIKEYFNAME     0x00001016
#define IDS_INIKEYFSIZE     0x00001017
#define IDS_INIAPPCOLOR     0x00001018
#define IDS_INIKEYSCHEME    0x00001019

#define IDS_INIAPPPRINT     0x00001020
#define IDS_INIKEYPDEVICE   0x00001021
#define IDS_INIKEYPQUEUE    0x00001022
#define IDS_INIKEYPPRINTER  0x00001023
#define IDS_INIKEYPFORM     0x00001024

#define IDS_COLNAME         0x00001030
/*  reserved up to 0x0000103F  */

#define IDS_STAT_SIZE       0x00001040
#define IDS_STAT_OFFSET     0x00001041
#define IDS_STAT_SELEND     0x00001042
#define IDS_STAT_LONGNAME   0x00001043
#define IDS_STAT_ACTIVE     0x00001044

#define IDS_ERROR       0x00002000
#define IDS_WARNING     0x00002001
#define IDS_INFO        0x00002002

#define IDS_ERR_NOT2WINDOW          0x00002010
#define IDS_ERR_CANTWRITEPRF        0x00002011

#define IDS_ERR_DOS_CANTCREATET2    0x00002030
#define IDS_ERR_DOS_CANTCREATESEM   0x00002031
#define IDS_ERR_DOS_WAITSEM         0x00002032


#endif
