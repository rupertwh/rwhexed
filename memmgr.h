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

/* Original file date: Mär-16-1995 */

#ifndef __MEMMGR_H__
#define __MEMMGR_H__


BOOL mem_HeapInit (ULONG ulHeapSize);
PVOID mem_HeapAlloc (ULONG ulSize);
void mem_HeapFree (PVOID pObject);
void mem_HeapDestroy (void);
APIRET mem_SetSize (PVOID pBase, ULONG ulOld, ULONG ulNew);


#endif
