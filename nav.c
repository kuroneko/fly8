/* --------------------------------- nav.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* handle nav data.
*/

#include "plane.h"


static int		maxnav = 0;
struct ils		FAR *ils = 0;
static struct ils	Tnav = {0};

#define READFLD		Tnav

static struct FldTab FAR NavTbl[] = {
	READS (name),
	READI (R[X]),
	READI (R[Y]),
	READI (R[Z]),
	READI (longitude),
	READI (latitude),
	READI (l[X]),
	READI (l[Y]),
	READI (g[X]),
	READI (g[Y]),
	READI (localizer),
	READI (glidepath),
{0, 0}};

#undef READFLD

LOCAL_FUNC void NEAR
ils_free (void)
{
	int	n;

	if (ils) {
		for (n = 0; n < maxnav; ++n)
			ils[n].name = STRfree (ils[n].name);
		ils = memory_cfree (ils, sizeof (*ils), maxnav + 1);
	}
}

extern int FAR
nav_init (void)
{
	int	i;
	int	n;
	long	t;
	FILE	*ifile;
	char	line[256];

	if (!st.navname)
		st.navname = STRdup ("fly");

	Sys->BuildFileName (st.filename, st.fdir, st.navname, NAV_EXT);
	if (F(ifile = fopen (st.filename, RTMODE))) {
		LogPrintf ("missing nav file: %s\n", st.filename);
		return (-1);
	}
	LogPrintf ("nav      %s\n", st.filename);

	st.lineno = 0;

	if (field_find (ifile, line))
		goto badret;
	if (1 != sscanf (line, "%ld", &t) || t < 1) {
		LogPrintf ("%s %ld: bad nav csize\n",
			st.filename, st.lineno);
		goto badret;
	}
	maxnav = (int)t;
	LogPrintf ("      %d points\n", maxnav);

	if (F(ils = (struct ils *)memory_calloc (sizeof (*ils), maxnav + 1))) {
		LogPrintf ("%s %ld: no nav mem\n",
			st.filename, st.lineno);
		goto badret;
	}

	for (n = 0; n < maxnav; ++n) {
	    for (i = 0; NavTbl[i].type > 0; ++i) {
		if (field_read (ifile, &NavTbl[i], line) < 0)
			goto badret;
	    }
	    memcpy (&ils[n], &Tnav, sizeof (Tnav));
	}
	ils[n].name = 0;

	fclose (ifile);
	return (0);

badret:
	fclose (ifile);
	ils_free ();
	return (-1);
}

extern void FAR
nav_term (void)
{
	ils_free ();
}


extern int FAR
ils_get (OBJECT *p)
{
	int		t;
	long		tx, ty, tz;
	ANGLE		a;
	struct ils	*b;

	t = EX->ils - 1;
	if (t < 0)
		t = p->home;
	b = &ils[t];

/* glide path deviation.
*/
	tx = (p->R[X] - b->R[X])/VONE;
	ty = (p->R[Y] - b->R[Y])/VONE;
	tz = (p->R[Z] - b->R[Z])/VONE;
	if (labs(tx) > 25000L || labs(ty) > 25000L || labs(tz) > 25000L) {
		EX->misc[14] = 0;
		EX->misc[15] = 0;
		return (1);			/* too far out */
	}

	t = ihypot2d ((int)tx-b->g[X], (int)ty-b->g[Y]); /* dist. to beacon */
	a = b->glidepath - ATAN ((int)tz, t);
	t = a - EX->misc[14];
	if (iabs(t) > 4)
		a = EX->misc[14] + t/4;
	EX->misc[14] = a;

/* localizer deviation.
*/
	a = ATAN ((int)tx-b->l[X], (int)ty-b->l[Y]) - b->localizer;
	EX->misc[15] = a;

	return (0);
}

#define XFULL	DEG(2.5)
#define YFULL	DEG(0.75)

extern void FAR
show_ils (HUD *h, OBJECT *p, int sx, int sy, int orgx, int orgy, int ss,
	int shifty)
{
	int		x, y, xref, yref, t, i, dd;
	ANGLE		a;
	struct ils	*b;

	if (!(EX->hud2 & HUD_ILS))
		return;

	orgy += shifty;

	b = &ils[EX->ils-1];

	gr_color (ST_HFG);

/* show navigation data
*/
	dd = num_size (9L, ss);
	x = orgx + sx - dd*9;
	y = orgy + sy - 2;

	t = ANG2DEG(EX->ilsHeading);
	if (t < 0)
		t += 360;
	stroke_frac (x, y, t , 3, 0, ss, ST_HFG);
	stroke_frac (x+dd*4, y, EX->ilsRange, 0, 1, ss, ST_HFG);

	y -= ss;
	stroke_str (x, y, b->name, ss, ST_HFG);

	xref = fmul (sx, FONE/8*7);
	yref = fmul (sy, FONE/8*7);

/* localizer deviation.
*/
	a = EX->misc[15];
	if (a > XFULL)
		x = xref;
	else if (a < -XFULL)
		x = -xref;
	else
		x = muldiv (xref, a, XFULL);
	x = orgx + x;
	gr_move (x, orgy-yref);
	gr_draw (x, orgy+yref);
	t = sx / 64;
	for (i = 1; i < 5; ++i) {
		y = muldiv (yref, i, 5);
		gr_move (x-t, orgy+y);
		gr_draw (x+t, orgy+y);
		gr_move (x-t, orgy-y);
		gr_draw (x+t, orgy-y);
	}
	gr_move (x-3*t, orgy);
	gr_draw (x+3*t, orgy);

/* glide slope deviation.
*/
	a = EX->misc[14];
	if (a > YFULL)
		y = yref;
	else if (a < -YFULL)
		y = -yref;
	else
		y = muldiv (yref, a, YFULL);
	y = orgy - y;
	gr_move (orgx-xref, y);
	gr_draw (orgx+xref, y);
	t = sy / 64;
	for (i = 1; i < 5; ++i) {
		x = muldiv (xref, i, 5);
		gr_move (orgx+x, y-t);
		gr_draw (orgx+x, y+t);
		gr_move (orgx-x, y-t);
		gr_draw (orgx-x, y+t);
	}
	gr_move (orgx, y-3*t);
	gr_draw (orgx, y+3*t);
}

#undef XFULL
#undef YFULL
	
extern int FAR
menu_ils (void)
{
	MENU	*MenuILS;
	OBJECT	*p;
	int	sel, i, n, nEntries, EntrySize;

	if (IS_PLANE(CV))
		p = CV;
	else {
		p = 0;
		MsgWPrintf (100, "not a aplane");
	}

	for (nEntries = 0; ils[nEntries].name; ++nEntries)
		;
	nEntries += 2;		/* "off" and "home" */
	EntrySize = 20;

	n = (nEntries+1) * sizeof (*MenuILS);
	if (F(MenuILS = (MENU *) memory_alloc (n)))
		return (1);

	sel = MENU_FAILED;
	for (i = 0; i < nEntries; ++i)
		if (F(MenuILS[i].text = (char *) memory_alloc (EntrySize)))
			goto ret;

	MenuILS[0].letter = '0';
	strcpy (MenuILS[0].text, "Off");
	for (i = 1; i < nEntries-1; ++i) {
		MenuILS[i].letter = i+'0';
		strcpy (MenuILS[i].text, ils[i-1].name);
	}
	MenuILS[i].letter = '=';
	strcpy (MenuILS[i].text, "Home");

	sel = p ? EX->ils : 0;
	sel = menu_open (MenuILS, sel);

	if (p) {
		switch (sel) {
		case MENU_ABORTED:
		case MENU_FAILED:
			break;
		case 0:
			EX->hud2 &= ~HUD_ILS;
			break;
		default:
			if (sel == nEntries-1)
				sel = st.home+1;
			if (EX->ils == sel) {
				EX->hud2 ^= HUD_ILS;
				break;
			}
			EX->hud2 |= HUD_ILS;
			EX->ils = sel;
			EX->misc[14] = 0;
			break;
		}
	} else
		MsgWPrintf (100, "not a aplane");
ret:
	for (i = 0; i < nEntries; ++i)
		if (MenuILS[i].text)
			memory_free (MenuILS[i].text, EntrySize);

	memory_free (MenuILS, n);

	if (MENU_FAILED == sel)
		return (1);

	menu_close ();
	return (0);
}

extern int FAR
nav_find (char *name)
{
	int	i;

	if (!name)
		return (0);
	for (i = 0; ils[i].name; ++i) {
		if (!stricmp (ils[i].name, name))
			return (i);
	}
	if (!name[0])
		return (i-1);
	return (-1);
}
