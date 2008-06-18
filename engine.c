/* --------------------------------- engine.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Calculate engine thrust.
 * Based on Stevens&Lewis AC&S, F16 model.
 *
 * To handle large numbers we divide altitude by 10. We can now handle alt over
 * the 65000 'short' limit. The 'thrust' returned is also divided by 10.
*/

#include "plane.h"


/* Return command power [0..10000] for a throttle setting [0..100].
 * At 75% setting we get 50% power command.
*/
LOCAL_FUNC xshort NEAR	/* x100 */
tgear (xshort thtl)	/* x100 */
{
	if (thtl <= 75)
		return (67 * thtl);
	else
		return (200 * thtl - 100*100);
}

/* Return reciprocal time constant [0..1000].
*/
LOCAL_FUNC xshort NEAR
rtau (xshort dp)		/* x100 */
{
	if (dp <= 25*100)
		return (1000);
	else if (dp >= 50*100)
		return (100);
	else
		return (1900 - muldiv (36, dp, 100));
}

/* Return rate of change of power [0..10000].
*/
LOCAL_FUNC xshort NEAR	/* x 100 */
pdot (xshort pow, xshort cpow)	/* x100, x100 */
{
	xshort	t;	/* x1000 */
	xshort	p2;	/* x100 */

	if (cpow >= 50*100) {
		if (pow >= 50*100) {
			t = 5*1000;
			p2 = cpow;
		} else {
			p2 = 60*100;
			t = rtau (p2-pow);
		}
	} else {
		if (pow >= 50*100) {
			t = 5*1000;
			p2 = 40*100;
		} else {
			p2 = cpow;
			t = rtau (p2-pow);
		}
	}
	return (muldiv (t, p2-pow, 1000));
}

/*	sl	10k	20k	30k	40k	50k */

static xshort a[6][6] = {				/* idle	*/
	{1060,	670,	880,	1140,	1500,	1860},	/* M0.0	*/
	{635,	425,	690,	1010,	1330,	1700},	/* M0.2	*/
	{60,	25,	345,	755,	1130,	1525},	/* M0.4	*/
	{-1020,	-710,	-300,	350,	910,	1360},	/* M0.6	*/
	{-2700,	-1900,	-1300,	-247,	600,	1100},	/* M0.8	*/
	{-3600,	-1400,	-595,	-342,	-200,	700}	/* M1.0	*/
};

static xshort b[6][6] = {				/* mil	*/
	{12680,	9150,	6200,	3950,	2450,	1400},
	{12680,	9150,	6313,	4040,	2470,	1400},
	{12610,	9312,	6610,	4290,	2600,	1560},
	{12640,	9839,	7090,	4660,	2840,	1660},
	{12390,	10176,	7750,	5320,	3250,	1930},
	{11680,	9848,	8050,	6100,	3800,	2310}
};

static xshort c[6][6] = {				/* ab	*/
	{20000,	15000,	10800,	7000,	4000,	2500},
	{21420,	15700,	11225,	7323,	4435,	2600},
	{22700,	16860,	12250,	8154,	5000,	2835},
	{24240,	18910,	13760,	9285,	5700,	3215},
	{26070,	21075,	15975,	11115,	6860,	3950},
	{28886,	23319,	18300,	13484,	8642,	5057}
};	

LOCAL_FUNC int NEAR
thrust (xshort pow, Uxshort alt, xshort amach)
{
	int	i, dh, m, dm, cdh, s, t, tmil, tidl, tmax;

	i  = alt / 1000;
	if (i > 4)
		i = 4;
	dh = alt - 1000*i;
	cdh = 1000 - dh;

	m  = amach*5 / 100;
	if (m > 4)
		m = 4;
	dm = amach*5 - 100*m;

	s = muldiv (b[m][i],   cdh, 1000) + muldiv (b[m][i+1],   dh, 1000);
	t = muldiv (b[m+1][i], cdh, 1000) + muldiv (b[m+1][i+1], dh, 1000);
	tmil = s + muldiv (t-s, dm, 100);
	if (pow < 50*100) {
		s = muldiv (a[m][i], cdh, 1000)
			+ muldiv (a[m][i+1], dh, 1000);
		t = muldiv (a[m+1][i], cdh, 1000)
			+ muldiv (a[m+1][i+1], dh, 1000);
		tidl = s + muldiv(t-s, dm , 100);
		t = tidl + muldiv (tmil-tidl, pow, 50*100);
	} else {
		s = muldiv (c[m][i], cdh, 1000)
			+ muldiv (c[m][i+1], dh, 1000);
		t = muldiv (c[m+1][i], cdh, 1000)
			+ muldiv (c[m+1][i+1], dh, 1000);
		tmax = s + muldiv (t-s, dm, 100);
		t = tmil + muldiv (tmax-tmil, pow-50*100, 50*100);
	}
	return (t/10);
}

extern void FAR
f16engine (OBJECT *p, xshort sos)
{
	xshort	thtl, neg, amach, t;
	long	alt;

	neg = FONE;
	if (100 == EX->throttle)
		thtl = 75 + muldiv (EX->afterburner, 100-75, 100);
	else {
		thtl = muldiv (EX->throttle, 75, 100);
		if (thtl < 0) {
			thtl = -thtl;
			neg = -FONE/2;	/* 50% efficient reverse thrust? */
		}
	}

	alt = p->R[Z] * 328L / (100L * VONE);
	if (alt > 320000L)
		alt = 320000L;

	if (EX->flags & PF_FLAMEOUT) {
		if (EX->fuel > 0 && alt*4 < EP->ceiling*3)
			EX->flags &= ~PF_FLAMEOUT;	/* re ingnite */
	} else if (alt > EP->ceiling || EX->fuel <= 0)
		EX->flags |= PF_FLAMEOUT;		/* flameout */

	if (EX->flags & PF_FLAMEOUT)
		t = 0;
	else
		t = tgear (thtl);
	t = pdot (EX->power, t);
	EX->power += TADJ (t);
	if (EX->power > 10000)
		EX->power = 10000;
	else if (EX->power < 0)
		EX->power = 0;

	alt /= 10;
	amach = muldiv (p->speed, 100, sos);
	t = fmul (neg, thrust (EX->power, (Uxshort)alt, amach));
	EX->thrust = muldiv (t, EP->ab_thrust, 2383);
	if ((EX->flags & PF_FLAMEOUT) && 0 == EX->power && EX->thrust > 0)
		EX->thrust = 0;

	if (EX->fuel > 0) {

/* sfc diminishes to 0.8 (of its sea-level value) at 36000 feet then stays
 * stable.
*/
		t = EX->afterburner ? EP->ab_sfc : EP->mil_sfc;
		if (alt < 3600L)
			t -= fmul (t, muldiv (FCON(0.2), (int)alt, 3600));
		else
			t -= fmul (t, FCON(0.2));
		EX->fuelRate = muldiv (iabs (EX->thrust), t, 60*60/10);
		EX->fuel -= TADJ(EX->fuelRate);
		if (EX->fuel < 0)
			EX->fuel = 0;
	}
}
