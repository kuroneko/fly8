/* --------------------------------- prm.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Manage parameter files (*.prm).
*/

#include "plane.h"


void (FAR* FAR flight_models[])(OBJECT *p, int action) = {
	dynamics_basic,
	dynamics_classic,
	dynamics_xplane,
	dynamics_yplane,
	dynamics_fplane
};

static struct e_parms	*parms = 0;
static struct e_parms	Tparm = {0};

#define READFLD		Tparm

#define READPID(name) \
	READI (name.Kp),		\
	READI (name.Iband),		\
	READI (name.Ki),		\
	READI (name.Dband),		\
	READI (name.Kd),		\
	READI (name.factor),		\
	READI (name.range)

#define READGEAR(n) \
	READI (gear[n].x),		\
	READI (gear[n].y),		\
	READI (gear[n].z),		\
	READI (gear[n].dgmax),		\
	READI (gear[n].dtp),		\
	READI (gear[n].P),		\
	READI (gear[n].Cv),		\
	READI (gear[n].emax),		\
	READI (gear[n].ur),		\
	READI (gear[n].ub),		\
	READI (gear[n].us),		\
	READI (gear[n].rateup),		\
	READI (gear[n].ratedn)

static struct FldTab FAR ParmTbl[] = {
	READI (weight),
	READI (Ixx),
	READI (Iyy),
	READI (Izz),
	READI (Izx),

	READI (liftoff_speed),
	READI (max_lift),
	READI (min_lift),
	READI (pitch_rate),
	READI (roll_rate),
	READI (eyez),
	READI (eyey),
	READI (gpitch),
	READI (ceiling),
	READI (stores[0]),
	READI (stores[1]),
	READI (stores[2]),
	READI (stores[3]),
	READI (stores[4]),

	READI (brake_mu),   	/* friction: brakes applied */
	READI (wheel_mu),   	/* friction: freewheeling */

	READI  (mil_thrust),
	READI (mil_sfc),
	READI (ab_thrust),
	READI (ab_sfc),
	READI (fuel_capacity),
	READI (Ea),			/* Engine rig ang vs. plane */
	READI (Eb),			/* Engine off ang vs. plane */
	READI (Er),			/* Engine off vs. cg */

	READI (wing_area),
	READI (wing_span),
	READI (wing_cord),
	READI (ACy),		/* wing ac forward of cg */
	READI (ACz),		/* wing ac upward of cg */
	READI (Aoffset),    	/* Wing rigging ang vs. plane */
	READI (Cl0),		/* alpha where Cl=0 */
	READI (maxCl),		/* max Cl for wing foil */
	READI (minCl),		/* min Cl for wing foil */
	READI (FEff),       	/* deg eff alpha/degs flaps */
	READI (FEffCl),		/* flaps Clmax rate/DegFlaps */
	READI (lefEffCl),       	/* Clmax/LE flaps deg */
	READI (efficiency_factor),
	READI (Cm0w),

	READI (tail_area),
	READI (tail_span),
	READI (TACy),		/* tail ac forward of cg */
	READI (TACz),		/* tail ac upward of cg */
	READI (Toffset),    	/* Tail rigging ang vs. wing */
	READI (Tvol),       	/* tail volume */
	READI (TmaxCl),
	READI (TminCl),

	READI (rudd_area),
	READI (rudd_span),
	READI (RACy),		/* rudder ac forward of cg */
	READI (RACz),		/* rudder ac upward of cg */
	READI (RmaxCl),

	READI (Cdp0),		/* parasitic: profile */
	READI (Cds),		/* parasitic: speed brakes */
	READI (Cdg),		/* parasitic: gear */
	READI (CdMK82),		/* parasitic: each MK82 */

	READI (MaxFlaps),		/* max flaps [ang] */
	READI (MaxLEFlaps),		/* max LE flaps [ang] */
	READI (MaxSpoilers),	/* max spoilers [ang] */
	READI (MaxElevators),	/* max elevators [ang] */
	READI (MaxAilerons),	/* max ailerons [ang] */
	READI (MaxRudder),		/* max rudder [ang] */

	READI (AFamin),		/* min aoa to engage */
	READI (AFrate),		/* flaps/aoa rate */
	READI (AFmax),		/* max flaps authority */

	READI (ALEFamin),		/* leFlaps: min aoa to engage */
	READI (ALEFrate),		/* leFlaps/aoa rate */

	READI (AErate),		/* speed rate */

	READI (APrate),		/* speed rate */

	READI (Cydr),		/* rudder sideforce */
	READI (Cybeta),		/* vx damping */

	READI (Cm0),
	READI (Cmde),		/* elevators effectiveness */
	READI (Cmq),		/* pitch damping */
	READI (Cmalpha),		/* stabilizer induced pitch */
	READI (Cmalphadot),		/* alpha rate induced pitch */

	READI (Clda),		/* aileron effectiveness */
	READI (Clp),		/* roll damping */
	READI (Clbeta),		/* dihedral effect */
	READI (Cldr),		/* roll from rudder */

	READI (Cndr),		/* rudder effectiveness */
	READI (Cnr),		/* yaw damping */
	READI (Cnbeta),		/* weathercock stability */
	READI (Cnda),		/* ailerons induced yaw */
	READI (Cnp),		/* roll induced yaw */

	READI (hudtype),

	READI (opt[0]),		/* options */
	READI (opt[1]),		/* options */
	READI (opt[2]),		/* options */
	READI (opt[3]),		/* options */
	READI (opt[4]),		/* options */
	READI (opt[5]),		/* options */
	READI (opt[6]),		/* options */
	READI (opt[7]),		/* options */
	READI (opt[8]),		/* options */
	READI (opt[9]),		/* options */

	READPID (PIDthrottle),
	READPID (PIDpitch),
	READPID (PIDroll),

	READGEAR (0),
	READGEAR (1),
	READGEAR (2),
	READGEAR (3),
	READGEAR (4),
{0, 0}};

#undef READFLD
#undef READPID
#undef READGEAR

extern struct e_parms * FAR
parms_get (char *pname)
{
	struct e_parms	*parm;
	int	i, l, t;
	FILE	*pfile;
	char	line[256];

	if (!pname)
		return (parms);		/* use default */

	for (parm = parms; parm; parm = parm->next)
		if (!stricmp (parm->name, pname))
			return (parm);

	Sys->BuildFileName (st.filename, st.fdir, pname, PRM_EXT);
	if (F(pfile = fopen (st.filename, RTMODE))) {
		LogPrintf ("missing parms file: %s\n", st.filename);
		return (0);
	}

	st.lineno = 0;
	for (i = 0; ParmTbl[i].type > 0; ++i) {
		if (field_read (pfile, &ParmTbl[i], line) < 0)
			goto badret;
	}
	fclose (pfile);

	if (F(NEW (parm))) {
		LogPrintf ("no memory for parms: %s\n", st.filename);
		return (0);
	}

	strncpy (Tparm.name, pname, sizeof (Tparm.name));
	memcpy (parm, &Tparm, sizeof (Tparm));

	if (parms) {
		parm->next = parms->next;	/* keep first as default */
		parms->next = parm;
	} else {
		parm->next = 0;
		parms = parm;
	}

	if ((sizeof(flight_models)/sizeof(*flight_models)) < parm->opt[0])
		parm->opt[0] = MODEL_BASIC;

	if (0 == parm->opt[1])
		parm->opt[1] = 2;		/* response=stiff */

	if (0 == parm->opt[3]) {
		if (MODEL_CLASSIC == parm->opt[0])
			parm->opt[3] = 4;	/* turn rate=average */
		else if (MODEL_BASIC == parm->opt[0])
			parm->opt[3] = 8;	/* lift rate=standard */
	}

/* Find a reasonable initial height for a parked plane. We place the gear
 * at half depression and look for the tallest one.
*/
	if (parm->opt[5] < 0) {
		l = 0;
		for (i = 0; i < rangeof(parm->gear) && parm->gear[i].z; ++i) {
			t = -parm->gear[i].z - parm->gear[i].dgmax/2;
			if (l < t)
				l = t;
		}
		parm->opt[5] = (short)(l/VONE);
	}

	LogPrintf ("Plane    %s\n", st.filename);
	LogPrintf ("         model %d\n", parm->opt[0]);

	return (parm);

badret:
	fclose (pfile);
	return (0);
}

extern void FAR
parms_free (void)
{
	struct e_parms	*p, *pp;

	for (p = parms; p; p = pp) {
		pp = p->next;
		DEL (p);
	}
	parms = 0;
}
