@echo off

:: config.bat
::
:: This is part of the flight simulator 'fly8'.
:: Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).

:: ZZ select config.h source
:: UU selects subdir makefile source
::

if "%1" == "v1" goto v1
if "%1" == "nd" goto nd
if "%1" == "v2" goto v2
echo use CONFIG XX where XX is one of: v1 nd v2
goto end

:v1
set ZZ=v1
set UU=dj
goto ok

:nd
set ZZ=v1
set UU=nd
goto ok

:v2
set ZZ=v2
set UU=dj
goto ok

:ok
if "%2" == "delete" goto delete
if not "%2" == "" goto error

if not exist config.h goto docopy
echo already configured!
echo Hit Ctrl-C to stop now, or hit enter to continue
pause

:docopy
::
:: Fly8 directories
::
copy djgpp\make%1		djgpp\makefile
copy djgpp\make%1.top		makefile
copy djgpp\make%UU%.sha		shapes\makefile
copy djgpp\make%UU%.par		parms\makefile
copy djgpp\make%UU%.uti		utils\makefile
copy djgpp\config%ZZ%.h		config.h
::
:: UDP support
::
::copy common\udp.c		djgpp
::copy common\udpmgr.c		djgpp
::copy common\fly8udp.c		djgpp
::copy common\fly8udp.h		djgpp
copy common\pcudp.h		djgpp
::
:: common files
::
copy common\bgr.c		djgpp
copy common\bgr.h		djgpp
copy common\bgrasm.x		djgpp
copy common\vgr.c		djgpp
copy common\vgr.h		djgpp
copy common\console.c		djgpp
copy common\dosstick.c		djgpp
copy common\grstat.c		djgpp
copy common\misc.c		djgpp
copy common\pcmouse.c		djgpp
copy common\pc8254.h		djgpp
copy common\plsound.c		djgpp
copy common\plsound.h		djgpp
copy common\plspeak.c		djgpp
copy common\plmidi.c		djgpp
copy common\stick.c		djgpp
copy common\stick.h		djgpp
copy common\pctimer.c		djgpp
copy common\plfm.h		djgpp
copy common\plfm.c		djgpp
copy common\pladlib.c		djgpp
copy common\banktool.c		djgpp
goto end


:delete
echo comparing files for "%1"				>config.log
::
:: Fly8 directories
::
fc djgpp\make%1			djgpp\makefile		>>config.log
fc djgpp\make%1.top		makefile		>>config.log
fc djgpp\make%UU%.sha		shapes\makefile		>>config.log
fc djgpp\make%UU%.par		parms\makefile		>>config.log
fc djgpp\make%UU%.uti		utils\makefile		>>config.log
fc djgpp\config%ZZ%.h		config.h		>>config.log
::
:: UDP support
::
::fc common\udp.c		djgpp\*.*		>>config.log
::fc common\udpmgr.c		djgpp\*.*		>>config.log
::fc common\fly8udp.c		djgpp\*.*		>>config.log
::fc common\fly8udp.h		djgpp\*.*		>>config.log
fc common\pcudp.h		djgpp\*.*		>>config.log
::
:: common files
::
fc common\bgr.c			djgpp\*.*		>>config.log
fc common\bgr.h			djgpp\*.*		>>config.log
fc common\bgrasm.x		djgpp\*.*		>>config.log
fc common\vgr.c			djgpp\*.*		>>config.log
fc common\vgr.h			djgpp\*.*		>>config.log
fc common\console.c		djgpp\*.*		>>config.log
fc common\dosstick.c		djgpp\*.*		>>config.log
fc common\grstat.c		djgpp\*.*		>>config.log
fc common\misc.c		djgpp\*.*		>>config.log
fc common\pcmouse.c		djgpp\*.*		>>config.log
fc common\pc8254.h		djgpp\*.*		>>config.log
fc common\plsound.c		djgpp\*.*		>>config.log
fc common\plsound.h		djgpp\*.*		>>config.log
fc common\plspeak.c		djgpp\*.*		>>config.log
fc common\plmidi.c		djgpp\*.*		>>config.log
fc common\stick.c		djgpp\*.*		>>config.log
fc common\stick.h		djgpp\*.*		>>config.log
fc common\pctimer.c		djgpp\*.*		>>config.log
fc common\plfm.c		djgpp\*.*		>>config.log
fc common\plfm.h		djgpp\*.*		>>config.log
fc common\pladlib.c		djgpp\*.*		>>config.log
fc common\banktool.c		djgpp\*.*		>>config.log

type config.log | more
echo hit Ctrl-C to NOT delete files!
pause
pause
pause
del config.log

::
:: Fly8 directories
::
del djgpp\makefile
del makefile
del config.h
del shapes\makefile
del parms\makefile
del utils\makefile
::
:: UDP support
::
::del djgpp\fly8udp.h
::del djgpp\udp.c
::del djgpp\udpmgr.c
::del djgpp\fly8udp.c
del djgpp\pcudp.h
::
:: common files
::
del djgpp\bgr.c
del djgpp\bgr.h
del djgpp\bgrasm.x
del djgpp\vgr.c
del djgpp\vgr.h
del djgpp\console.c
del djgpp\dosstick.c
del djgpp\grstat.c
del djgpp\misc.c
del djgpp\pc8254.h
del djgpp\pcmouse.c
del djgpp\plsound.c
del djgpp\plsound.h
del djgpp\plspeak.c
del djgpp\plmidi.c
del djgpp\stick.c
del djgpp\stick.h
del djgpp\pctimer.c
del djgpp\plfm.c
del djgpp\plfm.h
del djgpp\pladlib.c
del djgpp\banktool.c
goto end

:error
echo Second operand can only be "delete".
goto end

:end
set ZZ=
set UU=
