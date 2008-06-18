/* --------------------------------- obasic.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Dynamics of the Basic plane.
*/

#include "plane.h"


extern void FAR
dynamics_basic (OBJECT *p, int action)
{
	int	onground, taxiing;
	int	t, v, vmag, weight, force, drag, acc, lift, q, qf;
	int	rho, sos, Cd, Cl, Cdp, Cds, Cdg, Cdi, FEff, sina;
	long	tt;
	ANGLE	alpha, beta, stall, a;
	AVECT	da;

	if (action)
		return;

	if (dynamics_input (p))
		return;

	onground = EX->flags & PF_ONGROUND;
	EX->flags &= ~PF_GLIMIT;

	p->ab[X] = -fmul (GACC, p->T[X][Z]);	/* gravity */
	p->ab[Y] = -fmul (GACC, p->T[Y][Z]);
	p->ab[Z] = -fmul (GACC, p->T[Z][Z]);

	EX->Gforce = -p->ab[Z];		/* pilots gravity */
	v = p->vb[Y];

	t = muldiv (EP->pitch_rate, EX->elevators, 100);
	t = muldiv (t, v, 150*VONE);
	DAMPEN (p->da[X], t, EP->opt[1]);

	t = -muldiv (EP->roll_rate,  EX->ailerons, 100);
	t = muldiv (t, v, 150*VONE);
	DAMPEN (p->da[Y], t, EP->opt[1]);

	if (onground ? check_takeoff (p) : check_land (p)) {
		p->flags |= F_HIT;
		return;
	}

	taxiing = onground && p->speed < 40*VONE;	/* NWS */

	if (100 == EX->throttle) {
		t = EP->ab_thrust - EP->mil_thrust;
		t = EP->mil_thrust + muldiv (EX->afterburner, t, 100);
	} else
		t = muldiv (EX->throttle, EP->mil_thrust, 100);
	if (t < 0) {
		if (onground)
			t /= 2;		/* reverse thrust is 50% efficient? */
		else
			t = 0;
	}
	DAMPEN (EX->thrust, t, 8);
	EX->power = muldiv (EX->thrust, 10000, EP->ab_thrust);

	t = EX->afterburner ? EP->ab_sfc : EP->mil_sfc;
	t = muldiv (iabs(EX->thrust), t, 60*60/10);
	EX->fuelRate = t;
	EX->fuel -= TADJ(t);
	if (EX->fuel < 0) {
		EX->fuel = 0;
		EX->thrust = 0;
	}
	if (onground && 0 == p->speed && 0 == EX->thrust)
		supply (p, 0);

	force = fmul (EX->thrust*2, FCON(0.453*9.8/VONE*5));	/* N*VONE */
	tt = EP->weight + EX->fuel/100;				/* lb */
	weight = fmul ((int)(tt/VONE), FCON (0.453));		/* Kg*VONE */
	if ((t = EX->stores[WE_M61-1]) > 0)
		weight += (int)(t * st.bodies[O_M61]->shape->weight/1000/VONE);
	if ((t = EX->stores[WE_MK82-1]) > 0)
		weight += (int)(t * st.bodies[O_MK82]->shape->weight/1000/VONE);
	acc = muldiv (force, VONE, weight);
CCshow (p, 0, "thrust", (long)force);
CCshow (p, 0, "weight", (long)weight);
CCshow (p, 0, "thrA", (long)acc);

	if (taxiing) {
CCshow (p, 0, "vold", (long)p->speed);
		p->speed += TADJ(acc);				/* v */
CCshow (p, 0, "vnew", (long)p->speed);
		drag = EP->wheel_mu
			+ muldiv (EP->brake_mu-EP->wheel_mu, EX->brake, 100);
		drag = TADJ(fmul (drag, GACC));
CCshow (p, 0, "drag", (long)drag);
		if (p->speed > drag)
			p->speed -= drag;
		else if (p->speed < -drag)
			p->speed += drag;
		else
			p->speed = 0;				/* stop! */
CCshow (p, 0, "v", (long)p->speed);

		p->da[X] = (EP->gpitch - p->a[X])/VONE;
		p->da[Y] = -p->a[Y]/VONE;

		p->vb[X] = 0;
		p->vb[Y] = fmul (p->speed,  p->cosx);
		p->vb[Z] = fmul (p->speed, -p->sinx);
CCshow (p, 0, "vy", (long)p->vb[Y]);
/* 60 degrees/sec at full rudder when doing 10 m/sec.
*/
		t = muldiv (DEG(60)/VONE, EX->rudder, 100);
		p->da[Z] = muldiv (t, p->speed, 10*VONE);
		airdata (p->R[Z], 0, 0, 0, &sos);
	} else {
		if (v) {
			alpha = -ATAN (p->vb[Z], v);
			if (alpha > D90)			/* temp */
				alpha = D90;
			else if (alpha < -D90)
				alpha = -D90;
		} else
			alpha = 0;
		EX->aoa = alpha;
/*		p->da[X] -= alpha/64;*/

		t = ihypot2d (p->vb[Y], p->vb[Z]);
		beta = t ? ATAN (p->vb[X], t) : 0;
#if 1
		p->da[Z] = muldiv (p->speed, EX->rudder, 100*VONE);
		p->da[Z] -= beta/VONE;
		p->ab[X] -= p->vb[X]/2;
#endif
#if 0
		p->da[Z] = muldiv (p->speed, EX->rudder, 100*VONE);
		p->da[Z] -= beta/VONE;
#endif
#if 0
		p->ab[X] = -muldiv (p->speed, EX->rudder, 16*VONE);
		p->ab[X] -= p->vb[X] / (4*VONE);
		p->da[Z] = -beta/(VONE/2);
#endif

		EX->flags &= ~PF_STALL;
#if 0
		stall = DEG2ANG(25);
		stall -= muldiv (EP->MaxFlaps, EX->flaps, 8*100);
		if (alpha > D90/4) {
			t = (stall-alpha)/VONE/4;
			if (p->da[X] > t)
				p->da[X] = t;
			EX->flags |= PF_STALL;
			if (!(EX->flags & PF_NOSTALL)) {
				if (alpha > stall/2*3)
					alpha = 0;
				else
					alpha = (stall/2*3-alpha)*2;
			} else
				alpha = stall;
		} else if (alpha < -stall) {
			t = (-stall-alpha)/VONE/4;
			if (p->da[X] < t)
				p->da[X] = t;
			EX->flags |= PF_STALL;
			if (!(EX->flags & PF_NOSTALL)) {
				if (alpha < -stall/2*3)
					alpha = 0;
				else
					alpha = (-stall/2*3-alpha)*2;
			} else
				alpha = -stall;
		}
#else
		stall = 0;	/* avoid compiler warning */
#endif
		airdata (p->R[Z], 0, 0, &rho, &sos);
CCshow (p, 3, "rho", (long)fmul(rho, 1000));
CCshow (p, 0, "sos", (long)sos/VONE);

#if 0
		a = alpha + muldiv (EP->MaxFlaps, EX->flaps, 4*100);
#else
		FEff = FCON(0.5);
		a = alpha + fmul (FEff, muldiv (EP->MaxFlaps, EX->flaps, 100));
#endif
		sina = SIN (a);
CCshow (p, 2, "aoa", (long)ANG2DEG00 (alpha));
CCshow (p, 2, "aeff", (long)ANG2DEG00 (a));
		t = fdiv (EP->wing_area,
			muldiv (EP->wing_span, EP->wing_span, VONE));
		t = fmul (t, FCON(1.0/C_PI));
CCshow (p, 3, "t", (long)fmul(t, 1000));

/* the '/4' and (later) '*4' are to evoid overflow. Fractionc are limited
 * to the range [-2...+2].
*/
		Cl  = muldiv (sina, FONE/4, FCON(.17) + t);
		Cdi = fmul (Cl, t*4);
		Cdi = fmul (Cdi, Cl)*4;
		Cdi = muldiv (Cdi, 100, EP->efficiency_factor);
		Cdp = FCON(0.05);		/* parasitic */
		Cds = FCON(0.50);		/* speed brakes */
		Cdg = FCON(0.10);		/* gear */
CCshow (p, 3, "Cl", (long)fmul(Cl, 1000));
CCshow (p, 3, "Cdi", (long)fmul(Cdi, 1000));
CCshow (p, 3, "Cdp", (long)fmul(Cdp, 1000));
		vmag = iabs(p->speed/VONE);
		q = muldiv (vmag, vmag, 128);
		q = fmul (rho, q);
		if (q < VMAX/128) {
			q = fmul (rho, vmag) * vmag;
			qf = 1;
		} else
			qf = 128;

/* VONE is internal force factor
*/
		if (EP->wing_area > 2*VONE*2*VONE) {
			q = muldiv (q, EP->wing_area, 2*VONE*32*VONE);
			qf *= 32;
		} else {
			q = muldiv (q, EP->wing_area, 2*VONE*2*VONE);
			qf *= 2;
		}
CCshow (p, 0, "q", (long)q);
CCshow (p, 0, "qf", (long)qf);
		drag = fmul (q, Cdi);
CCshow (p, 0, "Di", (long)drag);
		Cd = Cdp;
CCshow (p, 3, "Cdp", (long)fmul(Cd, 1000));
		if (EX->airbrake)
			Cd += muldiv (Cds, EX->airbrake, 100);
CCshow (p, 3, "+Cds", (long)fmul(Cd, 1000));
		if (EX->equip & EQ_GEAR)
			Cd += Cdg;
CCshow (p, 3, "+Cdg", (long)fmul(Cd, 1000));
		drag += fmul (q, Cd);
CCshow (p, 0, "dragF", (long)drag);
		drag = muldiv (drag, VONE*qf, weight);
CCshow (p, 0, "dragA", (long)drag);
		if (v >= 0)
			acc -= drag;
		else
			acc += drag;
CCshow (p, 0, "acc", (long)acc);

		lift = muldiv (fmul (q, Cl), EP->opt[3], 4);
CCshow (p, 0, "liftF", (long)lift);
		lift = muldiv (lift, VONE*qf*4, weight);
CCshow (p, 0, "liftA", (long)lift);

#if 1
		if (onground) {
			t = p->speed - EP->liftoff_speed*VONE/2; /*nm->meter*/
			if (t < 0 && p->a[X] <= EP->gpitch)
				t = 0;
			if (p->da[X] > t)
				p->da[X] = (t - p->a[X])/VONE;
			if (p->da[X] < 0 && p->a[X] <= EP->gpitch)
				p->da[X] = -p->a[X]/VONE;
			p->da[Y] = -p->a[Y]/VONE;
		}
#else
		if (onground) {
			/* say torque is 1/4 of lift against 1/10 of the weight */
			t = lift/2 + p->ab[Z]/5;
			if (t < 0 && p->a[X] <= EP->gpitch)
				t = 0;
			if (p->da[X] > t)
				p->da[X] = (t - p->a[X])/VONE;
			if (p->da[X] < 0 && p->a[X] <= EP->gpitch)
				p->da[X] = -p->a[X]/VONE;
			p->da[Y] = -p->a[Y]/VONE;
		}
#endif
		if (onground) {
			t = EP->wheel_mu + muldiv (EP->brake_mu-EP->wheel_mu,
							EX->brake, 100);
			t = fmul (t, GACC);
			if (abs (v) <= t) {		/* stop! */
				acc = 0;
				lift = 0;
				p->vb[X] = p->vb[Y] = p->vb[Z] = 0;
			} else if (p->speed > 0)
				acc -= t;
			else
				acc += t;
		}

CCshow (p, 0, "G[Y]", (long)p->ab[Y]);
		p->ab[Y] += acc;
CCshow (p, 0, "AA[Y]", (long)p->ab[Y]);
		p->ab[Z] += lift;

		if (onground)
			EX->flags &= ~PF_STALL;
		else {
			EX->Gforce += p->ab[Z];
			if (EX->Gforce > EP->max_lift)
				EX->flags |= PF_GLIMIT;
			else if (EX->Gforce < EP->min_lift)
				EX->flags |= PF_GLIMIT;
			if (EX->equip & EQ_GEAR) {
				t = ~1 & fmul (p->speed, FCON(0.02));
				t = t*t;
				p->ab[X] += TADJ(Frand()%(1+t) - t/2);
				p->ab[Y] -= TADJ(Frand()%(  t));
				p->ab[Z] += TADJ(Frand()%(1+t) - t/2);
			}
		}

		p->vb[X] += TADJ(p->ab[X]);
CCshow (p, 0, "v[Y]", (long)p->vb[Y]);
		p->vb[Y] += (t = TADJ(p->ab[Y]));
CCshow (p, 0, "dv", (long)t);
CCshow (p, 0, "v[Y]", (long)p->vb[Y]);
		p->vb[Z] += TADJ(p->ab[Z]);
		VMmul (p->V, p->vb, p->T);
#if 0
		if (onground && p->V[Z] < 0) {
			p->V[Z] = 0;
			VxMmul (p->vb, p->V, p->T);
		}
#endif
		p->speed = ihypot3d (p->V);
	}

	da[X] = TADJ(p->da[X])*VONE;
	da[Y] = TADJ(p->da[Y])*VONE;
	da[Z] = TADJ(p->da[Z])*VONE;

	Myxz (p->T, da);		/* rebuild from scratch */

	if (!taxiing) {
		VECT	AA;

		Vcopy (AA, p->vb);
		VxMmul (p->vb, AA, p->T);
	}

	fMroty (p->T, p->siny, p->cosy);
	fMrotx (p->T, p->sinx, p->cosx);
	fMrotz (p->T, p->sinz, p->cosz);
	Mangles (p, p->T, p->a, da[Y]);

#if 0
	if (onground && p->a[X] < EP->gpitch) {
		p->a[X] = EP->gpitch;
		Mobj (p);
	}
#endif
	if (taxiing) {
		VMmul (p->V, p->vb, p->T);
		p->V[Z] = 0;
	} else if (onground && p->V[Z] < 0) {
		VECT	AA, BB;

		AA[X] = AA[Y] = 0;
		AA[Z] = p->V[Z];
		VxMmul (BB, AA, p->T);
		Vdec (p->vb, BB);
		p->V[Z] = 0;
		p->speed = ihypot3d (p->V);
	}

#define MAX_SPEED	1000
	if (p->speed > MAX_SPEED*VONE) {		/* temp */
		t = muldiv (FONE, (int)(MAX_SPEED*VONE), p->speed);
		p->vb[X] = fmul (t, p->vb[X]);
		p->vb[Y] = fmul (t, p->vb[Y]);
		p->vb[Z] = fmul (t, p->vb[Z]);
		p->V[X]  = fmul (t, p->V[X]);
		p->V[Y]  = fmul (t, p->V[Y]);
		p->V[Z]  = fmul (t, p->V[Z]);
		p->speed = ihypot3d (p->V);
	}

/* Mach number.
*/
	EX->mach = muldiv (p->speed, 1000, sos); /* good enough */

/* pull up warning time.
*/
	t = muldiv (4000, iabs(p->speed), 300*VONE);
	t = muldiv (t, iabs(p->a[X]), D90);
	if (t < 2000)
		t = 2000;
	EX->misc[8] = t;
}

#undef MAX_SPEED
