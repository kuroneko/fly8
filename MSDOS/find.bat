@echo off
:: ---------------------------------- find.bat -------------------------------

:: This is part of the flight simulator 'fly8'.
:: Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).

:: Find an address in the link map. used when a divide-overflow is reported
:: in the log file.

if not exist fly8.map goto err
if "%1" == "" goto auto
if "%2" == "" goto using

awk -f find.awk AD=%1 MAIN=%2 <fly8.map 
goto end

:err
echo missing map file 'fly8.map'
goto end

:auto
sed -n "s/^+++1 /call find /;/^call /p" fly.log >temp.bat
call temp.bat
del temp.bat
goto end

:using
echo usage: find ffff:ffff mmmm:mmmm
goto end

:end
