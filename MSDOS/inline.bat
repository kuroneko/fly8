@echo off
echo /* inline.h >inline.h
echo  *  >>inline.h
echo  * This is part of the flight simulator 'fly8'. >>inline.h
echo  * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au). >>inline.h
echo  * >>inline.h
echo  * An automatically generated file, do not edit it. >>inline.h
echo */ >>inline.h

:again
if "%1" == "" goto end
sed -n -f inline.sed %1 >>inline.h
shift
goto again

:end
