@echo off

if "%OS%" == "Windows_NT" goto nt

call qc
nmk -f msdos\makeqc.top backup 
call xqc
goto end

:nt
call vc
nmake -f mswin\makenv.top backup 
goto end


:end
