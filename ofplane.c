/* --------------------------------- ofplane.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Dynamics of the floating point expermental model.
 *
 * This model will eventualy do a proper aerodynamic simulation. It will
 * go as far as 6DOF (simplified) and will properly implement a linearized
 * serodynamics model. This program is being modified regularly as I learn
 * more about the subject and gather more data. If I get a stable version
 * then it will be separated into it's own module so as not to constantly
 * have a half working program. But not yet.
 *
 * Note that the external parameters (EP->*) and internal data (EX->*)
 * are read as fixed point so some conversion factors are attached to
 * each. Once converted these become SI units.
 *
 * ToDo:
 * 1 use proper trans- and super-sonic calcs for Cd (wave drag).
 * 2 add wing sweep to calcs?
 *
 *
 * The dimentionless derivatives are
 *
 * CL		lift, calculated from wing data
 * CD		drag, calculated from wing data
 * Cy.dr	rudder sideforce
 * Cy.beta	vx damping
 *
 * Cl.da	aileron effectiveness
 * Cl.p		roll damping
 * Cl.beta	dihedral effect
 * Cl.dr	roll from rudder
 * Cl.r		roll from yaw (always set to Cl/4.0)
 *
 * Cm0		total Cm at 0 alpha
 * Cm.de	elevators effectiveness
 * Cm.q		pitch damping
 * Cm.alpha	alpha (stabilizer) induced pitch
 * Cm.alphadot	alpha rate induced pitch
 *
 * Cn.dr	rudder effectiveness
 * Cn.r		yaw damping
 * Cn.beta	weathercock stability
 * Cn.da	ailerons induced yaw
 * Cn.p		roll induced yaw
 *
 *
 * The aerodynamic coefficients are:
 *
 * t  = - Cd		*cos(beta)
 * Cx =   Cl		*sin(alpha)	[lift componnent]
 *      + t		*cos(alpha)	[drag componnent]
 * Cz = - Cl		*cos(alpha)	[lift componnent]
 *      + t		*sin(alpha)	[drag componnent]
 * Cy = - Cd		*sin(beta)	[drag componnent]
 *	+ Cy.dr		*rudder
 *      + Cy.beta	*beta
 *
 * Cl =   Cl.da		*ailerons
 *      + Cl.beta	*beta
 *	+ Cl.dr		*rudder
 *      + Cl.p		*p		*b/2V
 *      + Cl.r		*r		*b/2V
 *
 * Cm =   Cm0
 *	+ Cm.de		*elevators
 *      + Cm.q		*q		*c/V
 *      + Cm.alpha	*alpha
 *      + Cm.alphadot	*alphadot	*c/V
 *      + Cn.beta	*beta
 *	+ Cn.da		*ailerons
 *      + Cn.p		*p		*b/2V
 *
 * Cn =   Cn.dr		*rudder
 *      + Cn.r		*r		*b/2V
 *
 * The aerodynamic forces are:
 *
 * X = Cx*q*S
 * Y = Cy*q*S
 * Z = Cz*q*S
 *
 * The aerodynamic moments are:
 *
 * L = Cl*q*S*b
 * M = Cm*q*S*c
 * N = Cn*q*S*b

*/

#include "plane.h"

#include <math.h>


#define FVONE	((float)VONE)
#define FFONE	((float)FONE)
#define F10FONE	(FFONE/10.0)
#define FA2R	(C_PI/2.0/FFONE)	/* angles to radians */
#define FR2D	(180.0/C_PI)		/* radians to degrees */

#define DOPITCH		(st.debug & DF_GPW)
#define DOYPLANE	(st.debug & DF_GPY)
#define FUNDAMENTAL	(st.debug & DF_GPZ)

static float fDD[] = {1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0, 1000000.0};
#define CCfshow(n,t,v)	CCshow(p, (n), t, (long)(fDD[n]*(v)))

LOCAL_FUNC void NEAR
fairdata (float height, float *rrho, float *rho, float *sos)
{
	int	irrho, irho, isos;

	airdata ((long)(height*VONE), 0, &irrho, &irho, &isos);
	if (rrho)
		*rrho = irrho/FFONE;
	if (rho)
		*rho  = irho/FFONE;
	if (sos)
		*sos  = isos/FVONE;
}

/* Get the total weight, including fuel and load.
*/
LOCAL_FUNC void NEAR
calc_weight (OBJECT *p, float *weight)
{
	float	t, tt;

	tt = EP->weight + EX->fuel/100.0;
	if ((t = EX->stores[WE_M61-1]) > 0)
		tt += t * st.bodies[O_M61]->shape->weight/1000.0;
	if ((t = EX->stores[WE_MK82-1]) > 0)
		tt += t * st.bodies[O_MK82]->shape->weight/1000.0;
	*weight = tt * 0.4536;					/* lb->Kg */
}

/* Get wing coefficients from foil parameters and flaps settings.
*/
LOCAL_FUNC void NEAR
fGetFoil (OBJECT *p, float Cl0, float max_cl, float min_cl, float flaps,
	float leFlaps, float *a0, float *maxCl, float *minCl)
{
	float	t;

	*a0 = Cl0 + EP->FEff/FFONE * flaps;
	t =  EP->FEffCl/FFONE/FA2R * flaps;
	t += EP->lefEffCl/FFONE/FA2R * leFlaps;
	*maxCl = max_cl + t;
	*minCl = min_cl + t;
}

/* Calculate Cl and Cdi.
*/	
LOCAL_FUNC int NEAR
fGetCl (OBJECT *p, float aoa, float a0, float pi_e_AR, float maxCl, float minCl,
	float *Cl, float *Cdi, float *Fstall, float *Downwash)
{
	float	a, aoaPstall, aoaNstall, cl, fstall;
	float	t;
	int	stall;

	a = 1.0/(2.0*C_PI) + pi_e_AR;
	aoa -= a0;				/* get absolute aoa */
	aoaPstall = maxCl * a;
	aoaNstall = minCl * a;
	if (aoa > aoaPstall) {
		cl = maxCl;
		if (!(EX->flags & PF_NOSTALL)) {
			t = (aoa - aoaPstall)*2;
			if (t >= aoaPstall)
				fstall = 0.0;
			else
				fstall = t / aoaPstall;
		} else
			fstall = 1.0;
		cl *= fstall;
		stall = 1;
	} else if (aoa < aoaNstall) {
		cl = minCl;
		if (!(EX->flags & PF_NOSTALL)) {
			t = (aoa - aoaNstall)*2;
			if (t <= aoaNstall)
				fstall = 0.0;
			else
				fstall = t / aoaNstall;
		} else
			fstall = 1.0;
		cl *= fstall;
		stall = 1;
	} else {
		fstall = 1.0;
		cl = aoa / a;
		stall = 0;
	}
	*Cl = cl;
	*Cdi = pi_e_AR * cl * cl;
	if (Fstall)
		*Fstall = fstall;
	if (Downwash)
		*Downwash = cl * pi_e_AR;	/* or 2.0 * ? */
	return (stall);
}

/* We use SI units (newton, kilogram, meter, second, radian) but read in some
 * data in other units (feet, pound, pound-force, degrees etc.).
*/
extern void FAR
dynamics_fplane (OBJECT *p, int action)
{
	int	onground, stall, it;
	float	t, weight, thrust;
	float	rrho, rho, sos, sa, ca, sb, cb;
	float	AR;		/* wing aspect ratio (1/AR) */
	float	pi_e_AR;	/* 1/(e*pi*AR) */
	float	aFlaps;		/* flaps */
	float	leFlaps;	/* leading edge flaps */
	float	alpha;		/* geometric angle of attack */
	float	walpha;		/* wing angle of attack */
	float	beta;		/* angle of sideslip */
	float	alphadot;
	float	downwash;	/* downwash angle */
	float	Cl;
	float	Cdi;
	float	Cstall;		/* stall level, [1...0]=[none...full] */
	float	Cd;
	float	S;
	float	b;
	float	c;
	float	V, h;
	float	qS_V;
	float	qS;
	float	qS_V_b2;
	float	qS_V_c;
	float	lift, drag;
	float	Crudder, Cailerons, Celevators;
	float	uu, vv, ww;
	float	pp, qq, rr;
	float	XX, YY, ZZ;
	float	LL, MM, NN;
	float	Fx, Fy, Fz;
	float	mx, my, mz;

	if (DOYPLANE) {
		dynamics_yplane (p, action);
		return;
	}

	if (action)
		return;

	if (dynamics_input (p))
		return;

	onground = EX->flags & PF_ONGROUND;

	if (onground ? check_takeoff (p) : check_land (p)) {
		p->flags |= F_HIT;
		return;
	}

	uu =  p->vb[Y] /FVONE;
	vv =  p->vb[X] /FVONE;
	ww = -p->vb[Z] /FVONE;

	pp =  p->da[Y] *FA2R*FVONE;
	qq =  p->da[X] *FA2R*FVONE;
	rr = -p->da[Z] *FA2R*FVONE;

	Celevators = -EP->MaxElevators*FA2R *
				(EX->elevators + EX->tElevators)/100.0;
	Cailerons = -EP->MaxAilerons*FA2R * EX->ailerons/100.0;
	Crudder = EP->MaxRudder*FA2R * (EX->rudder + EX->tRudder)/100.0;

	V = p->speed /FVONE;
	h = p->R[Z] /FVONE;

CCfshow (1, "uu", uu);
CCfshow (1, "vv", vv);
CCfshow (1, "ww", ww);
CCfshow (2, "pp", pp*FR2D);
CCfshow (2, "qq", qq*FR2D);
CCfshow (2, "rr", rr*FR2D);
CCfshow (2, "elv", Celevators*FR2D);
CCfshow (2, "ail", Cailerons*FR2D);
CCfshow (2, "rud", Crudder*FR2D);
CCfshow (1, "V", V);
CCfshow (1, "h", h);

/* Automatically refuel when stationery on ground.
*/
	if (onground && 0 == p->speed && 0 == EX->power)
		supply (p, 0);

	fairdata (h, &rrho, &rho, &sos);
CCfshow (4, "rrho", rrho);
CCfshow (4, "rho", rho);
CCfshow (1, "sos", sos);

/* These are common terms.
*/
	S = EP->wing_area /FVONE;
	b = EP->wing_span /FVONE;
	c = EP->wing_cord /FVONE;

	qS_V    = rho * V / 2.0 * S;	/* q*S/V */
	qS      = qS_V * V;		/* q*S */
	qS_V_b2 = qS_V * b / 2.0;	/* q*S/V*b/2 */
	qS_V_c  = qS_V * c;		/* q*S/V*c */

/* Engine thrust.
*/
	f16engine (p, (xshort)(sos*FVONE));

/* Thrust is read as lbf/10.
*/
	thrust = EX->thrust * 10.0 * 4.4482;			/* lbf->N */
CCfshow (0, "thrust", thrust);

/* Current weight.
*/
	calc_weight (p, &weight);
CCfshow (0, "weight", weight);

/* Find our alpha, beta and friends.
*/
	if (onground && fabs(uu) < 2.0)
		alphadot = walpha = alpha = beta = 0.0;
	else {
		alpha = atan2 (ww, uu);
		beta  = asin (vv/V);
		walpha = alpha + EP->Aoffset*FA2R;	/* wing rigging */
		alphadot = (-p->ab[Z]*uu - p->ab[Y]*ww)/FVONE
				/ (uu*uu + ww*ww);
	}
	EX->aoa = TANG(walpha/FA2R);
	sa = sin (alpha);
	ca = cos (alpha);
	sb = sin (beta);
	cb = cos (beta);
CCfshow (2, "alpha", walpha*FR2D);
CCfshow (2, "beta", beta*FR2D);
CCfshow (2, "alpha.", alphadot*FR2D);

/* Get flaps effect. We introduce automatic flaps in response to alpha.
*/
	aFlaps = EX->flaps;
	if (EX->flags & PF_AUTOFLAP) {
		if (aFlaps) {
			EX->leFlaps = EX->flaps;
		} else {
			if (walpha > EP->AFamin*FA2R) {
				aFlaps = (walpha-EP->AFamin*FA2R)
						* EP->AFrate/FFONE/FA2R;
				if (aFlaps > EP->AFmax*1.0)
					aFlaps = EP->AFmax*1.0;
			}
			if (walpha > EP->ALEFamin*FA2R) {
				EX->leFlaps = (int)((walpha-EP->ALEFamin*FA2R)
						* EP->ALEFrate/FFONE/FA2R);
				if (EX->leFlaps > 100)
					EX->leFlaps = 100;
			} else
				EX->leFlaps = 0;
		}
	} else {
		EX->leFlaps = 0;
	}
	aFlaps  = EP->MaxFlaps*FA2R * aFlaps/100.0;
	leFlaps = EP->MaxLEFlaps*FA2R * EX->leFlaps/100.0;

	if (EX->flaps)
		aFlaps = EP->MaxFlaps*FA2R * EX->flaps/100.0;
	else if (walpha > EP->AFamin*FA2R && (EX->flags & PF_AUTOFLAP)) {
		aFlaps = (walpha-EP->AFamin*FA2R) * EP->AFrate/FFONE;
		aFlaps = EP->MaxFlaps*FA2R * aFlaps;
		if (aFlaps > EP->AFmax*FA2R)
			aFlaps = EP->AFmax*FA2R;
	} else
		aFlaps = 0.0;
	leFlaps = 0.0;

/* Get some basic data.
*/
	AR = EP->wing_area / (EP->wing_span * (float)EP->wing_span)*FVONE;
						/* 1/AR */
	t = AR / C_PI;				/* 1/(pi*AR) */
	pi_e_AR = t / (EP->efficiency_factor/100.0);
						/* 1/(e*pi*AR) */
{
	float	maxCl, minCl;
	float	a0;

	fGetFoil (p, EP->Cl0*FA2R, EP->maxCl/F10FONE, EP->minCl/F10FONE, aFlaps,
			leFlaps, &a0, &maxCl, &minCl);
	stall = 0;
	EX->flags &= ~PF_STALL;
	if (fGetCl (p, walpha, a0, pi_e_AR, maxCl, minCl, &Cl, &Cdi, &Cstall,
			&downwash)) {
		EX->flags |= PF_STALL;
		if (!(EX->flags & PF_NOSTALL))
			stall = 1;
	}
	Cailerons *= Cstall;
CCfshow (3, "Cl", Cl);
CCfshow (3, "Cdi", Cdi);
CCfshow (2, "e", downwash*FR2D);
}

/* The ground effect formula is extracted from the graph in Smiths 'The
 * Illustrated Guide To Aerodynamics' end of chapter 3.
*/
	t = EP->wing_span /FVONE;
	if (h < t) {				/* ground effect */
		t = h / t;			/* h/b */
		if (t < 0.1)
			t *= 5.0;
		else
			t = 1.06 - 0.07 / t;
		Cdi *= t;
CCfshow (3, "Cdi", Cdi);
	}

	Cd = EP->Cdp0/FFONE;
	Cd += Cdi;
	if (EX->airbrake)
		Cd += EP->Cds/FFONE * EX->airbrake/100.0;
	if (EX->equip & EQ_GEAR)
		Cd += EP->Cdg/FFONE;
	if (EX->stores[WE_MK82-1] > 0)
		Cd += EX->stores[WE_MK82-1] * (EP->CdMK82/FFONE);
CCfshow (3, "Cd", Cd);

/* We now calculate the new forces, based on the state of the plane.
 * The given state is:
 *  pp		Rotation around y (right wing down, roll)
 *  qq		Rotation around x (nose up, pitch)
 *  rr		Rotation around z (nose right, yaw)
 *  Crudder	rudder left-turn angle
 *  Cailerons	ailerons roll-right (cwise) angle
 *  Celevators	elevators push-down angle
 *  alpha	nose above wind angle
 *  beta	nose left-of-wind angle
 * The forces are:
 *  XX		forward
 *  YY		right
 *  ZZ		down
 * The moments are:
 *  LL		around x (right wing down, roll)
 *  MM		around y (nose up, pitch)
 *  NN		around z (nose right, yaw)
 *
 * From each force we get acceleration [a = F/m] and then velocity [v += a*t]
 * and position [x += (v + a*t/2)*t].
 *
 * From each moment we get angular acceleration [aa = M/I], then we get
 * angular rate [da += aa*t] which we use to update the Euler angles.
*/

/* The auto-elevators reduces the elevators control sensitivity linearly
 * with the velocity. It engages at the prescribed velocity. Note the fudge
 * factor for the fundamental model.
*/
	if (EX->flags & PF_AUTOELEVATOR) {
		t = EP->AErate /FVONE;
		if (FUNDAMENTAL) {
			if (DOPITCH)
				t /= 1.8;	/* experimental */
			else
				t /= 4.0;
		} else
			t /= 1.2;
		if (V > t)
			Celevators *= t/V;
	}

	LL = MM = NN = 0.0;
	XX = YY = ZZ = 0.0;

	
/* Gravity.
*/
	YY -= weight * GACC/FVONE * p->T[X][Z]/FFONE;
	XX -= weight * GACC/FVONE * p->T[Y][Z]/FFONE;
	ZZ += weight * GACC/FVONE * p->T[Z][Z]/FFONE;


/* Engines thrust.
 * If the engine is mounted at [x,z] (forward and above cg) at an angle of
 * Ea above the x-y plane then we have:
 * Eb = atan2 (z,-x)
 * Er = sqrt (x*x+z*z)
*/
	XX += thrust * cos (EP->Ea*FA2R);
	ZZ -= thrust * sin (EP->Ea*FA2R);

/* Lift and drag.
*/
	lift =  Cl * qS;
	drag = -Cd * qS;

	Fy =  sb * drag;
	t  =  cb * drag;
	Fx =  sa * lift + ca * t;
	Fz = -ca * lift + sa * t;

	YY += Fy;
	XX += Fx;
	ZZ += Fz;
CCfshow (0, "lift", lift);
CCfshow (0, "drag", drag);
CCfshow (0, "X", Fx);
CCfshow (0, "Y", Fy);
CCfshow (0, "Z", Fz);

if (FUNDAMENTAL) {
/*
 **************************************************
 *
 * This section evaluates forces/moments from more
 * fundamental aerodynamic principles.
 *
 **************************************************
*/

/* Lift induced pitching moment.
*/
	my = -Fz*EP->ACy/FVONE - Fx*EP->ACz/FVONE;
	MM += my;
CCfshow (0, "Ml", my);

/* Wing pitching moment.
*/
	my = qS * EP->Cm0w/FFONE;
	MM += my * b;
CCfshow (0, "Mw", my*b);

/* Tail (stabilizers and elevators).
 * Here we actually simulate tailerons - the whole stabilizer is rotated
 * as an elevator, not as tail flaps.
 * Downash ignored.
*/
    {
	float	ClTail, CdTail, Tlift, Tdrag;
	float	ta, sta, cta, dta;
	float	TAR;
	int	estall;

	ta = alpha - downwash;			/* actual alpha */
	sta = sin (ta);
	cta = cos (ta);
	ta += EP->Toffset*FA2R;
	ta += Celevators;			/* tailerons */

	if (uu)
		ta += (dta = atan2 (-qq * EP->TACy/FVONE, uu));
	else
		dta = 0;
CCfshow (2, "Tda", dta*FR2D);
CCfshow (2, "Taoa", ta*FR2D);

	TAR = EP->tail_area / (EP->tail_span * (float)EP->tail_span)*FVONE;
	t = pi_e_AR * TAR/AR;
	estall = fGetCl (p, ta, 0.0, t, EP->TmaxCl/F10FONE, EP->TminCl/F10FONE,
			&ClTail, &CdTail, 0, 0);
	t = qS * EP->tail_area/EP->wing_area;
	Tlift = ClTail * t;
	Tdrag = -(CdTail + EP->Cdp0/FFONE) * t;
	Fx =  sta * Tlift + cta * Tdrag;
	Fz = -cta * Tlift + sta * Tdrag;
	my = -Fz * EP->TACy/FVONE - Fx * EP->TACz/FVONE;

	MM += my;
	XX += Fx;
	ZZ += Fz;
CCfshow (0, estall ? "Tstall" : "Tlift", Tlift);
CCfshow (0, "Tdrag", Tdrag);
CCfshow (0, "Xt", Fx);
CCfshow (0, "Zt", Fz);
CCfshow (0, "Mt", my);
    }

/* Body drag. Quite a kludge.
*/
	Fy  =	- qS * 10.0 * sb;
	YY += Fy;
	Fz  =	- qS * 0.5 * sa;
	ZZ += Fz;
CCfshow (0, "Dy", Fy);
CCfshow (0, "Dz", Fz);

/* Engine thrust induced pitching moment.
*/
	my = -thrust * sin ((EP->Ea + EP->Eb)*FA2R);
	MM += my * EP->Er/FVONE;
CCfshow (0, "Mp", my);

/* Pitching moment damping.
*/
if (DOPITCH) {
	my =	  qS_V_c  * EP->Cmq/F10FONE   * qq;	/* damping */
	MM +=  my * c;
CCfshow (0, "Md", my*c);
}
	my =	  qS_V_c  * EP->Cmalphadot/F10FONE * alphadot;
	MM +=  my * c;
CCfshow (0, "Ma.", my*c);

/* Rolling moment.
*/
#define Clr	(Cl/4.0)
	mx =	+ qS      * EP->Clda/FFONE    * Cailerons
		+ qS_V_b2 * EP->Clp/FFONE     * pp	/* damping */
		+ qS_V_b2 * Clr               * rr	/* yaw induced */
		+ qS      * EP->Clbeta/FFONE  * beta;	/* hihedral */
	LL += mx * b;
#undef Clr


/* Yawing moment.
*/

/* Rudder. We assume the foil is symmerical.
*/
    {
	float	ClRudd, CdRudd, Rlift, Rdrag;
	float	ta, dta, maxCl, minCl, a0;
	float	RAR;
	int	rstall;

	ta = -beta;
	if (uu)
		ta += (dta = atan2 (-rr * EP->RACy/FVONE, uu));
	else
		dta = 0;
CCfshow (2, "Rda", dta*FR2D);
CCfshow (2, "Raoa", ta*FR2D);

	fGetFoil (p, 0.0, EP->RmaxCl/F10FONE, -EP->RmaxCl/F10FONE, Crudder,
		0, &a0, &maxCl, &minCl);

	RAR = EP->rudd_area / (EP->rudd_span * (float)EP->rudd_span)*FVONE;
	t = pi_e_AR * RAR/AR;
	rstall = fGetCl (p, ta, a0, t, maxCl, minCl, &ClRudd, &CdRudd, 0, 0);
	t = qS * EP->rudd_area/EP->wing_area;
	Rlift = ClRudd * t;
	Rdrag = -(CdRudd + EP->Cdp0/FFONE) * t;
	Fx = sb * Rlift + cb * Rdrag;
	Fy = cb * Rlift - sb * Rdrag;
	mx =  Fy * EP->RACz/FVONE;
	my = -Fx * EP->RACz/FVONE;
	mz =  Fy * EP->RACy/FVONE;

	LL += mx;
	MM += my;
	NN += mz;

	XX += Fx;
	YY += Fy;
CCfshow (0, rstall ? "Rstall" : "Rlift", Rlift);
CCfshow (0, "Rdrag", Rdrag);
CCfshow (0, "Xr", Fx);
CCfshow (0, "Yr", Fy);
CCfshow (0, "Lr", mx);
CCfshow (0, "Mr", my);
CCfshow (0, "Nr", mz);
    }

} else {
/*
 **************************************************
 *
 * This section evaluates forces/moments from the
 * dimentionless derivatives.
 *
 **************************************************
*/
	my =	+ qS      * EP->Cmde/FFONE * Celevators;
	MM +=  my * c;
CCfshow (0, "Me", my*c);

/* Pitching moment damping.
*/
	my =	+ qS_V_c  * EP->Cmq/F10FONE   * qq;	/* damping */
	MM +=  my * c;
CCfshow (0, "Md", my*c);

/* Wing and tail stabilizer pitching moment.
*/
if (DOPITCH) {
	my =	+ qS      * EP->Cm0/FFONE
		+ qS      * EP->Cmalpha/FFONE      * alpha
		+ qS_V_c  * EP->Cmalphadot/F10FONE * alphadot;
	MM +=  my * c;
CCfshow (0, "Mt", my*c);
}

/* Rolling moment.
*/
#define Clr	(Cl/4.0)
	mx =	+ qS      * EP->Clda/FFONE    * Cailerons
		+ qS      * EP->Cldr/FFONE    * Crudder
		+ qS_V_b2 * EP->Clp/FFONE     * pp	/* damping */
		+ qS_V_b2 * Clr               * rr	/* yaw induced */
		+ qS      * EP->Clbeta/FFONE  * beta;	/* hihedral */
	LL += mx * b;
#undef Clr


/* Yawing moment.
*/
	Fy  =	  qS * (EP->Cydr/FFONE * Crudder + EP->Cybeta/F10FONE * beta);
	YY += Fy;					/* side force */
CCfshow (0, "sf", Fy);

	mz =	+ qS      * EP->Cndr/FFONE    * Crudder
		+ qS      * EP->Cnda/FFONE    * Cailerons
		+ qS_V_b2 * EP->Cnr/FFONE     * rr	/* damping */
		+ qS_V_b2 * EP->Cnp/FFONE     * pp	/* roll induced */
		+ qS      * EP->Cnbeta/FFONE  * beta;	/* sideslip induced */
	NN += mz * b;
CCfshow (0, "N", mz*b);
}

EX->misc[5] = qS ? (int)(MM/(qS*c)*1000000.0) : 0;
CCshow (p, 3, "Cm", EX->misc[5]);

{
	MAT	II;
	VECT	F, M;

	II[X][X] = (xshort)EP->Iyy;
	II[Y][Y] = (xshort)EP->Ixx;
	II[Z][Z] = (xshort)EP->Izz;
	II[Y][Z] = (xshort)EP->Izx;

	F[X] =  (xshort)(YY/weight *FVONE);
	F[Y] =  (xshort)(XX/weight *FVONE);
	F[Z] = -(xshort)(ZZ/weight *FVONE);

	M[X] =  (xshort)(MM/weight /FVONE/FVONE/FA2R);
	M[Y] =  (xshort)(LL/weight /FVONE/FVONE/FA2R);
	M[Z] = -(xshort)(NN/weight /FVONE/FVONE/FA2R);

	LandGear (p, F, M);

	SixDOF (p, F, M, II);
}

	Mobj (p);

	p->speed = ihypot3d (p->vb);
	VMmul (p->V, p->vb, p->T);

#define MAX_SPEED	(1000*VONE)
	if (p->speed > MAX_SPEED) {			/* temp */
		it = fdiv (MAX_SPEED, p->speed);
		p->vb[X] = fmul (it, p->vb[X]);
		p->vb[Y] = fmul (it, p->vb[Y]);
		p->vb[Z] = fmul (it, p->vb[Z]);
		p->V[X]  = fmul (it, p->V[X]);
		p->V[Y]  = fmul (it, p->V[Y]);
		p->V[Z]  = fmul (it, p->V[Z]);
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
	EX->mach = muldiv (p->speed, 1000, (int)(sos*FVONE));

/* pull up warning time.
*/
	it = muldiv (10000, iabs(p->speed), 300*VONE);
	it = muldiv (it, iabs(p->a[X]), D90);
	if (it < 2000)
		it = 2000;
	EX->misc[8] = it;
}

#undef FVONE
#undef FFONE
#undef FA2R
#undef DOPITCH
#undef DOYPLANE
#undef FUNDAMENTAL
#undef CCfshow
