@echo off
for %%i in (LOG PID OBJECTS SHOW REMOTE DEBUG PLAYER LOOP) do call sed1 %%i
for %%i in (VIEWS COMMAND NETMGR INFO TERM MEMORY) do call sed1 %%i
for %%i in (MSDOS\PCSERIAL MSDOS\SLIP MSDOS\PACKET) do call sed1 %%i
for %%i in (MSDOS\PCUDP UNIX\FIFO COMMON\GRSTAT COMMON\UDP) do call sed1 %%i
