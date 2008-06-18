/* --------------------------------- common.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
 *
 *  OS/2 support by Michael Taylor miket@pcug.org.au
*/

#include <os2.h>

#include "fly.h"

HPS	hps = 0;		/* handle of device context */
HWND	ghWndMain = 0;		/* Frame window handle */
HWND	hwndClient = 0;		/* Client window handle */
HAB	hab = 0;		/* PM anchor block handle */
HMQ	hmq = 0;		/* Message queue handle   */

#if HAVE_DIVE
int	usingDive = 0;
#endif
int	resetSSize = 0;
int	newx = 0;
int	newy = 0;

char	_Gbuf[2048] = "";


void
perror (const char *p)
{
   WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, (PSZ)p,
                  (PSZ)"Fly8 Error", 0, MB_OK);
}
