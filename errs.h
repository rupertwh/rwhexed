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

/* Original file date: Mär-19-1995 */

#define ERR_OPEN        0x00001000
#define ERR_SAVE        0x00002000

#define ERR_DESCMASK    0x00000FFF

#define ERR_OUTOFHEAP   0x00000010
#define ERR_OUTOFADR    0x00000011
#define ERR_OUTOFMEM    0x00000012
#define ERR_OPENFILE    0x00000013
#define ERR_FILEINFO    0x00000014
#define ERR_READFILE    0x00000015
#define ERR_EAINFO      0x00000016
#define ERR_EANAMES     0x00000017
#define ERR_READEAS     0x00000018
#define ERR_WRITEFILE   0x00000019
#define ERR_WRITEEAS    0x0000001A
