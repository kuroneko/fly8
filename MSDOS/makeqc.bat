@echo off

:: Must be executed from the main source /fly8 directory

cd ..\serial
nmk all >..\fly8\errs
if errorlevel 1 goto end

cd ..\net
nmk all >>..\fly8\errs
if errorlevel 1 goto end

cd ..\gr
nmk all >>..\fly8\errs
if errorlevel 1 goto end

cd ..\fly8

nmk sys >>errs
if errorlevel 1 goto end

nmk fly >>errs
if errorlevel 1 goto end

nmk parm shape util >>errs
if errorlevel 1 goto end

nmk install >>errs
if errorlevel 1 goto end

goto end


:end
cd ..\fly8
