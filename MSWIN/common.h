/* --------------------------------- common.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* System dependent configuration information: Microsoft WINDOWS, ms c8
 * Microsoft Windows  by Michael Taylor (miket@pcug.org.au)
*/

#ifndef FLY8_MSWIN_COMMON_H
#define FLY8_MSWIN_COMMON_H

extern HDC 	hdc;	/* handle of device context	*/

extern int 	usingWinG;
extern int	resetSSize;
extern int	newx, newy;

extern char	_Gbuf[2048];

extern HWND	ghWndMain;
extern HWND	ghWndText;

extern HINSTANCE	Fly8Instance;
extern char	FAR Fly8AppName[10];
extern char	FAR Fly8Message[15];
extern char	FAR Fly8Text[10];

extern Ulong	FAR GrStats[2048];

#endif
