/* --------------------------------- ifuncs.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handle integer trig and math functions
*/

#ifdef GEN_TAB
#undef USE_ASM
#endif

#include "fly.h"


#undef ihypot2d
extern Uxshort FAR FASTCALL
ihypot2d (int x, int y)
{
	return (SQRT (x*(long)x + y*(long)y));
}

#undef ihypot3d
extern Uxshort FAR FASTCALL
ihypot3d (VECT A)
{
	return (SQRT (A[X]*(long)A[X] + A[Y]*(long)A[Y] + A[Z]*(long)A[Z]));
}

#ifndef EXT_TABS
#include <math.h>
#endif

/* This part initializes the constant tables.
 * It either #includes constant ones or calculates them.
*/

#ifdef EXT_TABS
#include "itabs.h"

extern int FAR
funcs_init (void)
{return (0);}

extern void FAR
funcs_term (void)
{}

#else

/* angles are stored such that -pi..0..pi radians is 0x8000..0..0x7fff
*/

#define	P3_4	((1024/4)*3)
 
static xshort	NEAR log_tab[201] = {0};	/* exponential -100...+100 */
static xshort	NEAR sin_tab[1024+1] = {0};		/* 0..90 */
static ANGLE	NEAR asin_tab0[1024+1] = {0};		/* 0......1/4 */
static ANGLE	NEAR asin_tab1[P3_4+1] = {0};		/* 0......3/4 */
static ANGLE	NEAR asin_tab2[P3_4+1] = {0};		/* 3/4....15/16 */
static ANGLE	NEAR asin_tab3[1024+1] = {0};		/* 15/16..1 */
static ANGLE	NEAR atan_tab1[P3_4+1] = {0};		/* 0......3/4 */
static ANGLE	NEAR atan_tab2[P3_4+1] = {0};		/* 3/4....15/16 */
static ANGLE	NEAR atan_tab3[1024+1] = {0};		/* 15/16..1 */
static Uxshort	NEAR sqrtab[256+1] = {0};

LOCAL_FUNC void NEAR
set_log (void)
{
	int	i, v, s;

	v = FONE;				/* 100% start */
	s = FCON(0.955);			/* 4.5%   step */
	for (i = 100; i > 0; --i) {		/* about 100 steps */
		log_tab[100+i] = v;
		log_tab[100-i] = -v;
		v = fmul (v, s);
		i = iabs (i);			/* scare gcc bug */
	}
	log_tab[100] = 0;			/* nice zero */
}

LOCAL_FUNC xshort NEAR
my_round (double f)
{
	return ((xshort)((f<0) ? f-0.5 : f+0.5));
}

/* Fill in the trig tables.
*/
LOCAL_FUNC void NEAR
set_trig (void)
{
	double	s, c, t, d, sd, cd, pi;
	int	i;

	pi = atan(1.0)*4;
	d = pi/2048;			/* 0...pi */
	sd = sin (d);
	cd = cos (d);
	s = 0.0;
	c = 1.0*FONE;

	for (i = 0; i <= 1024; ++i) {
		sin_tab[i] = my_round (s);
		t = c*sd+s*cd;
		c = c*cd-s*sd;
		s = t;
	}

	for (i = 0; i <= 1024; ++i)
		asin_tab0[i] = TANG(my_round (asin (i/16.0/1024.0)
					*0x08000/pi));
	for (i = 0; i <= P3_4; ++i)
		asin_tab1[i] = TANG(my_round (asin (i*3.0/4.0/P3_4)
					*0x08000/pi));
	for (i = 0; i <= P3_4; ++i)
		asin_tab2[i] = TANG(my_round (asin (
					3.0/4.0+i*(15.0/16.0-3.0/4.0)/P3_4)
					*0x08000/pi));
	for (i = 0; i <= 1024; ++i)
		asin_tab3[i] = TANG(my_round (asin (
					15.0/16.0+i*(1.0-15.0/16.0)/1024)
					*0x08000/pi));

	for (i = 0; i <= P3_4; ++i)
		atan_tab1[i] = TANG(my_round (atan (i*3.0/4.0/P3_4)
					*0x08000/pi));
	for (i = 0; i <= P3_4; ++i)
		atan_tab2[i] = TANG(my_round (atan (
					3.0/4.0+i*(15.0/16.0-3.0/4.0)/P3_4)
					*0x08000/pi));
	for (i = 0; i <= 1024; ++i)
		atan_tab3[i] = TANG(my_round (atan (
					15.0/16.0+i*(1.0-15.0/16.0)/1024)
					*0x08000/pi));
}

#undef P3_4

LOCAL_FUNC Uxshort NEAR
lsqrt (Ulong x)				/* used for initialization only */
{
	long	e;
	Ulong	r, t;

	if (x & 0xffff0000L)
		r = 662 + x / 17916;
	else if (x & 0x0000ff00L)
		r = 3 + x / 70;
	else
		r = 2 + x / 11;

	do {
		t = x / r;
#if SYS_OS2
		e = (long)(r - t);
		e /= 2;
#else
		e = (long)(r - t) / 2;
#endif
		r = (r + t) / 2;
	} while (e);
	return ((Uxshort)r);
}

LOCAL_FUNC void NEAR
set_lsqrt (void)
{
	int	i;

	for (i = 0; i < 256; ++i)
		sqrtab[i] = lsqrt (i*256L*256L*256L);
	sqrtab[256] = 0xffffU;
}

extern int FAR
funcs_init (void)
{
	set_log ();
	set_trig ();
	set_lsqrt ();
	return (0);
}

extern void FAR
funcs_term (void)
{}

#endif

/* Integer trig and math functions.
*/
extern xshort FAR FASTCALL
lin2log (xshort linear)
{
	return (fmul (log_tab[100+linear], 100));
}

extern xshort FAR FASTCALL
my_sinc (ANGLE d)
{
	int	f, l, ind;

	ind = 0;
	if (d < 0) {
		;
		if ((d = -d) < 0)	/* -180 degrees! */
			return (0);
		ind = -1;
	}
	if (d > D90) {
		d -= D180;
		d = -d;
	}

	f = d&15;			/* d%16 */
	d >>= 4;			/* d/16 */
	l = sin_tab[d];
	l += (8+(sin_tab[d+1] - l)*f)>>4;	/* (8+(h-l)*f)/16; */

	return ((xshort)((l^ind)-ind));
}

extern ANGLE FAR FASTCALL
my_asin (int d)
{
	int	f;
	ANGLE	l, ind;

	ind = 0;
	if (d < 0) {
		d = -d;
		ind = -1;
	}

	if (d < FONE/16)		/* 0..1/16 */
		l = asin_tab0[d];
	else if (d < (FONE/4)*3) {	/* 1/16..3/4 */
		f = d&15;
		d = d>>4;
		l = asin_tab1[d];
		l += (8+(asin_tab1[d+1] - l)*f)>>4;
	} else if (d < (FONE/16)*15) {	/* 3/4..15/16 */
		d -= (FONE/4)*3;	/* 0..1/4 */
		f = d&3;
		d = d>>2;
		l = asin_tab2[d];
		l += (2+(asin_tab2[d+1] - l)*f)/4;
	} else {			/* 15/16..1 */
		if (d > FONE)
			d = FONE;
		d -= (FONE/16)*15;	/* 0..1/16 */
		l = asin_tab3[d];
	}
	return (TANG((l^ind)-ind));
}

extern ANGLE FAR FASTCALL
my_atan (int y, int x)
{
	int	eight, f, i, d;
	ANGLE	l;

	eight = 0;
	if (y < 0) {
		y = -y;
		if (y < 0) {
			y = -(y/2);
			x = x/2;
		}
		eight = 1;
	}
	if (x < 0) {
		x = -x;
		if (x < 0) {
			y = y/2;
			x = -(x/2);
		}
		eight += 2;
	}
	if (y > x) {
		d = y;
		y = x;
		x = d;
		eight += 4;
	} else if (x == 0)
		return (0);

	d = fdiv (y, x);		/* temp */

	if (d < (FONE/4)*3) {		/* 0..3/4 */
		f = d&15;
		i = d>>4;
		l = atan_tab1[i];
		l += (8+(atan_tab1[i+1] - l)*f)>>4;
	} else if (d < (FONE/16)*15) {	/* 3/4..15/16 */
		d -= (FONE/4)*3;
		f = d&3;
		i = d>>2;
		l = atan_tab2[i];
		l += (2+(atan_tab2[i+1] - l)*f)>>2;
	} else {			/* 15/16..1 */
		if (d > FONE)
			d = FONE;
		d -= (FONE/16)*15;
		l = atan_tab3[d];
	}

	switch (eight) {
	case 0:				/* ++ */
		return (l);
	case 1:				/* -+ */
		return (-l);
	case 2:				/* +- */
		return (D180-l);
	case 3:				/* -- */
		return (D180+l);
	case 4:				/* ++ rev */
		return (D90-l);
	case 5:				/* -+ rev */
		return (-D90+l);
	case 6:				/* +- rev */
		return (D90+l);
	case 7:				/* -- rev */
		return (-D90-l);
	}
	return (0);			/* never reached */
}

extern Uxshort FAR FASTCALL
my_sqrt (Ulong x)
{
	register Uint	r, t;
	register int	e;

	if (x >= 0x00010000UL)
		if (x >= 0x01000000UL)
			if (x >= 0xfffe0001UL)
				return (0xffffU);
			else
				r = sqrtab[(x>>24)+1];
		else
			r = sqrtab[(x>>16)+1] >> 4;
	else if ((Uint)x >= 0x0100U)
		r = sqrtab[((Uint)x>>8)+1] >> 8;
	else
		return (sqrtab[x] >> 12);

	do {
		t = (Uint)(x / r);
		e = (int)(r - t) / 2;
		r -= e;
	} while (e);

	return ((Uxshort)t);
}

extern Ulong FAR FASTCALL
lhypot3d (LVECT A)		/* a bit of a cheat, but good enough! */
{
	Ulong	x, y, z;
	Uint	f, xyz;

	x = labs(A[X]);
	y = labs(A[Y]);
	z = labs(A[Z]);
	xyz = (Uint)((x/2+y/2+z/2) >> 15);
	for (f = 1; xyz; xyz /= 2, f *= 2) {
		x /= 2;
		y /= 2;
		z /= 2;
	}
	return (f * (Ulong)SQRT (x*x + y*y + z*z));
}

extern Ulong FAR FASTCALL
ldist3d (LVECT A, LVECT B)
{
	LVECT	LL;

	Vsub (LL, A, B);
	return (lhypot3d (LL));
}

extern Uxshort FAR FASTCALL
est_hyp (register int x, register int y, register int z)
{
	register Uint	d;

	if (x < 0)
		x = -x;
	if (y < 0)
		y = -y;
	if (z < 0)
		z = -z;
	if (x >= y)
		if (x >= z)
			d = x + ((Uint)(y+z)>>2);	/* x > y,z */
		else
			d = z + ((Uint)(y+x)>>2);	/* z > x > y */
	else
		if (y >= z)
			d = y + ((Uint)(x+z)>>2);	/* y > x,z */
		else
			d = z + ((Uint)(x+y)>>2);	/* z > y > x */
	return (d > 0x7fffU ? 0x7fff : d);
}

extern Uxshort FAR FASTCALL
est_dist (LVECT R1, LVECT R2)
{
	long	x, y, z;

	x = (R1[X] - R2[X])/VONE;
	if (x < 0)
		x = -x;
	if (x > 0x7fffL)
		return (0x7fff);
	y = (R1[Y] - R2[Y])/VONE;
	if (y < 0)
		y = -y;
	if (y > 0x7fffL)
		return (0x7fff);
	z = (R1[Z] - R2[Z])/VONE;
	if (z < 0)
		z = -z;
	if (z > 0x7fffL)
		return (0x7fff);
	return (est_hyp ((int)x, (int)y, (int)z));
}

/* This part generates the constant tables.
*/

#ifdef GEN_TAB

/* We need 'status' and 'LogPrintf' for debug.c
*/
struct status	NEAR st = {0};		/* needed for debug.c */

extern int FAR
LogPrintf (const char *fmt, ...)
{
	va_list		ap;
	int		i;

	va_start (ap, fmt);
	i = vfprintf (stderr, fmt, ap);
	va_end (ap);

	return (i);
}

LOCAL_FUNC void NEAR
emit_xshort (char *q, char *name, xshort *p, int size)
{
	int	i;

	size /= sizeof (*p);
	printf ("%s xshort %s[] = {", q, name);
	for (i = 0; i < size; ++i) {
		if (!(i%8))
			printf ("\n   ");
		printf ("%d%s ", (int)*p++, (i < size-1) ? "," : "");
	}
	printf ("\n};\n");
}

LOCAL_FUNC void NEAR
emit_Uxshort (char *q, char *name, Uxshort *p, int size)
{
	int	i;

	size /= sizeof (*p);
	printf ("%s Uxshort %s[] = {", q, name);
	for (i = 0; i < size; ++i) {
		if (!(i%8))
			printf ("\n   ");
		printf ("%u%s ", (Uint)*p++, (i < size-1) ? "," : "");
	}
	printf ("\n};\n");
}

LOCAL_FUNC void NEAR
emit_angle (char *q, char *name, ANGLE *p, int size)
{
	int	i;

	size /= sizeof (*p);
	printf ("%s ANGLE %s[] = {", q, name);
	for (i = 0; i < size; ++i) {
		if (!(i%8))
			printf ("\n   ");
		printf ("%hd%s ", *p++, (i < size-1) ? "," : "");
	}
	printf ("\n};\n");
}

extern int
main (int argc, char *argv[])
{
	init_funcs ();
	emit_xshort ("static", "FAR log_tab",   log_tab,   sizeof (log_tab));
	emit_xshort ("",       "NEAR sin_tab",  sin_tab,   sizeof (sin_tab));
	emit_Uxshort("static", "FAR sqrtab",    sqrtab,    sizeof (sqrtab));
	emit_angle  ("static", "FAR asin_tab0", asin_tab0, sizeof (asin_tab0));
	emit_angle  ("static", "FAR asin_tab1", asin_tab1, sizeof (asin_tab1));
	emit_angle  ("static", "FAR asin_tab2", asin_tab2, sizeof (asin_tab2));
	emit_angle  ("static", "FAR asin_tab3", asin_tab3, sizeof (asin_tab3));
	emit_angle  ("static", "FAR atan_tab1", atan_tab1, sizeof (atan_tab1));
	emit_angle  ("static", "FAR atan_tab2", atan_tab2, sizeof (atan_tab2));
	emit_angle  ("static", "FAR atan_tab3", atan_tab3, sizeof (atan_tab3));

	exit (0);
	return (0);
}
#endif
