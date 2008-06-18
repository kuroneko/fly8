/* ------------------------------------ mat.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Matrix and vector manipulation.
*/

#include "plane.h"

#undef Mident
extern void FAR FASTCALL
Mident (MAT m)
{
	m[0][1] = m[0][2] = m[1][0] = 
	m[1][2] = m[2][0] = m[2][1] = 0;
	m[0][0] = m[1][1] = m[2][2] = FONE;
}

#undef Mxpose
extern void FAR FASTCALL
Mxpose (MAT m)
{
	register xshort	t;

	t = m[0][1];	m[0][1] = m[1][0];	m[1][0] = t;
	t = m[0][2];	m[0][2] = m[2][0];	m[2][0] = t;
	t = m[1][2];	m[1][2] = m[2][1];	m[2][1] = t;
}

extern void FAR FASTCALL
fMrotx (MAT m, register xshort s, register xshort c)
{
	register int	t;

	t = m[0][1];
	m[0][1] = fmul(t,c) - fmul(m[0][2],s);
	m[0][2] = fmul(t,s) + fmul(m[0][2],c);

	t = m[1][1];
	m[1][1] = fmul(t,c) - fmul(m[1][2],s);
	m[1][2] = fmul(t,s) + fmul(m[1][2],c);

	t = m[2][1];
	m[2][1] = fmul(t,c) - fmul(m[2][2],s);
	m[2][2] = fmul(t,s) + fmul(m[2][2],c);
}

extern void FAR FASTCALL
fMroty (MAT m, register xshort s, register xshort c)
{
	register int	t;

	t = m[0][0];
	m[0][0] =  fmul(t,c) + fmul(m[0][2],s);
	m[0][2] = -fmul(t,s) + fmul(m[0][2],c);

	t = m[1][0];
	m[1][0] =  fmul(t,c) + fmul(m[1][2],s);
	m[1][2] = -fmul(t,s) + fmul(m[1][2],c);

	t = m[2][0];
	m[2][0] =  fmul(t,c) + fmul(m[2][2],s);
	m[2][2] = -fmul(t,s) + fmul(m[2][2],c);
}

extern void FAR FASTCALL
fMrotz (MAT m, register xshort s, register xshort c)
{
	register int	t;

	t = m[0][0];
	m[0][0] = fmul(t,c) - fmul(m[0][1],s);
	m[0][1] = fmul(t,s) + fmul(m[0][1],c);

	t = m[1][0];
	m[1][0] = fmul(t,c) - fmul(m[1][1],s);
	m[1][1] = fmul(t,s) + fmul(m[1][1],c);

	t = m[2][0];
	m[2][0] = fmul(t,c) - fmul(m[2][1],s);
	m[2][1] = fmul(t,s) + fmul(m[2][1],c);
}

extern void FAR
Mobj (register OBJECT *p)
{
	build_mat (p->T,
		p->sinx = SIN(p->a[X]),
		p->cosx = COS(p->a[X]),
		p->siny = SIN(p->a[Y]),
		p->cosy = COS(p->a[Y]),
		p->sinz = SIN(p->a[Z]),
		p->cosz = COS(p->a[Z]));
}

extern void FAR
Myxz (MAT m, AVECT a)
{
	build_mat (m, SIN(a[X]), COS(a[X]), SIN(a[Y]), COS(a[Y]),
			SIN(a[Z]), COS(a[Z]));
}

extern void FAR
VxMmul (VECT R, VECT V, MAT M)
{
	Mxpose (M);
	VMmul (R, V, M);
	Mxpose (M);
}

extern void FAR
Mmul (MAT m, MAT t)
{
	VECT	temp;

	VMmul (temp, m[0], t);	Vcopy (m[0], temp);
	VMmul (temp, m[1], t);	Vcopy (m[1], temp);
	VMmul (temp, m[2], t);	Vcopy (m[2], temp);
}

extern void FAR
Vscale (VECT a, VECT b, int scalar)
{
	a[X] = fmul (b[X], scalar);
	a[Y] = fmul (b[Y], scalar);
	a[Z] = fmul (b[Z], scalar);
}

extern void FAR
Vmuldiv (VECT a, VECT b, int m, int d)
{
	if (1 == d) {
		if (1 == m)
			Vcopy (a, b);
		else {
			a[X] = b[X] * m;
			a[Y] = b[Y] * m;
			a[Z] = b[Z] * m;
		}
	} else if (1 == m) {
		a[X] = b[X] / d;
		a[Y] = b[Y] / d;
		a[Z] = b[Z] / d;
	} else {
		a[X] = muldiv (b[X], m, d);
		a[Y] = muldiv (b[Y], m, d);
		a[Z] = muldiv (b[Z], m, d);
	}
}

/* This routine builds the cosine matrix from the Euler angles.
 *
 *	cy*cz-sy*sx*sz	cy*sz+sy*sx*cz	-sy*cx
 *	-cx*sz		cx*cz		sx
 *	sy*cz+cy*sx*sz	sy*sz-cy*sx*cz	cy*cx
 *
 *	tt1 = cy*sz
 *	tt2 = sy*cz
 *	tt3 = cy*cz
 *	tt4 = sy*sz
 *
 *	tt3-tt4*sx	tt1+tt2*sx	-sy*cx
 *	-cx*sz		cx*cz		sx
 *	tt2+tt1*sx	tt4-tt3*sx	cy*cx
*/
extern void FAR
cbuild_matyxz (MAT T, int sx, int cx, int sy, int cy, int sz, int cz)
{
	int	tt1, tt2, tt3, tt4;
						/* x = pitch (up)	*/
						/* y = roll (c'wise)	*/
						/* z = yaw (left)	*/
	tt1 = fmul (cy, sz);
	tt2 = fmul (sy, cz);
	tt3 = fmul (cy, cz);
	tt4 = fmul (sy, sz);

	T[0][0] = tt3 - fmul (tt4, sx);
	T[0][1] = tt1 + fmul (tt2, sx);
	T[0][2] = -fmul (sy, cx);

	T[1][0] = -fmul (cx, sz);
	T[1][1] = fmul (cx, cz);
	T[1][2] = sx;

	T[2][0] = tt2 + fmul (tt1, sx);
	T[2][1] = tt4 - fmul (tt3, sx);
	T[2][2] = fmul (cy, cx);
}

/* This routine extracts the Euler angles from the cosine matrix. In the case
 * that the pitch is too high it attempts to recover by first observing that
 * the (roll+-heading) is readily available and then estimating the roll and
 * seting the heading.
 *
 * This is how the matrix is interpreted in the critical angles:
 *
 * rot(y)*rot(x)*rot(z):
 *
 *	.[0]		.[1]		.[2]
 *
 * [0].	cy*cz-sy*sz*sx	cy*sz+sy*cz*sx	-sy*cx	[0].
 * [1].	-cx*sz		cx*cz		sx	[1].
 * [2].	sy*cz+cy*sz*sx	sy*sz-cy*cz*sx	cy*cx	[2].
 *
 * If cx == 0 we have:
 *
 * [0].	cy*cz-sy*sz*sx	cy*sz+sy*cz*sx	0	[0].
 * [1].	0		0		sx	[1].
 * [2].	sy*cz+cy*sz*sx	sy*sz-cy*cz*sx	0	[2].
 *
 * which when sx == 1 is:
 *
 * [0].	c(y+z)          s(y+z)		0	[0].
 * [1].	0		0		1	[1].
 * [2].	s(y+z)		-c(y+z)		0	[2].
 *
 * and when sx == -1 is:
 *
 * [0].	c(y-z)		-s(y-z)		0	[0].
 * [1].	0		0		-1	[1].
 * [2].	s(y-z)		c(y-z)		0	[2].
 *
 *	.[0]		.[1]		.[2]
*/

extern void FAR
Mangles (OBJECT *p, MAT m, AVECT a, ANGLE dy)
{
	ANGLE	t1;

	if (iabs(m[1][2]) >= FCON (0.999)) {		/* ~2.5 degrees	*/
		a[X] = ATAN (m[1][2], ihypot2d (m[2][2], m[0][2]));

		a[Y] += dy;
		if ((m[2][2] < 0) != (iabs(a[Y]) > D90))
			a[Y] += D180;
		t1 = ATAN (m[2][0], m[0][0]);		/* roll+-dir	*/
		if (m[1][2] >= 0)			/* roll+dir	*/
			a[Z] = t1 - a[Y];		/* dir		*/
		else					/* roll-dir	*/
			a[Z] = a[Y] - t1;		/* dir		*/
	} else {
		a[X] = ASIN (m[1][2]);			/* pitch	*/
		a[Z] = ATAN (-m[1][0], m[1][1]);	/* dir		*/
		a[Y] = ATAN (-m[0][2], m[2][2]);	/* roll		*/
	}

	if (p) {
		p->sinx = SIN(p->a[X]);
		p->cosx = COS(p->a[X]);
		p->siny = SIN(p->a[Y]);
		p->cosy = COS(p->a[Y]);
		p->sinz = SIN(p->a[Z]);
		p->cosz = COS(p->a[Z]);
	}

}

/* This function uses the current Euler angles and the angular
 * velocities (p, q, r) to compute the new Euler angles. However, there is
 * an artifact of the Euler angles which makes the roll/heading indication
 * very jerky when the pitch is too close to the zenith/nadir. This is not
 * just a problem with accuracy but when the nose traverses a line (great
 * circle) that passes close to the zenith it actually will move rapidly
 * through different headings. Since the entity that is mostly stable at
 * this time is [heading-roll] (when diving it is [heading+roll] that is
 * stable) then the roll varies rapidly as well.
 *
 * This makes a pitch ladder un-usable in that position. But not all is lost
 * - this routine will stabilise the ladder (if requested) inside the (about)
 * 2 degrees circle around the vertical. This can be done by fixing the
 * roll (or the heading) and allowing the other angle to vary freely.
 *
 * Fixing the heading will mean that when climbing, executing a roll will
 * cause the ladder to roll as expected (while the heading indicates the
 * value as of the time this high pitch was entered). This gives s good
 * visual cue for orientation awareness, however you do not know the true
 * heading at which you will exit the maneuvre.
 *
 * Fixing the roll means that the ladder freezes at high pitch angles. The
 * heading however will be correct, and if one rolls at a high pitch and
 * then pulls 'up' to exit the vertical then the heading is correctly
 * predicted. This is usefull if you want to exit at a prescribed heading.
*/

#define HOLDRADIUS	573		/* 2 degrees */
#define BADRADIUS	57		/* 0.2 degrees */

extern void FAR
Euler (OBJECT *p)
{
	int	cx, sy, cy, options, dy /*, dz*/;

	options = IS_PLANE(p) ? EE(p)->ladder : 0;

	cx = COS(p->a[X]);
	cy = COS(p->a[Y]);
	sy = SIN(p->a[Y]);

	p->dae[X] = fmul (p->da[X], cy) + fmul (p->da[Z], sy);
	dy = /* dz = */ 0;

	if (cx >= BADRADIUS) {
		p->dae[Z] = fmul (p->da[Z], cy) - fmul (p->da[X], sy);
		if ((iabs(p->dae[Z])>>1) >= (Uint)cx)		/* truncate */
			p->dae[Z] = (int)(p->dae[Z] * (long)FONE / cx);
		else
			p->dae[Z] = fdiv (p->dae[Z], cx);
		p->dae[Y] = p->da[Y] - fmul (p->dae[Z], SIN(p->a[X]));
		if (cx < HOLDRADIUS && (options & LD_HOLD)) { /* too jumpy */
			if (options & LD_HOLDROLL) {
				sy = fdiv (HOLDRADIUS-cx, HOLDRADIUS-BADRADIUS);
				if (iabs(p->a[Y]) > D90)
					dy = TADJ(D180-p->a[Y]); /* roll->180 */
				else
					dy = TADJ(-p->a[Y]);	/* roll->0 */
				dy = fmul (2*dy, sy);	/* gradual */
				cy = fmul (p->dae[Y], sy);
				if (p->a[X] > 0) {
					p->dae[Z] += cy;
/*					dz =  -dy;*/
				} else {
					p->dae[Z] -= cy;
/*					dz =   dy;*/
				}
				p->dae[Y] -= cy;	/* hold roll */
			} else {
				if (p->a[X] > 0)
					p->dae[Y] += p->dae[Z];
				else
					p->dae[Y] -= p->dae[Z];
				p->dae[Z] = 0;		/* hold heading */
			}
		}
	} else {
		if (options & LD_HOLDROLL) {
			p->dae[Y] = 0;			/* hold roll */
			p->dae[Z] = -p->da[Y];
		} else {
			p->dae[Z] = 0;			/* hold heading */
			p->dae[Y] =  p->da[Y];
		}
	}
}

/*************** from here on, only old (obsolete) stuff *******************
*/

#if 0
extern void FAR
Mangles1 (MAT m, AVECT a)		/* obsolete */
{
	ANGLE	t1;
	int	t2;

	t1 = ASIN (m[1][2]);				/* pitch +-pi/2 */
	t2 = COS (t1);					/* always +ve	*/
	if (iabs(t1) > (D90-D90/16)) {			/* 6 degrees	*/
		t2 = ihypot2d (m[1][0], m[1][1]);
		a[X] = ATAN (m[1][2], t2);

		a[Y] = 0;
		if (m[2][2] < 0)
			a[Y] = D180 - a[Y];
		t1 = ATAN (m[2][0], m[0][0]);		/* roll+-dir	*/
		if (m[1][2] >= 0)			/* roll+dir	*/
			a[Z] = t1 - a[Y];		/* dir		*/
		else					/* roll-dir	*/
			a[Z] = a[Y] - t1;		/* dir		*/
	} else {
		a[X] = t1;				/* pitch	*/
		a[Z] = -ATAN (m[1][0], m[1][1]);	/* dir		*/
		a[Y] = -ATAN (m[0][2], m[2][2]);	/* roll		*/
	}
}

extern void FAR
Mangles (MAT m, AVECT a)
/*
 * Extract angles from orientation matrix. The pitch is always in the range
 * -90...+90 (cos is +ve). When pointing along the z coord the direction
 * and roll are mixed, so we assume old roll stays and we extract new
 * direction only. There is some problem with the resolution of the arcsin
 * near this vertical direction (actualy, whenever sin is close to 1).
*/
{
	ANGLE	t1;
	int	t2;

	t1 = ASIN (m[1][2]);				/* pitch +-pi/2 */
	a[X] = t1;
#if 1
	t2 = COS (t1);					/* always +ve	*/
#else
	t2 = fmul(m[1][0],m[1][0])+fmul(m[1][1],m[1][1]); /* cx**2 */
#endif
	if (t2 < 64) {
#if 0
		if (m[0][2]) {
			t2 = SIN (a[Y]);
			t2 = fmul (m[1][2], iabs (t2));
			a[X] = ATAN (t2, iabs (m[0][2]));
		} else if (m[2][2]) {
			t2 = COS (a[Y]);
			t2 = fmul (m[1][2], iabs (t2));
			a[X] = ATAN (t2, iabs (m[2][2]));
		}
#endif
#if 0
		if (m[1][0]) {
			t2 = SIN (a[Z]);
			t2 = fmul (m[1][2], iabs (t2));
			a[X] = ATAN (t2, iabs (m[1][0]));
		} else if (m[1][1]) {
			t2 = COS (a[Z]);
			t2 = fmul (m[1][2], iabs (t2));
			a[X] = ATAN (t2, iabs (m[1][1]));
		}
#endif
		t1 = ATAN (m[2][0], m[0][0]);		/* roll+-dir	*/
#if 0
						/* keep old roll a[Y]	*/
		if (a[X] >= 0)				/* roll+dir	*/
			a[Z] = t1 - a[Y];		/* dir		*/
		else					/* roll-dir	*/
			a[Z] = a[Y] - t1;		/* dir		*/
#endif
#if 0
						/* keep old dir a[Z]	*/
		if (a[X] >= 0)				/* roll+dir	*/
			a[Y] = t1 - a[Z];		/* roll		*/
		else					/* roll-dir	*/
			a[Y] = t1 + a[Z] ;		/* roll		*/
#endif
#if 1
		a[Y] = 0;				/* zero roll	*/
		if (a[X] >= 0)				/* roll+dir	*/
			a[Z] = t1;			/* dir		*/
		else					/* roll-dir	*/
			a[Z] = -t1;			/* dir		*/
#endif
	} else {
		a[Z] = -ATAN (m[1][0], m[1][1]);	/* dir		*/
		a[Y] = -ATAN (m[0][2], m[2][2]);	/* roll		*/
	}
}
#endif

#if 0
}
	ANGLE	t1;
	int	t2, t3;

	t1 = ASIN(m[1][2]);				/* pitch +-pi/2*/
	a[X] = t1;
	t2 = COS (t1);					/* always +ve	*/
	if (t2 < 1) {
		t1 = ASIN (m[2][0]);			/* roll+-dir	*/
		if (m[0][0] < 0)
			t1 = D180 - t1;
#if 0
		/* keep old roll a[Y]					*/
		if (a[X] >= 0)				/* roll+dir	*/
			a[Z] = t1 - a[Y];		/* dir		*/
		else					/* roll-dir	*/
			a[Z] = a[Y] - t1;		/* dir		*/
#else
		/* keep old dir a[Z]					*/
		if (a[X] >= 0)				/* roll+dir	*/
			a[Y] = t1 - a[Z];		/* toll		*/
		else					/* roll-dir	*/
			a[Y] = t1 + a[Z] ;		/* roll		*/
#endif
	} else {
		if (m[1][0] > t2)
			t3 = FONE;
		else if (-m[1][0] > t2)
			t3 = -FONE;
		else
			t3 = fdiv(m[1][0],t2);
		a[Z] = -ASIN (t3);			/* dir		*/
		if (m[1][1] < 0)
			a[Z] = D180 - a[Z];

		if (m[0][2] > t2)
			t3 = FONE;
		else if (-m[0][2] > t2)
			t3 = -FONE;
		else
			t3 = fdiv(m[0][2],t2);
		a[Y] = -ASIN (t3);			/* roll		*/
		if (m[2][2] < 0)
			a[Y] = D180 - a[Y];
	}
}
#endif
