# debug.mak
# Created by IBM WorkFrame/2 MakeMake at 11:47:41 on 1 April 1997
#
# The actions included in this make file are:
#  Compile::Resource Compiler
#  Compile::C++ Compiler
#  Link::Linker
#  Bind::Resource Bind

.SUFFIXES: .C .RC .obj .res 

.all: \
    g:\source\hexedit\rwhexed.exe

.RC.res:
    @echo " Compile::Resource Compiler "
    rc.exe -r %s %|dpfF.RES

.C.obj:
    @echo " Compile::C++ Compiler "
    icc.exe /DDEBUG /Tdc /Q /Wdcleffenugeniniobsordprorearettrdtruuse /Ti /W2 /Gm /G4 /Fo"%|dpfF.obj" /C %s

g:\source\hexedit\rwhexed.exe: \
    g:\source\hexedit\STATUS.obj \
    g:\source\hexedit\THREAD2.obj \
    g:\source\hexedit\ABOUT.obj \
    g:\source\hexedit\addeawnd.obj \
    g:\source\hexedit\askval.obj \
    g:\source\hexedit\CLIENT.obj \
    g:\source\hexedit\COLORS.obj \
    g:\source\hexedit\DEBUG.obj \
    g:\source\hexedit\FILEIO.obj \
    g:\source\hexedit\FONTDLG.obj \
    g:\source\hexedit\FONTS.obj \
    g:\source\hexedit\GLOBALS.obj \
    g:\source\hexedit\HELPERS.obj \
    g:\source\hexedit\HEWND.obj \
    g:\source\hexedit\INIDLG.obj \
    g:\source\hexedit\INIT.obj \
    g:\source\hexedit\LONGNAME.obj \
    g:\source\hexedit\MAIN.obj \
    g:\source\hexedit\MEMMGR.obj \
    g:\source\hexedit\print.obj \
    g:\source\hexedit\prnstat.obj \
    g:\source\hexedit\prsetup.obj \
    g:\source\hexedit\SEARCH.obj \
    g:\source\hexedit\SETTINGS.obj \
    g:\source\hexedit\RES.res \
    {$(LIB)}DEF.DEF
    @echo " Link::Linker "
    @echo " Bind::Resource Bind "
    icc.exe @<<
     /B" /de /dbgpack /pmtype:pm /nologo"
     /Feg:\source\hexedit\rwhexed.exe 
     DEF.DEF
     g:\source\hexedit\STATUS.obj
     g:\source\hexedit\THREAD2.obj
     g:\source\hexedit\ABOUT.obj
     g:\source\hexedit\addeawnd.obj
     g:\source\hexedit\askval.obj
     g:\source\hexedit\CLIENT.obj
     g:\source\hexedit\COLORS.obj
     g:\source\hexedit\DEBUG.obj
     g:\source\hexedit\FILEIO.obj
     g:\source\hexedit\FONTDLG.obj
     g:\source\hexedit\FONTS.obj
     g:\source\hexedit\GLOBALS.obj
     g:\source\hexedit\HELPERS.obj
     g:\source\hexedit\HEWND.obj
     g:\source\hexedit\INIDLG.obj
     g:\source\hexedit\INIT.obj
     g:\source\hexedit\LONGNAME.obj
     g:\source\hexedit\MAIN.obj
     g:\source\hexedit\MEMMGR.obj
     g:\source\hexedit\print.obj
     g:\source\hexedit\prnstat.obj
     g:\source\hexedit\prsetup.obj
     g:\source\hexedit\SEARCH.obj
     g:\source\hexedit\SETTINGS.obj
<<
    rc.exe g:\source\hexedit\RES.res g:\source\hexedit\rwhexed.exe

g:\source\hexedit\RES.res: \
    g:\source\hexedit\RES.RC \
    {$(INCLUDE)}text.ptr \
    {$(INCLUDE)}icon.ico \
    {$(INCLUDE)}dialog.dlg \
    {$(INCLUDE)}dialog.h \
    {$(INCLUDE)}res.h

g:\source\hexedit\STATUS.obj: \
    g:\source\hexedit\STATUS.C \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}fonts.h \
    {g:\source\hexedit;$(INCLUDE);}status.h

g:\source\hexedit\SETTINGS.obj: \
    g:\source\hexedit\SETTINGS.C \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}fonts.h \
    {g:\source\hexedit;$(INCLUDE);}settings.h \
    {g:\source\hexedit;$(INCLUDE);}colors.h \
    {g:\source\hexedit;$(INCLUDE);}fontdlg.h \
    {g:\source\hexedit;$(INCLUDE);}inidlg.h

g:\source\hexedit\SEARCH.obj: \
    g:\source\hexedit\SEARCH.C \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}search.h

g:\source\hexedit\prsetup.obj: \
    g:\source\hexedit\prsetup.c \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}fonts.h \
    {g:\source\hexedit;$(INCLUDE);}print.h \
    {g:\source\hexedit;$(INCLUDE);}prsetup.h

g:\source\hexedit\prnstat.obj: \
    g:\source\hexedit\prnstat.c \
    {g:\source\hexedit;$(INCLUDE);}wmdefs.h \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}prnstat.h

g:\source\hexedit\print.obj: \
    g:\source\hexedit\print.c \
    {g:\source\hexedit;$(INCLUDE);}wmdefs.h \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}fonts.h \
    {g:\source\hexedit;$(INCLUDE);}print.h \
    {g:\source\hexedit;$(INCLUDE);}prnstat.h

g:\source\hexedit\MEMMGR.obj: \
    g:\source\hexedit\MEMMGR.C \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}debug.h

g:\source\hexedit\MAIN.obj: \
    g:\source\hexedit\MAIN.C \
    {g:\source\hexedit;$(INCLUDE);}wmdefs.h \
    {g:\source\hexedit;$(INCLUDE);}thread2.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}init.h

g:\source\hexedit\LONGNAME.obj: \
    g:\source\hexedit\LONGNAME.C \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}longname.h

g:\source\hexedit\INIT.obj: \
    g:\source\hexedit\INIT.C \
    {g:\source\hexedit;$(INCLUDE);}wmdefs.h \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}thread2.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}fileio.h \
    {g:\source\hexedit;$(INCLUDE);}client.h \
    {g:\source\hexedit;$(INCLUDE);}fonts.h \
    {g:\source\hexedit;$(INCLUDE);}hewnd.h \
    {g:\source\hexedit;$(INCLUDE);}status.h \
    {g:\source\hexedit;$(INCLUDE);}debug.h \
    {g:\source\hexedit;$(INCLUDE);}init.h

g:\source\hexedit\INIDLG.obj: \
    g:\source\hexedit\INIDLG.C \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}fileio.h \
    {g:\source\hexedit;$(INCLUDE);}inidlg.h

g:\source\hexedit\HEWND.obj: \
    g:\source\hexedit\HEWND.C \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}fonts.h \
    {g:\source\hexedit;$(INCLUDE);}hewnd.h \
    {g:\source\hexedit;$(INCLUDE);}debug.h

g:\source\hexedit\HELPERS.obj: \
    g:\source\hexedit\HELPERS.C \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h

g:\source\hexedit\GLOBALS.obj: \
    g:\source\hexedit\GLOBALS.C \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}types.h

g:\source\hexedit\FONTS.obj: \
    g:\source\hexedit\FONTS.C \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}fonts.h

g:\source\hexedit\FONTDLG.obj: \
    g:\source\hexedit\FONTDLG.C \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}fonts.h \
    {g:\source\hexedit;$(INCLUDE);}fontdlg.h

g:\source\hexedit\FILEIO.obj: \
    g:\source\hexedit\FILEIO.C \
    {g:\source\hexedit;$(INCLUDE);}wmdefs.h \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}fileio.h \
    {g:\source\hexedit;$(INCLUDE);}fonts.h \
    {g:\source\hexedit;$(INCLUDE);}hewnd.h \
    {g:\source\hexedit;$(INCLUDE);}debug.h \
    {g:\source\hexedit;$(INCLUDE);}errs.h

g:\source\hexedit\DEBUG.obj: \
    g:\source\hexedit\DEBUG.C \
    {g:\source\hexedit;$(INCLUDE);}debug.h

g:\source\hexedit\COLORS.obj: \
    g:\source\hexedit\COLORS.C \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}fonts.h \
    {g:\source\hexedit;$(INCLUDE);}hewnd.h \
    {g:\source\hexedit;$(INCLUDE);}colors.h

g:\source\hexedit\CLIENT.obj: \
    g:\source\hexedit\CLIENT.C \
    {g:\source\hexedit;$(INCLUDE);}wmdefs.h \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}about.h \
    {g:\source\hexedit;$(INCLUDE);}addeawnd.h \
    {g:\source\hexedit;$(INCLUDE);}askval.h \
    {g:\source\hexedit;$(INCLUDE);}fileio.h \
    {g:\source\hexedit;$(INCLUDE);}client.h \
    {g:\source\hexedit;$(INCLUDE);}fonts.h \
    {g:\source\hexedit;$(INCLUDE);}search.h \
    {g:\source\hexedit;$(INCLUDE);}longname.h \
    {g:\source\hexedit;$(INCLUDE);}settings.h \
    {g:\source\hexedit;$(INCLUDE);}hewnd.h \
    {g:\source\hexedit;$(INCLUDE);}status.h \
    {g:\source\hexedit;$(INCLUDE);}print.h \
    {g:\source\hexedit;$(INCLUDE);}prsetup.h \
    {g:\source\hexedit;$(INCLUDE);}debug.h

g:\source\hexedit\askval.obj: \
    g:\source\hexedit\askval.c \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}askval.h

g:\source\hexedit\addeawnd.obj: \
    g:\source\hexedit\addeawnd.c \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h \
    {g:\source\hexedit;$(INCLUDE);}addeawnd.h

g:\source\hexedit\ABOUT.obj: \
    g:\source\hexedit\ABOUT.C \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}about.h

g:\source\hexedit\THREAD2.obj: \
    g:\source\hexedit\THREAD2.C \
    {g:\source\hexedit;$(INCLUDE);}wmdefs.h \
    {g:\source\hexedit;$(INCLUDE);}res.h \
    {g:\source\hexedit;$(INCLUDE);}helpers.h \
    {g:\source\hexedit;$(INCLUDE);}thread2.h \
    {g:\source\hexedit;$(INCLUDE);}memmgr.h \
    {g:\source\hexedit;$(INCLUDE);}globals.h \
    {g:\source\hexedit;$(INCLUDE);}dialog.h \
    {g:\source\hexedit;$(INCLUDE);}types.h
