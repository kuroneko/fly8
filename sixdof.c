/* --------------------------------- sixdof.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Resolve the plane's motion. The main program uses a different axes system
 * than is common in aerodynamics. Here is the relationship:
 * standard	fly8	meaning
 * x		 y	forward
 * y		 x	right
 * z		-z	down
 * p		 dy	roll (clockwise)
 * q		 dx	pitch (up)
 * r		-dz	yaw (right)
*/

#include "plane.h"

#define FUNDAMENTAL	(st.debug & DF_GPZ)

extern void FAR
SixDOF (OBJECT *p, VECT F, VECT MM, MAT I)
{
	int	t, tt, onground;
	AVECT	dae;
	xshort	Gforce;

	if (IS_PLANE (p))
		onground = EX->flags & PF_ONGROUND;
	else
		onground = 1;

/* This is the 'simple' way of doing the kinematics.
*/
if (!FUNDAMENTAL) {
	t = (int)(FONE/(C_PI/2*VONE));

	p->ab[Y] = F[Y] + muldiv (p->da[X], p->vb[Z], t)
				- muldiv (p->da[Z], p->vb[X], t);
	p->ab[X] = F[X] - muldiv (p->da[Y], p->vb[Z], t)
				+ muldiv (p->da[Z], p->vb[Y], t);
	p->ab[Z] = F[Z] + muldiv (p->da[Y], p->vb[X], t)
				- muldiv (p->da[X], p->vb[Y], t);
	Gforce = F[Z];

	p->dda[X] = muldiv (MM[X], FONE/10, I[X][X]);
	if (p->dda[X] > FONE/VONE)
		p->dda[X] = FONE;
	else if (p->dda[X] < -FONE/VONE)
		p->dda[X] = -FONE;
	else
		p->dda[X] = muldiv (MM[X], FONE/10*VONE, I[X][X]);

	p->dda[Y] = muldiv (MM[Y], FONE/10, I[Y][Y]);
	if (p->dda[Y] > FONE/VONE)
		p->dda[Y] = FONE;
	else if (p->dda[Y] < -FONE/VONE)
		p->dda[Y] = -FONE;
	else
		p->dda[Y] = muldiv (MM[Y], FONE/10*VONE, I[Y][Y]);

	p->dda[Z] = muldiv (MM[Z], FONE/10*VONE, I[Z][Z]);

	tt = muldiv (p->da[Y], p->da[Z], t);
	p->dda[X] -= muldiv (tt, I[Z][Z] - I[Y][Y], I[X][X]);

	tt = muldiv (p->da[X], p->da[Z], t);
	p->dda[Y] -= muldiv (tt, I[Z][Z] - I[X][X], I[Y][Y]);

	tt = muldiv (p->da[Y], p->da[X], t);
	p->dda[Z] -= muldiv (tt, I[Y][Y] - I[X][X], I[Z][Z]);
} else {
	double	u, v, w, _p, q, r;
	double	L, M, N, XX, YY, ZZ;
	double	Ixx, Iyy, Izz, Ixz;
	double	Dp, Dq, Dr, Du, Dv, Dw;
	double	VtoM, AtoR;
	double	f;

/* These are the 6DOF rigid body equations. They are presented in over-
 * simplified form (assume Jxz is negligible), rough form (assume Jxz is
 * small so ignore Ixz^2), complete but optimized (using C0-C10) and raw
 * form.
 *
 * It is all done in body axes.
 *
 * Note that the forces and moments are divided by the weight but this has
 * no effect on the calcs.
*/

/* First convert state to SI units.
 * Forces and moments are scaled by 1/M.
 * Inertia moments are scaled by 1/(10*M).
*/
	AtoR = (double)VONE*C_PI/2/FONE;	/* Angles->Radians */
	VtoM = 1/(double)VONE;			/* Vunits->Meters */

	u  =  p->vb[Y] *VtoM;
	v  =  p->vb[X] *VtoM;
	w  = -p->vb[Z] *VtoM;

	_p =  p->da[Y] *AtoR;
	q  =  p->da[X] *AtoR;
	r  = -p->da[Z] *AtoR;

	L  =  MM[Y] *AtoR*VONE;
	M  =  MM[X] *AtoR*VONE;
	N  = -MM[Z] *AtoR*VONE;

	XX =  F[Y] *VtoM;
	YY =  F[X] *VtoM;
	ZZ = -F[Z] *VtoM;

	Ixx = I[Y][Y] *10.0/FONE;
	Iyy = I[X][X] *10.0/FONE;
	Izz = I[Z][Z] *10.0/FONE;
	Ixz = I[Y][Z] *10.0/FONE;

/* Get acceleration about c.g.
*/
	Du = XX -  q*w + r*v;
	Dv = YY + _p*w - r*u;
	Dw = ZZ - _p*v + q*u;

/* Get rotations about c.g.
*/

#if 0
/* The plain 6DOF calculations. Of course, even here we assume that Jxz is
 * the only significant small cross term.
*/
{
	double	Irr, Ppq, Pqr, Pn, Pl, Qpp, Qpr, Qrr, Qm, Rpq, Rqr, Rn,
		Rl;

	Irr = Ixx*Izz - Ixz*Ixz;

	Ppq = (Ixx - Iyy + Izz)*Ixz/Irr;
	Pqr = ((Iyy - Izz)*Izz - Ixz*Ixz)/Irr;
	Pn  = Ixz/Irr;
	Pl  = Izz/Irr;

	Qpp = -Ixz/Iyy;
	Qpr = (Izz - Ixx)/Iyy;
	Qrr = Ixz/Iyy;
	Qm  = 1/Iyy;

	Rpq = ((Ixx - Iyy)*Ixx + Ixz*Ixz)/Irr;
	Rqr = (Iyy - Izz - Ixx)*Ixz/Irr;
	Rn  = Ixx/Irr;
	Rl  = Ixz/Irr;

	Dp  = Ppq*_p*q  + Pqr* q*r + Pn*N    + Pl*L;
	Dq  = Qpp*_p*_p + Qpr*_p*r + Qrr*r*r + Qm*M;
	Dr  = Rpq*_p*q  + Rqr* q*r + Rl*L    + Rn*N;
}
#endif

#if 0
/* These are the same formulaes with the terms reorganized.
*/
{
	double	C0, C1, C2, C3, C4, C5, C6, C7, C8, C9, C10;

	C0 = Ixx*Izz - Ixz*Ixz;
	C1 = Izz/C0;
	C2 = Ixz/C0;
	C3 = C2*(Ixx - Iyy + Izz);
	C10 = C2*Ixz;
	C4 = C1*(Iyy - Izz) - C10;
	C5 = 1/Iyy;
	C6 = C5*Ixz;
	C7 = C5*(Izz - Ixx);
	C8 = Ixx/C0;
	C9 = C8*(Ixx - Iyy) + C10;

	Dp = (Tp = L*C1) + N*C2 + (_p*C3 - r*C4)*q;
	Dq = (Tq = M*C5) + (r+_p)*(r-_p)*C6 + r*_p*C7;
	Dr = (Tr = N*C8) + L*C2 + (_p*C9 - r*C3)*q;
}
#endif

#if 1
/* Here we assume that Ixz*Ixz is effectively zero. These can be simplified
 * a bit more. Note that we can now defer the divisions so the 1/Ixx term was
 * extracted from all the coefficients. C0, C1, C5, C6 and C8 were trivialized
 * and C10 is zero.
*/
{
	double	C2, C3, C4, C6, C7, C9;
	double	pq, rq;

	C2 = Ixz/Izz;
	C3 = C2*(Ixx - Iyy + Izz);
	C4 = Iyy - Izz;
	C6 = Ixz;
	C7 = Izz - Ixx;
	C9 = Ixx - Iyy;

	pq = _p*q;
	rq =  r*q;

	Dp = (L + N*C2 + pq*C3 - rq*C4)/Ixx;
	Dq = (M + (r+_p)*(r-_p)*C6 + r*_p*C7)/Iyy;
	Dr = (N + L*C2 + pq*C9 - rq*C3)/Izz;
}
#endif

#if 0
/* Here we assume that Ixz is effectively zero.
 * we get C2 = C3 = C6 = 0 which leaves us with C4, C7 and C9.
*/
	Dp = (L +  q*r*(Izz - Iyy))/Ixx;
	Dq = (M + _p*r*(Izz - Ixx))/Iyy;
	Dr = (N + _p*q*(Ixx - Iyy))/Izz;
#endif

#if 0
/* Here we assume small rates and thus ignore all but the simple angular
 * velocity that each axis incures.
*/
	Dp = L/Ixx;
	Dq = M/Iyy;
	Dr = N/Izz;
#endif

	p->ab[X] =  (int)(Dv /VtoM);
	p->ab[Y] =  (int)(Du /VtoM);
	p->ab[Z] = -(int)(Dw /VtoM);

	Gforce = -(int)(ZZ/VtoM);

	p->dda[X] =  (int)(Dq /AtoR);

	f =  Dp /AtoR;
	if (f > (double)FONE)
		f = (double)FONE;
	else if (f < -(double)FONE)
		f = -(double)FONE;
	p->dda[Y] = (int)f;

	p->dda[Z] = -(int)(Dr /AtoR);
}
	Gforce += fmul (GACC, p->T[Z][Z]);
	if (IS_PLANE (p))
		EX->Gforce = Gforce;

/* The if() in the next segment are for stabilizing the simulation. Since
 * we use integers, and the simulation is discrete (not continuous), it is
 * likely that a 'standing still' craft will actually appear to be badly
 * shiverring instead. This is the result of applying the friction force
 * one way, then the other way on the next frame. We avoid this by forcing
 * the rates to become zero before changing direction. Well, yes, you could
 * call it a kludge.
*/
	t = p->vb[X];
	p->vb[X] += TADJ(p->ab[X]);
	if (onground && ((t > 0 && p->vb[X] < 0) || (t < 0 && p->vb[X] > 0)))
		p->vb[X] = 0;
		
	t = p->vb[Y];
	p->vb[Y] += TADJ(p->ab[Y]);
	if (onground && ((t > 0 && p->vb[Y] < 0) || (t < 0 && p->vb[Y] > 0)))
		p->vb[Y] = 0;

	t = p->vb[Z];
	p->vb[Z] += TADJ(p->ab[Z]);
	if (onground && ((t > 0 && p->vb[Z] < 0) || (t < 0 && p->vb[Z] > 0)))
		p->vb[Z] = 0;

	t = p->da[X];
	p->da[X] += TADJ(p->dda[X]);
	if (onground && ((t > 0 && p->da[X] < 0) || (t < 0 && p->da[X] > 0)))
		p->da[X] = 0;

	t = p->da[Y];
	p->da[Y] += TADJ(p->dda[Y]);
	if (onground && ((t > 0 && p->da[Y] < 0) || (t < 0 && p->da[Y] > 0)))
		p->da[Y] = 0;

	t = p->da[Z];
	p->da[Z] += TADJ(p->dda[Z]);
	if (onground && ((t > 0 && p->da[Z] < 0) || (t < 0 && p->da[Z] > 0)))
		p->da[Z] = 0;

	AVcopy (dae, p->dae);

	Euler (p);
#if 0								/* Adams 2nd */
	p->a[X] = TANG(p->a[X] + (t = p->dae[X] + (p->dae[X]-dae[X])/2,
		iabs(t) < VMAX/VONE ? TADJ(t*VONE) : TADJ(t)*VONE));
	p->a[Y] = TANG(p->a[Y] + (t = p->dae[Y] + (p->dae[Y]-dae[Y])/2,
		iabs(t) < VMAX/VONE ? TADJ(t*VONE) : TADJ(t)*VONE));
	p->a[Z] = TANG(p->a[Z] + (t = p->dae[Z] + (p->dae[Z]-dae[Z])/2,
		iabs(t) < VMAX/VONE ? TADJ(t*VONE) : TADJ(t)*VONE));
#endif
#if 0								/* trapezoid */
	p->a[X] = TANG(p->a[X] + (t = (dae[X]+p->dae[X])/2,
		iabs(t) < VMAX/VONE ? TADJ(t*VONE) : TADJ(t)*VONE));
	p->a[Y] = TANG(p->a[Y] + (t = (dae[Y]+p->dae[Y])/2,
		iabs(t) < VMAX/VONE ? TADJ(t*VONE) : TADJ(t)*VONE));
	p->a[Z] = TANG(p->a[Z] + (t = (dae[Z]+p->dae[Z])/2,
		iabs(t) < VMAX/VONE ? TADJ(t*VONE) : TADJ(t)*VONE));
#endif
#if 1								/* rectangle */
	p->a[X] = TANG(p->a[X] + (t = p->dae[X],
		iabs(t) < VMAX/VONE ? TADJ(t*VONE) : TADJ(t)*VONE));
	p->a[Y] = TANG(p->a[Y] + (t = p->dae[Y],
		iabs(t) < VMAX/VONE ? TADJ(t*VONE) : TADJ(t)*VONE));
	p->a[Z] = TANG(p->a[Z] + (t = p->dae[Z],
		iabs(t) < VMAX/VONE ? TADJ(t*VONE) : TADJ(t)*VONE));
#endif

	if (iabs (p->a[X]) > D90) {
		p->a[X] = TANG(          D180 - p->a[X]);
		p->a[Y] = TANG(p->a[Y] + D180);
		p->a[Z] = TANG(p->a[Z] + D180);
	}
}
#undef FUNDAMENTAL
