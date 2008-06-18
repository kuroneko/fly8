# readme.txt
#
# This is part of the flight simulator 'fly8'.
# Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
#
# MS Windows part by: Michael Taylor (miket@pcug.org.au)
# Win95/Nt4 DirectDraw & Joystick support by:
#		Chris Collins (ccollins@pcug.org.au)

Last Modification: 21 February 1998


To install:

Extract the files into a subdirectory e.g. c:\games\fly8\wg 
Add a new program item in the program manager in the GAMES group. 
Set the default directory to be where you have the fly.ini and other files.
(NOTE fly8 has no icon so it will get the default icon, however you can
      use the fly8.ico file for an icon if you wish).

Now double click on the icon to run Fly8. (or use the file manager to
run it).

If Fly8 fails to run check the file fly.log in the fly8 directory for
diagnostic information. 


IMPORTANT: keep all the files together otherwise fly8 will not be able to
find its support files.

VIDEO DRIVERS ARE:

GrWinG		uses the WinG library for no flicker and better performance.
		Fastest for NT/95.
GrvWinG		as above with improved drawing speed.
		Fastest for 3.1
GrGDI		uses standard Windows API and flickers.
GrBitBlt	uses standard WIndows API and BitBlt for no flicker.
		Slowest.
GrDDraw		(Windows 95/NT4 ONLY) uses Microsoft DirectDraw
		(from DirectX 3) for full screen, Hi-res video. It is also
		flicker-free.

Win32 Joystick Driver:

Technical explanation:
	The Fly8 joystick driver exists in two parts, a High-Level and
Low-level part.  The High-Level part refers generically to "Pointer"
devices, such as Joysticks, Mice or a Keyboard, where the Low-level part
is being called specifically by the High-Level driver for the Joystick to
get the joystick's status.  In Windows 3.1, the old DOS driver might work.
In Windows NT, the old DOS driver does not work, and can not work due to the
nature of Windows NT.  To work around this, a new low-level driver was writen
to use the Multimedia Joystick driver present in Windows 95 & NT4.
	The new Win32 joystick Driver works under Windows 95, and should
work under Windows NT4 if a suitable Joystick driver is installed. The
driver reads the options string and uses that to determine what the joystick
driver should try to emulate - the options you pass specify what the
high-level part expects, and what the low-level part tries to emulate.

"Huh, I didn't understand a word of that" explanation:
	the options that are passed to the Fly8 Joystick driver function
differently under Windows 95 & Windows NT, IF you are using the 32 bit
native version of Fly8, to the DOS & Win3.11 version.

Here is an explanation of the changed functions:

	Option		Meaning
	======		=======
	ttl[-]		Enable Throttle & Disable HAT style 1.
			"-" if throttle is backwards.
	hat:		Enable HAT style 1
			*	Doesn't work with throttle
			*	You can use joystick buttons whilst using
				the hat.
	chpro:		Enable HAT style 2:
			*	Works with throttle
			*	You can not use joystick buttons whlist
				using the hat.
			*	You cannot use Fire Buttons
				1 & 2 Simulataniously.

NT note
=======

I found that when I tick the 'this stick has throttle or Z" on the
multimedia driver, the actual reading actually claims that I have
a rudder instead! To get around this problem I add an axis resssign:
	dpAstick:hat:sy2=3:four:d=046:rd=2
This will source the Y2 channel (4th axis, throttle/hat) from
joystick 3rd channel. This makes my FCS work properly.


Using gnuwin32
==============

I found that you need to do a few things. I assume that you installed
cdk.exe into '\gnuwin32\b18'.

1) copy mmsystem.cyg to
   \gnuwin32\b18\H-i386-cygwin32\i386-cygwin32\include\Windows32\mmsystem.h

2) edit
   \gnuwin32\b18\H-i386-cygwin32\i386-cygwin32\include\Windows32\Sockets.h

   line 94, comment out the BSD block as:

#if 0
/* BSD */
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;
#endif

   from line 120 to the enf of this file replace all 'fd_set' with
   'xfd_set' (leave alone the FD_SET). It starts with:

typedef struct xfd_set {
        u_int   fd_count;
        SOCKET  fd_array[FD_SETSIZE];
} xfd_set;

Using mingw32
=============

Copy mmsystem.cyg to '\mingw32\include\mmsystem.h'. If the make fails
with "The batch file was not found" then just retry.

Using lcc-win32
===============

Added wing32.exp to /buildlib and created wing32.lib.
