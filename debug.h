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
