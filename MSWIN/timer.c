/* --------------------------------- timer.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Time services for Microsoft Windows. The resolution is 1ms
*/

#include <windows.h>
#include <mmsystem.h>		/* timeGetTime */
#include <time.h>

#include "fly.h"

static Ulong FAR
MsTmMilli (void)
{
	return (timeGetTime());		/* time since start in milliseconds */
}

static int FAR
MsTmHires (void)			/* get fastest timer available */
{
	return ((int)timeGetTime());	/* time since start in milliseconds */
}

static char * FAR
MsTmCtime (void)
{
	time_t	tm;
	char	*t;

	tm = time (0);
	t = ctime (&tm);
	t[strlen (t) - 1] = '\0';	/* kill NewLine */
	return (t);
}

#define NINTS		10

static Ulong FAR
MsTmInterval (int mode, Ulong res)
{
	static Ulong	last_time[NINTS];
	static int	n = -1;
	Ulong		t, tt = 0;

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
		tt = MsTmMilli ();

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

struct TmDriver NEAR TmDriver = {
	"MMSystem",
	0,
	NULL,	/* extra */
	0, 	/* Init */
	0,	/* Term */
	MsTmMilli,
	MsTmHires,
	MsTmCtime,
	MsTmInterval
};
