@echo off

:: build.bat
::
:: This is part of the flight simulator 'fly8'.
:: Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).

:: This batch file will process the definition files of Fly8 into simple
:: numeric form.
::
:: You must have "cpp.exe" and 'awk.exe' available in your path!

if "" == "%1" goto err
if "" == "%2" goto err
if not "" == "%4" goto err

if "prm" == "%2" goto prm
if "nav" == "%2" goto nav
if "lnd" == "%2" goto lnd
goto err

:prm
set ZZC=prc
goto doit

:nav
set ZZC=nac
goto doit

:lnd
set ZZC=lnc
goto doit

:doit
if not exist %1.%ZZC% goto fileerr
cpp %1.%ZZC% >%1.zzz
if errorlevel 1 goto cpperr
awk -f expr.awk %1.zzz %1.%2
if errorlevel 1 goto awkerr
if "" == "%3" goto end
copy %1.%2 %3
if errorlevel 1 goto copyerr
rem unfortunately the previous test does not work for the copy command...
goto end

:err
echo use "build Name Type [Dir]"
echo     Name does NOT have an extension.
echo     Type is "prm", "nav" or "lnd".
echo     Dir is optional directory to copy result into.
goto end

:fileerr
echo file "%1.%ZZC%" not found.
goto end

:cpperr
echo basic error(s) found.
goto end

:awkerr
echo expression syntax error(s) found.
goto end

:copyerr
echo copy of "%1.%2" to directory "%3" failed.
goto end

:end
if exist %1.zzz del %1.zzz
set ZZC=
