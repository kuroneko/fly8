@echo off

:: config.cmd
::
:: This is part of the flight simulator 'fly8'.
:: Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
::
:: configure Fly8 for os/2
::

if "%1" == "va"  goto ok
if "%1" == "ex"  goto ok
echo use 'config xx [dive] [udp] [delete]' where 'xx' is one of: va ex
goto end

:ok
set ZZ=%1
shift
set GG=
set UU=
set LL=copy

:options
if "%1" == "dive" goto o%1
if "%1" == "udp" goto o%1
if "%1" == "delete" goto o%1
if not "%1" == "" goto error
goto %LL%

:odive
set GG=%1
shift
goto options

:oudp
set UU=%1
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
copy os2\make%ZZ%.top		makefile
copy os2\make%ZZ%		os2\makefile
copy os2\config%ZZ%.h		config.h
copy os2\make%ZZ%.sha		shapes\makefile
copy os2\make%ZZ%.par		parms\makefile
copy os2\make%ZZ%.uti		utils\makefile
::
:: some files shared with msdos
::
rem copy msdos\msubs.qc		os2
::
:: Common files
::
set DO=copy
if "%GG%" == "" set DO=rem
%DO% common\bgr.c		os2
%DO% common\bgr.h		os2
%DO% common\bgrasm.x		os2
copy common\grstat.c		os2
copy common\misc.c		os2
copy common\stick.c		os2
copy common\stick.h		os2
copy common\mouse.c		os2
copy common\mouse.h		os2
copy common\plsound.c           os2
copy common\plsound.h           os2
::
:: UDP
::
set DO=copy
if "%UU%" == "" set DO=rem
%DO% common\udp.c		os2
%DO% common\udpmgr.c		os2
%DO% common\fly8udp.c		os2
%DO% common\fly8udp.h		os2

goto end


:delete

echo comparing files for "%ZZ% %GG% %UU%"
echo comparing files for "%ZZ% %GG% %UU%"		 >config.log

set FC=diff -c
set IF=if exist os2
set DO=%FC% common

::
:: Fly8 directories
::
%FC% os2\make%ZZ%		os2\makefile		>>config.log
%FC% os2\make%ZZ%.top		makefile		>>config.log
%FC% os2\config%ZZ%.h		config.h		>>config.log
%FC% os2\make%ZZ%.sha		shapes\makefile		>>config.log
%FC% os2\make%ZZ%.par		parms\makefile		>>config.log
%FC% os2\make%ZZ%.uti		utils\makefile		>>config.log
::
:: Common files
::
%IF%\bgr.c	%DO%\bgr.c	os2\bgr.c	>>config.log
%IF%\bgr.h	%DO%\bgr.h	os2\bgr.h	>>config.log
%IF%\bgrasm.x	%DO%\bgrasm.x	os2\bgrasm.x	>>config.log
%DO%\grstat.c			os2\grstat.c	>>config.log
%DO%\misc.c			os2\misc.c	>>config.log
%DO%\stick.c			os2\stick.c	>>config.log
%DO%\stick.h			os2\stick.h	>>config.log
%DO%\mouse.c			os2\mouse.c	>>config.log
%DO%\mouse.h			os2\mouse.h	>>config.log
%DO%\plsound.c			os2\plsound.c	>>config.log
%DO%\plsound.h			os2\plsound.h	>>config.log
::
:: UDP
::
%IF%\udp.c	%DO%\udp.c	os2\udp.c	>>config.log
%IF%\udpmgr.c	%DO%\udpmgr.c	os2\udpmgr.c	>>config.log
%IF%\fly8udp.c	%DO%\fly8udp.c	os2\fly8udp.c	>>config.log
%IF%\fly8udp.h	%DO%\fly8udp.h	os2\fly8udp.h	>>config.log

type config.log|more
echo hit Ctrl-C to NOT delete files!
pause
pause
pause
del config.log

set DO=del os2

::
:: Fly8 directories
::
del os2\makefile
del makefile
del config.h
del shapes\makefile
del parms\makefile
del utils\makefile
rem del os2\msubs.qc
::
:: Common files
::
%IF%\bgr.c	%DO%\bgr.c
%IF%\bgr.h	%DO%\bgr.h
%IF%\bgrasm.x	%DO%\bgrasm.x
%DO%\grstat.c
%DO%\misc.c
%DO%\stick.c
%DO%\stick.h
%DO%\mouse.c
%DO%\mouse.h
%DO%\plsound.c
%DO%\plsound.h
::
:: UDP
::
%IF%\udp.c	%DO%\udp.c
%IF%\udpmgr.c	%DO%\udpmgr.c
%IF%\fly8udp.c	%DO%\fly8udp.c
%IF%\fly8udp.h	%DO%\fly8udp.h

goto end


:error
echo Usage: os2\config [dive] [udp] [delete]
goto end

:end
set ZZ=
set GG=
set UU=
set LL=
set FC=
set IF=
set DO=
