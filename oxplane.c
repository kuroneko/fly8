/* --------------------------------- oxplane.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Dynamics of the eXpermental plane.
 * This model will eventualy do a proper aerodynamic simulation. It will NOT
 * go as far as non-linear 6DOF but will properly implement the linearized
 * model. This program is being modified regularly as I learn more about
 * the subject and gather more data. If I get a stable version then it will
 * be separated into it's own module so as not to constantly have a half
 * working program. But not yet.
 *
 * ToDo:
 * 1 document units/scale used for all variables.
 * 2 select aero formulas and break into classes:
 *   - input parameters (from .prm)
 *   - calculated fixed parameters
 *   - slow changing parameters
 *   - dynamically calculated variables.
 * 3 use proper trans- and super-sonic calcs for Cd.
 * 4 add wing sweep to calcs?
*/

#include "plane.h"


/* Formulas used:
 *
 * b	wing span
 * S	wing area
 * ClRate0 slope of Cl for foil
 * e	Oswald's (or span) efficiency factor
 * aoa0	angle of attack where Cl is zero
 * feff	degrees aoa effective for 1 degree flaps
 *
 * AR	wing aspect ratio
 * k_AR	1/(pi*e*AR)
 * ClRate  slope of Cl for wing
 * e1	drag span-efficiency-factor. We use 'e' for it.
 *
 * V	airspeed
 * rho	air density
 * aoa	angle of attack (geometric)
 * aoaf	angle of attack effect due to flaps
 * aoai	induced angle of attack
 *	= Cl / (pi*e1*AR), but folded into ClRate
 * aeff	effective angle of attack
 *  ... all of these angles in degrees!
 * q	dynamic pressure
 * Cl	lift coefficient
 * Cd	drag coefficient
 * Cdi	induced Cd
 * Cdp	profile Cd
 *
 * AR = b^2 / S
 * aoai = Cl / (pi*e1*AR)
 * ClRate = ClRate0 / (1+57.3*ClRate0/(pi*e1*AR))
 *
 * aoaf = flaps*feff
 * aeff = aoa + aoaf
 * Cl = (aeff - aoa0) * ClRate
 *
 *
 *
 *
 * Cdi = Cl^2 /(pi*e*AR)
 * Cdp = Cdp_characteristic + Cd_flaps + Cd_gear + Cd_bombs + Cd_speedbrakes
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
dynamics_xplane (OBJECT *p, int action)
{
	int	onground, taxiing;
	int	t, vmag, weight, thrust, push, drag, lift, k_ar;
	int	rho, sos, Cd, Cl, ClTail, Cdi, sa, ca, sb, cb;
	int	aFlaps, ClMax, ClMin, ClStall, Clrate, stall;
	int	rVS, S, b2, b2VV, c2, c2VV, vmagV;
	int	Crudder, Cailerons, Celevators, Cb, Cdx, Cdy, Cdz;
	int	Fx, Fy, Fz, Mx, My, Mz, Ixx, Iyy, Izz;
	long	tt;
	ANGLE	alpha, beta, a;
	AVECT	da;

	if (action)
		return;

	if (dynamics_input (p))
		return;

	onground = EX->flags & PF_ONGROUND;

/* keep the load within the designate range by limiting the elevators.
*/
	if ((t = EX->Gforce - (EP->max_lift-EP->max_lift/4)) > 0) {
		if (t > (EP->max_lift>>1))
			EX->elevators = 0;
		else
			EX->elevators -= muldiv (EX->elevators, t,
							EP->max_lift>>1);
	} else if ((t = EX->Gforce - (EP->min_lift-(EP->min_lift>>2))) < 0) {
		if (t < (EP->min_lift>>1))
			EX->elevators = 0;
		else
			EX->elevators -= muldiv (EX->elevators, t,
							EP->min_lift>>1);
	}
	EX->flags &= ~PF_GLIMIT;

	p->ab[X] = -fmul (GACC, p->T[X][Z]);	/* forces: gravity */
	p->ab[Y] = -fmul (GACC, p->T[Y][Z]);
	p->ab[Z] = -fmul (GACC, p->T[Z][Z]);

	EX->Gforce = -p->ab[Z];			/* pilot's gravity */

	da[X] = da[Y] = da[Z] = 0;		/* rotations */

	if (onground ? check_takeoff (p) : check_land (p)) {
		p->flags |= F_HIT;
		return;
	}

	taxiing = onground && p->speed < 40*VONE;	/* NWS */

	airdata (p->R[Z], 0, 0, &rho, &sos);
CCshow (p, 3, "rho", (long)fmul(rho, 1000));
CCshow (p, 0, "sos", (long)DV(sos));

/* Get engine thrust.
*/
	f16engine (p, sos);

/* Automatically refuel when stationery on ground.
*/
	if (onground && 0 == p->speed && 0 == EX->power)
		supply (p, 0);

	thrust = fmul (EX->thrust*2, FCON(0.453*9.8/VONE*5));	/* N*VONE */
	tt = EP->weight + EX->fuel/100;				/* lb */
	weight = fmul ((int)DV(tt), FCON (0.453));		/* Kg*VONE */
	if ((t = EX->stores[WE_M61-1]) > 0)
		weight += (int)DV(t * st.bodies[O_M61]->shape->weight/1000);
	if ((t = EX->stores[WE_MK82-1]) > 0)
		weight += (int)DV(t * st.bodies[O_MK82]->shape->weight/1000);
	push = muldiv (thrust, VONE, weight);
CCshow (p, 0, "thrust", (long)thrust);
CCshow (p, 0, "weight", (long)weight);
CCshow (p, 0, "thrA", (long)push);

	if (taxiing) {

		EX->flags &= ~(PF_GLIMIT|PF_STALL);
/* Taxiing, ignore aerodynamics altogether. Only NWS and wheels friction.
*/
CCshow (p, 0, "vold", (long)p->speed);
		p->speed += TADJ(push);			/* v */
CCshow (p, 0, "vnew", (long)p->speed);
		drag = EP->wheel_mu
			+ muldiv (EP->brake_mu-EP->wheel_mu, EX->brake, 100);
		drag = TADJ (fmul (drag, GACC));	/* assume no lift */
CCshow (p, 0, "drag", (long)drag);
		if (p->speed > drag)
			p->speed -= drag;
		else if (p->speed < -drag)
			p->speed += drag;
		else
			p->speed = 0;			/* stop! */
CCshow (p, 0, "v", (long)p->speed);

		if (p->a[X] > EP->gpitch)		/* slam down */
			p->a[X] = TANG(p->a[X] - TADJ(VD90));
		if (p->a[X] < EP->gpitch)
			p->a[X] = EP->gpitch;

		if (p->a[Y])				/* level wings */
			p->a[Y] = TANG(p->a[Y] - TADJ(p->a[Y]));

		p->vb[X] = 0;
		p->vb[Y] = fmul (p->speed,  p->cosx);
		p->vb[Z] = fmul (p->speed, -p->sinx);

		p->da[X] = p->da[Y] = 0;
		t = muldiv (EP->MaxRudder, EX->rudder, RAD2ANG(100));
		p->da[Z] = muldiv (p->speed, t, VONE); /* NWS */
		da[X] = da[Y] = da[Z] = 0;
	} else {

/* Flying, although may still be on ground. Must do full arerodynamics but
 * allow for ground contact.
 *
 * Ideal Cl rate is 2*pi, and for a given aspect ratio A it is 2*pi*A/(A+2).
 * We represent angles as 2.0 angle = pi radians, so the formula is:
 * pi*pi*A/(A+2). We prefer to keep the inverse which is: (1+2/A)/(pi*pi).
 * And to avoid fraction overflow inside the parens: 2*(0.5+1/A)/(pi*pi). We
 * keep 1/A in k_ar [which later is 1/(pi*A)].
*/
		k_ar = fdiv (EP->wing_area,
				muldiv (EP->wing_span, EP->wing_span, VONE));
CCshow (p, 3, "1/AR", (long)fmul(k_ar, 1000));
		Clrate = fmul (FCON(2.0/(C_PI*C_PI)), FCON(0.5) + k_ar);
CCshow (p, 2, "Clrate", (long)ANG2DEG00 (Clrate));
		k_ar = fmul (k_ar, FCON(1.0/C_PI));		/* 1/(pi*AR) */
CCshow (p, 3, "k/AR", (long)fmul(k_ar, 1000));

		alpha = p->vb[Y] ? -ATAN (p->vb[Z], p->vb[Y]) : 0;
		alpha += EP->Aoffset;
		beta = p->speed ? ASIN (fdiv (p->vb[X], p->speed)) : 0;
CCshow (p, 2, "alpha", (long)ANG2DEG00 (alpha));
CCshow (p, 2, "beta", (long)ANG2DEG00 (beta));
		sa = SIN (alpha);
		ca = COS (alpha);
		sb = SIN (beta);
		cb = COS (beta);
		EX->aoa = alpha;

		a = alpha - EP->Cl0;
		ClTail = muldiv (EP->Tvol, a+EP->Toffset, Clrate);
CCshow (p, 3, "ClTail", (long)fmul(ClTail, 1000));

		aFlaps = fmul (EP->FEff, muldiv (EP->MaxFlaps, EX->flaps, 100));
		a += aFlaps;
CCshow (p, 2, "aeff", (long)ANG2DEG00 (a));

/* 'CLf' is to avoid overflow (fractions are limited to the range [-2...+2)).
*/
#define	CLf	4
		if (iabs(a>>1) < (Uint)(CLf*Clrate))
			Cl = muldiv (FONE/CLf, a, Clrate);
		else if (a > 0)
			Cl =  FCON(1.99);
		else
			Cl = -FCON(1.99);

		t = muldiv (FONE/CLf, aFlaps, Clrate);
		ClMax = EP->maxCl/CLf*10 + t;
		ClMin = EP->minCl/CLf*10 + t;
		EX->flags &= ~PF_STALL;
		stall = 0;
		ClStall = FONE;
		if (Cl > ClMax) {
			EX->flags |= PF_STALL;
			if (!(EX->flags & PF_NOSTALL)) {
				stall = 1;
				Cl = Cl/4 - (Cl-ClMax);
				if (Cl < 0)
					Cl = 0;
				else
					Cl *= 4;
			} else {
				Cl = ClMax;
				ClStall = iabs (ca);
			}
		} else if (Cl < ClMin) {
			EX->flags |= PF_STALL;
			if (!(EX->flags & PF_NOSTALL)) {
				stall = 1;
				Cl = Cl/4 + (ClMin-Cl);
				if (Cl > 0)
					Cl = 0;
				else
					Cl *= 4;
			} else {
				Cl = ClMin;
				ClStall = iabs (ca);
			}
		}

		Cdi = fmul (Cl, k_ar*CLf);
		Cdi = fmul (Cdi, Cl)*CLf;
		Cdi = muldiv (Cdi, 100, EP->efficiency_factor);

/* The groung effect formula is extracted from the graph in Smiths 'The
 * Illustrated Guide To Aerodynamic' end of chapter 3.
*/
		t = EP->wing_span;
		if (p->R[Z] < (long)t) {		/* ground effect */
			t = fdiv ((int)p->R[Z], t);	/* h/b */
CCshow (p, 3, "h/b", (long)fmul(t, 1000));
			if (t < FCON(0.1))
				t = 5 * t;
			else
				t = FCON(1.06) - fdiv (FCON(0.07), t);
CCshow (p, 3, "gef", (long)fmul(t, 1000));
			Cdi = fmul (Cdi, t);
		}
CCshow (p, 3, "Cl", (long)fmul(Cl, CLf*1000));
CCshow (p, 3, "Cdi", (long)fmul(Cdi, 1000));

		Cd = EP->Cdp0;
CCshow (p, 3, "Cdp0", (long)fmul(Cd, 1000));
		Cd += Cdi;
CCshow (p, 3, "+Cdi", (long)fmul(Cd, 1000));
		if (EX->airbrake) {
			Cd += muldiv (EP->Cds, EX->airbrake, 100);
CCshow (p, 3, "+Brks", (long)fmul(Cd, 1000));
		}
		if (EX->equip & EQ_GEAR) {
			Cd += EP->Cdg;
CCshow (p, 3, "+Gear", (long)fmul(Cd, 1000));
		}
		if (EX->stores[WE_MK82-1] > 0) {
			Cd += EX->stores[WE_MK82-1] * EP->CdMK82;
CCshow (p, 3, "+MK82", (long)fmul(Cd, 1000));
		}

/* We now calculate the new forces, based on the state of the plane.
 * The given state is:
 *  Cdx		Rotation around x (nose up, pitch)
 *  Cdy		Rotation around y (right wing down, roll)
 *  Cdz		Rotation around z (nose right, yaw)
 *  Crudder	rudder right-turn angle
 *  Cailerons	ailerons roll-right angle
 *  Celevators	elevators pull-up angle
 *  alpha	nose above wind angle
 *  Cb		nose left-of-wind angle
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
		vmagV = p->speed;
		vmag = DV(vmagV);

/* 0.093 is for converting 'ft^2' to 'm^2'
*/
		S = DV(EP->wing_area);
		rVS = muldiv (fmul (fmul (rho, FCON(1.225)), vmag), S, VONE);
CCshow (p, 0, "rVS", (long)rVS);
		b2 = EP->wing_span>>1;
CCshow (p, 2, "b2", DV(b2*100L));
		b2VV = VONE*16*b2;
CCshow (p, 0, "b2VV", (long)b2VV);
		c2 = EP->wing_cord>>1;
CCshow (p, 2, "c2", DV(c2*100L));
		c2VV = VONE*16*c2;
CCshow (p, 0, "c2VV", (long)c2VV);

		Crudder = -muldiv (EP->MaxRudder, EX->rudder, RAD2ANG(100));
		Cailerons = stall ? 0 :
			-muldiv (EP->MaxAilerons, EX->ailerons, RAD2ANG(100));
		Celevators = stall ? 0 :
			-muldiv (EP->MaxElevators, EX->elevators, RAD2ANG(100));
		Cb = beta;
		Cdx = p->da[X];
		Cdy = p->da[Y];
		Cdz = -p->da[Z];
CCshow (p, 3, "Crdr", (long)fmul(Crudder, 1000));
CCshow (p, 3, "Cail", (long)fmul(Cailerons, 1000));
CCshow (p, 3, "Celev", (long)fmul(Celevators, 1000));
CCshow (p, 3, "Cb", (long)fmul(Cb, 1000));
CCshow (p, 3, "Cdx", (long)fmul(Cdx, 1000));
CCshow (p, 3, "Cdy", (long)fmul(Cdy, 1000));
CCshow (p, 3, "Cdz", (long)fmul(Cdz, 1000));

		t = fmul (Cl*CLf, vmagV>>1);
		lift = muldiv (rVS, t, weight)*2;	/* cheat */
CCshow (p, 0, "lift", (long)lift);
		t = fmul (Cd, vmagV>>1);
		drag = muldiv (rVS, t, weight);
CCshow (p, 0, "drag", (long)drag);

/* This formula is faking behaviour at high aoas (usually when stall is
 * disabled).
*/
#if 0
		drag += fmul (iabs(lift), FONE-ClStall);
#endif
		lift = fmul (lift, ClStall);

/* Fx,Fy,Fz: aerodynamic forces on the plane, operating on the aero. center.
*/
		t =	fmul (vmagV, fmul (EP->Cydr, Crudder)) +
			fmul (Cb, EP->Cybeta)*10
			;
		Fx = muldiv (rVS, t, weight);
		Fx += fmul (sb, ihypot2d (drag, lift));
CCshow (p, 0, "Fx", (long)Fx);
		Fy = fmul (cb, fmul (sa, lift) - fmul (ca, drag));
CCshow (p, 0, "Fy", (long)Fy);
		Fz = fmul (cb, fmul (ca, lift) + fmul (sa, drag));
CCshow (p, 0, "Fz", (long)Fz);

		t = fmul (ClTail, vmagV>>1);
		t = muldiv (rVS, t, weight);	/* Tail lift */
		t = fmul (ca, t);
CCshow (p, 0, "Tlift", (long)t);

		t =	fmul (c2VV, fmul (EP->Cmq, Cdx))*10	/* damping */
		      +	fmul (t, EP->Cmalpha)			/* stabilizers */
		      +	fmul (vmagV, fmul (EP->Cmde, Celevators));
		t = muldiv (c2, t, VONE);
		Mx = muldiv (rVS, t, VONE*VONE*VONE);

#define Clr	(Cl/4)

		t =	fmul (b2VV, fmul (EP->Clp, Cdy))	/* damping */
		      + fmul (b2VV, fmul (Clr, Cdz))
		      + fmul (EP->Clbeta, Cb)
		      + fmul (vmagV, fmul (EP->Clda, Cailerons));
		t = muldiv (b2, t, VONE);
		My = muldiv (rVS, t, VONE*VONE*VONE);

		t =	fmul (vmagV, fmul (EP->Cndr, Crudder))
		      + fmul (b2VV, fmul (EP->Cnp, Cdy))
		      + fmul (b2VV, fmul (EP->Cnr, Cdz))	/* damping */
		      + fmul (EP->Cnbeta, Cb)
		      +	fmul (vmagV, fmul (EP->Cnda, Cailerons));
		t = muldiv (b2, t, VONE);
		Mz = muldiv (rVS, t, VONE*VONE*VONE);
CCshow (p, 0, "Mx", (long)Mx);
CCshow (p, 0, "My", (long)My);
CCshow (p, 0, "Mz", (long)Mz);

		p->ab[X] += Fx;
		p->ab[Y] += Fy + push;
		p->ab[Z] += Fz;

#if 0
/* Examining the moment graphs at the end of chapter 13 of "The Design Of The
 * Aeroplane" [Darrol Stinton, pub. Collins] one can roughly extract the
 * following values for weights around 15000Kg (the very top end):
 * Roll  moment (Ixx) = weight * 9.
 * Pitch moment (Iyy) = weight * 7.
 * yaw   moment (Izz) = weight * 17.
 * Ixz is ignored.
 * Note that our x,y,z are non-standard!
*/
		Ixx = fmul (weight, FCON(0.72));
		Iyy = fmul (weight, FCON(0.17));
		Izz = fmul (weight, FCON(0.86));
#else
		Ixx = fmul (weight, EP->Iyy);
		Iyy = fmul (weight, EP->Ixx);
		Izz = fmul (weight, EP->Izz);
#endif
CCshow (p, 0, "Ixx", (long)Ixx);
CCshow (p, 0, "Iyy", (long)Iyy);
CCshow (p, 0, "Izz", (long)Izz);

		da[X] += muldiv (DEG(57)/VONE, Mx, Ixx);
		da[Y] += muldiv (DEG(57)/VONE, My, Iyy);
		da[Z] -= muldiv (DEG(57), Mz, Izz);	/* note *VONE */
		if (stall) {
			da[X] += Frand()%(VD90/16*2+1) - (VD90/16);
			da[Y] += Frand()%(VD90/16*2+1) - (VD90/16);
			da[Z] -= Frand()%(VD90/16*2+1) - (VD90/16);
		}

		if (onground) {
			t = DV(p->speed) - (EP->liftoff_speed>>1); /*nm->meter*/
			if (t < 0 && p->a[X] <= EP->gpitch)
				t = 0;
			if (da[X] > t)
				da[X] = t;
			if (da[X] < 0 && p->a[X] <= EP->gpitch)
				da[X] = 0;

			if (p->a[Y])			/* level wings */
				p->a[Y] = TANG(p->a[Y] - TADJ(p->a[Y]));

			da[Z] += -TADJ(p->da[Z]); /* no rotation */

			p->da[Y] = 0;
			da[Y] = 0;

/* Effective weight is reduced by lift (it is assumed that the lift is
 * directed straight up which is reasonable when one is moving parallel to
 * the ground). We must be going fast enough for the brakes to be unable to
 * completely stop us within this interval.
*/
			drag = EP->wheel_mu + muldiv (EP->brake_mu-EP->wheel_mu,
							EX->brake, 100);
			drag = fmul (drag, lift - GACC);
			if (p->vb[Y] < 0)
				drag = -drag;
			p->ab[Y] += fmul (ca, drag);
			p->ab[Z] += fmul (sa, drag);
			EX->flags &= ~PF_STALL;
			stall = 0;
		} else {
			EX->Gforce += p->ab[Z];
			if (EX->Gforce > EP->max_lift || EX->Gforce < EP->min_lift)
				EX->flags |= PF_GLIMIT;
			if (EX->equip & EQ_GEAR) {	/* gear shaking */
				t = ~1 & fmul (p->speed, FCON(0.02));
				t = t*t;
				p->ab[X] += TADJ(Frand()%(1+t) - (t>>1));
				p->ab[Y] -= TADJ(Frand()%(1+t));
				p->ab[Z] += TADJ(Frand()%(1+t) - (t>>1));
			}
		}

		p->vb[X] += TADJ(p->ab[X]);
		p->vb[Y] += TADJ(p->ab[Y]);
		p->vb[Z] += TADJ(p->ab[Z]);
		p->speed = ihypot3d (p->vb);
		VMmul (p->V, p->vb, p->T);
#if 0
		if (onground && p->V[Z] < 0) {
			p->V[Z] = 0;
			VxMmul (p->vb, p->V, p->T);
		}
#endif
	}

	p->da[X] += TADJ (da[X])*VONE;	/* rotations */
	p->da[Y] += TADJ (da[Y])*VONE;
	p->da[Z] += TADJ (da[Z]);

	da[X] = TADJ(p->da[X])*VONE;
	da[Y] = TADJ(p->da[Y])*VONE;
	da[Z] = TADJ(p->da[Z])*VONE;
	Myxz (p->T, da);

	if (!taxiing) {
		VECT	AA;

		Vcopy (AA, p->vb);
		VxMmul (p->vb, AA, p->T);
	}

	fMroty (p->T, p->siny, p->cosy);
	fMrotx (p->T, p->sinx, p->cosx);
	fMrotz (p->T, p->sinz, p->cosz);
	Mangles (p, p->T, p->a, da[Y]);

	if (onground && p->a[X] < EP->gpitch) {
		p->a[X] = EP->gpitch;
		Mobj (p);
	}

	if (taxiing) {
		VMmul (p->V, p->vb, p->T);
		p->V[Z] = 0;
	}

	if (onground && p->V[Z] < 0) {
		p->V[Z] = 0;
		VxMmul (p->vb, p->V, p->T);
		p->vb[X] = 0;	/* testing */
		p->speed = ihypot3d (p->V);
	}

#define MAX_SPEED	(1000*VONE)
	if (p->speed > MAX_SPEED) {			/* temp */
		t = fdiv (MAX_SPEED, p->speed);
		p->vb[X] = fmul (t, p->vb[X]);
		p->vb[Y] = fmul (t, p->vb[Y]);
		p->vb[Z] = fmul (t, p->vb[Z]);
		p->V[X]  = fmul (t, p->V[X]);
		p->V[Y]  = fmul (t, p->V[Y]);
		p->V[Z]  = fmul (t, p->V[Z]);
		p->speed = ihypot3d (p->V);
	}
#undef MAX_SPEED

/* Mach number.
*/
	EX->mach = muldiv (p->speed, 1000, sos);

/* pull up warning time.
*/
	t = muldiv (4000, iabs(p->speed), 300*VONE);
	t = muldiv (t, iabs(p->a[X]), D90);
	if (t < 2000)
		t = 2000;
	EX->misc[8] = t;
}

#undef CLf
#undef Clr
