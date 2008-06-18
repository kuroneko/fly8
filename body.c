/* -------------------------------- body.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handle body descriptions.
*/

#include "fly.h"


LOCAL_FUNC int NEAR
init_internals (void)
{
	st.bodies[O_GROUND]  = &BoGround;
	st.bodies[O_BOX]     = &BoBox;
	st.bodies[O_PLANE]   = &BoPlane;
	st.bodies[O_RUNWAY]  = &BoRunway;
	st.bodies[O_M61]     = &BoM61;
	st.bodies[O_TARGET]  = &BoTarget;
	st.bodies[O_BROKEN]  = &BoBroken;
	st.bodies[O_VIEWER]  = &BoViewer;
	st.bodies[O_CHUTE]   = &BoChute;
	st.bodies[O_HOUSE]   = &BoHouse;
	st.bodies[O_TOWER]   = &BoTower;
	st.bodies[O_LOW]     = &BoLow;
	st.bodies[O_GTARGET] = &BoGtarget;
	st.bodies[O_MK82]    = &BoMK82;
	st.bodies[O_CRATER]  = &BoCrater;
	st.bodies[O_SMOKE]   = &BoSmoke;
	st.bodies[O_CAR]     = &BoCar;

	return (0);
}

extern BODY * FAR
bodies_new (ONAME n)
{
	BODY	*b;

	if (n >= 0) {
		if (n < 0 || n >= O_ALL || st.bodies[n])
			return (0);
	} else {
		for (n = O_LOCAL; st.bodies[n]; ++n)
			if (n >= O_ALL)
				return (0);
	}

	if (F(NEW (b)))
		return (0);

	st.bodies[n] = b;
	b->name = n;

	return (b);
}

extern void FAR
bodies_extent (ONAME name)
{
	Uint	extent;
	VERTEX	*v;

	extent = 0;
	for (v = st.bodies[name]->shape->v; v->flags; ++v) {
		if (extent < iabs(v->V[X]))
			extent = iabs (v->V[X]);
		if (extent < iabs(v->V[Y]))
			extent = iabs (v->V[Y]);
		if (extent < iabs(v->V[Z]))
			extent = iabs (v->V[Z]);
	}
	if (st.bodies[name]->shape->flags & SH_FINE)
		extent /= VONE;
	if (0 == extent)
		extent = 1;
	st.bodies[name]->shape->extent = (Uxshort)extent;
}

extern int FAR
bodies_init (void)
{
	ONAME	i;

	if (F(st.bodies = (BODY **)memory_calloc (O_ALL+1, sizeof (BODY *))))
		return (1);

	if (init_internals ())
		return (1);

	for (i = 0; st.bodies[i]; ++i) {
		st.bodies[i]->name = i;
		if (st.bodies[i]->init && st.bodies[i]->init(st.bodies[i]))
			return (1);
		bodies_extent (i);
	}
	return (0);
}

extern BODY * FAR
bodies_del (ONAME name)
{
	if (name < 0 || name >= O_ALL || !st.bodies || !st.bodies[name])
		return (0);

	if (st.bodies[name]->term)
		st.bodies[name]->term (st.bodies[name]);

	if (name >= O_INT)
		DEL0 (st.bodies[name]);
	else
		st.bodies[name] = 0;
	return (0);
}

extern void FAR
bodies_term (void)
{
	ONAME	name;

	if (!st.bodies)
		return;

	for (name = 0; name < O_ALL; ++name) {
		if (!st.bodies[name])
			continue;
		bodies_del (name);
	}

	st.bodies = memory_cfree (st.bodies, O_ALL+1, sizeof (*st.bodies));
}

extern ONAME FAR
body_name (char *title)
{
	int	i;

	if (!st.bodies)
		return (-1);

	for (i = 0; i < O_ALL; ++i) {
		if (st.bodies[i] && !stricmp (st.bodies[i]->title, title))
			return (i);
	}

	return (-1);
}
