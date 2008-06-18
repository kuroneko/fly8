/* --------------------------------- object.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* maintain object lists.
*/

#include "plane.h"


LOCAL_FUNC OBJECT * NEAR
list_addobj (OBJECT *list[], ONAME name, int init, int attail)
{
	OBJECT	*obj;

	if (F(NEW (obj)))
		return (0);
	obj->name = name;
	obj->id = ++st.object_id;
	obj->flags |= F_NEW;
	obj->shflags = SH(obj)->flags;
	obj->rtime = st.present;			/* default time */
	obj->tob = st.present;
	obj->tom = st.present;
	obj->maint_rate = st.maint_rate;
	obj->maint_last = st.present;
	if (init) {
		if ((st.bodies[name]->create)(obj)) {
			DEL0 (obj);
			return (0);
		}
	}

	if (attail || (obj->shflags & SH_BEHIT)) {	/* targets at end */
		obj->next = 0;				/* f'ward chain */
		obj->prev = list[TAIL];			/* b'ward chain */
		if (list[TAIL])				/* if have list */
			list[TAIL]->next = obj;		/* then link to tail */
		else
			list[HEAD] = obj;		/* else set head */
		list[TAIL] = obj;			/* now new tail */
	} else {
		obj->next = list[HEAD];			/* f'ward chain */
		obj->prev = 0;				/* b'ward chain */
		if (list[HEAD])				/* if have list */
			list[HEAD]->prev = obj;		/* then link to head */
		else
			list[TAIL] = obj;		/* else set tail */
		list[HEAD] = obj;			/* now new head */
	}

	++st.nobjects;
	return (obj);
}

extern OBJECT * FAR
create_object (ONAME name, int init)
{
	return (list_addobj (&CO, name, init, 0));
}

extern OBJECT * FAR
create_land (ONAME name)
{
	return (list_addobj (&CL, name, 1, 1));
}

extern OBJECT * FAR
get_owner (OBJECT *object)
{
	OBJECT		*owner;

/* The owner may have been deleted, so we try to verify it. This may
 * not be perfect but hopefully good for practical purposes.
 * Cases where the owner is gone: chute, bullets etc. where the owner
 * plane is gone. A chute can stick around for a long while...
*/
	if (NULL == (owner = object->owner) ||
	    owner->id != object->ownerid)
		return (NULL);

	if (owner->name < 0 || owner->name > O_ALL ||
	    owner->e_type < 0 || owner->e_type > ET_MAX ||
	    owner->color >= NCOLORS) {
		++STATS_FIX1;
		return (NULL);
	}

	return (owner);
}

LOCAL_FUNC OBJECT * NEAR
list_delobj (OBJECT *list[], OBJECT *object)
{
	if (!object)
		return (0);

	(st.bodies[object->name]->delete) (object);

	if (object->pointer)
		object->pointer = pointer_release (object->pointer);

	if (object->viewport)
		DEL0 (object->viewport);

	switch (object->e_type) {
	case ET_PLANE:
		DEL0 (EE(object));
		break;
	case ET_IMPORTED:
		DEL0 (EIM(object));
		break;
	case ET_BOMB:
		DEL0 (EBM(object));
		break;
	case 0:
		break;
	default:		/* should not happen! */
		LogPrintf ("list_delobj: unknown e_type=%d\n",
			(int)object->e_type);
		die ();
		break;
	}
	object->e_type = 0;

	if (object->next)		/* not last, fix b'ward chain */
		object->next->prev = object->prev;
	else				/* last, new tail */
		list[TAIL] = object->prev;
	if (object->prev)		/* not first, fix f'ward chain */
		object->prev->next = object->next;
	else				/* first, new head */
		list[HEAD] = object->next;

/* This is to make get_owner() safe
*/
	object->id = 0;
	object->name = -1;
	object->e_type = -1;
	object->color = NCOLORS;
	
	--st.nobjects;
	return (DEL (object));
}

extern OBJECT * FAR
delete_object (OBJECT *object)
{
	return (list_delobj (&CO, object));
}

extern OBJECT * FAR
delete_land (OBJECT *object)
{
	return (list_delobj (&CL, object));
}

extern void FAR
list_clear (OBJECT *list[])
{
	OBJECT	*p;

	if (list[HEAD]) {
		for (p = list[HEAD]; p;)
			p = delete_object (p);
		list[HEAD] = list[TAIL] = 0;
	}
}

extern void * FAR
shape_free (VERTEX *vx)
{
	int	n;

	if (vx) {
		for (n = 0; vx[n].flags; ++n)
			;
		memory_cfree (vx, sizeof (*vx), n+1);
	}
	return (NULL);
}

LOCAL_FUNC int NEAR
shape_line (char *VxFileName, FILE *vxfile, char *line, int linesize, long *l)
{
	do {
		fgets (line, linesize, vxfile);
		if (ferror (vxfile)) {
			LogPrintf ("%s %ld: shape read error\n",
				VxFileName, *l);
			return (1);
		}
		++*l;
	} while ('\n' == line[0] || '#' == line[0]);

	return (0);
}

extern int FAR
shape_read (SHAPE *shape, char *VxFileName)
{
	int	i, n, fine, resp;
	long	l;
	FILE	*vxfile;
	VERTEX	*vx;
	int	V[4];
	char	line[256], *p;

	for (p = line; *VxFileName; ++p, ++VxFileName)
		*p = (char)tolower (*VxFileName);
	*p = '\0';
	
	Sys->BuildFileName (st.filename, st.fdir, line, SHP_EXT);
	vxfile = fopen (st.filename, RTMODE);
	if (!vxfile) {
		LogPrintf ("missing shape file: %s\n", st.filename);
		return (1);
	}

	l = 0;

	if (shape_line (st.filename, vxfile, line, sizeof (line), &l)) {
		fclose (vxfile);
		return (0);
	}

	resp = sscanf (line, "%d %d", &n, &fine);
	if (2 != resp || n <= 0 || fine < 1 || fine > 2) {
		LogPrintf ("%s %ld: bad shape size/detail\n", st.filename, l);
		fclose (vxfile);
		return (1);
	}

	vx = (VERTEX *)memory_alloc ((n+1)*sizeof (*vx));
	if (!vx) {
		LogPrintf ("%s %ld: no shape memory: %s\n", st.filename, l);
		fclose (vxfile);
		return (1);
	}

	for (i = 0; i < n; ++i) {
		if (shape_line (st.filename, vxfile, line, sizeof (line), &l)) {
			shape_free (vx);
			fclose (vxfile);
			return (1);
		}
		resp = sscanf (line, "%d %d %d %d",
			&V[X], &V[Y], &V[Z], &V[3]);
		if (4 != resp) {
			LogPrintf ("bad shape line %d: %s\n", l, st.filename);
			shape_free (vx);
			fclose (vxfile);
			return (1);
		}
		vx[i].V[X] = V[X];
		vx[i].V[Y] = V[Y];
		vx[i].V[Z] = V[Z];
		vx[i].flags = V[3];
	}
	fclose (vxfile);

	vx[i].flags = 0;	/* end of list */
	shape->v = vx;
	shape->flags |= SH_DYNVERTEX;
	if (fine & V_FINE)
		shape->flags |= SH_FINE;

	LogPrintf ("Shape    %s size %d\n", st.filename, n);

	return (0);
}
