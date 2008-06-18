@echo off

:: config.bat
::
:: This is part of the flight simulator 'fly8'.
:: Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).

if "%1" == "qc" goto ok
if "%1" == "c7" goto ok
if "%1" == "c8" goto ok
if "%1" == "b2" goto ok
if "%1" == "b3" goto ok
if "%1" == "b4" goto ok
echo use 'config xx' where xx is one of: qc c7 c8 b2 b3 b4
goto end

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
copy msdos\make%1.top		makefile
copy msdos\make%1		msdos\makefile
copy msdos\config%1.h		config.h
copy msdos\make%1.sha		shapes\makefile
copy msdos\make%1.par		parms\makefile
copy msdos\make%1.uti		utils\makefile
::
:: special msdos support
::
copy ..\serial\make%1		..\serial\makefile
copy ..\net\make%1		..\net\makefile
copy ..\gr\make%1		..\gr\makefile
::
:: UDP support
::
copy common\udp.c		msdos
copy common\udpmgr.c		msdos
copy common\fly8udp.c		msdos
copy common\fly8udp.h		msdos
copy common\pcudp.h		msdos
::
:: Common files
::
copy common\console.c		msdos
copy common\dosstick.c		msdos
copy common\grstat.c		msdos
copy common\misc.c		msdos
copy common\pcmouse.c		msdos
copy common\pc8254.h		msdos
copy common\plsound.c		msdos
copy common\plsound.h		msdos
copy common\plspeak.c		msdos
copy common\plmidi.c		msdos
copy common\stick.c		msdos
copy common\stick.h		msdos
copy common\pctimer.c		msdos
copy common\vgr.c		msdos
copy common\vgr.h		msdos
copy common\plfm.h		msdos
copy common\plfm.c		msdos
copy common\pladlib.c		msdos
copy common\banktool.c		msdos

goto end

:delete
echo comparing files for "%1"				>config.log
::
:: Fly8 directories
::
fc msdos\make%1			msdos\makefile		>>config.log
fc msdos\make%1.top		makefile		>>config.log
fc msdos\config%1.h		config.h		>>config.log
fc msdos\make%1.sha		shapes\makefile		>>config.log
fc msdos\make%1.par		parms\makefile		>>config.log
fc msdos\make%1.uti		utils\makefile		>>config.log
::
:: special msdos support
::
fc ..\serial\make%1		..\serial\makefile	>>config.log
fc ..\net\make%1		..\net\makefile		>>config.log
fc ..\gr\make%1			..\gr\makefile		>>config.log
::
:: UDP support
::
fc common\udp.c			msdos\*.*		>>config.log
fc common\udpmgr.c		msdos\*.*		>>config.log
fc common\fly8udp.c		msdos\*.*		>>config.log
fc common\fly8udp.h		msdos\*.*		>>config.log
fc common\pcudp.h		msdos\*.*		>>config.log
::
:: common files
::
fc common\console.c		msdos\*.*		>>config.log
fc common\dosstick.c		msdos\*.*		>>config.log
fc common\grstat.c		msdos\*.*		>>config.log
fc common\misc.c		msdos\*.*		>>config.log
fc common\pcmouse.c		msdos\*.*		>>config.log
fc common\pc8254.h		msdos\*.*		>>config.log
fc common\plsound.c		msdos\*.*		>>config.log
fc common\plsound.h		msdos\*.*		>>config.log
fc common\plspeak.c		msdos\*.*		>>config.log
fc common\plmidi.c		msdos\*.*		>>config.log
fc common\stick.c		msdos\*.*		>>config.log
fc common\stick.h		msdos\*.*		>>config.log
fc common\pctimer.c		msdos\*.*		>>config.log
fc common\vgr.c			msdos\*.*		>>config.log
fc common\vgr.h			msdos\*.*		>>config.log
fc common\plfm.c		msdos\*.*		>>config.log
fc common\plfm.h		msdos\*.*		>>config.log
fc common\pladlib.c		msdos\*.*		>>config.log
fc common\banktool.c		msdos\*.*		>>config.log

type config.log | more
echo hit Ctrl-C to NOT delete files!
pause
pause
pause
del config.log

::
:: Fly8 directories
::
del msdos\makefile
del makefile
del config.h
del shapes\makefile
del parms\makefile
del utils\makefile
::
:: special msdos support
::
del ..\serial\makefile
del ..\net\makefile
del ..\gr\makefile
::
:: UDP support
::
del msdos\udp.c
del msdos\udpmgr.c
del msdos\fly8udp.c
del msdos\fly8udp.h
del msdos\pcudp.h
::
:: common files
::
del msdos\console.c
del msdos\dosstick.c
del msdos\grstat.c
del msdos\misc.c
del msdos\pcmouse.c
del msdos\pc8254.h
del msdos\plsound.c
del msdos\plsound.h
del msdos\plspeak.c
del msdos\plmidi.c
del msdos\stick.c
del msdos\stick.h
del msdos\pctimer.c
del msdos\vgr.c
del msdos\vgr.h
del msdos\plfm.c
del msdos\plfm.h
del msdos\pladlib.c
del msdos\banktool.c
goto end

:error
echo Second operand can only be "delete".
goto end

:end
