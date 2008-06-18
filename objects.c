/* --------------------------------- objects.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Some generic object behaviour routines and hit detection.
*/

#include "fly.h"


/* how many object refreshes are allowed each frame
*/
static int	RefreshPerFrame = 1;

/* hit routines.
*/
extern void FAR
object_rand (OBJECT *obj, int speed, int extent, int noise)
{
	int	e, i;

	if (O_GTARGET != obj->name) {
		e =  SH(obj)->extent;
		if (!e)
			e = 1;
		if (extent/64 <= e) {
			if (0 == (i = SH(obj)->extent))
				i = 1;
			i = muldiv (speed/64, extent, i);
		} else
			i = speed;
		if (i) {
			obj->V[X] += Frand () % i;
			obj->V[Y] += Frand () % i;
			obj->V[Z] += Frand () % i;
		}
		i = 0x7fff/e;
		if (i) {
			obj->speed = ihypot3d (obj->V);
			obj->da[X] = Frand ()%i-i/2;
			obj->da[Y] = Frand ()%i-i/2;
			obj->da[Z] = Frand ()%i-i/2;
		}
	}

	if (noise && st.quiet)
		Snd->Effect (EFF_HIT, SND_ON);
}

extern int FAR
object_break (int n, int speed, int extent, int noise)
{
	int	i;
	OBJECT	*p;

	for (i = 0; i < n; ++i) {
		if (F(p = create_object (O_BROKEN, 1)))
			break;
		p->flags |= F_HIT;
		object_rand (p, speed, extent, noise);
	}
	return (i);
}

extern void FAR
object_hit (OBJECT *obj, int seed, int speed, int extent, int damaging)
{
	Fsrand (seed);

	obj->damage -= damaging;

	if (obj == CC) {
		MsgWPrintf (50, "damage %d", obj->damage);
		if (st.quiet)
			Snd->Effect (EFF_DAMAGE, SND_ON);
	}

	st.bodies[obj->name]->hit (obj, speed, extent, damaging);

	if (obj->flags & F_EXPORTED)	/* exported victim */
		remote_imhit (obj, seed, speed, extent, damaging);
}

LOCAL_FUNC int NEAR
hit (VECT p, VECT d, int r)	/* initial distance p, closure speed r */
{
	long	x, y, z, s, t;

	x = p[X];
	y = p[Y];
	z = p[Z];

	s = x*d[X]+y*d[Y]+z*d[Z];
	if (s < 0)		/* objects getting away from each other */
		return (0);
	if (s > 0) {
		t = d[X]*(long)d[X]+d[Y]*(long)d[Y]+d[Z]*(long)d[Z];
		if (s >= t) {
#if 0
			x -= d[X];	/* not really needed now */
			y -= d[Y];
			z -= d[Z];
#endif
			return (0);	/* objects will be closer */
		} else {
			x -= d[X]*s/t;
			y -= d[Y]*s/t;
			z -= d[Z]*s/t;
		}
	}

#if 0
	return ((x*x+y*y+z*z) < r*(long)r);
#else
	return (est_hyp ((int)x, (int)y, (int)z) < (Uint)r);
#endif
}

LOCAL_FUNC void NEAR
check_hit (OBJECT *p)
{
	register int	hit_dist;
	register OBJECT	*w;
	int		t2, extent;
	long		t1, t3;
	VECT		r, d, V;
	LVECT		R;
	OBJECT		*owner;

	if (!(p->shflags & SH_HIT) &&
	    (st.flags & SF_BLANKER || !(p->flags & F_HIT)))
		return;

	if (p->flags & F_IMPORTED)		/* ignore foreign bullets */
		return;

	V[X] =  p->V[X]/VONE;
	V[Y] =  p->V[Y]/VONE;
	V[Z] =  p->V[Z]/VONE;

	R[X] =  p->R[X]/VONE;
	R[Y] =  p->R[Y]/VONE;
	R[Z] =  p->R[Z]/VONE;

	extent = SH(p)->extent;
	owner = get_owner (p);

	for (w = COT; w; w = w->prev) {
		if (!(w->shflags & SH_BEHIT))
		break;			/* all BEHIT's are at end! */
		if (w == p || w == owner)
			continue;
		if (w->flags & F_DEL)
			continue;
		if (TIMEOUT (w->tom) < 0)	/* immature? */
			continue;

		hit_dist = fmul (FONE/4*3, SH(w)->extent + extent);

		t1 = w->R[X]/VONE - R[X];
		t2 = TADJ(w->V[X]/VONE - V[X]);
		t3 = t1 - t2;
		if (t1 >  hit_dist && t3 >  hit_dist)
			continue;
		if (t1 < -hit_dist && t3 < -hit_dist)
			continue;
		r[X] = (int)t1;
		d[X] = t2;

		t1 = w->R[Y]/VONE - R[Y];
		t2 = TADJ(w->V[Y]/VONE - V[Y]);
		t3 = t1 - t2;
		if (t1 >  hit_dist && t3 >  hit_dist)
			continue; 
		if (t1 < -hit_dist && t3 < -hit_dist)
			continue;
		r[Y] = (int)t1;
		d[Y] = t2;

		t1 = w->R[Z]/VONE - R[Z];
		t2 = TADJ(w->V[Z]/VONE - V[Z]);
		t3 = t1 - t2;
		if (t1 >  hit_dist && t3 >  hit_dist)
			continue;
		if (t1 < -hit_dist && t3 < -hit_dist)
			continue;
		r[Z] = (int)t1;
		d[Z] = t2;

		if (hit (r, d, hit_dist)) {
			if (!(w->flags & F_HIT) && owner)
					++p->owner->score;
			p->flags |= F_HIT|F_MOD;
			p->color = ST_FIRE1;
			if (w->flags & F_IMPORTED)	/* imported victim */
				remote_urhit (w, p->speed, extent,
					p->damaging);
			else				/* local victim */
				object_hit (w, rand(), p->speed, extent,
					p->damaging);
		}
	}
}

/* objects handling.
*/

#define NM	(1852*VONE)

extern void FAR
object_update (OBJECT *p)
{
	int	tx, ty;

	p->R[X] += (tx = TADJ (p->V[X]));
	p->R[Y] += (ty = TADJ (p->V[Y]));
	p->R[Z] += TADJ (p->V[Z]);

	if (p->flags & F_KEEPNAV) {
		p->longmin += tx;
		tx = fmul (NM, COS(muldiv(p->latitude, D90, 90*60)));
		if (tx) {
			while (p->longmin >= tx) {
				p->longmin -= tx;
				if (++p->longitude >= 180*60)
					p->longitude -= 360*60;
			}
			while (p->longmin < 0) {
				p->longmin += tx;
				if (--p->longitude < -180*60)
					p->longitude += 360*60;
			}
		}

		p->latmin += ty;
		for (;;) {
			if (p->latmin >= NM) {
				p->latmin -= NM;
				if (++p->latitude >= 90*60) {
					p->latitude = 180*60 - p->latitude;
					p->latmin = -p->latmin;
				}
			} else if (p->latmin < 0) {
				p->latmin += NM;
				if (--p->latitude <= -90*60) {
					p->latitude = -180*60 - p->latitude;
					p->latmin = -p->latmin;
				}
			} else
				break;
		}
	}

	if (p->time != FOREVER) {
		p->time -= muldiv (st.interval, TIMEPSEC, 1000);
		if (p->time <= 0)
			p->flags |= F_DEL;	/* no notification */
	}
}

#undef NM

extern void FAR
object_dynamics (OBJECT *p)
{
	BODY	*b;
	int	save_interval;

	if (p->flags & F_DONE)
		return;
	p->flags |= F_DONE;

	st.owner = p;
	b = st.bodies[p->name];

	if (p->flags & F_IMPORTED) {
		remote_receive (p);
		Frandomize ();		/* the net syncs the machines */
		if (ET_IMPORTED == p->e_type &&
					TIMEOUT (EIM(p)->timeout) > 0) {
			p->flags |= F_DEL;		/* object timeout */
			++STATS_NETERRLOST;
		} else {
			save_interval = st.interval;
			st.interval = (int)TIMEOUT (p->rtime);
			if (st.interval) {
				if (st.interval > 1000)	/* ignore long pauses */
					st.interval = 1000;
				else if (st.interval < -1000)
					st.interval = -1000;
				if (b->shape->flags & SH_LOCALSIM)
					b->dynamics (p);
				else
					gen_dynamics (p);
				p->rtime = st.present;
			}
			st.interval = save_interval;
		}
	} else {
		b->dynamics (p);
		check_hit (p);
	}
}

extern int FAR
objects_dynamics (void)
{
	OBJECT	*p, *q;
	int	refreshed;

	remote_receive (0);
	Frandomize ();		/* the net syncs the machines */

	Tm->Interval (TMR_START, 0L);
	for (p = CO; p; p = p->next) {
		if (T(q = get_owner (p)))
			object_dynamics (q);
		object_dynamics (p);
	}
	STATS_TIMESIM += Tm->Interval (TMR_STOP, 10000L);

	remote_refresh ();
	players_flush ();	/* del bad messages */
	players_purge ();	/* del bad players */

/* If I am deleted then create a new local player. Copy some of the old
 * plane settings to the new.
*/
	if (CC->flags & F_DEL) {
		int	font;

		p = CC;
		st.options = st.ptype;
		font = font_set (-1);
		if (F(CC = create_object (p->name, 1))) {
			MsgEPrintf (-50, "no new CC");
			return (1);
		}
		font_set (font);
		MsgPrintf (50, "new CC");
		if (T(CC->pointer = p->pointer))
			(*CC->pointer->control->Center)(CC->pointer);

		if (IS_PLANE (CC))
			plane_xfer (p);
	}

	if (CV->flags & F_DEL) {
		if (T(p = get_owner (CV)) && !(p->flags & F_DEL))
			CV = p;
		else
			CV = CC;
		if (CV->viewport)
			get_viewport (CV);
	}

/* export, refresh and delete as needed.
*/
	refreshed = 0;
	st.ntargets = 0;
	for (p = CO; p;) {
		if (p->flags & F_EXPORTED) {
			if (refreshed < RefreshPerFrame &&
			    !(p->flags & (F_DEL|F_NEW|F_ALIVE))) {
				p->flags |= F_MOD|F_ALIVE;
				++refreshed;
			}
			send_obj (p, st.all_pports);
		}

		if (p->flags & F_DEL) {
			p = delete_object (p);
			continue;
		}

		if ((p->name == O_TARGET || p->name == O_GTARGET)
		    && !(p->flags & F_HIT))
			++st.ntargets;
		p->flags &= ~(F_NEW|F_MOD|F_DONE);
		p = p->next;
	}

/* Object refresh logic.
*/
#define REFRESH_MARGIN	(OBJECT_REFRESH/4)

	if (0 == refreshed) {	/* cycle just ended */
		if (TIMEOUT (st.Refresh_next) < -REFRESH_MARGIN) {

/* Finished too soon - reduce the rate
*/
			if (--RefreshPerFrame <= 0)
				++RefreshPerFrame;
		}
		if (TIMEOUT (st.Refresh_next) >= 0) {

/* Need to start new cycle, reset all objects
*/
			for (p = CO; p; p = p->next)
				if (p->flags & F_EXPORTED)
					p->flags &= ~F_ALIVE;
			st.Refresh_next = st.present + OBJECT_REFRESH;
		}
	} else if (TIMEOUT (st.Refresh_next) > REFRESH_MARGIN) {

/* Finished too late - increate the rate
*/
		++RefreshPerFrame;	/* missed one, so hurry up */
		st.Refresh_next = st.present + REFRESH_MARGIN;
	}
#undef REFRESH_MARGIN

	send_obj (0, 0);			/* flush netports */
	return (0);
}
