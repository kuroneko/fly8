/* --------------------------------- timer.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Time services for OS/2. The resolution is 1ms
 * OS/2 support by Michael Taylor miket@pcug.org.au
*/

#define INCL_DOS
#define INCL_GPI
#define INCL_WIN
#include <os2.h>
#include <time.h>

#include "fly.h"

/* at least three methods of reading the time are possible
	SYSINFO:
	uses DosQuerySysInfo
	DOSTMR
	uses DosTmrQueryFreq and DosTmrQueryTime
	WINTIME
	uses WinGetCurrentTime
	
	The DOSTMR method has a resolution of 1ms whereas the other
	methods have resolutions of 32msecs approx.
*/
#define SYSINFO	0
#define DOSTMR	1
#define WINTIME	0

static ULONG	freq=0, prevlo = 0, prevms = 0, msinc = 0;

static int
OS2TmInit (char *options)
{
	APIRET rc;
#if SYSINFO
   	rc = DosQuerySysInfo (QSV_MS_COUNT, QSV_MS_COUNT, &freq, sizeof(ULONG));
#endif
#if DOSTMR
	rc = DosTmrQueryFreq (&freq);
	if (!rc) {
/*		LogPrintf ("timer: freq=%lu Hz\n", freq);*/
		freq = freq / 1000;	/* in milliseconds */
		msinc = (ULONG)(0xffffffffUL / freq);
/*		LogPrintf ("timer: msinc=%lu\n", msinc);*/
	}
#endif
#if WINTIME
	rc = 0;
#endif
   	if (rc) {
      		LogPrintf ("timer: error reading system timer, rc=%d\n", rc);
      		die ();
	}

LogPrintf ("timer: init ok\n");

   	return (0);
}

static Ulong
OS2TmMilli (void)
{
#if SYSINFO
	/* resolution is about 32 msecs */
   	ULONG time;

   	DosQuerySysInfo (QSV_MS_COUNT, QSV_MS_COUNT, &time, sizeof(ULONG));
	
	return time;
#endif
#if DOSTMR
	/* resolution is in theory about 1ms 
	*  NOTE: there is some rounding as calculations are done 
	*  in ULONGs.
	*/
	QWORD time;

	DosTmrQueryTime (&time);
	
	/* if time has wrapped then add increment */
	if (prevlo > time.ulLo)
		prevms += msinc;
	prevlo = time.ulLo;
	
	/* return in milliseconds - NOTE: freq has msecs factored in */
	return (ULONG)(prevms + (ULONG)(time.ulLo / freq));
#endif
#if WINTIME
	/* resultion is about 32ms */
	extern HAB hab;

	if (!hab)
	        return (0);

	return WinGetCurrentTime (hab);
#endif
}

static int
OS2TmHires (void)        /* get fastest timer available */
{
   	return (int) OS2TmMilli ();
}

static char *
OS2TmCtime (void)
{
   	time_t   tm;
   	char  *t;

   	tm = time (0);
   	t = ctime (&tm);
   	t[strlen (t) - 1] = '\0';  /* kill NewLine */
   	return (t);
}

#define NINTS     10

static Ulong
OS2TmInterval (int mode, Ulong res)
{
   	static Ulong   last_time[NINTS];
   	static int  n = -1;
   	Ulong    t, tt = 0;

   	if (mode & TMR_PUSH) {
      		++n;
      		if (n >= NINTS) {
         		LogPrintf ("timer: too many PUSHes... aborting\n");
         		die ();
      		}
   	} else if (n < 0) {
      		LogPrintf ("timer: too many POPs... aborting\n");
      		die ();
   	}

   	if (mode & (TMR_READ|TMR_SET))
      		tt = OS2TmMilli ();

   	if (mode & TMR_READ) {
      		t = tt - last_time[n];
      		if (res)
         		t = t * res / 1000L;
   	} else
      		t = 0;

   	if (mode & TMR_SET)
      		last_time[n] = tt;

   	if (mode & TMR_POP)
      		--n;
   	return (t);
}

struct TmDriver  TmDriver = {
   	"OS/2",
   	0,
   	NULL, 		/* extra */
   	OS2TmInit,    	/* Init */
   	0, 		/* Term */
   	OS2TmMilli,
   	OS2TmHires,
   	OS2TmCtime,
   	OS2TmInterval
};
