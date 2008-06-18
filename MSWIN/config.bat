@echo off

:: config.bat
::
:: This is part of the flight simulator 'fly8'.
:: Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
::
:: configure Fly8 for mswin (3.1 and nt)
::

for %%c in (vc gw mingw lcc) do if "%1" == "%%c" goto o%1
goto error

:ovc
:: compiler
set ZC=v
set DZZ=MSDOS\makec8
set BGRASM=c
goto ok

:ogw
:: compiler
set ZC=g
set DZZ=UNIX\makelx
set BGRASM=x
goto ok

:omingw
:: compiler
set ZC=m
set DZZ=MSWIN\makenm
set BGRASM=x
goto ok

:olcc
:: compiler
set ZC=l
set DZZ=MSWIN\makenl
set BGRASM=x
goto ok

:ok
:: system
set ZS=w
shift
set OG=
set OU=
:: action
set LL=copy

:options
for %%c in (nt wing ddraw udp delete) do if "%1" == "%%c" goto o%1
if not "%1" == "" goto error

set BGRASM=bgrasm.%BGRASM%

set ZZ=%ZS%%ZC%

goto %LL%

:ont
set ZS=n
shift
goto options

:owing
set OG=%1
shift
goto options

:oddraw
set OG=%1
shift
goto options

:oudp
set OU=%1
shift
goto options

:odelete
set LL=%1
shift
goto options

:copy
if not exist config.h goto docopy
echo already configured!
echo Hit Ctrl-C to stop now, or hit enter to continue
pause

:docopy
::
:: Fly8 directories
::
copy MSWIN\make%ZZ%.top		Makefile
copy MSWIN\make%ZZ%		MSWIN\Makefile
copy MSWIN\make%ZZ%.uti		utils\Makefile
copy MSWIN\config%ZZ%.h		config.h
::
:: some files shared with msdos
::
copy %DZZ%.sha			shapes\Makefile
copy %DZZ%.par			parms\Makefile
if "wv" == "%ZZ%" copy MSDOS\msubs.qc		mswin
::
:: Common files
::
set DO=copy
if "%OG%" == "" set DO=rem
%DO% common\bgr.c		mswin
%DO% common\bgr.h		mswin
%DO% common\%BGRASM%		mswin
%DO% common\vgr.c		mswin
%DO% common\vgr.h		mswin
if "w" == "%ZS%" copy common\dosstick.c		mswin
copy common\grstat.c		mswin
copy common\misc.c		mswin
copy common\mouse.c		mswin
copy common\mouse.h		mswin
copy common\plsound.c		mswin
copy common\plsound.h		mswin
copy common\stick.c		mswin
copy common\stick.h		mswin
::
:: UDP
::
set DO=copy
if "%OU%" == "" set DO=rem
%DO% common\udp.c		mswin
%DO% common\udpmgr.c		mswin
%DO% common\fly8udp.c		mswin
%DO% common\fly8udp.h		mswin

goto end


:delete

echo comparing files for "%ZZ% %OG% %OU%"
echo comparing files for "%ZZ% %OG% %OU%"		 >config.log

set IF=if exist mswin
set DO=fc common

::
:: Fly8 directories
::
fc MSWIN\make%ZZ%		MSWIN\Makefile		>>config.log
fc MSWIN\make%ZZ%.top		Makefile		>>config.log
fc MSWIN\make%ZZ%.uti		utils\Makefile		>>config.log
fc MSWIN\config%ZZ%.h		config.h		>>config.log
::
:: some files shared with msdos
::
fc %DZZ%.sha			shapes\Makefile		>>config.log
fc %DZZ%.par			parms\Makefile		>>config.log
if "wv" == "%ZZ%" fc/b MSDOS\msubs.qc MSWIN\msubs.qc	>>config.log
::
:: Common files
::
%IF%\bgr.c	%DO%\bgr.c	MSWIN\*.*		>>config.log
%IF%\bgr.h	%DO%\bgr.h	MSWIN\*.*		>>config.log
%IF%\%BGRASM%	%DO%\%BGRASM%	MSWIN\*.*		>>config.log
%IF%\vgr.c	%DO%\vgr.c	MSWIN\*.*		>>config.log
%IF%\vgr.h	%DO%\vgr.h	MSWIN\*.*		>>config.log
if "w" == "%ZS%" %DO%\dosstick.c MSWIN\*.*		>>config.log
%DO%\grstat.c			MSWIN\*.*		>>config.log
%DO%\misc.c			MSWIN\*.*		>>config.log
%DO%\mouse.c			MSWIN\*.*		>>config.log
%DO%\mouse.h			MSWIN\*.*		>>config.log
%DO%\plsound.c			MSWIN\*.*		>>config.log
%DO%\plsound.h			MSWIN\*.*		>>config.log
%DO%\stick.c			MSWIN\*.*		>>config.log
%DO%\stick.h			MSWIN\*.*		>>config.log
::
:: UDP
::
%IF%\udp.c	%DO%\udp.c	MSWIN\*.*		>>config.log
%IF%\udpmgr.c	%DO%\udpmgr.c	MSWIN\*.*		>>config.log
%IF%\fly8udp.c	%DO%\fly8udp.c	MSWIN\*.*		>>config.log
%IF%\fly8udp.h	%DO%\fly8udp.h	MSWIN\*.*		>>config.log

type config.log|more
echo hit Ctrl-C to NOT delete files!
pause
pause
pause
del config.log

set DO=del mswin

::
:: Fly8 directories
::
del Makefile
del config.h
%DO%\Makefile
::
:: some files shared with msdos
::
del shapes\Makefile
del parms\Makefile
del utils\Makefile
%IF%\msubs.qc	%DO%\msubs.qc
::
:: Common files
::
%IF%\bgr.c	%DO%\bgr.c
%IF%\bgr.h	%DO%\bgr.h
%IF%\%BGRASM%	%DO%\%BGRASM%
%IF%\vgr.c	%DO%\vgr.c
%IF%\vgr.h	%DO%\vgr.h
if "w" == "%ZS%" %DO%\dosstick.c
%DO%\grstat.c
%DO%\misc.c
%DO%\mouse.c
%DO%\mouse.h
%DO%\plsound.c
%DO%\plsound.h
%DO%\stick.c
%DO%\stick.h
::
:: UDP
::
%IF%\udp.c	%DO%\udp.c
%IF%\udpmgr.c	%DO%\udpmgr.c
%IF%\fly8udp.c	%DO%\fly8udp.c
%IF%\fly8udp.h	%DO%\fly8udp.h

goto end


:error
echo Usage: MSWIN\config cc [nt] [wing] [ddraw] [udp] [delete]
echo cc:     compiler is: vc gw mingw lcc (gw/mingw/lcc only on nt)
echo nt:     build for nt [default is vc on win 3.1]
echo wing:   enable WinG support
echo wing:   enable DirectDraw support (NT/95)
echo udp:    enable UDP support using winsock
echo delete: undo the configuration
goto end

:end
set ZS=
set ZC=
set ZZ=
set OG=
set OU=
set LL=
set IF=
set DO=
set BGRASM=
set DZZ=
