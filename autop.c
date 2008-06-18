/* --------------------------------- autop.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Autopilot related functions.
*/

#include "plane.h"


#define IS_CLASSIC(p)	(MODEL_CLASSIC == EEP(p)->opt[0])

/*#define	DEBUG_SPEED*/

#define	RANDTIME	8000		/* in milliseconds */
#define	MAXPITCH	(D90/3*2)
#define	MAXALT		5000
#define	MINALT		500
#define	MAXRANGE	(5000L*VONE)
#define	MINCLIMB	(500*VONE)

#ifndef DEBUG_SPEED
#define	MINSPEED	(100L*VONE)
#define	MAXSPEED	(250L*VONE)
#define	MINSHOOT	(2000L*VONE)
#else
#define	MINSPEED	(300L*VONE)		/* overflow testing */
#define	MAXSPEED	(2000L*VONE)
#define	MINSHOOT	(3000L*VONE)
#endif

#define	PATROLSPEED	250
#define	RANDOMSPEED	150

#define	MINDIST		(500L*VONE)

#define SETFLAPS	50			/* flaps for takeoff */
#define F16FACT		(D90)			/* smaller = faster */
#define CLFACT		(D90/4)			/* smaller = faster */
#define SENSPITCH	(D90/16)		/* smaller = faster */
#define SENSROLL	(D90/4 )		/* smaller = faster */
#if 0
#define LIFTOFF		(2*fmul (EEP(p)->liftoff_speed, 8428))	/* meters/sec */
#else
#define LIFTOFF		(fmul (EEP(p)->liftoff_speed, 8428/2*3))	/* meters/sec */
#endif
#define ABMIN		20			/* when to set afterburner */

LOCAL_FUNC void NEAR
SetMeetCorrection (OBJECT *p, OBJECT *target, VECT R)
{
	int	dist, rv, dt, speed;
	LVECT	LL;

	if (!target->speed) {
		R[X] = R[Y] = R[Z] = 0;
		return;
	}

	LL[X] = p->V[X] - (long)EX->tspeed[X];	/* target rel v */
	LL[Y] = p->V[Y] - (long)EX->tspeed[Y];
	LL[Z] = p->V[Z] - (long)EX->tspeed[Z];
	rv = (int)(lhypot3d (LL)/VONE);			/* my closure v */
	dist = (int)(ldist3d (target->R, p->R)/VONE);	/* target distance */

	if (rv <= dist/8)
		dt = FONE;
	else
		dt = muldiv (FONE/8, dist , rv);
	speed = ihypot3d (EX->tspeed); /* don't trust target->speed */
	if (VMAX/8 <= fmul (dt, speed))
		dt = muldiv (FONE, VMAX/8, speed);
	Vmuldiv (R, EX->tspeed, dt, FONE/8);
}

extern void FAR
SetKillCorrection (OBJECT *p, OBJECT *target, VECT R, int *tti)
{
	int	dist, rv, dt, g, speed;
	long	ldist;
	VECT	V;
	LVECT	LL;

	speed = BulletSpeed (p, V);			/* bullet speed */

	Vsub (LL, p->R, target->R);
	ldist = lhypot3d (LL);

	if (ldist)
		rv = speed/VONE - (int)((LL[X]*EX->tspeed[X] +
					 LL[Y]*EX->tspeed[Y] +
					 LL[Z]*EX->tspeed[Z])/(ldist*VONE));
	else
		rv = 0;

	if (ldist > VMAX)
		dist = VMAX/VONE;
	else
		dist = (int)(ldist/VONE);

	if (rv <= dist/32)
		dt = 32*1000;
	else
		dt = muldiv (1000, dist , rv);

	*tti = dt;

	if (!target->speed && !(st.flags1 & SF_USEG)) {
		R[X] = R[Y] = R[Z] = 0;
		return;
	}

	if (dt < 4000) {
/* projected equivalent speed:
 * 	V+A*t/2
*/
		R[X] = EX->tspeed[X] + muldiv (EX->taccel[X], dt, 1000*2);
		R[Y] = EX->tspeed[Y] + muldiv (EX->taccel[Y], dt, 1000*2);
		R[Z] = EX->tspeed[Z] + muldiv (EX->taccel[Z], dt, 1000*2);
		if (st.flags1 & SF_USEG)
			R[Z] += muldiv (GACC, dt, 2*1000);	/*bullet G!*/
/* projected movement:
 *	V*t
*/
		R[X] = muldiv (R[X], dt, 1000);
		R[Y] = muldiv (R[Y], dt, 1000);
		R[Z] = muldiv (R[Z], dt, 1000);
	} else {
		speed = ihypot3d (EX->tspeed); /* don't trust target->speed */
		if (!speed)
			R[X] = R[Y] = R[Z] = 0;
		else if (VMAX/speed <= dt/1000)
			Vmuldiv (R, EX->tspeed, VMAX, speed);
		else
			Vmuldiv (R, EX->tspeed, dt, 1000);

		if ((st.flags1 & SF_USEG) && dt < 3000) {
			g = muldiv (GACC, dt, 2000);
			g = muldiv (g, dt, 1000);
			R[Z] += g;
		}
	}
}

LOCAL_FUNC void NEAR
set_throttle (OBJECT *p, POINTER *ptr, int speed)
{
	int	throttle, n;

	if (speed < 0)
		speed = 0;
	throttle = (int)pid_calc (EX->PIDthrottle, speed - (long)p->speed,
					st.interval);
	if (throttle > 0) {
		if (ptr->l[8] <= 5)			/* speed-brakes off */
			ptr->l[8] = 0;
		else
			ptr->l[8] -= 5;
		if (ptr->l[3] < 100-2)
			ptr->l[3] += 2;
		else
			ptr->l[3] = 100;
	} else if (throttle < 0) {
		if (throttle <= -20) {
			if (ptr->l[8] < 100-5)		/* speed-brakes on */
				ptr->l[8] += 5;
			else
				ptr->l[8] = 100;
			n = 5;
		} else
			n = 2;
		if (ptr->l[3] > n)
			ptr->l[3] -= n;
		else
			ptr->l[3] = 0;
	}
}

LOCAL_FUNC void NEAR
go_to (OBJECT *p, long altitude, ANGLE heading, int speed)
{
	POINTER	*ptr;
	ANGLE	pitch, roll, rdiff, pdiff, hdiff;
	int	pset, hset, rset, tset, ddiff, upright, factor, maxpitch;
	long	ldiff;

	if (F(ptr = p->pointer))
		return;

	EX->misc[10] = speed - p->speed;

	if (IS_CLASSIC(p))
		factor = CLFACT;
	else
		factor = F16FACT;

	if (ptr->l[6])
		maxpitch = D90/8;
	else {
		maxpitch = p->speed/VONE/2;
		if (maxpitch > LIFTOFF)
			maxpitch = MAXPITCH;
		else
			maxpitch = muldiv (MAXPITCH, maxpitch, LIFTOFF);
	}
	ldiff = altitude - p->R[Z]; 		/* climb rate */
	if (ldiff > MINCLIMB)
		pitch = maxpitch;
	else if (ldiff < -MINCLIMB)
		pitch = -maxpitch;
	else
		pitch = muldiv ((int)ldiff, maxpitch, MINCLIMB);

#define PITCHBAND	D90/4
	pdiff = TANG(pitch + EX->aoa*0 - p->a[X]);	/* pitch rate */
	EX->misc[12] = pdiff;
	if (pdiff > PITCHBAND)
		pset = 100;
	else if (pdiff < -PITCHBAND)
		pset = -100;
	else {
#if 0
		pset  = muldiv (pdiff, 100, PITCHBAND);
#else
		pset = (int)pid_calc (EX->PIDpitch, (long)pdiff, st.interval);
		pset += ptr->l[1];
		if (pset > 100)
			pset = 100;
		else if (pset < -100)
			pset = -100;
#endif
	}
	hdiff = TANG(heading - p->a[Z]);
	if (hdiff > factor)			/* turn rate */
		hset = 100;
	else if (hdiff < -factor)
		hset = -100;
	else
		hset  = muldiv (hdiff, 100, factor);

	upright = iabs (hdiff) < D90/16 && iabs (pdiff) < D90/16;

	tset = ihypot2d (hset, pset);
	if (0 == tset) {
		roll = 0;
		rdiff = -p->a[Y];
	} else {
		if (pset < 0)
			tset = -tset;
		roll = -ASIN (fdiv (hset, tset));
		rdiff = TANG(roll - p->a[Y]);
		if (!upright && iabs (rdiff) > D90) { /* go the short way */
			tset = -tset;
			rdiff += D180;
			roll += D180;
		}
		if (tset < -100) {		/* avoid redout */
			tset = 0;
			rdiff += D180;
			roll += D180;
		}
	}
	EX->misc[13] = rdiff;
	if (F(EX->flags & (PF_CHASE|PF_AUTO)))
		return;

	if (rdiff > factor)			/* set ailerons */
		rset = -100;
	else if (rdiff < -factor)
		rset = 100;
	else
		rset = -muldiv (rdiff, 100, factor);
	ddiff = rset - ptr->l[0];
	if (ddiff > 25)
		ddiff = 25;
	else if (ddiff < -25)
		ddiff = -25;
	ptr->l[0] += ddiff;


	tset = fmul (pset, p->cosy) - fmul (hset, p->siny);

	if (tset > 100)				/* set elevators */
		tset = 100;
	else if (tset < -100)
		tset = -100;
	ddiff = tset - ptr->l[1];
	if (ddiff > 25)
		ddiff = 25;
	else if (ddiff < -25)
		ddiff = -25;
	ptr->l[1] += ddiff;

	set_throttle (p, ptr, speed);
}

LOCAL_FUNC void NEAR
set_flaps (OBJECT *p)
{
	int	t, k;
	POINTER	*ptr;

	if (F(EX->flags & (PF_CHASE|PF_AUTO)))
		return;

	if (F(ptr = p->pointer))
		return;

	if (ptr->l[6]) {
		k = LIFTOFF;
		t = k*2 - p->speed/VONE;
		if (t > 0) {
			t = SETFLAPS * t / k;
			if (ptr->l[6] > t)
				--ptr->l[6];
		} else if (ptr->l[6])
			--ptr->l[6];
	}
	if ((EX->equip & EQ_GEAR) && p->R[Z] > 20*VONE)
		ptr->b[5] = 1;
	
}

LOCAL_FUNC void NEAR
set_rudder (OBJECT *p)
{
	POINTER	*ptr;

	if (F(EX->flags & (PF_CHASE|PF_AUTO)))
		return;

	if (F(ptr = p->pointer))
		return;

	if (ptr->l[2] > 0)
		--ptr->l[2];
	else if (ptr->l[2] < 0)
		++ptr->l[2];
}

LOCAL_FUNC void NEAR
takeoff (OBJECT *p)
{
	POINTER	*ptr;

	if (F(ptr = p->pointer))
		return;
	if (F(EX->flags & (PF_AUTO|PF_CHASE)))
		return;
	if (ptr->l[9] > 10)		/* release brakes */
		ptr->l[9] -= 10;
	else
		ptr->l[9] = 0;
	if (ptr->l[6]  < SETFLAPS)	/* set flaps */
		++ptr->l[6];
	SPEED(p) = PATROLSPEED;
	HEADING(p) = 0;
	if (p->speed >= LIFTOFF)
		ALTITUDE(p) = 2000;
	else
		ALTITUDE(p) = (IS_CLASSIC(p) ? 0 : 100);
	go_to (p, ALTITUDE(p)*(long)VONE, HEADING(p), SPEED(p)*VONE);
}

LOCAL_FUNC void NEAR
chase (OBJECT *p)
{
	POINTER	*ptr;
	ANGLE	rdiff, pdiff, a;
	int	t, speed, pset, rset, kill;
	long	dist;
	OBJECT	*target;
	VECT	R, RR, VV;

	if (F(ptr = p->pointer))
		return;

	if (T(target = EX->target) && target->id != EX->tid)
		target = 0;

	set_flaps (p);
	set_rudder (p);
	if (ptr->l[6]) {
		if (F(EX->flags & (PF_AUTO|PF_CHASE)))
			return;
		pdiff = TANG(D90/8 - p->a[X]);
		rdiff = 0 - p->a[Y];
		a = D90;
		kill = 0;
		speed = PATROLSPEED*VONE;
		dist = 0;
	} else if (!target) {
		if (F(EX->flags & (PF_AUTO|PF_CHASE))) {
			EX->misc[12] = -p->a[X];
			EX->misc[13] = -p->a[Y];
			EX->misc[10] = 0;
			return;
		}
		a = p->a[Z];
		if (p->R[Y] >  MAXRANGE && iabs(a) < D90)
			a = D180;
		else if (p->R[Y] < -MAXRANGE && iabs(a) > D90)
			a = 0;
		if (p->R[X] >  MAXRANGE && a < 0)
			a = D90;
		else if (p->R[X] < -MAXRANGE && a > 0)
			a = -D90;
		go_to (p, 2000L*VONE, a, PATROLSPEED*VONE);
		return;
	} else {
		if ((EX->flags & PF_KILL) || !(EX->flags & (PF_AUTO|PF_CHASE)))
			SetKillCorrection (p, target, R, &t /*dummy*/);
		else
			SetMeetCorrection (p, target, R);

		R[X] = (int)((target->R[X] + R[X] - p->R[X])/VONE);
		R[Y] = (int)((target->R[Y] + R[Y] - p->R[Y])/VONE);
		R[Z] = (int)((target->R[Z] + R[Z] - p->R[Z])/VONE);
		VxMmul (RR, R, p->T);
		pdiff = ATAN (RR[Z], iabs(RR[Y]));	/* pitch error */
		t = ihypot2d (RR[Z], RR[X]);
		a = ATAN (t, iabs(RR[Y]));		/* total error */

		VxMmul (VV, EX->tspeed, p->T);
		RR[X] += VV[X]/VONE;
		RR[Z] += VV[Z]/VONE;
		rdiff = ATAN (RR[X], iabs(RR[Z]));	/* roll error */

		dist = ihypot3d (RR) * (long)VONE;
		if (T(kill = dist > 0)) {
			if (dist > MAXSPEED*3+MINDIST)
				speed = MAXSPEED;
			else {
				speed = (int)((dist-MINDIST)/3);
				if (speed < MINSPEED)
					speed = MINSPEED;
			}
		} else {
			speed = MINSPEED;
			dist = -dist;
		}
	}
	EX->misc[12] = pdiff;
	EX->misc[13] = rdiff;
	EX->misc[10] = speed - p->speed;
	if (F(EX->flags & (PF_CHASE|PF_AUTO)))
		return;

	if (kill && (EX->flags & PF_KILL) && (EX->radar & R_SHOOT))
		++ptr->b[2];

#if 0
DDshow (p, 4, 0, 13);
DDshow (p, 0, "dist", (long)dist/VONE);
DDshow (p, 2, "pdiff", (long)ANG2DEG00(pdiff));
DDshow (p, 2, "rdiff", (long)ANG2DEG00(rdiff));
DDshow (p, 2, "a",     (long)ANG2DEG00(a));
#endif

/* Set ailerons
*/

#define CHAOS_BAND	DEG(3)
	if (a < CHAOS_BAND) {				/* limiting? */
		t = muldiv (D90, a, CHAOS_BAND);
		if (t < DEG(4))
			t = DEG(4);
		if (rdiff > t)
			rdiff = t;
		else if (rdiff < -t)
			rdiff = -t;
#if 0
DDshow (p, 2, "rdiff", (long)ANG2DEG00(rdiff));
#endif
	}
#undef CHAOS_BAND

	rset = (int)pid_calc (EX->PIDroll, (long)rdiff, st.interval);
#if 0
DDshow (p, 0, "rset", (long)rset);
#endif
	if (rset > ptr->l[0]+20)
		ptr->l[0] += 20;
	else if (rset < ptr->l[0]-20)
		ptr->l[0] -= 20;
	else
		ptr->l[0] = rset;

/* Set elevators
*/
#if 1
#define DEG100	DEG(0)
#define DEG000	DEG(60)
	if ((t = iabs (rdiff)) > DEG100) {
		if (t > DEG000)
			pdiff = 0;
		else {
			t = muldiv (D90, DEG000-t, DEG000-DEG100);
			if (pdiff > t)
				pdiff = t;
			else if (pdiff < -t)
				pdiff = -t;
		}
#if 0
DDshow (p, 2, "pdiff", (long)ANG2DEG00(pdiff));
#endif
	}
#undef DEG100
#undef DEG000
#endif
	pset = (int)pid_calc (EX->PIDpitch, (long)pdiff, st.interval);
#if 0
DDshow (p, 0, "pset", (long)pset);
#endif
	pset += ptr->l[1];
	if (pset > 100)
		pset = 100;
	else if (pset < -100)
		pset = -100;
	ptr->l[1] = pset;

	set_throttle (p, ptr, speed);
}

#define XFULL	DEG(2.5)
#define XMAX	DEG(90)
#define XRATIO	8
#define YFULL	DEG(0.75)
#define YMAX	10000
#define YRATIO	8

LOCAL_FUNC void NEAR
do_ils (OBJECT *p)
{
	int	t;

/* heading error.
*/
	t = -EX->misc[15];
	if (t < -XMAX/XRATIO)
		t = -XMAX;
	else if (t > XMAX/XRATIO)
		t = XMAX;
	else
		t *= XRATIO;
	t -= EX->ilsHeading;
	HEADING(p) = t;

/* pitch error.
*/
	t = fmul (100*EX->ilsRange, SIN(EX->misc[14]));
	if (t < -(YMAX/YRATIO))
		t = -YMAX;
	else if (t > (YMAX/YRATIO))
		t = YMAX;
	else
		t *= YRATIO;
	ALTITUDE(p) = (int)(p->R[Z]/VONE) + t;
	SPEED(p) = 100;
	go_to (p, ALTITUDE(p)*(long)VONE, HEADING(p),
		SPEED(p)*VONE);
}
#undef XFULL
#undef XMAX
#undef XRATIO
#undef YFULL
#undef YMAX
#undef YRATIO

LOCAL_FUNC void NEAR
go_wild (OBJECT *p)
{
	POINTER	*ptr;
	ANGLE	a;
	int	t;

	if (F(ptr = p->pointer))
		return;

	set_flaps (p);
	if (ptr->l[6])
		;					/* stay on course */
	else if ((LIFETIME(p) -= st.interval) <= 0) {
		LIFETIME(p) = RANDTIME;			/* every few seconds */
#ifdef DEBUG_SPEED
		SPEED(p) = (Frand()%80 + 20)*20;	/* overflow testing */
#else
		SPEED(p) = RANDOMSPEED;
#endif
		a = HEADING(p) + (Frand()%D90) * 2 - D90;
		if ((p->R[Y] >  MAXRANGE && iabs(a) < D90) ||
		    (p->R[Y] < -MAXRANGE && iabs(a) > D90))
			a = D180 - a;
		if ((p->R[X] >  MAXRANGE && a < 0) ||
		    (p->R[X] < -MAXRANGE && a > 0))
			a = -a;
		HEADING(p) = a;

		t = ALTITUDE(p) + Frand()%4000 - 2000;
		if (t > MAXALT)
			t = 2*MAXALT-t;
		else if (t < MINALT)
			t = 2*MINALT-t;
		ALTITUDE(p) = t;
	}
	go_to (p, ALTITUDE(p)*(long)VONE, HEADING(p), SPEED(p)*VONE);
}

extern void FAR
dynamics_auto (OBJECT *p)
{
	EX->misc[12] = 0;
	EX->misc[13] = 0;
	EX->misc[10] = 0;

	if (EX->hud2 & HUD_ILS && !ils_get (p))
		do_ils (p);
	else if (!(EX->flags & (PF_AUTO | PF_CHASE)))
		chase (p);		/* for director */
	else if (EX->flags & PF_ONGROUND)
		takeoff (p);
	else if (EX->flags & PF_CHASE)
		chase (p);
	else /* if (EX->flags & PF_AUTO) */
		go_wild (p);
}

#undef IS_CLASSIC
#undef RANDTIME
#undef MAXPITCH
#undef MAXALT
#undef MINALT
#undef MAXRANGE
#undef MINCLIMB
#undef MINSPEED
#undef MAXSPEED
#undef MINSHOOT
#undef PATROLSPEED
#undef RANDOMSPEED
#undef MINDIST
#undef SETFLAPS
#undef F16FACT
#undef CLFACT
#undef SENSPITCH
#undef SENSROLL
#undef LIFTOFF
#undef ABMIN
#undef PITCHBAND
