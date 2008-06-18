/*------------------------------ os2sound.c ----------------------------------*/

#define  INCL_OS2MM                      /* Required for MCI & MMIO headers   */
#define  INCL_WIN                        /* Required to use Win APIs.         */
#define  INCL_PM                         /* Required to use PM APIs.          */
#define  INCL_GPI                        /* Required to use GPI OS/2 APIs.    */
#define  INCL_DOS                        /* Required to use Semaphores.       */

#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fly.h"

/* following define using DosBeep and the PC speaker */
#define SPEAKER	1
/* following define using Multimedia support */
#define MMEDIA	0


/* Initialize this module.
*/
extern BOOL FAR
FInitSnd (void)
{
#if	MMEDIA
#endif
    	return TRUE;
}

/* Close this module.
*/
extern void FAR
CloseSnd (void)
{
#if	MMEDIA
#endif
}

/* Play the given sound.
 * rFreq : Frequency of sound (Hz).
 * cms   : Duration (ms).
 * csps  : Sampling rate (Hz).
 * cbit  : Sample size (bits/sample/channel).
 * cchn  : Number of channels.
*/
static BOOL
FPlaySnd (float rFreq, long cms, UINT csps, UINT cbit, UINT cchn)
{
#if	MMEDIA
#endif
#if	SPEAKER
	DosBeep (rFreq, cms);
#endif

    	return TRUE;
}

extern void FAR
Fly8PlaySnd (int Freq)
{
#if	MMEDIA
#endif
#if	SPEAKER
	DosBeep (Freq, 10);
#endif
}

/* Stop playing the current sound.
*/
extern void FAR
StopSnd (void)
{
#if	MMEDIA
#endif
}
