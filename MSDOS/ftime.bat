@echo off

echo FPROFILE TIME >%1.pcf
echo PROGRAM %1.EXE add >>%1.pcf
echo MAKEPBI %1.PBI >>%1.pcf
echo MAKEPBT %1.PBT >>%1.pcf

prep /FT /P %1.exe /OT %1.pbt /OI %1.pbi
profile %1 %2 %3 %4 %5 %6 %7 %8 %9
prep /IT %1.pbt /IO %1.pbo /OT %1.pbt
plist /ST %1.pbt >%1.out

del %1.pcf
del %1.pbo
del %1.pbi
del %1.pbt

