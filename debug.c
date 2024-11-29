/* Original file date: Dez-1-1996 */

#include <stdio.h>
#include <string.h>

#include "debug.h"

void dbg_MsgDo (char* pszMsg, char* pszFile, int nLine)
{
    char*   p;

    if (p = strrchr (pszFile, '\\'))
        p++;
    else
        p = pszFile;

    fprintf (stdout, "%.13s: Line %d: %s\n", p, nLine, pszMsg);
}
