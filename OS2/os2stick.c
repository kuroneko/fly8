/* --------------------------------- os2stick.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Low level joystick reading.
 *
 * OS/2 support by Michael Taylor miket@pcug.org.au
*/

#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#define INCL_DOSMEMMGR

#include <os2.h>
#include <stdlib.H>

#include "fly.h"

#if	HAVE_JOYSTICK

#include "stick.h"
#include "joyos2.h"

#define JOY_STANDARD    0
#define JOY_EXTENDED    1

static	HFILE	hGame;               	/* handle to device GAME$ */
static	ULONG	action;              	/* action taken by DosOpen */
static	GAME_PARM_STRUCT gameParms;     /* status on the installed joystick */
static	GAME_CALIB_STRUCT gameCalib;    /* calibration values */
static	GAME_STATUS_STRUCT gameStatus; 	/* data of joysticks and buttons */
static	ULONG	dataLen;	    	/* length of parameters */
static	USHORT	usJoyType;           	/* type of joystick */
static	APIRET	rc;               	/* return code */

extern int FAR
initstick (int which, char *options, int opts)
{
   	USHORT 	usTmp1;
   	USHORT 	usTmp2;

/* Open the device "GAME$". "GAME$" was loaded by the OS/2
 * Joystick Device Driver (GAMEDD.SYS) at initialization time.
*/
	rc = DosOpen(GAMEPDDNAME, &hGame, &action, 0, FILE_READONLY,
		FILE_OPEN, OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE,
		NULL);
	if (rc) {
		LogPrintf ("initstick: Could not open device\n");
	        return (rc);
	}

   	/*
    	* API function 02h: GAME_GET_PARMS
    	*/
	dataLen = sizeof(gameParms);
	rc = DosDevIOCtl( hGame, IOCTL_CAT_USER,  GAME_GET_PARMS, NULL, 0, NULL,
		&gameParms, dataLen, &dataLen);
	if (rc) {
		LogPrintf ("initstick: Could not GET_PARMS\n");
		DosClose(hGame);
		return (rc);
	}

	if ((gameParms.useA == 0) && (gameParms.useB == 0)) {
		LogPrintf ("initstick: No joysticks detected\n");
		DosClose(hGame);
		return (rc);
	}

   	/*
    	* Keep only bits defined X and Y axis, they are bit 1 and bit 2
    	*/
   	usTmp1 = gameParms.useA & GAME_USE_BOTH_NEWMASK;
   	usTmp2 = gameParms.useB & GAME_USE_BOTH_NEWMASK;

	if ( usTmp1 == GAME_USE_BOTH_NEWMASK ) {
      		if ( usTmp2 == GAME_USE_BOTH_NEWMASK ) {
 		 	LogPrintf ("initstack: Two standard joysticks detected\n");
	         	usJoyType = JOY_STANDARD;
      		}
      		else if ( !usTmp2 ) {
 		      	LogPrintf ("initstack: One standard joystick detected\n");
            		usJoyType = JOY_STANDARD;
      		}
      		else if ( usTmp2 ) {
            		LogPrintf ("initstack: One joystick with extended feature detected\n");
            		usJoyType = JOY_EXTENDED;
      		}
   	}

   	/*
    	* API function 04h: GAME_GET_CALIB
    	*/
	dataLen = sizeof(gameCalib);
	rc = DosDevIOCtl( hGame, IOCTL_CAT_USER,  GAME_GET_CALIB, NULL, 0, NULL,
		&gameCalib, dataLen, &dataLen);
	if (rc != 0) {
		LogPrintf ("initstack: ERROR  Could not GET_CALIB\n");
		return (rc);
	}

   	/*
    	* API function 10h: GAME_GET_STATUS
    	*/
	dataLen = sizeof(gameStatus);
	rc = DosDevIOCtl( hGame, IOCTL_CAT_USER, GAME_GET_STATUS, NULL, 0, NULL,
		&gameStatus, dataLen, &dataLen);
	if (rc != 0) {
		LogPrintf ("initstick: ERROR - Could not GET_STATUS\n");
		return (rc);
	}

	LogPrintf ("initstick: ok\n");

        return (0);
}

extern int FAR
termstick (int which, int opts)
{
LogPrintf ("in termstick: before DosClose...\n");
   	/*
    	* Close the device "GAME$"
    	*/
	DosClose(hGame);

	LogPrintf ("termstick: ok\n");
        return (0);
}

extern Uint FAR
readstick (int which, STICK *s, int mask, int opts)
{
	dataLen = sizeof(gameStatus);
	rc = DosDevIOCtl( hGame, IOCTL_CAT_USER, GAME_GET_STATUS,
			NULL, 0, NULL, &gameStatus, dataLen, &dataLen);
	if (rc != 0)
	        return (rc);	/* error */

/* Set analog values.
*/
	if (which) {
		s->a[0] = gameStatus.curdata.B.x;
		s->a[1] = gameStatus.curdata.B.y;
	} else {
		s->a[0] = gameStatus.curdata.A.x;
		s->a[1] = gameStatus.curdata.A.y;
		s->a[2] = gameStatus.curdata.B.x;
		s->a[3] = gameStatus.curdata.B.y;
	}

/* Set button values.
*/
#if 1
	if (which) {
		s->b[0] = (char)!(gameStatus.curdata.butMask & JOY_BUT3_BIT);
		s->b[1] = (char)!(gameStatus.curdata.butMask & JOY_BUT4_BIT);
	} else {
		s->b[0] = (char)!(gameStatus.curdata.butMask & JOY_BUT1_BIT);
		s->b[1] = (char)!(gameStatus.curdata.butMask & JOY_BUT2_BIT);
		s->b[2] = (char)!(gameStatus.curdata.butMask & JOY_BUT3_BIT);
		s->b[3] = (char)!(gameStatus.curdata.butMask & JOY_BUT4_BIT);
	}
#else
	if (which) {
		s->b[0] = (char)gameStatus.b3cnt;
		s->b[1] = (char)gameStatus.b4cnt;
	} else {
		s->b[0] = (char)gameStatus.b1cnt;
		s->b[1] = (char)gameStatus.b2cnt;
		s->b[2] = (char)gameStatus.b3cnt;
		s->b[3] = (char)gameStatus.b4cnt;
	}
#endif
	return (0);
}

#endif /* if HAVE_JOYSTICK */
