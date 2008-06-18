/* --------------------------------- gear.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Account for landing gear effects.
 *
 * Not sure how it works, but I think that once I add the ground contact as
 * a series of forces (one for each contact) then I get:
 *
 * P0 = center of gravity, [0,0,0] in body axes.
 * P1 = contact point
 * F  = force at P1
 * P  = P1-P0, P1 in body axes
 *
 * Now a vector from F1=F*P is the component of force that operates on the
 * cg and generates body forces while the rest F2=F-F1 at a distance of |P|
 * is the moment.
*/

#include "plane.h"


#define HEIGHT		EX->misc[6]
#define MAXFORCE	FCON(0.05)

static int	sx, cx, sy, cy, sz, cz, cxcy;
static MAT	FM;

LOCAL_FUNC void NEAR
GearCalcs (OBJECT *p, struct gear *g, VECT MM, VECT F, int flag)
{
	VECT	GFe, GFb, GMb;
	int	t, dg, ddg, vfact;
	int	Ux, Uy, Vx, Vy, Vt, Fu, Fs, ce, se;
	ANGLE	e, gamma;

/* Find gear deflection.
*/
	dg = fdiv (-HEIGHT -fmul (g->y, sx) +fmul (g->x, sy), cxcy) -g->z;
	if (dg <= 0) {
		EX->equip &= ~flag;
		return;			/* no ground contact */
	} else
		EX->equip |= flag;

/* Find gear upwards force. If the gear deflection is less than the tyre
 * deflection then we have force (Ft) linear with the deflection. After
 * this point the precharge force (P) is exceeded and the force (Fs) is
 * linear with the reciplrocal of the strut length ( when the strut is at
 * half length the force is 2P, at 1/3rd length it is 3P etc.). The strut
 * has a damping force (Fd) linear with the deflection rate (ddg) and in
 * reverse direction.
*/
	if (dg <= g->dtp)		/* only tyre deflection */
		GFe[Z] = muldiv (g->P, dg, g->dtp);
	else {
		t = fmul ((int)(FONE/(VONE*C_PI/2)), cxcy);
		ddg = - fdiv (p->V[Z]*VONE, cxcy)
		      - muldiv (fmul (g->y, cx), p->dae[X], t)
		      + muldiv (fmul (g->x, cy), p->dae[Y], t);

		if (dg > g->dgmax)
			t = MAXFORCE;
		else {
			t = fdiv (g->dgmax-dg+g->dtp, g->dgmax);
			if (t < MAXFORCE)
				t = MAXFORCE;
		}

		GFe[Z] = fdiv (g->P, t) + muldiv (ddg, g->Cv, VONE*VONE);
		if (GFe[Z] < 0)
			GFe[Z] = 0;
	}

/* Get basic friction coefficients. In reality one should check for the
 * wheel being locked (and slipping) and apply the relevant co. but we
 * assume it does not happen.
*/
	Ux = g->us;
	Uy = muldiv (EX->brake, g->ub, 100) + g->ur;

/* Find what velocity the tyre actually sees.
 * 'vfact' allows accounting for angular velocity when the linear velocity
 * is small.
*/
	vfact = p->speed < 100*VONE ? VONE : 1;
	t = (int)(FONE/(C_PI/2))/vfact;
	Vx = p->vb[X]*vfact - muldiv (g->y, p->da[Z], t);
	Vy = p->vb[Y]*vfact + muldiv (g->x, p->da[Z], t);

/* 'gamma' is the angle (relative to the plane forward axis) of the tyre
 * velocity.
*/
	gamma = ATAN(Vx, iabs(Vy));
	Vt = ihypot2d (Vx, Vy);
	if (Vy < 0)
		Vt = -Vt;

/* These are the basic friction forces.
*/
	Fs = -fmul (GFe[Z], Ux);
	Fu = -fmul (GFe[Z], Uy);

	if (g->emax && EX->rudder) {

/* This tyre is steering. Find the steering angle e. Pedals effectiveness
 * is reduced with speed.
*/
		if (EP->APrate && p->speed > EP->APrate)
			t = muldiv (EX->rudder, EP->APrate, p->speed);
		else
			t = EX->rudder;

		e = -muldiv (t, g->emax, 100);
		ce = COS(e);
		se = SIN(e);

/* Find the velocities in tyre coordinates.
*/
		t  = fmul (Vx, ce) - fmul (Vy, se);
		Vy = fmul (Vy, ce) + fmul (Vx, se);
		Vx = t;

		Fs = Vx ? (Vx<0 ? -Fs : Fs) : 0;
		Fu = Vy ? (Vy<0 ? -Fu : Fu) : 0;

/* Now we assume that the side force is dependent on the difference
 * between the desired rolling angle (e) and the actual rolling angle
 * (gamma) and is linear with the forward velocity and the effective
 * load on the tyre. We do not allow for slip here.
*/
		t = fmul (GFe[Z], Ux);
		Fs += muldiv (fmul (t, SIN (e-gamma)), Vy, VONE*vfact);

/* Now we relate the tyre forces back to body axes.
*/
		GFe[X] = fmul (Fs, ce) + fmul (Fu, se);
		GFe[Y] = fmul (Fu, ce) - fmul (Fs, se);
	} else {
		GFe[X] = Vx ? (Vx<0 ? -Fs : Fs) : 0;
		GFe[Y] = Vy ? (Vy<0 ? -Fu : Fu) : 0;
	}

	VMmul (GFb, GFe, FM);
	Vinc (F, GFb);

/* get moments.
*/

/*
	.Y	.X	.Z

Y.	 0	-z	 x
X.	 z	 0	-y
Z.	-x	 y	 0

*/
	t = g->z + dg;

/* The units involved are:
 *
 * g->x		*VONE*VONE
 * F[]		*VONE/W
 * M[]		*2/PI/VONE/VONE/W	[frac]
 *
 * So we need the following factor for unit conversion.
 * The trailing '*4' is a fudge factor...
*/
#define VVV	(int)(VONE*VONE*C_PI/2*VONE*VONE*VONE/FONE)*4

	GMb[X] = -muldiv (GFb[Y], t,    VVV) + muldiv (GFb[Z], g->y, VVV);
	GMb[Y] =  muldiv (GFb[X], t,    VVV) - muldiv (GFb[Z], g->x, VVV);
	GMb[Z] = -muldiv (GFb[X], g->y, VVV) + muldiv (GFb[Y], g->x, VVV);
#undef VVV

	Vinc (MM, GMb);
}

/* Landing gear action.
*/
extern void FAR
LandGear (OBJECT *p, VECT F, VECT MM)
{
	int	t;
	Ushort	flag;

/* landing gear buffeting
*/
	if (EX->equip & EQ_GEAR) {
		t = ~1 & (p->speed/256);
		t = t*t;
		F[X] += Frand()%(1+2*t) - t;
		F[Y] -= (Frand()%(1+2*t))/64;
		F[Z] += Frand()%(1+2*t) - t;
	}

	if (!(EX->flags & PF_ONGROUND))
		return;

	HEIGHT += TADJ(p->V[Z]*VONE);
	p->R[Z] = HEIGHT/VONE;

	cx = p->cosx;
	cy = p->cosy;

	if (cx < FCON(0.5) || cy < FCON(0.5))
		return;				/* no ground contact */

	sx = p->sinx;
	sy = p->siny;

	sz = p->sinz;
	cz = p->cosz;

	cxcy = fmul (cx, cy);

/*
	 X.	 Y.	 Z.

.X	 cy	 sx*sy	-cx*sy
.Y	 0	 cx	 sx
.Z	 sy	-sx*cy	 cx*cy

*/
	FM[X][X] =  cy;
	FM[X][Y] =  0;
	FM[X][Z] =  sy;
	FM[Y][X] =  fmul (sx, sy);
	FM[Y][Y] =  cx;
	FM[Y][Z] = -fmul (sx, cy);
	FM[Z][X] = -fmul (cx, sy);
	FM[Z][Y] =  sx;
	FM[Z][Z] =  cxcy;

	for (flag = EQ_GEAR1, t = 0; t < rangeof(EP->gear) && EP->gear[t].z;
									++t) {
		GearCalcs (p, &EP->gear[t], MM, F, flag);
		flag <<= 1;
	}
}

#undef HEIGHT
