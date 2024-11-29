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
