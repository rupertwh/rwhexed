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

/* Original file date: Dez-21-1996 */

#define INCL_DOS
#define INCL_DOSERRORS


#define HF_STDOUT   1
#define HF_STDERR   2


HFILE s_redir (HFILE hfile);

static char *plog = NULL;


BOOL lg_setup (void)
{
    HFILE   hFile;
}

HFILE s_redir (HFILE hfile)
{
    HPIPE   rpipe, wpipe;

    if (DosCreatePipe (&rpipe, &wpipe, 0))
        return NULLHANDLE;
    if (DosDupHandle (wpipe, &hfile)) {
        DosClose (rpipe);
        DosClose (wpipe);
        return NULLHANDLE;
    }
    DosClose (wpipe);
    return rpipe;
}
