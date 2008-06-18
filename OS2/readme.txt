# readme.txt
#
# This is part of the flight simulator 'fly8'.
# Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
#
# OS/2 port by: Michael Taylor (miket@pcug.org.au)

Last Modification: 28th August 1996


To install:

Extract the files into a subdirectory e.g. c:\games\fly8\os2 Add
a new program item in a group or on the desktop (for example -
open the templates group and drag the program template to the
desktop). Set the default directory to be where you have the
fly.ini and other files. (NOTE fly8 has no icon so it will get
the default icon).

Now double click on the icon to run Fly8.

If Fly8 fails to run check the file fly.log in the fly8 directory for
diagnostic information. 


IMPORTANT: keep all the files together otherwise fly8 will not be able to
find its support files.

VIDEO DRIVERS ARE:

GrDive		uses the Dive library for no flicker and better performance
GrGPI		uses standard OS/2 API and flickers
GrBitBlt	uses standard OS/2 API and BitBlt for no flicker

If building this from the source release then the OS2\config.cmd
takes two parameters. Adding a second parameter builds the
Dive version.

NOTE: to use emx you will need to build the mmpm2.a
e.g.
	emxomf -o c:\emx\lib\mmpm2.a d:\toolkit\lib\mmpm2.lib
or
	emxomf -o c:\emx\lib\mmpm2.a d:\ibmcpp\lib\mmpm2.lib
Also add the directory for the OS/2 toolkit (dive.h etc) includes
to C_INCLUDE_PATH. See example setup for EMX below.

Also required: 
To use a joystick you will need the IBM OS/2 joystick drivers. 
To use the emx compiled version you will need to get emxrt.zip for the
emx runtime DLLs.

NOTES: as at 2/Apr/96
The emx/gcc port has network support (udp) but the VisualAge C++ port
does not.

Example script to setup emx

REM
REM EMX 0.9A setup
REM
SET PATH=c:\EMX\BIN;%OLDPATH%
SET DPATH=c:\EMX\BOOK;%OLDDPATH%
rem SET BOOKSHELF=c:\EMX\BOOK;%bookshelf%
set C_INCLUDE_PATH=c:/emx/include;d:/toolkit/h
set LIBRARY_PATH=c:/emx/lib
set CPLUS_INCLUDE_PATH=c:/emx/include/cpp;c:/emx/include
set PROTODIR=c:/emx/include/cpp/gen
set OBJC_INCLUDE_PATH=c:/emx/include
set GCCLOAD=5
set GCCOPT=-pipe
set INFOPATH=c:/emx/info
set EMXBOOK=emxdev.inf+libref.inf+gnudev.inf+bsddev.inf

set HELPNDX=emxbook.ndx

set MANPATH=c:/man
set PAGER=list

set GROFF_FONT_PATH=c:/lib/groff/font
set GROFF_TMAC_PATH=c:/lib/groff/tmac
set REFER=c:/lib/groff/dict/papers/ind


