/* --------------------------------- common.h ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* System dependent configuration information:	OS/2
 * By Michael Taylor (miket@pcug.org.au)
*/

#ifndef FLY8_OS2_COMMON_H
#define FLY8_OS2_COMMON_H

extern HPS 	hps;		/* handle of device context	*/
extern HWND	ghWndMain;
extern HWND	hwndClient;	/* Client window handle */
extern HAB	hab;		/* PM anchor block handle */
extern HMQ	hmq;		/* Message queue handle   */

#if HAVE_DIVE
extern int 	usingDive;
#endif
extern int	resetSSize;
extern int	newx;
extern int	newy;

extern char	_Gbuf[2048];

extern Ulong	FAR GrStats[2048];

#endif
