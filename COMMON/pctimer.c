/* --------------------------------- pctimer.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* High resolution time services directly reading the 8254.
*/

#include "fly.h"
#include "pc8254.h"

#include <time.h>

#if SYS_DJGPP
#include <pc.h>

#define inp(p) 		inportb(p)
#define outp(p,b)	outportb(p,b)
#else
#include <conio.h>
#endif


static Ushort	counter = 0;
static Ulong	seconds = 0, ticks = 0;
static Ulong	timer_period = TIMER_PERIOD;

LOCAL_FUNC int FAR
PcTmHires (void)		/* get fastest timer available */
{
	Ushort		t, status;
	Ulong		flags;

	do {
		flags = Sys->Disable ();
		outp (COMMAND_REG, READ_SPECIAL);
		iefbr14 ();				/* delay */
		status = (Ushort) inp (CHANNEL_0);	/* get status */
		iefbr14 ();				/* delay */
		t  = (Ushort)inp (CHANNEL_0);		/* low byte */
		iefbr14 ();				/* delay */
		t += (Ushort)(inp (CHANNEL_0) << 8);	/* high byte */
		Sys->Enable (flags);
	} while (0 == t);

	if ((status & TIMER_MODES) == TIMER_MODE*3) {
		t >>= 1;
		if (status & TIMER_OUT)
			t += (Ushort)((timer_period+1)/2);
	}
	t = (Ushort)(timer_period - t);		/* make it count up */
	return ((int)t);
}

LOCAL_FUNC void NEAR
PcTmGet (void)
{
	Ushort		t;

	t = (Ushort) PcTmHires ();

	if (t >= counter)
		ticks += t - counter;
	else
		ticks += (t+timer_period) - counter;
	counter = t;
	while (ticks >= XTAL) {
		ticks -= XTAL;
		++seconds;
	}
}

LOCAL_FUNC Ulong FAR
PcTmMilli (void)
{
	PcTmGet ();
	return (seconds * 1000L + ticks * 1000L / XTAL);
}

#define NINTS		10

LOCAL_FUNC Ulong FAR
PcTmInterval (int mode, Ulong res)
{
	static Ulong	last_seconds[NINTS], last_ticks[NINTS];
	static int	n = -1;
	Ulong		t = 0;

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
		PcTmGet ();

	if (mode & TMR_READ) {
		if (res > XTAL)
			res = 0;
		if (res > 0x7fffffffL/XTAL) {
			Ulong	tt;

			tt = res/(0x7fffffffL/XTAL) + 1;
			t = (seconds-last_seconds[n]) * res;
			if (ticks > last_ticks[n])
				t += ((ticks-last_ticks[n]) * (res/tt))
					/ (XTAL/tt);
			else
				t -= ((last_ticks[n]-ticks) * (res/tt))
					/ (XTAL/tt);
		} else if (res)
			t = (seconds-last_seconds[n]) * res
				+ (long)((ticks-last_ticks[n]) * res) / XTAL;
		else
			t = (seconds-last_seconds[n]) * XTAL
				+ (long)(ticks-last_ticks[n]);
	}

	if (mode & TMR_SET) {
		last_seconds[n] = seconds;
		last_ticks[n]   = ticks;
	}

	if (mode & TMR_POP)
		--n;
	return (t);
}

LOCAL_FUNC int FAR
PcTmInit (char *options)
{
	int		status;
	Ulong		flags;
	long		l;

	if (options) {
		if (!get_narg (options, "rate", &l)
		    && l > 0 && l <= 0x10000L)
			timer_period = (Ulong)l;
		else
			timer_period = TIMER_PERIOD;
		LogPrintf ("%s: period %ld\n", Tm->name, timer_period);
	}
	
	flags = Sys->Disable ();
	outp (COMMAND_REG, READ_SPECIAL);
	iefbr14 ();
	status = inp (CHANNEL_0);			/* get status */
	(void)inp (CHANNEL_0);
	(void)inp (CHANNEL_0);
	Sys->Enable (flags);

	seconds = time (NULL);
	counter = 0;
	ticks = 0;

	status &= TIMER_MODES;
	return (status != TIMER_MODE*2 && status != TIMER_MODE*3);
}
#undef NINTS

LOCAL_FUNC char * FAR
PcTmCtime (void)
{
	time_t	tm;
	char	*t;
	int	bug;

	tm = time (0);
	t = ctime (&tm);
	bug = strlen (t);	/* micro&soft hits again! */
	t[bug - 1] = '\0';	/* kill NewLine */
	return (t);
}

struct TmDriver NEAR TmDriver = {
	"PC8254",
	0,
	NULL,	/* extra */
	PcTmInit,
	0, 	/* Term */
	PcTmMilli,
	PcTmHires,
	PcTmCtime,
	PcTmInterval
};
