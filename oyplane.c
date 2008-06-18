/* --------------------------------- oyplane.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Dynamics of the second eXpermental plane.
 * This model will eventualy do a proper aerodynamic simulation. It will
 * go as far as 6DOF (simplified) and will properly implement a linearized
 * serodynamics model. This program is being modified regularly as I learn
 * more about the subject and gather more data. If I get a stable version
 * then it will be separated into it's own module so as not to constantly
 * have a half working program. But not yet.
 *
 * ToDo:
 * 1 document units/scale used for all variables.
 * 2 select aero formulas and break into classes:
 *   - input parameters (from .prm)
 *   - calculated fixed parameters
 *   - slow changing parameters
 *   - dynamically calculated variables.
 * 3 use proper trans- and super-sonic calcs for Cd (wave drag).
 * 4 add wing sweep to calcs?
*/

#include "plane.h"


#define DOPITCH		(st.debug & DF_GPW)

/* 'CLf' is to avoid overflow (fractions are limited to the range [-2...+2)).
*/
#define	CLf	4

LOCAL_FUNC void NEAR
GetFoil (OBJECT *p, ANGLE flaps, ANGLE leFlaps, ANGLE *a0, int *maxCl,
	int *minCl)
{
	int	t;

	*a0 = EP->Cl0 + fmul (EP->FEff, flaps);
	t =  muldiv (EP->FEffCl, flaps, CLf);
	t += muldiv (EP->lefEffCl, leFlaps, CLf);
	*maxCl = EP->maxCl/CLf*10 + t;
	*minCl = EP->minCl/CLf*10 + t;
}
	
LOCAL_FUNC int NEAR
GetCl (OBJECT *p, ANGLE aoa, ANGLE a0, int a, int maxCl, int minCl,
	int *Cl, int *Fstall)
{
	int	aoaPstall, aoaNstall;
	int	fstall, cl, stall;
	ANGLE	t;

	aoa -= a0;				/* get absolute aoa */
	a *= CLf;
	aoaPstall = fmul (maxCl, a);
	aoaNstall = fmul (minCl, a);
	if (aoa > aoaPstall) {
		cl = maxCl;
		if (!(EX->flags & PF_NOSTALL)) {
			t = aoa - aoaPstall;
			if (t >= aoaPstall/2)
				fstall = 0;
			else
				fstall = fdiv (t, aoaPstall)*2;
		} else
			fstall = FONE;
		cl = fmul (cl, fstall);
		stall = 1;
	} else if (aoa < aoaNstall) {
		cl = minCl;
		if (!(EX->flags & PF_NOSTALL)) {
			t = aoa - aoaNstall;
			if (t <= aoaNstall/2)
				fstall = 0;
			else
				fstall = fdiv (t, aoaNstall)*2;
		} else
			fstall = FONE;
		cl = fmul (cl, fstall);
		stall = 1;
	} else {
		fstall = FONE;
		cl = fdiv (aoa, a);
		stall = 0;
	}
	*Cl = cl;
	if (Fstall)
		*Fstall = fstall;
	return (stall);
}

/* We use SI units (Newton, Kilogram, meter, second, radian) but read in some
 * data in other units (feet, pound, pound-force, degrees etc.).
 *
 * Since we use integers, some scaling factors are introduced, sometimes
 * varying along the calculations. Angles are stored in a special format
 * were pi/2 radians (90 degrees) is 1.0, and the representation wraps around
 * at 2.0, the full range being [-2.0...2.0).
 *
 * Definitions:
 *
 * b	wing span (m)
 * S	wing area (m^2)
 * ClRate0 slope of Cl for the wing foil (1/r)
 *	we assume it to be 2*pi 
 * e	Oswald's (or span) efficiency factor
 * aoa0	angle of attack where Cl is zero (r)
 * feff	degrees aoa effective for 1 degree flaps
 *
 * AR	wing aspect ratio
 * k_AR	1/(pi*e*AR)
 * ClRate  slope of Cl for wing (1/r)
 * e1	drag span-efficiency-factor. We use 'e' for it.
 *
 * V	airspeed (m/s)
 * rho	air density (k/m^3)
 * aoa	angle of attack, geometric (r)
 * aoaf	angle of attack effect due to flaps (r)
 * aoai	induced angle of attack (r)
 *	= Cl / (pi*e1*AR), but folded into ClRate
 * aeff	effective angle of attack (r)
 *  ... all of these angles in degrees!
 * q	dynamic pressure (k/m^2)
 * Cl	lift coefficient
 * Cd	drag coefficient
 * Cdi	induced Cd
 * Cdp	profile Cd
 *
 * Formulas used:
 *
 * AR = b^2 / S
 * aoai = Cl / (pi*e1*AR)
 * ClRate = ClRate0 / (1+ClRate0/(pi*e1*AR))
 *  or:
 * 1/ClRate = 1/ClRate0 + 1/(pi*e1*AR)
 *  since we assume ClRate0 = 2*pi we now have
 * 1/ClRate = 2*pi*e1*AR/(e1*AR+2)
 *
 * aoaf = flaps * feff
 * aeff = aoa + aoaf
 * Cl = (aeff - aoa0) * ClRate
 *
 *
 *
 *
 * Cdi = Cl^2 /(pi*e*AR)
 * Cdp = Cdp_characteristic + Cdp_flaps + Cdp_gear + Cdp_bombs + Cdp_speedbrakes
 * Cd = Cdi + Cdp
 * q = rho * V^2 / 2
 * lift is perpendicular to airflow:
 * 	lift = Cl * q * S
 * drag is parallel to airflow:
 * 	drag = Cd * q * S
 *
 * We need these factors:
 * Ixx	pitching moment of inertia
 * Iyy	rolling moment of inertia
 * Izz	yawing moment of inertia
 * we calculate the moments as:
 * Mx = lift*Lc + drag*Dc + thrust*Tc + (stablilizer+elevators)*Sc
 * Lc, Dc, Tc, Sc: effective moments levers.
 * My,Mz = f(many things...)
*/

extern void FAR
dynamics_yplane (OBJECT *p, int action)
{
	int	onground;
	int	t, weight, thrust, wfact;
	int	rrho, rho, sos, sa, ca, sb, cb;
	int	stall;
	long	tt;

	if (action)
		return;

	if (dynamics_input (p))
		return;

	onground = EX->flags & PF_ONGROUND;

	if (onground ? check_takeoff (p) : check_land (p)) {
		p->flags |= F_HIT;
		return;
	}

/* Automatically refuel when stationery on ground.
*/
	if (onground && 0 == p->speed && 0 == EX->power)
		supply (p, 0);

	airdata (p->R[Z], 0, &rrho, &rho, &sos);
CFshow (p, "rrho", rrho);
CFshow (p, "rho", rho);
CCshow (p, 0, "sos", (long)sos/VONE);

/* Get engine thrust.
*/
	f16engine (p, sos);

/* We select a factor to use in the integer arithmetic to avoid overflow
 * and maintain resolution.
*/
	wfact = (int)(EP->weight/1000)+1;
CCshow (p, 0, "wfact", (long)wfact);

/* Thrust is stored as lbf/10.
*/
	t = muldiv (4*10, EX->thrust, wfact);
	thrust = fmul (t, FCON(4.4482/4));			/* lbf->N */
CCshow (p, 0, "athrust", (long)thrust*wfact);

	tt = EP->weight + EX->fuel/100;				/* lb */
	if ((t = EX->stores[WE_M61-1]) > 0)
		tt += t * st.bodies[O_M61]->shape->weight/1000;
	if ((t = EX->stores[WE_MK82-1]) > 0)
		tt += t * st.bodies[O_MK82]->shape->weight/1000;
	weight = fmul ((int)(tt/wfact), FCON (0.4536));		/* Kg */
CCshow (p, 0, "weight", (long)weight*wfact);

	thrust = muldiv (thrust, VONE, weight);
CVshow (p, "thrust", thrust);

/* Flying, although may still have ground contact. Must do full aerodynamics.
*/
	{
	typedef int	FRACTION;

	FRACTION	AR;		/* wing aspect ratio (1/AR) */
	FRACTION	pi_e_AR;	/* 1/(e*pi*AR) */
	FRACTION	a;		/* Cl rate */
	ANGLE		aFlaps;		/* flaps */
	ANGLE		leFlaps;	/* leading edge flaps */
	ANGLE		alpha;		/* angle of attack (aoa) */
	ANGLE		beta;		/* angle of sideslip */
	FRACTION	Cl;
	int		Fstall;
	FRACTION	Cdi;
	FRACTION	Cd;
	int		S;
	int		b;
	int		c;
	int		V;
	int		qS_V;
	int		qS;
	int		qS_V_b2;
	int		qS_V_c;
	int		lift;
	int		drag;
	int		sf;		/* side force */
	int		b2fact;
	int		qSfact;
	int		m;
	FRACTION	Crudder, Cailerons, Celevators;
	FRACTION	Ca, Cb, Cdx, Cdy, Cdz;
	VECT		F, M;
	MAT		I;

/* First get some basic data.
*/
	AR = fdiv (EP->wing_area, muldiv (EP->wing_span, EP->wing_span, VONE));
						/* AR */
	t = fmul (AR, FCON(1.0/C_PI));		/* 1/(pi*AR) */
	pi_e_AR = muldiv (t, 100, EP->efficiency_factor);
						/* 1/(e*pi*AR) */

	a = FCON(1/(2*C_PI));			/* note we keep 1/a */
	a += pi_e_AR;
	a = RAD2ANG (a);

/* Find our alpha, beta and friends.
*/
	if (onground && iabs(p->vb[Y]) < 2*VONE)
		alpha = beta = 0;
	else {
		alpha = -ATAN (p->vb[Z], p->vb[Y]);
		beta  =  ASIN (fdiv (p->vb[X], p->speed));
	}
	alpha += EP->Aoffset;			/* wing rigging offset */
	EX->aoa = alpha;
	sa = SIN (alpha);
	ca = COS (alpha);
	sb = SIN (beta);
	cb = COS (beta);
CAshow (p, "alpha", alpha);
CAshow (p, "beta", beta);

/* Get flaps effect. We introduce automatic flaps in response to alpha.
*/
	aFlaps = EX->flaps;
	if (EX->flags & PF_AUTOFLAP) {
		if (aFlaps) {
			EX->leFlaps = aFlaps;
		} else {
			if (alpha > EP->AFamin) {
				aFlaps = fmul (alpha-EP->AFamin,
								EP->AFrate);
				if (aFlaps > EP->AFmax)
					aFlaps = EP->AFmax;
			} 
			if (alpha > EP->ALEFamin) {
				EX->leFlaps = fmul (alpha-EP->ALEFamin,
								EP->ALEFrate);
				if (EX->leFlaps > 100)
					EX->leFlaps = 100;
			} else
				EX->leFlaps = 0;
		}
	} else {
		EX->leFlaps = 0;
	}
	aFlaps  = muldiv (EP->MaxFlaps, aFlaps, 100);
	leFlaps = muldiv (EP->MaxLEFlaps, EX->leFlaps, 100);
{
	int	maxCl, minCl;
	ANGLE	a0;

	GetFoil (p, aFlaps, leFlaps, &a0, &maxCl, &minCl);
	stall = 0;
	EX->flags &= ~PF_STALL;
	if (GetCl (p, alpha, a0, a, maxCl, minCl, &Cl, &Fstall)) {
		EX->flags |= PF_STALL;
		if (!(EX->flags & PF_NOSTALL))
			stall = 1;
	}
}
	Cdi = fmul (pi_e_AR, Cl)*CLf;
	Cdi = fmul (Cdi, Cl)*CLf;

CFshow (p, "Cl", Cl);
CFshow (p, "Cdi", Cdi);

/* The groung effect formula is extracted from the graph in Smiths 'The
 * Illustrated Guide To Aerodynamics' end of chapter 3.
*/
	t = EP->wing_span;
	if (p->R[Z] < (long)t) {		/* ground effect */
		t = fdiv ((int)p->R[Z], t);	/* h/b */
CFshow (p, "h/b", t);
		if (t < FCON(0.1))
			t = 5 * t;
		else
			t = FCON(1.06) - fdiv (FCON(0.07), t);
CFshow (p, "gef", t);
		Cdi = fmul (Cdi, t);
CFshow (p, "Cdi", Cdi);
	}

	Cd = EP->Cdp0;
CFshow (p, "Cdp0", Cd);
	Cd += Cdi;
CFshow (p, "+Cdi", Cd);
	if (EX->airbrake) {
		Cd += muldiv (EP->Cds, EX->airbrake, 100);
CFshow (p, "+Brks", Cd);
	}
	if (EX->equip & EQ_GEAR) {
		Cd += EP->Cdg;
CFshow (p, "+Gear", Cd);
	}
	if (EX->stores[WE_MK82-1] > 0) {
		Cd += EX->stores[WE_MK82-1] * EP->CdMK82;
CFshow (p, "+MK82", Cd);
	}

/* We now calculate the new forces, based on the state of the plane.
 * The given state is:
 *  Cdx		Rotation around x (nose up, pitch)
 *  Cdy		Rotation around y (right wing down, roll)
 *  Cdz		Rotation around z (nose right, yaw)
 *  Crudder	rudder left-turn angle
 *  Cailerons	ailerons roll-right (cwise) angle
 *  Celevators	elevators push-down angle
 *  Ca		alpha: nose above wind angle
 *  Cb		beta:  nose left-of-wind angle
 * The forces are:
 *  Fx		right
 *  Fy		forward
 *  Fz		up
 * The moments are:
 *  Mx		around x (nose up, pitch)
 *  My		around y (right wing down, roll)
 *  Mz		around z (nose right, yaw)
 *
 * From each force we get acceleration [a = F/m] and then velocity [v += a*t]
 * and position [x += (v + a*t/2)*t].
 *
 * From each moment we get angular acceleration [aa = M/I], then we get
 * rotation rate [da += aa*t] and angle [a += (da + aa*t/2)*t].
*/
	V = p->speed/VONE;
CCshow (p, 0, "V", (long)V);
	S = EP->wing_area;
CCshow (p, 0, "S", (long)S/VONE);
	b = EP->wing_span;
CCshow (p, 2, "b", b*100L/VONE);
	c = EP->wing_cord;
CCshow (p, 2, "c", c*100L/VONE);

/* Scale factors: These were selected so avoid integer overflow while
 * maintaining good resolution, so they may look weird.
 *
 * item:	is larger than real life
 *		by a factor of:
 *
 * b		*VONE
 * c		*VONE
 * S		*VONE
 * qS_V		*VONE/wfact
 * qS		*VONE/W
 * qS_V_b2	/VONE/W			[frac]
 * qS_V_c	/VONE/W			[frac]
 * Cdx		*2/PI/VONE		[frac]
 * Cdy		*2/PI/VONE		[frac]
 * Cdz		*2/PI/VONE		[frac]
 * lift		*VONE/W
 * drag		*VONE/W
 * side force	*VONE/W
 * F[]		*VONE/W
 * M[]		*2/PI/VONE/VONE/W	[frac]
 * I[]		/W/10			[frac]
*/
	qS_V    = muldiv (fmul (rho, S), V, 2*wfact);
	qS      = muldiv (qS_V, V, weight);
	qS_V_b2 = muldiv (qS_V, (FONE/VONE/VONE/VONE)*b/2, weight);
	qS_V_c  = muldiv (qS_V, (FONE/VONE/VONE/VONE)*c, weight);
CCshow (p, 0, "qS", (long)qS);
CCshow (p, 0, "qS_V_b2", (long)qS_V_b2);
CCshow (p, 0, "qS_V_c", (long)qS_V_c);

/* Keep the load within the designated range by limiting the elevators.
*/
	Celevators = EX->elevators + EX->tElevators;
	Celevators = fmul (Celevators, Fstall);
	if (EX->flags & PF_AUTOELEVATOR) {
		t = EP->AErate/VONE;
		if (V > t)
			Celevators = muldiv (Celevators, t, V);
	}
	Celevators = -muldiv (EP->MaxElevators, Celevators, RAD2ANG(100));

	Cailerons = -muldiv (EP->MaxAilerons, EX->ailerons, RAD2ANG(100));
	Cailerons = fmul (Cailerons, Fstall);

	Crudder = muldiv (EP->MaxRudder, EX->rudder+EX->tRudder, RAD2ANG(100));
	Crudder = fmul (Crudder, Fstall);

	Cdx =  p->da[X];
	Cdy =  p->da[Y];
	Cdz = -p->da[Z];

	if (alpha >= FCON(4/C_PI))
		Ca = FCON(1.99);
	else if (alpha <= -FCON(4/C_PI))
		Ca = -FCON(1.99);
	else
		Ca = ANG2RAD(alpha);

	Cb = ANG2RAD(beta);
CFshow (p, "Crdr", Crudder);
CFshow (p, "Cail", Cailerons);
CFshow (p, "Celev", Celevators);
CFshow (p, "Cdx", Cdx);
CFshow (p, "Cdy", Cdy);
CFshow (p, "Cdz", Cdz);
CFshow (p, "Ca", Ca);
CFshow (p, "Cb", Cb);

	M[X] = M[Y] = M[Z] = 0;

/* Lift, drag and side-force.
*/
	lift =  fmul (Cl, qS * CLf);
	drag = -fmul (Cd, qS);
	sf   =	fmul (qS, fmul (EP->Cydr, Crudder));
	if (Cb < FCON(0.1))
		sf += fmul (qS, fmul (EP->Cybeta, Cb*10));
	else {
		t = fmul (EP->Cybeta, Cb);
		if (t < FCON(0.2))
			sf += fmul (qS, t*10);
		else
			sf += fmul (qS, t)*10;
	}
CVshow (p, "lift", lift);
CVshow (p, "drag", drag);
CVshow (p, "sf", sf);

/* Aerodynamic forces about the c.g..
*/
	F[X] = fmul (sb, drag) + sf;
	t    = fmul (cb, drag);
	F[Y] = fmul (sa, lift) + fmul (ca, t);
	F[Z] = fmul (ca, lift) - fmul (sa, t);

/* Engine forces about the c.g..
 *
 * We break the thrust into linear and angular components.
*/
	F[Y] += fmul (thrust, COS(EP->Ea));
	F[Z] += fmul (thrust, SIN(EP->Ea));

	t = -fmul (thrust, SIN(EP->Ea + EP->Eb));	/* angular */
	M[X] += fmul (t, EP->Er);

CVshow (p, "Tx", F[X]);
CVshow (p, "Ty", F[Y]);
CVshow (p, "Tz", F[Z]);

/* Add force of gravity.
*/
	F[X] -= fmul (GACC, p->T[X][Z]);
	F[Y] -= fmul (GACC, p->T[Y][Z]);
	F[Z] -= fmul (GACC, p->T[Z][Z]);

/* Attempt to improve resolution.
*/
#define MAXB2	FCON(0.5)
	if (qS_V_b2 < MAXB2/(VONE*VONE))	/* too small */
		b2fact = VONE*VONE;
	else if (F(b2fact = MAXB2/qS_V_b2))	/* just right */
		b2fact = 1;	 		/* too large */
CCshow (p, 0, "b2fact", b2fact);
	qS_V_b2 *= b2fact;
	qS_V_c  *= b2fact;
	b2fact	*= VONE;
#undef MAXB2

#define MAXQS	FCON(C_PI/2*VONE*VONE*VONE/FONE)
	if (qS < MAXQS/(VONE*VONE))		/* too small */
		qSfact = VONE*VONE;
	else if (F(qSfact = MAXQS/qS))		/* just right */
		qSfact = 1;	 		/* too large */
CCshow (p, 0, "qSfact", qSfact);
	qS      *= qSfact;
	qSfact	*= VONE;

	qS = fdiv (qS, MAXQS);
#undef MAXQS


/* Pitching moment.
*/

if (DOPITCH) {
	m =	+ fmul (qS, EP->Cm0)
		+ fmul (fmul (qS, EP->Cmalpha), Ca);
	M[X] += muldiv (m, c, qSfact);
CFshow (p, "Cmx", m);
}
	m =	+ fmul (fmul (qS, EP->Cmde), Celevators);
	M[X] += muldiv (m, c, qSfact);

	m =	+ fmul (fmul (qS_V_c, EP->Cmq), Cdx);
	M[X] += muldiv (m, 10*c, b2fact);

	
/* Rolling moment.
*/
	m =	+ fmul (fmul (qS, EP->Clda), Cailerons)
		+ fmul (fmul (qS, EP->Cldr), Crudder)
		+ fmul (fmul (qS, EP->Clbeta), Cb);	/* hihedral */
	M[Y] += muldiv (m, b, qSfact);

#define Clr	(Cl*(CLf/4))
	m =	+ fmul (fmul (qS_V_b2, EP->Clp), Cdy)	/* damping */
		+ fmul (fmul (qS_V_b2, Clr), Cdz);	/* yaw induced */
	M[Y] += muldiv (m, b, b2fact);
#undef Clr


/* Yawing moment.
*/
	m =	+ fmul (fmul (qS, EP->Cndr), Crudder)
		+ fmul (fmul (qS, EP->Cnda), Cailerons)
		+ fmul (fmul (qS, EP->Cnbeta), Cb);	/* weathercock */
	M[Z] -= muldiv (m, b, qSfact);

	m =	+ fmul (fmul (qS_V_b2, EP->Cnr), Cdz)	/* damping */
		+ fmul (fmul (qS_V_b2, EP->Cnp), Cdy);
	M[Z] -= muldiv (m, b, b2fact);

CFshow (p, "Mx", M[X]);
CFshow (p, "My", M[Y]);
CFshow (p, "Mz", M[Z]);

	I[X][X] = EP->Iyy;
	I[Y][Y] = EP->Ixx;
	I[Z][Z] = EP->Izz;
	I[Y][Z] = EP->Izx;

	LandGear (p, F, M);

	SixDOF (p, F, M, I);

	Mobj (p);

CCshow (p, 3, "G", EX->Gforce*1000L/GACC);
tt = p->R[Z] + V*VONE*(long)V/(2L*GACC/VONE);
CCshow (p, 0, "He", (long)(tt*3.28/VONE));
tt = p->V[Z] + VONE*V*(long)ihypot3d(p->ab)/GACC;
CCshow (p, 0, "Ps", (long)(tt*3.28/VONE));
tt = tt*60L;
CCshow (p, 0, "RC", (long)(tt*3.28/VONE));
}

	p->speed = ihypot3d (p->vb);
	VMmul (p->V, p->vb, p->T);

#define MAX_SPEED	(1000*VONE)
	if (p->speed > MAX_SPEED) {			/* temp */
		t = fdiv (MAX_SPEED, p->speed);
		p->vb[X] = fmul (t, p->vb[X]);
		p->vb[Y] = fmul (t, p->vb[Y]);
		p->vb[Z] = fmul (t, p->vb[Z]);
		p->V[X]  = fmul (t, p->V[X]);
		p->V[Y]  = fmul (t, p->V[Y]);
		p->V[Z]  = fmul (t, p->V[Z]);
		p->speed = ihypot3d (p->vb);
	}
#undef MAX_SPEED

	if (onground)
		EX->flags &= ~PF_STALL;

	if (EX->Gforce > EP->max_lift || EX->Gforce < EP->min_lift)
		EX->flags |= PF_GLIMIT;
	else
		EX->flags &= ~PF_GLIMIT;

/* Mach number.
*/
	EX->mach = muldiv (p->speed, 1000, sos);

/* pull up warning time.
*/
	t = muldiv (10000, iabs(p->speed), 300*VONE);
	t = muldiv (t, iabs(p->a[X]), D90);
	if (t < 2000)
		t = 2000;
	EX->misc[8] = t;
}

#undef DOPITCH
#undef CLf
