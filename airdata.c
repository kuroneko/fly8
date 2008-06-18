/* --------------------------------- airdata.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* get standard atmospheric adta.
 * Works in the range -16,000...100,000 feet.
*/

#include "fly.h"


static short NEAR rrhotab[] = {
	FCON(1.2550),	/* -5,000 */
	FCON(1.2019),	/* -4,000 */
	FCON(1.1495),	/* -3,000 */
	FCON(1.0984),	/* -2,000 */
	FCON(1.0485),	/* -1,000 */

	FCON(1.0000),	/*    000 */
	FCON(0.9526),	/*  1,000 */
	FCON(0.9064),	/*  2,000 */
	FCON(0.8615),	/*  3,000 */
	FCON(0.8178),	/*  4,000 */
	FCON(0.7753),	/*  5,000 */
	FCON(0.7340),	/*  6,000 */
	FCON(0.6939),	/*  7,000 */
	FCON(0.6551),	/*  8,000 */
	FCON(0.6174),	/*  9,000 */

	FCON(0.5810),	/* 10,000 */
	FCON(0.5457),	/* 11,000 */
	FCON(0.5045),	/* 12,000 */
	FCON(0.4664),	/* 13,000 */
	FCON(0.4312),	/* 14,000 */
	FCON(0.3987),	/* 15,000 */
	FCON(0.3686),	/* 16,000 */
	FCON(0.3408),	/* 17,000 */
	FCON(0.3151),	/* 18,000 */
	FCON(0.2913),	/* 19,000 */

	FCON(0.2694),	/* 20,000 */
	FCON(0.2485),	/* 21,000 */
	FCON(0.2295),	/* 22,000 */
	FCON(0.2118),	/* 23,000 */
	FCON(0.1957),	/* 24,000 */
	FCON(0.1808),	/* 25,000 */
	FCON(0.1673),	/* 26,000 */
	FCON(0.1545),	/* 27,000 */
	FCON(0.1431),	/* 28,000 */
	FCON(0.1322),	/* 29,000 */

	FCON(0.1224),	/* 30,000 */
	FCON(0.1126)	/* 31,000 */
};

/* Calculate atmospheric parameters.
 * rrho	air density (relative to sea level)
 * rho	air density
 * sos	speed of sound
*/
extern void FAR
airdata (long height, int *srho, int *rrho, int *rho, int *sos)
{
	long	t;
	int	i, r;

	t = height/VONE + 5000;
	i = (int)(t/1000);
	if (i < 0)
		r = rrhotab[0];
	else if (i >= rangeof(rrhotab)-1)
		r = rrhotab[rangeof(rrhotab)-1];
	else {
		r = rrhotab[i];
		r += muldiv (rrhotab[i+1]-r, (int)(t-i*1000L), 1000);
	}
	if (srho)
		*srho = r;
	r = fmul (r, r);
	if (rrho)
		*rrho = r;
	if (rho)
		*rho = fmul (r, FCON(1.225));

	if (sos) {
		if (t <= 11000L)
			*sos = 340*VONE
				- muldiv ((340-295)*VONE, (int)t, 11000);
		else if (t <= 20000L)
			*sos = 295*VONE;
		else
			*sos = 295*VONE
				+ fmul (FCON(0.67), (int)((t - 20000L)/1000));
	}
}
