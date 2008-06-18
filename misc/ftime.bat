@echo off

::
:: Use this for sampling.
::

prep /FS /P %1.exe /OT %1.pbt /OI %1.pbi
profile /S 100 %1 %2 %3 %4 %5 %6 %7 %8 %9
prep /IT %1.pbt /IO %1.pbo /OT %1.pbt
plist /SC %1.pbt >%1.out

del %1.pbo
del %1.pbi
del %1.pbt
goto end

::
:: Use this for timing.
::

prep /FT /P %1.exe /OT %1.pbt /OI %1.pbi
profile %1 %2 %3 %4 %5 %6 %7 %8 %9
prep /IT %1.pbt /IO %1.pbo /OT %1.pbt
plist /ST %1.pbt >%1.out

:end
