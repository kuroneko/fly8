/* --------------------------------- common.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Common mswin definitions.
 * Windows support by Michael Taylor miket@pcug.org.au
*/

#include <windows.h>

#include "fly.h"

HDC	hdc = 0;	/* handle of device context */

#ifdef USE_WING	
int 	usingWinG = 0;
#endif

int	resetSSize = 0;
int	newx = 0, newy = 0;

char _Gbuf[2048] = "";

HWND		ghWndMain  = (HWND)0;
HWND		ghWndText  = (HWND)0;

void FAR
perror (const char *p)
{
	MessageBox((HWND)0, (LPCSTR)p, (LPCSTR)"Fly8 Error", MB_OK);
}
