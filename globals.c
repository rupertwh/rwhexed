/* Original file date: Jan-29-1995 */

#define __GLOBALS_C__

#include <os2.h>

#include "types.h"
#include "globals.h"



PGLOBALSTRUCT glb_GetPointer (void)
{
    return &g;
}
