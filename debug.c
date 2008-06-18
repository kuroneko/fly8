/* --------------------------------- debug.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Routines that get used only for debug.
*/

#include "fly.h"


static int	muldiv_flag = 0;

extern long FAR
lcheck (long x, char *name, char *file, int line)
{
	if (x > (long)0x00007fffL) {
		muldiv_flag = 1;
		LogPrintf ("check> %s(%u) %s>%ld\n", file, line, name, x);
		++STATS_DEBUG;
		return (0x00007fffL);
	}
	if (x < (long)0xffff8000L) {
		muldiv_flag = 2;
		LogPrintf ("check> %s(%u) %s<%ld\n", file, line, name, x);
		++STATS_DEBUG;
		return (0xffff8000L);
	}
	return (x);
}

extern int FAR
fmulchk (long x, long y, char *file, int line)
{
	long	r, tx, ty, tr;
	int	t;

	t = muldiv_flag;
	muldiv_flag = 0;

	tx = lcheck (x, "x", file, line);
	ty = lcheck (y, "y", file, line);
	r = (tx * ty) >> FSCALE;
	tr = lcheck (r, "r", file, line);
	if (muldiv_flag)
		LogPrintf ("fmul (%ld, %ld)= %ld\n", x, y, r);
	else
		muldiv_flag = t;
	return ((int)tr);
}

extern int FAR
fdivchk (long x, long y, char *file, int line)
{
	long	r, tx, ty, tr;
	int	t;

	t = muldiv_flag;
	muldiv_flag = 0;

	tx = lcheck (x, "x", file, line);
	if (y == 0L) {
		muldiv_flag = 3;
		LogPrintf ("check> %s(%u) y=%ld\n", file, line, y);
		++STATS_DEBUG;
		tr = r = 0x00007fffL;
	} else {
		ty = lcheck (y, "y", file, line);
		r = (tx << FSCALE) / ty;
		tr = lcheck (r, "r", file, line);
	}
	if (muldiv_flag)
		LogPrintf ("fdiv (%ld, %ld)= %ld\n", x, y, r);
	else
		muldiv_flag = t;
	return ((int)tr);
}

extern int FAR
muldivchk (long x, long y, long z, char *file, int line)
{
	long	r, tx, ty, tz, tr;
	int	t;

	t = muldiv_flag;
	muldiv_flag = 0;

	tx = lcheck (x, "x", file, line);
	ty = lcheck (y, "y", file, line);
	if (0L == z) {
		muldiv_flag = 3;
		LogPrintf ("check> %s(%u) z=%ld\n", file, line, z);
		++STATS_DEBUG;
		tr = r = 0x00007fffL;
	} else {
		tz = lcheck (z, "z", file, line);
		r = (tx * ty) / tz;
		tr = lcheck (r, "r", file, line);
	}
	if (muldiv_flag)
		LogPrintf ("muldiv (%ld, %ld, %ld)= %ld\n", x, y, z, r);
	else
		muldiv_flag = t;
	return ((int)tr);
}
