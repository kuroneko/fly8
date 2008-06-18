/* --------------------------------- grstat.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Display video driver statistics.
*/

#include "fly.h"


Ulong	NEAR GrStats[2048] = {0};

extern void FAR
LogStats (void)
{
	int	i;
	Ulong	sp, sl, l, p, t;

	l = 0;
	for (i = 0; i < 2048; ++i) {
		if (l < GrStats[i])
			l = GrStats[i];
	}
	sp = sl = 1L;
	if        (l >= 1000000000L) {
		sp = 1000L;
		sl = 100L;
	} else if (l >= 100000000L) {
		sp = 100L;
		sl = 10L;
	} else if (l >= 10000000L)
		sp = 10L;

	l = p = 0L;
	LogPrintf ("Line drawing summary:\n");
	LogPrintf ("  len          count              volume\n");
	for (i = 0; i < 2048; ++i) {
		if (GrStats[i]) {
			LogPrintf (" %4u %14s", i, show_ul (GrStats[i]));
			LogPrintf (" %19s\n", show_ulf (GrStats[i], (Ulong)i));
			l += GrStats[i]/sl;
			p += GrStats[i]/sp*i;
		}
	}
	LogPrintf ("  tot %14s", show_ulf (l, sl));
	LogPrintf (" %19s\n", show_ulf (p, sp));

	if (T(t = l/sp*sl/1000))
		LogPrintf (" mp/l %14s\n", show_ul (p/t));
	if (T(t = STATS_FRAMESCOUNT/sl))
		LogPrintf ("  l/f %14s\n", show_ul (l/t));
	if (T(t = STATS_FRAMESCOUNT/sp))
		LogPrintf ("  p/f %14s\n", show_ul (p/t));
	if (T(t = STATS_TIMEVIDEO / 10000 / sp))
		LogPrintf ("  p/s %14s\n", show_ul (p/t));
	if (T(t = STATS_TIMEVIDEO / 10000 / sl))
		LogPrintf ("  l/s %14s\n", show_ul (l/t));
}
