@echo off

if "2" == "%2" goto level%2
if "3" == "%2" goto level%2
if ""  == "%2" goto level1

echo usage :torture Directory
echo example: torture c8

:level1
del %1\fly.log
call pkt
for %%i in (1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0) do call torture %1 2
call pktrm
goto end

:level2
for %%i in (1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0) do call torture %1 3
goto end

:level3
cd %1
for %%i in (1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0) do .\fly8 z5 oq nt10
cd ..
goto end

:end
