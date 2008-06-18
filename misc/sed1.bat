@echo off
if not exist %1.sed goto err
if not exist %2     goto err
if "" == "%3"       goto err
sed -f %1.sed <%2 >%3
goto end

:err
echo usage: sed1 CommandFile InFile OutFile
goto end

:end
