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

/* Original file date: Sep-16-1995 */

#define WMUSR_ST_SETSIZE        (WM_USER + 200)
#define WMUSR_ST_SETOFFSET      (WM_USER + 201)
#define WMUSR_ST_SETSEL         (WM_USER + 202)
#define WMUSR_ST_SETLONGNAME    (WM_USER + 203)
#define WMUSR_ST_SETACTIVE      (WM_USER + 204)
#define WMUSR_ST_QUERYOPTHIGHT  (WM_USER + 205)

BOOL st_RegisterClass (HAB hab, PSZ pszName);
