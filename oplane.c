/* --------------------------------- oplane.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Object description: shape and behaviour.
 * Various planes (then see oclassic.c, obasic.c, oxplane.c).
*/

#include "plane.h"


#define	RADAR_RANGE	10000
#define	MINSHOOT	(2000L*VONE)
#define	NOSHOOT		(200L*VONE)
#define	PDAMAGE		10

static SHAPE shape_plane = {
	0,
	0,
	SH_BEHIT,
	10000*1000L,	/* weight */
	0		/* drag */
};

#define CCSIZE	8
#define CCCOL	CCSIZE
#define CCROW	(CCSIZE+1)

extern void FASTCALL FAR
fDDshow (int frac, char *title, long value)
{
	static int	toprow = 4, row = 4, col = 12;
	int		savefont;

	if (!title) {
		if (frac >= 0)
			toprow = row = frac;
		if (value >= 0)
			col = (int)value;
		return;
	}
	if (row*CCROW >= CS->sizey) {
		row = toprow;
		col += 12;
	}
	savefont = font_set (0);
	stroke_str  (col*CCCOL,     row*CCROW, title, CCSIZE, ST_HFG);
	stroke_frac ((col+6)*CCCOL, row*CCROW, value, 0, frac, CCSIZE,
		ST_HFG);
	font_set (savefont);
	++row;
}

extern void FASTCALL FAR
fCCshow (int frac, char *title, long value)
{
	static int	toprow = 4, row = 4, col = 1;
	int		savefont;

	if (!title) {
		if (frac >= 0)
			toprow = row = frac;
		if (value >= 0)
			col = (int)value;
		return;
	}
	if (row*CCROW >= CS->sizey) {
		row = toprow;
		col += 12;
	}
	savefont = font_set (0);
	stroke_str  (col*CCCOL,     row*CCROW, title, CCSIZE, ST_HFG);
	stroke_frac ((col+6)*CCCOL, row*CCROW, value, 0, frac, CCSIZE,
		ST_HFG);
	font_set (savefont);
	++row;
}
#undef CCSIZE
#undef CCCOL
#undef CCROW

extern void FASTCALL FAR
fCFshow (char *title, int value)
{
	fCCshow (3, title, value*1000L/FONE);
}

extern void FASTCALL FAR
fCAshow (char *title, int value)
{
	fCCshow (2, title, (long)ANG2DEG00 (value));
}

extern void FASTCALL FAR
fCVshow (char *title, int value)
{
	fCCshow (2, title, value*100L/VONE);
}

extern void FAR
CCnote (OBJECT *p, char *note)
{
	if (CC == p)
		MsgPrintf (50, note);
}

extern void FAR
CCland (OBJECT *p)
{
	if (CC == p)
		CCnote (p, "touchdown");
}

extern void FAR
CCfly (OBJECT *p)
{
	if (CC == p) {
		CCnote (p, "takeoff");
		EX->maxG = 10;
	}
}

/* We are off the ground. Did we land?
*/
extern int FAR
check_land (OBJECT *p)
{
	int	errs;

	if (p->R[Z] > EP->opt[5]*2)
		return (0);
	
	EX->flags |= PF_ONGROUND;

	EX->misc[6] = (short)p->R[Z]*VONE;

	errs = 0;

	if (p->R[Z] < 0L) {
		CCnote (p, "under");
		++errs;
	}

	if (!(EX->equip & EQ_GEAR)) {
		CCnote (p, "belly");
		++errs;
	}

	if (p->a[Y] > D90/4) {
		CCnote (p, "chinese");
		++errs;
	}
#if 0
	p->a[Y] = 0;
	p->da[Y] = 0;
#endif
#if 0
	if (p->a[X] < EP->gpitch-DEG(2)) {
		CCnote (p, "nose down");
		++errs;
	}
#endif
	if (p->vb[X] < -10*VONE || p->vb[X] > 10*VONE) {
		CCnote (p, "sideways");
		++errs;
	}
#if 0
	p->vb[X] = 0;
#endif
#if 0
	if (p->V[Z] < -7*VONE) {
		CCnote (p, "crash");
		++errs;
	}
	p->V[Z] = 0;
#endif
	if (!on_runway (p)) {
		CCnote (p, "grass");
		++errs;
	}
#if 0
	p->R[Z] = 0;
#endif
	if (!errs)
		CCland (p);
	return (errs);
}

/* We are on the ground. Did we taking off?
*/
extern int FAR
check_takeoff (OBJECT *p)
{
	int	errs;

	errs = 0;
	if (p->R[Z] < 0L) {
		CCnote (p, "under");
		++errs;
	}
	if (!on_runway (p)) {
		CCnote (p, "grass");
		++errs;
	}
	if (!(EX->equip & EQ_GEAR)) {
		CCnote (p, "belly");
		++errs;
	}
	if (p->a[Y] > D90/4) {
		CCnote (p, "chinese");
		++errs;
	}
#if 0
	if (p->a[X] < EP->gpitch-DEG(2)) {
		CCnote (p, "nose down");
		++errs;
	}
#endif
	if (!errs && p->R[Z] > EP->opt[5]*2) {
		EX->flags &= ~PF_ONGROUND;
		CCfly (p);
	}
	return (errs);
}

extern void FAR
supply (OBJECT *p, int mode)
{
	if (!mode) {
		EX->fuel += TADJ(100)*100L;		/* 100lb/sec */
		if (EX->fuel < EP->fuel_capacity*100L)
			return;
	}
	EX->fuel = EX->parms->fuel_capacity*100L;	/* full tank */
	memcpy (EX->stores, EX->parms->stores, sizeof (EX->stores));
	p->damage = PDAMAGE;
}

LOCAL_FUNC void NEAR
btrail_delete (BTRAIL *h)
{
	BTRAIL	*hh;

	while (h) {
		hh = h->next;
		DEL (h);
		h = hh;
	}
}

LOCAL_FUNC void NEAR
free_plane (OBJECT *p)
{
	if (IS_PLANE(p)) {
		if (EP)
			(*flight_models[EP->opt[0]]) (p, 2);
		DEL0 (EX->PIDthrottle);
		DEL0 (EX->PIDpitch);
		DEL0 (EX->PIDroll);
		btrail_delete (EX->btrail);
		DEL0 (EX);
		p->e_type = 0;
	}
}

extern int FAR
dampen (int old, int new, int factor)
{
	int	diff;

	factor = muldiv (factor, 67, st.interval);
	if (factor < 1)
		factor = 1;
	diff = new - old;
	if (diff > factor || diff < -factor)
		diff /= factor;
	else if (diff > 0)
		diff = 1;
	else if (diff < 0)
		diff = -1;
	return (old + diff);
}

LOCAL_FUNC void NEAR
ShootCue (OBJECT *p, OBJECT *target)
{
	VECT	R, RR;
	ANGLE	perr, err;
	int	t, limit, dist;
	LVECT	IP;

	if (WE_MK82 == EX->weapon) {
		BombSpeed (p, R);
		BombIP (p->R, R, target->R[Z], IP);
		if (est_dist (target->R, IP) < (Uint)SH(target)->extent)
			EX->radar |= R_SHOOT;
		return;
	}

	SetKillCorrection (p, target, R, &t);
#if 0
	R[X] += TADJ (EX->tspeed[X]);
	R[Y] += TADJ (EX->tspeed[Y]);
	R[Z] += TADJ (EX->tspeed[Z]);
#endif
	R[X] = (int)((target->R[X] + R[X] - p->R[X])/VONE);
	R[Y] = (int)((target->R[Y] + R[Y] - p->R[Y])/VONE);
	R[Z] = (int)((target->R[Z] + R[Z] - p->R[Z])/VONE);
	dist = ihypot3d (R);

	if (dist < MINSHOOT/VONE && dist > NOSHOOT/VONE) {
		BulletSpeed (p, RR);
		R[X] -= muldiv (RR[X], t, VONE*1000);
		R[Y] -= muldiv (RR[Y], t, VONE*1000);
		R[Z] -= muldiv (RR[Z], t, VONE*1000);
		VxMmul (RR, R, p->T);
		if (RR[Y] > 0) {
			t = ihypot2d (RR[Z], RR[X]);
			perr = ATAN (t, dist);
			limit =  SH(target)->extent;
			err = ATAN (limit/4, dist) + BULLETSCATTER/2;
			if (perr < err)
				EX->radar |= R_SHOOT;
		}
#if 0
DDshow (p, 4, 0, 25);
DDshow (p, 0, "range", (long)dist);
DDshow (p, 0, "miss", (long)t);
DDshow (p, 2, "perr", (long)ANG2DEG00(perr));
DDshow (p, 2, "err", (long)ANG2DEG00(err));
#endif
	}
}

LOCAL_FUNC void NEAR
do_radar (OBJECT *p)
{
	int	radar, dist, x, t, fref, shifty;
	OBJECT	*pp, *target;
	VECT	R, RR;
	MAT	T;

	EX->radar &= ~R_SHOOT;
	radar = EX->radar;

	if (!(radar & R_ON) || !EX->weapon) {
		EX->target = 0;
		return;
	}
/* target selection
*/

#define SEL3D	(D90/54)		/* 3.3 degrees wide circle */
#define SEL5DX	(D90/34)		/* 5.3 degrees wide */
#define SEL5DY	(D90/3*2)		/* 60 degrees high */

	if (T(target = EX->target) && target->id != EX->tid)
		EX->target = target = 0;

	if (!target || !(radar & R_LOCK) ||	/* have target? */

/* Do not automatically release a locked target any more...
*/
	    ((EX->flags & (PF_AUTO|PF_CHASE)) &&
			(target->flags & (F_HIT|F_STEALTH))) ||
	    est_dist (p->R, target->R) > RADAR_RANGE) {

		target = 0;			/* no, seek new one */
		dist = RADAR_RANGE;

		if (radar & R_SELECT3) {
			fref = SEL3D;
			shifty = 0;
		} else if (radar & R_SELECT20) {
			fref = DEG2ANG (EX->hudarea);
			shifty = fmul (EX->hudshift, fref);
		} else if (radar & R_SELECT5) {
			fref = -1;
			shifty = fmul (EX->hudshift, DEG2ANG (EX->hudarea));
		} else {
			fref = 0;
			shifty = 0;
		}

		Mcopy (T, p->T);
		Mxpose (T);
		Mrotx (T, shifty);

		for (pp = CO; pp; pp = pp->next) {
			if (!(pp->shflags & SH_BEHIT)
			    || (pp->flags & (F_HIT|F_STEALTH))
			    || ((radar & R_INTELCC) && !(pp->flags & F_CC))
			    || pp == p
			    || (p->flags & pp->flags & F_FRIEND))
				continue;
			if (WE_MK82 == EX->weapon && pp->R[Z] != 0L)
				continue;
			x = est_dist (p->R, pp->R);
			if (fref && x < dist) {
				R[X] = (int)((pp->R[X]-p->R[X])/VONE);
				R[Y] = (int)((pp->R[Y]-p->R[Y])/VONE);
				R[Z] = (int)((pp->R[Z]-p->R[Z])/VONE);
				VMmul (RR, R, T);
				if (RR[Y] > 0) {
					if (fref < 0) {
						t = ATAN (RR[X], RR[Y]);
						if (iabs (t) > SEL5DX)
							continue;
						t = ATAN (RR[Z], RR[Y]);
						if (t < 0 || t > SEL5DY)
							continue;
					} else {
						t = ihypot2d (RR[X], RR[Z]);
						if (ATAN (t, RR[Y]) > fref)
							continue;
					}
				} else
					continue;
			}
			if (x < dist) {
				dist = x;
				target = pp;
			}
		}
		if (!target) {				/* no new target */
			EX->target = 0;
			target = 0;
			return;
		}
	}

/* target tracking data
*/
	object_dynamics (target);
	if (EX->target != target) {			/* new target */
		EX->target = target;
		EX->tid = target->id;
		Vcopy (EX->tspeed, target->V);
		EX->taccel[X] = EX->taccel[Y] = EX->taccel[Z] = 0;
		NEWTGT(p) = 11;		/* odd will show on first frame */

		if (EX->PIDthrottle) {
			EX->PIDthrottle->I = 0L;
			EX->PIDthrottle->Pprev = 0L;
		}
		if (EX->PIDpitch) {
			EX->PIDpitch->I = 0L;
			EX->PIDpitch->Pprev = 0L;
		}
		if (EX->PIDroll) {
			EX->PIDroll->I = 0L;
			EX->PIDroll->Pprev = 0L;
		}

		if (st.quiet && p == CV)	/*fix 2*/
			Snd->Effect (EFF_NOTICE, SND_ON);
	} else {
		EX->taccel[X] = target->V[X] - EX->tspeed[X];
		EX->taccel[Y] = target->V[Y] - EX->tspeed[Y];
		EX->taccel[Z] = target->V[Z] - EX->tspeed[Z];

		EX->tspeed[X] += muldiv (EX->taccel[X], st.interval, 1000);
		EX->tspeed[Y] += muldiv (EX->taccel[Y], st.interval, 1000);
		EX->tspeed[Z] += muldiv (EX->taccel[Z], st.interval, 1000);
	}
	ShootCue (p, target);
}

extern void FAR
eject (OBJECT *p)
{
	OBJECT	*obj;

	st.owner = p;
	if (T(obj = create_object (O_CHUTE, 1))) {
		p->gpflags &= ~GPF_PILOT;	/* indicate no one home */
		p->flags |= F_MOD;
		if (p == CC) {
			obj->gpflags |= GPF_PILOT;	/* mark the chute */
			p->flags |= F_HIT;		/* kill the plane */
			MsgWPrintf (50, "ejected");
		}
		if (p == CV) {
			save_viewport (CV);
			CV = obj;
			get_viewport (CV);
		}
	}
}

/* Check if our object (assuming z=0) is inside the runway boundary.
*/
extern int FAR
on_runway (OBJECT *p)
{
	OBJECT	*w;
	int	c, s;
	long	lx, ly;

	for (w = CL; w; w = w->next) {
		if (w->name != O_RUNWAY)
			continue;
		lx = p->R[X] - w->R[X];
		ly = p->R[Y] - w->R[Y];
		if (w->a[Z]) {
			s = w->sinz;
			c = w->cosz;
			lx = (lx * c + ly * s)/FONE;
			ly = (ly * c - lx * s)/FONE;
		}
#ifdef ACM_RUNWAY
#define	W	(25L*VONE)
#define	L	(2000L*VONE)
#define	S	(0L*VONE)
		if (labs(lx) < W) {
			if (ly < L && ly > -S)
				return (1);
		}
#else
#define	W	(64L*VONE)
#define	L	(1750L*VONE)
#define	S	(250L*VONE)
		if (labs (lx) < W) {
			if (ly < L && ly > -S)
				return (1);
		} else if (labs (ly) < W) {
			if (lx < L && lx > -S)
				return (1);
		}
#endif
#undef	W
#undef	L
#undef	S
	}
	return (0);
}

extern void FAR
shoot (OBJECT *p, int weapon, int n, int seed, int interval)
{
	OBJECT	*w;
	int	i, local, limited;

	st.owner = p;
	Fsrand (seed);

	local = IS_PLANE(p);
	limited = local && (EX->flags & PF_LIMITED);

    	if (WE_MK82 == weapon) {
		st.owner = p;
		for (i = 0; i < n;  ++i) {
			if (limited && EX->stores[WE_MK82-1] <= 0)
				break;
			if (F(w = create_object (O_MK82, 1)))
				break;
			if (local)
				--EX->stores[WE_MK82-1];
			else
				++p->misc[0];
		}
	} else if (WE_M61 == weapon) {
		for (i = 0; i < n;  ++i) {
			if (limited && EX->stores[WE_M61-1] <= 0)
				break;
			if (F(w = create_object (O_M61, 1)))
				break;
			if (local) {
				if (!(EX->stores[WE_M61-1]-- % 8))
					w->flags |= F_VISIBLE;
			} else {
				if (!(p->misc[0]++ % 8))
					w->flags |= F_VISIBLE;
			}
		}
	} else
		i = 0;
	if (i && (p->flags & F_EXPORTED))
		remote_shoot (p, weapon, i, seed, interval);
}

extern void FAR
plane_smoke (OBJECT *p)
{
	int	i, t;
	OBJECT	*s;

	if (p->damage >= PDAMAGE ||
	    !(st.flags1 & SF_SMOKE) ||
	    st.interval <= 0)
		return;

	if (p->damage < PDAMAGE && (st.flags1 & SF_SMOKE)) {
		for (i = 2*TADJ(PDAMAGE-p->damage); i-- > 0;)
			if (T(s = create_object (O_SMOKE, 1))) {
				s->time = 3*TIMEPSEC;
				t = Frand () % st.interval;
				s->R[X] -= muldiv (p->V[X], t, 1000);
				s->R[Y] -= muldiv (p->V[Y], t, 1000);
				s->R[Z] -= muldiv (p->V[Z], t, 1000);
			}
	}
}

extern int FAR
dynamics_input (OBJECT *p)
{
	POINTER	*ptr;
	int	i, t;
	Uxshort	temp;

	if (F(ptr = p->pointer) || !IS_PLANE(p))
		return (1);

	if (p->flags & F_HIT) {
		gen_dynamics (p);
		return (1);
	}

	plane_smoke (p);

	if (!(p->gpflags & GPF_PILOT))
		return (0);
#if 0
	if ((EX->flags & PF_AUTO) && !(EX->flags & PF_CHASE))
		dynamics_auto (p);
	else {
		if (pointer_read (ptr, !(EX->flags & PF_CHASE))) {
			MsgEPrintf (10, "pointer lost!");
			Snd->Effect (EFF_NO_POINTER, SND_ON);
			return (1);
		}
		dynamics_auto (p);
	}
#else
	if (pointer_read (ptr, !(EX->flags & (PF_AUTO | PF_CHASE)))) {
		MsgEPrintf (10, "pointer lost!");
		Snd->Effect (EFF_NO_POINTER, SND_ON);
		return (1);
	}
	dynamics_auto (p);
#endif

/* pointer usage:
 * a[0]	roll clockwise
 * a[1]	pitch up
 * a[2]	rudder right
 * a[3]	throttle
 * a[4]	trim pitch up
 * a[5]	trim rudder right
 * a[6]	flaps
 * a[7]	spoilers
 * a[8]	speed brakes
 * a[9]	ground brakes
 *
 * b[0]  level
 * b[1]  origin
 * b[2]  fire
 * b[3]  stable
 * b[4]  reset roll
 * b[5]  gear up/down
 * b[6]  release radar lock
 *
 * opt[0] flight model
 * opt[1] response
 * opt[2]
 * opt[3] turn speed (classic)
*/

	DAMPEN (EX->ailerons, ptr->l[0], EP->opt[1]);
	DAMPEN (EX->elevators, ptr->l[1], EP->opt[1]);

	if (ptr->b[0]) {
		p->a[X]  = 0;
		p->a[Y]  = 0;
		p->a[Z]  = 0;
		p->da[X] = 0;
		p->da[Y] = 0;
		p->da[Z] = 0;
		p->dae[X] = 0;
		p->dae[Y] = 0;
		p->dae[Z] = 0;
		Mident (p->T);
		(*ptr->control->Center)(ptr);
	}

	if (ptr->b[1]) {
		p->R[X] = 0;
		p->R[Y] = 0;
		p->R[Z] = 0;
		ptr->l[3] = 0;
	}

	if (ptr->b[2]) {
		if (WE_MK82 == EX->weapon) {
			if (EX->misc[11] + ptr->b[2] > 4)
				i = 4 - EX->misc[11];
			else
				i = ptr->b[2];
			if (i)
				shoot (p, WE_MK82, i, Frand(), st.interval);
		} else if (WE_M61  == EX->weapon) {
			shoot (p, WE_M61, 1+st.interval/(1000/(3000/60)),
				Frand(), st.interval);
		}
	}

	if (ptr->b[3])
		(*ptr->control->Center)(ptr);

	if (ptr->b[4])
		p->a[Y] = 0;

	if (ptr->b[5] > 0) {
		if (EX->flags & PF_ONGROUND) {
			MsgWPrintf (10, "Gear locked");
		} else {
			temp = EX->equip;
			while (ptr->b[5] > 0) {
				SetOption (&EX->equip, EQ_GEAR);
				--ptr->b[5];
			}
			if ((temp ^ EX->equip) & EQ_GEAR) {
				if (CC == p) {
					MsgPrintf (50, "Gear %s",
						EX->equip&EQ_GEAR
						? "lowered" : "raised");
					if (st.quiet)
						Snd->Effect (EFF_GEAR, SND_ON);
				}
			}
		}
	}
	if (EX->equip & EQ_GEAR) {		/* lower */
		for (i = 0; i < rangeof(EP->gear) && EP->gear[i].ratedn; ++i) {
			EX->gear[i] += TADJ (EP->gear[i].ratedn);
			if (EX->gear[i] > 100)
				EX->gear[i] = 100;
		}
	} else {				/* raise */
		for (i = 0; i < rangeof(EP->gear) && EP->gear[i].rateup; ++i) {
			EX->gear[i] -= TADJ (EP->gear[i].rateup);
			if (EX->gear[i] < 0)
				EX->gear[i] = 0;
		}
	}

	if (ptr->b[6])
		EX->target = 0;

	EX->rudder = ptr->l[2];
	if (ptr->l[3] > 75) {
		EX->throttle = 100;
		EX->afterburner = muldiv (ptr->l[3]-75, 100, 100-75);
	} else {
		EX->throttle = muldiv (ptr->l[3], 100, 75);
		EX->afterburner = 0;
	}
	EX->tElevators = ptr->l[4];
	EX->tRudder = ptr->l[5];
	EX->flaps = ptr->l[6];
	EX->spoilers = ptr->l[7];
	EX->airbrake = ptr->l[8];
	EX->brake = ptr->l[9];

	memset (ptr->b, 0, sizeof (ptr->b));	/* clear all buttons */

	if ((t = EX->tRudder+EX->rudder) > 100)
		EX->rudder = 100 - EX->tRudder;
	else if (t < -100)
		EX->rudder = -100 - EX->tRudder;

	if ((t = EX->tElevators+EX->elevators) > 100)
		EX->elevators = 100 - EX->tElevators;
	else if (t < -100)
		EX->elevators = -100 - EX->tElevators;

	return (0);
}

LOCAL_FUNC void NEAR
do_btrail (OBJECT *p)
{
	BTRAIL	*h;

	if (WE_M61 != EX->weapon || !(EX->hud2 & HUD_BTRAIL)) {
		btrail_delete (EX->btrail);
		EX->btrail = 0;
		return;
	}

	h = EX->btrail;
	if (!h || h->ms >= 1000/25) {		/* limit frequency */
		if (F(NEW (h)))
			return;
		BulletSpeed (p, h->V);
		LVcopy (h->R, p->R);
		h->ms = 0;
		h->next = EX->btrail;
		EX->btrail = h;
	}
	for (; h; h = h->next) {
		h->ms += st.interval;
		h->V[Z] -= TADJ (GACC);
		h->R[X] += TADJ (h->V[X]);
		h->R[Y] += TADJ (h->V[Y]);
		h->R[Z] += TADJ (h->V[Z]);
		if (h->ms > 1500) {
			btrail_delete (h->next);
			h->next = 0;
			break;
		}
	}
}

/* arm the lapms panel.
*/
LOCAL_FUNC void NEAR
do_lamps (OBJECT *p)
{
	long	t;
	int	i;

	if (IS_PLANE(p) && (p->gpflags & GPF_PILOT)) {
		LAMP_SET_OFF (LAMP_GLIMIT);
		LAMP_SET_OFF (LAMP_STALL);
		LAMP_SET_OFF (LAMP_FUEL);
		LAMP_SET_OFF (LAMP_ALT);
		LAMP_SET_OFF (LAMP_PULLUP);
		LAMP_SET_OFF (LAMP_EJECT);
		LAMP_SET_OFF (LAMP_DAMAGE);
		LAMP_SET_OFF (LAMP_EJECT);
		LAMP_SET_OFF (LAMP_DAMAGE);
		LAMP_SET_OFF (LAMP_GEAR);

		if (EX->flags & PF_GLIMIT)
			LAMP_SET_RED (LAMP_GLIMIT, LAMP_ON);

		if (EX->flags & PF_STALL)
			LAMP_SET_RED (LAMP_STALL, LAMP_ON);

		if (EX->fuel <= 0)
			LAMP_SET_RED (LAMP_FUEL, LAMP_ON|LAMP_BLINK);
		else {
			t = 1000L * EX->fuel / EP->fuel_capacity;
			if (t < 10000L)
				LAMP_SET_RED (LAMP_FUEL, LAMP_ON);
		}

		get_cue (p);

		if (!(EX->flags & PF_ONGROUND) && !(EX->equip & EQ_GEAR)) {
			if ((int)EX->misc[17] < FONE) {
				LAMP_SET_RED (LAMP_ALT, LAMP_ON);
				if ((int)EX->misc[17] < FCON(0.2))
					LAMP_SET_RED (LAMP_PULLUP,
							LAMP_ON|LAMP_BLINK);
			}
		}

		if (p->damage <= 0)
			LAMP_SET_RED (LAMP_EJECT, LAMP_ON|LAMP_BLINK);
		else if (p->damage <= 3)
			LAMP_SET_RED (LAMP_DAMAGE, LAMP_ON);

		for (i = 0; i < rangeof (EP->gear) && EP->gear[i].z; ++i) {
			if (EX->gear[i] > 0 && EX->gear[i] < 100) {
				LAMP_SET_GREEN (LAMP_GEAR, LAMP_ON|LAMP_BLINK);
				break;
			}
		}

#define GEARVLIMIT	(150*VONE)
		if (EX->equip & EQ_GEAR) {
			if (p->speed > GEARVLIMIT)
				LAMP_SET_RED (LAMP_GEAR, LAMP_ON|LAMP_BLINK);
			else
				LAMP_SET_GREEN (LAMP_GEAR, LAMP_ON);
		}
#undef GEARVLIMIT
	}
}

extern void FAR
place_plane (OBJECT *p, short home)
{
	p->home = home;

	p->R[X] = ils[home].R[X];
	p->R[Y] = ils[home].R[Y];
	p->R[Z] += ils[home].R[Z];
	p->longitude = ils[home].longitude;
	p->latitude  = ils[home].latitude;
	p->a[Z] = -ils[home].localizer;
	Mobj (p);
}

extern void FAR
emit_drone (void)
{
	OBJECT	*p;
	POINTER *ptr;

	if (F(ptr = pointer_select ("random")))
		return;
	st.options = st.dtype;
	if (F(p = create_object (st.d_name, 1)))
		return;
	p->pointer = ptr;
	if (IS_PLANE (p)) {
		p->gpflags |= GPF_PILOT;
		EX->flags |= PF_AUTO;
		place_plane (p, nav_find (""));
		LIFETIME(p) = 5*1000;	/* let it take off nicely */
		SPEED(p) = 250;
		HEADING(p)  = p->a[Z];
		ALTITUDE(p) = (int)(p->R[Z]/VONE) + 100;
	}
}

extern char * FAR
get_wname (int w)
{
	switch (w) {
	case WE_M61:
		return ("GUN");
	case WE_MK82:
		return ("MK82");
	default:
		return ("XXX");
	}
}

extern void FAR
plane_xfer (OBJECT *p)
{
	OBJECT	*q;

	if (!IS_PLANE (CC) || !IS_PLANE (p))
		return;

	CC->pointer->l[9] = 100;	/* brake */

	EE(CC)->flags |= EE(p)->flags;
	EE(CC)->hud  = EE(p)->hud;
	EE(CC)->hud1 = EE(p)->hud1;
	EE(CC)->hud2 = EE(p)->hud2;
	EE(CC)->hud3 = EE(p)->hud3;
	EE(CC)->hudmode = EE(p)->hudmode;
	EE(CC)->hdd = EE(p)->hdd;
	EE(CC)->ladder = EE(p)->ladder;
	EE(CC)->radar = EE(p)->radar;
	EE(CC)->hudarea = EE(p)->hudarea;
	EE(CC)->tapelen = EE(p)->tapelen;
	EE(CC)->weapon = EE(p)->weapon;
	EE(CC)->ils = EE(p)->ils;
	EE(CC)->hudshift = EE(p)->hudshift;
	EE(CC)->ldgap = EE(p)->ldgap;
	EE(CC)->ldstep = EE(p)->ldstep;
	EE(CC)->ldstep0 = EE(p)->ldstep0;
	EE(CC)->ldstepg = EE(p)->ldstepg;
	EE(CC)->ldtip = EE(p)->ldtip;
	EE(CC)->ldndash = EE(p)->ldndash;
	EE(CC)->hudFontSize = EE(p)->hudFontSize ;
	CC->flags |= p->flags & ~(F_HIT|F_DEL);
	CC->gpflags = p->gpflags;
	if (!(CC->gpflags & GPF_PILOT))
		CC->flags |= F_STEALTH;		/* wait for pilot */
	CC->color = ST_FRIEND;
	CC->score  = p->score;
	p->pointer = 0;
	CC->viewport = p->viewport;
	p->viewport = 0;
	if (st.flags & SF_BLANKER)
		EE(CC)->weapon = WE_M61;
	for (q = CO; q; q = q->next) {
		if (O_BROKEN == q->name && q->owner == p) {
			q->owner = CC;
			q->ownerid = CC->id;
		}
	}
	place_plane (CC, st.home);
}

LOCAL_FUNC void FAR
term_plane (BODY *b)
{
	parms_free ();
	gen_term (b);
}

LOCAL_FUNC int FAR
create_plane (OBJECT *p)
{
	E_PLANE	*e;
	char	*ptype;
	int	i;

	if (F(NEW (e)))
		return (1);
	p->e_type = ET_PLANE;
	p->extra = e;

	if (F(NEW (e->PIDthrottle)) ||
	    F(NEW (e->PIDpitch)) ||
	    F(NEW (e->PIDroll))) {
	    	free_plane (p);
	    	return (1);
	}

	if (T(ptype = get_siarg (st.options, 1))) {
		e->parms = parms_get (ptype);
		STRfree (ptype);
		ptype = NULL;
	}
	if (!e->parms) {
	    	free_plane (p);
		return (1);
	}

	memcpy (e->PIDthrottle, &e->parms->PIDthrottle, sizeof (F8PID));
	memcpy (e->PIDpitch,    &e->parms->PIDpitch,    sizeof (F8PID));
	memcpy (e->PIDroll,     &e->parms->PIDroll,     sizeof (F8PID));

	e->flags |= PF_ONGROUND | PF_AUTOFLAP | PF_AUTOELEVATOR;

	e->hud |= HUD_ON;
	e->hud1 |= e->parms->hudtype;

	supply (p, 1);
	hud_setup (p);

	e->equip |= EQ_GEAR;
	for (i = 0; i < rangeof(e->parms->gear) && e->parms->gear[i].z; ++i)
		e->gear[i] = 100;
	e->radar = 3*R_MODE;

	p->a[X] = e->parms->gpitch;
	p->color = CC_WHITE;
	p->time = FOREVER;
	p->tom = p->tob + 5000;		/* 5 seconds grace */
	p->damaging = 5;
	p->flags |= F_VISIBLE | F_EXPORTED | F_MAINT | F_KEEPNAV;

	CP->eyez = e->parms->eyez;
	CP->eyey = e->parms->eyey;
	p->R[Z] = e->parms->opt[5];
	e->maxG = 0;

	place_plane (p, 0);		/* default main runway */

	e->misc[6] = (short)p->R[Z]*VONE;	/* hi-res height */

	(*flight_models[EP->opt[0]]) (p, 1);

	return (0);
}

LOCAL_FUNC void FAR
delete_plane (OBJECT *p)
{
	free_plane (p);
	p->owner = 0;
	p->ownerid = 0;
}

LOCAL_FUNC void FAR
dynamics_plane (OBJECT *p)
{
CCshow (p, 2, 0,  1);
DDshow (p, 2, 0, 15);
	if (MODEL_CLASSIC != EP->opt[0]) {
		int	interval;
		int	intsave;
		int	intref;

		interval = intsave = st.interval;
		st.interval = st.interval_max;
		if (EX->flags & PF_ONGROUND)
			st.interval /= 4;
		if (st.interval < MIN_FRAME_RATE)
			st.interval = MIN_FRAME_RATE;
		intref = st.interval + st.interval/2;
		for (; interval > intref; interval -= st.interval) {
			(*flight_models[EP->opt[0]]) (p, 0);
			object_update (p);
		}
		st.interval = interval;
		(*flight_models[EP->opt[0]]) (p, 0);
		object_update (p);
		st.interval = intsave;
	} else {
		(*flight_models[EP->opt[0]]) (p, 0);
		object_update (p);
	}
	do_radar (p);
	do_btrail (p);
	do_lamps (p);
	sys_poll (8);		/* give the timer a chance */
}

LOCAL_FUNC void FAR
hit_plane (OBJECT *p, int speed, int extent, int damaging)
{
	if (p->damage <= 0 && !(p->flags & F_HIT)) {
		p->color = ST_FIRE1;
		p->flags |= F_HIT|F_MOD;
		p->shflags |= SH_G;
		p->time = FOREVER;
		if (p == CC && !(EX->hud1&HUD_VALARM))
			MsgEPrintf (50, "YOU'RE HIT, EJECT!");
		if (p->damage == 0 && !(p->flags & F_IMPORTED) &&
		    !(p->flags&F_CC))
			eject (p);
	}

/* explode.
*/
	st.owner = p;
	if (p->damage >= -20)		/* limit vicious loops */
		object_break (Frand()%5 + 2, speed, extent, 0);
	object_rand (p, speed, extent, 1);
}


BODY FAR BoPlane = {
	0,
	0,
	"PLANE",
	&shape_plane,
	gen_read,
	term_plane,
	create_plane,
	delete_plane,
	dynamics_plane,
	hit_plane
};

#undef RADAR_RANGE
#undef MINSHOOT
#undef PDAMAGE
#undef SEL3D
#undef SEL5DX
#undef SEL5DY
