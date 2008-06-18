/* --------------------------------- oclassic.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Dynamics of the Classic.
*/

#include "plane.h"


extern void FAR
dynamics_classic (OBJECT *p, int action)
{
	int	t;
	VECT	OLDV, A;
	AVECT da;

	if (action)
		return;

	if (dynamics_input (p))
		return;

#define ADELTA	(VD90/45*EP->opt[3])
	p->da[Y] = -muldiv (2*EX->ailerons, ADELTA*10, 100);
	p->da[X] = muldiv (EX->elevators, ADELTA*10/2, 100);
#undef ADELTA

	if (100 == EX->throttle) {
		t = EP->ab_thrust - EP->mil_thrust;
		t = EP->mil_thrust + muldiv (EX->afterburner, t, 100);
	} else {
		t = muldiv (EX->throttle, EP->mil_thrust, 100);
		if (t < 0)
			t /= 2;		/* reverse thrust is 50% efficient */
	}

	DAMPEN (EX->thrust, t, 8);
	EX->power = muldiv (EX->thrust, 10000, EP->ab_thrust);

	t = (EX->afterburner) ? EP->ab_sfc : EP->mil_sfc;
	EX->fuelRate = muldiv (iabs(EX->thrust), t, 60*60/10);
	if ((EX->fuel -= TADJ (EX->fuelRate)) < 0) {
		EX->fuel = 0;
		EX->thrust = 0;
	}

	DAMPEN (p->speed, EX->thrust/4*10, 16);

	t = muldiv (EP->MaxRudder, EX->rudder, 100);
	p->da[Z] = muldiv (p->speed, t, VMAX);

	da[X] = TADJ (p->da[X]) * VONE;
	da[Y] = TADJ (p->da[Y]) * VONE;
	da[Z] = TADJ (p->da[Z]) * VONE;
	Myxz (p->T, da);

	fMroty (p->T, p->siny, p->cosy);
	fMrotx (p->T, p->sinx, p->cosx);
	fMrotz (p->T, p->sinz, p->cosz);
	Mangles (p, p->T, p->a, da[Y]);

	Vcopy (OLDV, p->V);
	p->vb[Y] = p->speed;
	Vscale (p->V, p->T[Y], p->speed);	/* faster than VMmul() */
	Vsub (A, p->V, OLDV);
	t = ihypot3d (A);
	if (t >= (VMAX/1000)*st.interval)
		EX->Gforce = VMAX;
	else {
		EX->Gforce = muldiv (t, 1000, st.interval)
			   + fmul (GACC, p->T[Z][Z]);	/* pilot's gravity */
	}

	if ((EX->flags & PF_ONGROUND) && p->R[Z])
		EX->flags &= ~PF_ONGROUND;

/* Mach number.
*/
	EX->mach = muldiv (p->speed, 1000, 340*VONE);

/* pull up warning time.
*/
	EX->misc[8] = 2000;
}
