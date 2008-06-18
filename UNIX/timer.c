/* --------------------------------- timer.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Time services for UNIX. The rsolution depends on your ftime().
*/

#include "fly.h"

#include <time.h>
#include <sys/timeb.h>

#if HAVE_GETTIME
static struct timespec	tp0;
#elif HAVE_GETTIMEOFDAY
#include <sys/time.h>
static struct timeval	tv0;
#endif

static int
utm_init (char *options)
{
#if HAVE_GETTIME
	clock_gettime (CLOCK_REALTIME, &tp0);
#elif HAVE_GETTIMEOFDAY
	gettimeofday (&tv0, NULL);
#endif
	return (0);
}

static Ulong FAR
utm_milli (void)
{
	Ulong	t;

#if HAVE_GETTIME
	struct timespec	tp;

	clock_gettime (CLOCK_REALTIME, &tp);
	t = (tp.tv_sec-tp0.tv_sec)*1000 
				+ (tp.tv_nsec-tp0.tv_nsec)/1000000;
#elif HAVE_GETTIMEOFDAY
	struct timeval	tv;

	gettimeofday (&tv, NULL);
	t = (tv.tv_sec-tv0.tv_sec)*1000 
				+ (tv.tv_usec-tv0.tv_usec)/1000;
#else
	struct timeb	tm;

	ftime (&tm);
	t = tm.time*1000L + tm.millitm;
#endif
	return (t);
}

static int FAR
utm_hires (void)		/* get fastest timer available */
{
	static int last = 0;

	return (++last);	/* how fast do you want it? */
}

static char * FAR
utm_ctime (void)
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
utm_interval (int mode, Ulong res)
{
	static Ulong	last_time[NINTS];
	static int	n = -1;
	Ulong		t, tt = 0;

	if (mode & TMR_PUSH) {
		++n;
		if (n >= NINTS) {
			LogPrintf ("%s: too many PUSHes... aborting\n",
				Tm->name);
			die ();
		}
	} else if (n < 0) {
		LogPrintf ("%s: too many POPs... aborting\n", Tm->name);
		die ();
	}

	if (mode & (TMR_READ|TMR_SET))
		tt = utm_milli ();

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
#if HAVE_GETTIME
	"gettime",
#else
	"ftime",
#endif
	0,
	NULL,	/* extra */
	utm_init,
	0,	/* term */
	utm_milli,
	utm_hires,
	utm_ctime,
	utm_interval
};
