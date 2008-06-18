/* --------------------------------- timer.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Time services for UNIX. The rsolution depends on your ftime().
*/

#include "fly.h"

#include <time.h>
#include <sys/timeb.h>


static Ulong FAR
timer_milli (void)
{
	struct timeb	tm;

	ftime (&tm);
	return (tm.time*1000L + tm.millitm);
}

static int FAR
timer_hires (void)		/* get fastest timer available */
{
	static int last = 0;

	return (++last);	/* how fast do you want it? */
}

static int FAR
timer_init (char *options)
{return (0);}

static void FAR
timer_term (void)
{}

static char * FAR
timer_ctime (void)
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
timer_interval (int mode, Ulong res)
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
		tt = timer_milli ();

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

struct TmDriver TmDriver = {
	"ftime()",
	0,
	NULL,	/* extra */
	timer_init,
	timer_term,
	timer_milli,
	timer_hires,
	timer_ctime,
	timer_interval
};
