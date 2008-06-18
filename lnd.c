/* -------------------------------- lnd.c ----------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Read in the *.lnd file.
*/

#include "fly.h"


LOCAL_FUNC int NEAR	lnd_read1 (FILE *vxfile, char *line, int name);

static struct {
	short	name;
	Ushort	color;
	LVECT	R;
	AVECT	a;
} Tlnd;

#define READFLD		Tlnd

static struct FldTab FAR LndTbl[] = {
	READI (name),
	READI (color),
	READI (R[X]),
	READI (R[Y]),
	READI (R[Z]),
	READI (a[X]),
	READI (a[Y]),
	READI (a[Z]),
{0, 0}};

#undef READFLD

extern int FAR
lnd_read (void)
{
	int	i, n;
	int	nlocals;
	OBJECT	*p;
	FILE	*ifile;
	char	line[256];

	if (!st.lndname)
		st.lndname = STRdup ("fly");

	Sys->BuildFileName (st.filename, st.fdir, st.lndname, LND_EXT);
	if (F(ifile = fopen (st.filename, RTMODE))) {
		LogPrintf ("%s: missing land file\n", st.filename);
		return (-1);
	}
	LogPrintf ("Land     %s\n", st.filename);

	st.lineno = 0;
	nlocals = 0;

	for (n = 0;; ++n) {
		if (field_read (ifile, &LndTbl[0], line) < 0)
			break;
		if (O_EOF == Tlnd.name)
			break;
		else if (Tlnd.name & O_DEFINE) {
			Tlnd.name = O_LOCAL + (Tlnd.name & ~O_DEFINE);
			if (Tlnd.name < O_LOCAL || Tlnd.name >= O_ALL) {
				LogPrintf ("%s %ld: bad object name\n",
					st.filename, st.lineno);
				goto badret;
			}
			if (lnd_read1 (ifile, line, Tlnd.name))
				goto badret;
			++nlocals;
			continue;
		}
		if (Tlnd.name < 0 || Tlnd.name >= O_ALL ||
		    !st.bodies[Tlnd.name]) {
			LogPrintf ("%s %ld: bad object name\n",
				st.filename, st.lineno);
			goto badret;
		}
		for (i = 1; LndTbl[i].type > 0; ++i) {
			if (field_read (ifile, &LndTbl[i], line) < 0)
				goto badret;
		}
		Tlnd.a[Z] = -Tlnd.a[Z];
		p = create_land (Tlnd.name);
		if (!p) {
			LogPrintf ("%s %ld: create land failed\n",
				st.filename, st.lineno);
			goto badret;
		}
		if ((Ushort)CC_DEFAULT != Tlnd.color)
			p->color = Tlnd.color;
		LVcopy (p->R, Tlnd.R);
		LVcopy (p->a, Tlnd.a);
		Mobj (p);
	}
	LogPrintf ("      created %d objects\n", n);
	LogPrintf ("      defined %d locals\n", nlocals);

	fclose (ifile);
	return (0);
badret:
	fclose (ifile);
	return (-1);
}

LOCAL_FUNC xshort NEAR
lnd_getXYZ (long l, int fine)
{
	if (fine & V_METERS)
		l /= VONE;
	if (l > VMAX || l < -VMAX)
		LogPrintf ("%s %ld: coordinate truncated\n",
			st.filename, st.lineno);
	return ((xshort)l);
}

LOCAL_FUNC int NEAR
lnd_read1 (FILE *vxfile, char *line, int name)
{
	int	i, n, fine;
	long	l;
	VERTEX	*vx;
	BODY	*b;

	vx = 0;
	b = 0;

	if (st.bodies[Tlnd.name]) {
		LogPrintf ("%s %ld: object already defined\n",
			st.filename, st.lineno);
		goto badret;
	}

	if (field_long (vxfile, line, &l))
		goto badret;
	if (l < 1 || l > 2) {
		LogPrintf ("%s %ld: bad detail level\n",
			st.filename, st.lineno);
		goto badret;
	}
	fine = (int)l;

	if (field_long (vxfile, line, &l))
		goto badret;
	if (l <= 0) {
		LogPrintf ("%s %ld: bad shape size\n",
			st.filename, st.lineno);
		goto badret;
	}
	n = (int)l;

	vx = (VERTEX *)memory_calloc (sizeof (*vx), n+1);
	if (!vx) {
		LogPrintf ("%s %ld: no shape memory\n",
			st.filename, st.lineno);
		goto badret;
	}

	for (i = 0; i < n; ++i) {
		if (field_long (vxfile, line, &l))
			goto badret;
		vx[i].V[X] = lnd_getXYZ (l, fine);
		if (field_long (vxfile, line, &l))
			goto badret;
		vx[i].V[Y] = lnd_getXYZ (l, fine);
		if (field_long (vxfile, line, &l))
			goto badret;
		vx[i].V[Z] = lnd_getXYZ (l, fine);
		if (field_long (vxfile, line, &l))
			goto badret;
		vx[i].flags = (Ushort)l;
	}

	vx[i].flags = V_EOF;	/* end of list */

	if (F(b = bodies_new (Tlnd.name))) {
		LogPrintf ("%s %ld: no body\n",
			st.filename, st.lineno);
		goto badret;
	}

	if (F(NEW (b->shape))) {
		LogPrintf ("%s %ld: no shape\n",
			st.filename, st.lineno);
		goto badret;
	}
	b->flags |= BO_DYNSHAPE;
	b->shape->flags |= SH_DYNVERTEX;

	b->shape->v = vx;
	if (fine & V_FINE)
		b->shape->flags |= SH_FINE;

	b->title = "LOCAL";
	b->shape->weight = 1L;
	b->shape->drag = 0;

	b->init = gen_init;
	b->term = gen_term;
	b->create = gen_create;
	b->delete = gen_delete;
	b->dynamics = gen_nodynamics;
	b->hit = gen_nohit;

	bodies_extent (b->name);

	LogPrintf ("      local %d size %d\n", Tlnd.name-O_LOCAL, n);

	return (0);
badret:
	if (b) {
		if (b->shape)
			DEL0 (b->shape);
		bodies_del (b->name);
	}
	shape_free (vx);
	return (-1);
}
