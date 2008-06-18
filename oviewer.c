/* --------------------------------- oviewer.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* The special 'viewer' object is used in the external view modes.
*/

#include "plane.h"


static SHAPE shape_vwr = {
	0,
	0,
	0,
	1L,		/* weight */
	0		/* drag */
};

LOCAL_FUNC int FAR
create_vwr (OBJECT *p)
{
	if (!CV)
		return (1);

	p->color = CC_WHITE;
	p->time = FOREVER;
	p->owner = CV;
	p->ownerid = CV->id;

	Vcopy (p->a, CV->a);
	Mcopy (p->T, CV->T);
	LVcopy (p->R, CV->R);
	return (0);
}

LOCAL_FUNC void FAR
dynamics_vwr (OBJECT *p)
{
	OBJECT		*owner, *target;
	int		mode, f;
	long		t;
	ANGLE		a;
	VECT		V, V1;
	AVECT		da;
	MAT		M;
	LVECT		*R;
	VIEWPORT	*ovp;

	if (CV != (owner = get_owner (p))) {
		owner = CV;
		p->owner = CV;
		p->ownerid = CV->id;
		Vcopy (p->a, CV->a);
		Mcopy (p->T, CV->T);
		LVcopy (p->R, CV->R);
	}
	if (F(ovp = owner->viewport))
		ovp = CP;

	p->viewport->flags &= ~VF_MIRROR;

	mode = p->misc[0];
	switch (mode) {
	case HDT_MAP:					/* map */
	case HDT_RADAR:					/* new map */
		p->R[X] = owner->R[X];
		p->R[Y] = owner->R[Y];
		p->R[Z] = 48000L*VONE;
		p->a[X] = -D90;
		p->a[Y] = 0;
		if (HDT_RADAR == mode)
			p->a[Z] = owner->a[Z];
		else
			p->a[Z] = 0;

		Mobj (p);
		break;
	case HDT_GAZE:
		f = COS(CP->rotx);
		f = fmul (f, 50*VONE);
		t = owner->R[X] - fmul (f, SIN(CP->rotz));
		p->R[X] += (t - p->R[X])/1;

		t = owner->R[Y] - fmul (f, COS(CP->rotz));
		p->R[Y] += (t - p->R[Y])/1;

		f = SIN(CP->rotx);
		f = fmul (f, 50*VONE);
		t = owner->R[Z] + f;
		p->R[Z] += (t - p->R[Z])/1;

		p->a[X] = 0;
		p->a[Y] = 0;
		p->a[Z] = 0;

		Mident (p->T);
		p->sinx = FCON(0.0);
		p->cosx = FCON(1.0);
		p->siny = FCON(0.0);
		p->cosy = FCON(1.0);
		p->sinz = FCON(0.0);
		p->cosz = FCON(1.0);
		break;
	case HDT_CHASE:					/* chase orig */
	case HDT_FOLLOW:				/* chase orig, Y=0 */
		f = 50*VONE + muldiv (owner->speed, 500*VONE, VMAX);
		if (owner->speed)
			Vmuldiv (V, owner->V, f, owner->speed);
		else
			Vscale (V, owner->T[Y], f);
		Vsub (p->R, owner->R, V);

		p->a[X] = TANG(p->a[X] + TANG(owner->a[X] - p->a[X])/4);
		if (HDT_CHASE == mode)
			p->a[Y] = TANG(p->a[Y] + TANG(owner->a[Y] - p->a[Y])/8);
		else
			p->a[Y] = 0;
		p->a[Z] = TANG(p->a[Z] + TANG(owner->a[Z] - p->a[Z])/4);
		Mobj (p);
		break;
	case HDT_TARGET:			/* focus on target */
	case HDT_PAN:				/* panning target */
		LVcopy (p->R, owner->R);
		Vcopy (p->V, owner->V);
		p->da[X] = p->da[Y] = p->da[Z] = 0;
		f = IS_PLANE (owner);
		if (f && T(target = EE(owner)->target) &&
						target->id == EE(owner)->tid)
			R = &target->R;
		else if (f && (EE(owner)->hud2 & HUD_ILS))
			R = &ils[EE(owner)->ils-1].R;
		else {
#if 0
			Vcopy (p->a, owner->a);
			Mcopy (p->T, owner->T);
			break;
#else
			if (owner->speed) {
				V[X] = fdiv (owner->V[X], owner->speed);
				V[Y] = fdiv (owner->V[Y], owner->speed);
				V[Z] = fdiv (owner->V[Z], owner->speed);
				goto lll1;
			} else {
				Vcopy (p->a, owner->a);
				Mcopy (p->T, owner->T);
				break;
			}
#endif
		}
		V[X] = (int)(((*R)[X] - owner->R[X])/VONE);
		V[Y] = (int)(((*R)[Y] - owner->R[Y])/VONE);
		V[Z] = (int)(((*R)[Z] - owner->R[Z])/VONE);
lll1:
		if (HDT_PAN == mode) {
#if 1
			VxMmul (V1, V, p->T);

			f = TADJ (VD180)*VONE;
			a = ATAN (V1[Z], V1[Y]);
			if (a > f)
				da[X] = f;
			else if (a < -f)
				da[X] = -f;
			else
				da[X] = a;
			a = -ATAN (V1[X], V1[Y]);
			if (a > f)
				da[Z] = f;
			else if (a < -f)
				da[Z] = -f;
			else
				da[Z] = a;
			da[Y] = 0;

			Myxz (M, da);
			Mmul (M, p->T);
			Mangles (0, M, p->a, da[Y]);
			p->a[Y] = 0;
#endif
#if 0
			DAMPEN (p->a[Z], -ATAN (V[X], V[Y]), 4);
			f = ihypot2d (V[X], V[Y]);
			DAMPEN (p->a[X], ATAN (V[Z], f), 4);
			p->a[Y] = 0;
#endif
#if 0
			i = p->a[Z];
			DAMPEN (i, -ATAN (V[X], V[Y]), 4);
			p->a[Z] = i;

			f = ihypot2d (V[X], V[Y]);
			i = p->a[X];
			DAMPEN (i, ATAN (V[Z], f), 4);
			p->a[X] = i;

			if (iabs(p->a[X]) > D90) {
				p->a[X] = D180 - p->a[X];
				p->a[Z] = D180 + p->a[Z];
			}
			p->a[Y] = 0;
#endif
#if 0
			da[X] = TADJ(owner->da[X])*VONE;
			da[Y] = TADJ(owner->da[Y])*VONE;
			da[Z] = TADJ(owner->da[Z])*VONE;
			Myxz (M, da);
			Mmul (M, p->T);
			Mangles (0, M, p->a, da[Y]);

			i = p->a[Z];
			DAMPEN (i, -ATAN (V[X], V[Y]), 4);
			p->a[Z] = i;
			f = ihypot2d (V[X], V[Y]);
			i = p->a[X];
			DAMPEN (i, ATAN (V[Z], f), 4);
			p->a[X] = i;
			if (iabs(p->a[X]) > D90) {
				p->a[X] = TANG(D180 - p->a[X]);
				p->a[Y] = TANG(D180 + p->a[Y]);
			}
			i = p->a[Y];
			DAMPEN (i, 0, 4);
			p->a[Y] = i;
#endif
		} else {
			p->a[Z] = -ATAN (V[X], V[Y]);
			f = ihypot2d (V[X], V[Y]);
			p->a[X] = ATAN (V[Z], f);
			p->a[Y] = 0;
		}
		Mobj (p);
		break;
	case HDT_MIRROR:
		p->viewport->flags |= VF_MIRROR;
	case HDT_REAR:
#if 0
		Vcopy (p->a, owner->a);
		Mcopy (p->T, owner->T);
		LVcopy (p->R, owner->R);
		p->viewport->rotz = D180;
#else
		LVcopy (p->R, owner->R);
		p->a[X] = -owner->a[X];
		p->a[Y] = -owner->a[Y];
		p->a[Z] = TANG(D180 + owner->a[Z]);
		Mobj (p);
#endif
		break;
	case HDT_RIGHT:
	case HDT_LEFT:
		if (p->viewport->zoom != ovp->zoom)
			zoom (p->viewport, ovp->zoom-p->viewport->zoom);
		a = ATAN (ovp->maxx, ovp->z);
		if (HDT_RIGHT == mode)
			p->viewport->rotz = ovp->rotz + 2*a;
		else
			p->viewport->rotz = ovp->rotz - 2*a;
		goto def_case;
	case HDT_FRONT:
	default:
def_case:
		Vcopy (p->a, owner->a);
		p->sinx = owner->sinx;
		p->cosx = owner->cosx;
		p->siny = owner->siny;
		p->cosy = owner->cosy;
		p->sinz = owner->sinz;
		p->cosz = owner->cosz;
		Mcopy (p->T, owner->T);
		LVcopy (p->R, owner->R);
		break;
	}
/* No need to update.
 *
 * object_update (p);
*/
}

BODY FAR BoViewer = {
	0,
	0,
	"VIEWER",
	&shape_vwr,
	gen_read,
	gen_term,
	create_vwr,
	gen_delete,
	dynamics_vwr,
	gen_hit
};
