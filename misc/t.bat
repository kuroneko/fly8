@echo off
call pkt
del fly.log

:: default options
::
if "" == "%1" set ZZ=z5

.\fly8 %ZZ% %1 %2 %3 %4 %5 %6 %7 %8 %9
set ZZ=
call pktrm
