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

/* Original file date: Dez-1-1996 */

#ifndef DEBUG_H
#define DEBUG_H

#define dbg_Msg(m) dbg_MsgDo (m, __FILE__, __LINE__)

#ifdef __cplusplus
    extern "C" {
#endif

void dbg_MsgDo (char* pszMsg, char* pszFile, int nLine);

#ifdef __cplusplus
    }
#endif


#endif
