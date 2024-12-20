/*
 *  This file is part of RW Hex Editor (RWHEXED).
 *
 *  Copyright (c) 1994-1997, Rupert Weber.
 *
 *  RWHEXED is free software: you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation, either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  RWHEXED is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with RWHEXED. If not, see <https://www.gnu.org/licenses/>.
 */

/* Original file date: Apr-4-1997 */

#ifndef HELPERS_H
#define HELPERS_H

#define HLP_MSGLEVEL_INF        1
#define HLP_MSGLEVEL_WRN        2
#define HLP_MSGLEVEL_ERR        3

void hlp_init_hextable (void);
TID hlp_gettid (void);
void hlp_Init (HMODULE hResourcex);
PSZ hlp_GetString (HAB hab, ULONG ulID);
void hlp_FreeString (PSZ pszString);
void hlp_MsgRes (HAB hab, ULONG ulID, int nLevel);
void hlp_Msg (HAB hab, PSZ pszMsg, int nLevel);
void hlp_MsgResDos (HAB hab, ULONG ulID, APIRET rc, int nLevel);
void hlp_MsgDos (HAB hab, PSZ pszMsg, APIRET rc, int nLevel);
void hlp_DlgNicePos (HWND hwndDlg);
BOOL hlp_AscToInt (char *pStr, int *pInt);
HWND hlp_RealOwner (HWND hwndKid);
void hlp_ComboSelect (HWND hwnd, LONG lSel);
int hlp_printPMError (HAB hab);

#define RED(l)      (((l)>>16)&0x000000FF)
#define GREEN(l)    (((l)>>8)&0x000000FF)
#define BLUE(l)     ((l)&0x000000FF)

#define RGBFROMVALS(r,g,b)  ( (((r)<<16)&0xFF0000L) | (((g)<<8)&0xFF00L) | ((b)&0xFFL) )

#define VALSFROMRGB(rgb,r,g,b) r=RED(rgb);g=GREEN(rgb);b=BLUE(rgb)

#define FLAGON(v,f) (v = v | (f))
#define FLAGOFF(v,f) (v = v & ~(f))

#ifndef HELPERS_C
        extern int hextable[256];
#endif

#endif
