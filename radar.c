/* --------------------------------- radar.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* paint the radar info onto the Head Up Display.
*/

#include "plane.h"


LOCAL_FUNC void NEAR show_all_intel (VIEW *view, OBJECT *pov, OBJECT *p, int sx,
	int sy, int bspeed, int orgx, int orgy, int maxx, int maxy, int ss,
	int mode, int shifty);
LOCAL_FUNC void NEAR show_intel (VIEW *view, OBJECT *pov, OBJECT *p,
	OBJECT *target, int sx, int sy, int orgx, int orgy, int maxx, int maxy,
	int ss, int mode, int shifty);
LOCAL_FUNC void NEAR show_box (OBJECT *p, OBJECT *target, int x, int y, int dx,
	int dy, int mode, int off_screen, int dist, int tti,
	int closure, int bottom, int right, int hbottom, int hleft, int ss,
	int shifty);
LOCAL_FUNC void NEAR show_range (OBJECT *p, int dist, int closure, int orgx,
	int orgy, int sx, int sy, int tx, int ss, int VVD[2]);
LOCAL_FUNC void NEAR check_shoot (OBJECT *p, int orgx, int orgy, int sx, int sy,
	int dx, int dy, int ty, int ss);
LOCAL_FUNC int  NEAR show_select (VIEW *view, OBJECT *p, int radar, int orgx,
	int orgy, int sx, int sy, int maxx, int shifty);
LOCAL_FUNC void NEAR show_bombs (VIEW *view, OBJECT *p, OBJECT *pov, int orgx,
	int orgy, int sx, int sy, int maxx, int maxy, int clipx, int clipy,
	int shifty, int ss);
LOCAL_FUNC void NEAR show_target (OBJECT *p, OBJECT *target, VECT RR, int dist,
	int closure, int tti, int orgx, int orgy, int maxx, int maxy, int ss);

extern ANGLE FAR
screen_direction (VIEW *view, VECT RR)
{
	int	s, x, y, z;

	s = VP->z;				/* get maximum */
	if (s < VP->maxx)
		s = VP->maxx;
	if (s < VP->maxy)
		s = VP->maxy;
	x = muldiv (RR[X], VP->maxx, s);
	y = muldiv (RR[Z], VP->maxy, s);
	z = muldiv (RR[Y], VP->z, s);

	s = ihypot2d (x, y);
	return (ATAN (s, z));
}

extern void FAR
show_radar (HUD *h, VIEW *view, OBJECT *p, OBJECT *pov, int orgx, int orgy,
	int maxx, int maxy, int tx, int ty, int ss, int clipx, int clipy,
	int sx, int sy, int shifty, int VVD[2], int mode, int hon)
{
	int	dx, dy, ds, x, y, closure, have_reticule, f16, f156cl, fa18;
	int	i, dist, off_screen, toff, bspeed, rv, dt, dti, tti, tbox;
	int	tdir, bx, by, radar, selecting, dmode;
	Ushort	savehud;
	OBJECT	*target;
	VECT	RRT, RR, V;
	LVECT	LL, IP;
	int	D[2], DD[2];
	long	ldist;

	if (h->flags & HF_ETHER) {
		clipx = maxx;
		clipy = maxy;
		shifty = 0;
	}

/* bomb impact points
*/
	if (WE_MK82 == EX->weapon)
		show_bombs (view, p, pov, orgx, orgy, sx, sy, maxx, maxy, clipx,
			clipy, shifty, ss);

	show_compass (h, p, mode);

	radar = EX->radar;
	if (!(radar & R_ON))
		return;

	i = EX->hud1 & HUD_TYPES;
	f16 = i == HUD_F16;
	f156cl = f16 || i == HUD_F15 || i == HUD_CLASSIC;
	fa18 = i == HUD_FA18;
	have_reticule = 0;
	dx = ds = toff = 0;	/* avoid compiler warning */

	if (WE_MK82 == EX->weapon)
		bspeed = BombSpeed (p, V)/VONE;
	else
		bspeed = BulletSpeed (p, V)/VONE;

	if ((radar & R_INTEL) ||
	    (mode == HDT_MAP || mode == HDT_RADAR)) {
		show_all_intel (view, pov, p, sx, sy, bspeed, orgx, orgy,
			maxx, maxy, ss, mode, (mode == HDT_HUD) ? shifty: 0);
		return;
	}
/* Check if we have a target at all.
*/
	if (F(target = EX->target) || target->id != EX->tid) {
		target = 0;
		EX->target = 0;
	}
/* if selecting, show selection circle and best target (if we have one).
*/
	selecting = EX->weapon
			&& (radar & (R_SELECT3|R_SELECT20|R_SELECT5))
			&& !(target && (radar & R_LOCK));

	if (selecting && hon)
		dy = show_select (view, p, radar, orgx, orgy, sx, sy, maxx,
				shifty);
	else
		dy = 0;			/* avoid compiler warning */

	if (!target)
		return;

/* get target position, distance, closure and time to intercept.
*/
	Vsub (LL, p->R, target->R);
	ldist = lhypot3d (LL);
	dti = dist = (int)(ldist/VONE);
	if (WE_MK82 == EX->weapon) {
		closure = (int)((p->R[Z]-target->R[Z])/VONE);
		dt = BombIP (p->R, V, target->R[Z], IP);
		tti = (dt < 6000) ? dt/10 : 600;
	} else {
		if (dist)
			closure = (int)(-(LL[X]*(p->V[X] - EX->tspeed[X]) +
					  LL[Y]*(p->V[Y] - EX->tspeed[Y]) +
					  LL[Z]*(p->V[Z] - EX->tspeed[Z])
					 ) / (ldist*VONE));
		else
			closure = 0;
		tti = (dist/60 < closure) ? muldiv (dist, 10, closure) : 600;
	}

	objects_show (1, view, pov, target->R, RRT);
	tdir = screen_direction (view, RRT);

	tbox = fa18 ? F18TBOX : (f16 ? F16TBOX : F15TBOX);
	bx = fmul (sx, tbox);
	by = fmul (sy, tbox);
	if (selecting) {
		if (hon)
			show_data (p, orgx-3*ss, orgy+dy+ss, 1,
				EX->hud1 & HUD_KNOTS, dist, closure,
				get_name (p, target, 0), tti, 0, ss, ST_HFG);
		off_screen = clip_to_screen (D, RRT, maxx, maxy,
				clipx-bx, clipy-by, shifty);
		show_brect (orgx+D[X], orgy-D[Y], bx, by, FCON(.5), ST_HFG,
			off_screen);
		return;
	}
/* target highlighting box
*/
	if (mode == HDT_TARGET || mode == HDT_PAN) {
		show_target (p, target, RRT, dist, closure, tti, orgx, orgy,
				maxx, maxy, ss);
		return;
	}

	off_screen = clip_to_screen (D, RRT, maxx, maxy, clipx-bx, clipy-by,
			shifty);

	if (HDT_HUD != mode && !(HDT_FRONT == mode && EX->hud & HUD_RETICLE))
		goto no_reticule;

	rv = fa18 ? F18RAIM : (f16 ? F16RAIM : F15RAIM);
	dx = fmul (sx, rv);
	dy = fmul (sy, rv);
	ds = 3;

/* bomb aiming reticule
*/
	if (WE_MK82 == EX->weapon) {
		objects_show (1, view, pov, IP, RR);

		toff = clip_to_screen (DD, RR, maxx, maxy, clipx-dx, clipy-dy,
					shifty);
		gr_color (ST_HFG);
		gr_move (orgx+VVD[X], orgy-VVD[Y]);
		gr_draw (orgx+DD[X],  orgy-DD[Y]);

		have_reticule = 1;
		goto no_reticule;
	}
/* target-speed corrected gun aiming reticule
*/
	dt = rv = 0;

	if (dist/4 > bspeed)
		goto no_reticule;
	have_reticule = 1;
	SetKillCorrection (p, target, RR, &dt);

	if (EX->hud & HUD_ROSS) {

/* Ross style aiming reticule
*/
		LVcopy (LL, target->R);
		Vinc (LL, RR);
		objects_show (1, view, p, LL, RR);

		toff = clip_to_screen (DD, RR, maxx, maxy,
			clipx-2*tx, clipy-2*ty, shifty);
		show_brect (orgx+DD[X], orgy-DD[Y], tx*2, ty*2, FCON(.5),
			ST_HFGI, toff);
		DD[X] = DD[Y] = 0;	/* reticule at center */
		toff = 0;

	} else {

/* Eyal style aiming reticule
*/
		Vsub (LL, p->R, RR);
		LL[X] += muldiv (V[X], dt, VONE*1000) * (long)VONE;
		LL[Y] += muldiv (V[Y], dt, VONE*1000) * (long)VONE;
		LL[Z] += muldiv (V[Z], dt, VONE*1000) * (long)VONE;

		objects_show (1, view, p, LL, RR);
		toff = clip_to_screen (DD, RR, maxx, maxy, clipx-dx, clipy-dy,
					shifty);
	}
	tti = dt/100;
no_reticule:

	if (EX->hud1 & HUD_CORNER) {
		dmode = 2;
		x = orgx-sx+2;
		y = orgy+sy-2;
		if (EX->hud & HUD_BIG) {
			x += 6*tx;
			if (!(EX->hud1 & HUD_TOP))
				y -= 6*ty + ss;
		} else {
			if (!(EX->hud1 & HUD_TOP))
				y -= ss;
		}
	} else {
		dmode = 0;
		x = y = 0;
	}

/* Show the aiming reticule.
*/
	savehud = EX->hud;
	if (have_reticule) {
		show_piper (h, p, target, orgx+DD[X], orgy-DD[Y],
			dx, dy, ds, dmode, toff, dti, tti, closure,
			orgx, orgy, clipx, clipy, y, x, ss, shifty);
		check_shoot (p, orgx, orgy, sx, sy, DD[X], DD[Y], dy,
			ss);
		EX->hud &= ~HUD_DATA;	/* don't show data again */
	}

/* Show the target designator and target pointer.
*/
	if (EX->hud & HUD_TARGET) {
		if ((mode == HDT_FRONT || mode == HDT_HUD) &&
		    !((EX->hud2 & HUD_HIDETGT) && have_reticule &&
		      iabs (D[X]-DD[X]) < (Uint)2*bx &&
		      iabs (D[Y]-DD[Y]) < (Uint)2*by)) {
			if (off_screen && (EX->hud2 & HUD_TPOINTER)) {
				tdir = ANG2DEG(tdir);
				if (EX->hud2 & HUD_VPOINTER) {
					if ((i = tdir) > 90)
						i = 90;
					dx = muldiv (D[X], i, 90);
					dy = muldiv (D[Y], i, 90);
				} else {
					i = ihypot2d (D[X], D[Y]);
					dx = h->sy - h->shifty;
					i = muldiv (i, h->sy, dx);
					i *= 2;
					dx = muldiv (sx, D[X], i);
					dy = muldiv (sy, D[Y], i);
				}
				gr_color (h->fg);
				gr_move (orgx,    orgy);
				gr_draw (orgx+dx, orgy-dy);
				i = (tdir >= 100) ? 3 : 2;
				stroke_frac (h->orgx - 3*h->tx - i*h->dd - 2,
					h->orgy + h->ss/2, (long)tdir,
					i, 0, h->ss, h->fg);
			}
			if (off_screen && (h->flags & HF_ETHERFRAME)) {
				clip_to_ether (h, D, clipx-bx, clipy-by);
				show_rect (orgx+D[X], orgy-D[Y], h->ethertx,
					h->etherty, h->fg, 0);
			} else
				show_box (p, target, orgx+D[X], orgy-D[Y],
					bx, by, dmode, off_screen, dti, tti,
					closure, orgy+clipy, orgx+clipx, y, x,
					ss, shifty);
		      }
	}
	if (f156cl && hon &&
	    (have_reticule || (target && (EX->hud & HUD_TARGET))))
		show_range (p, dti, closure, orgx, orgy, sx, sy, tx, ss, VVD);
	EX->hud = savehud;
}

LOCAL_FUNC void NEAR
show_all_intel (VIEW *view, OBJECT *pov, OBJECT *p, int sx, int sy,
	int bspeed, int orgx, int orgy, int maxx, int maxy, int ss, int mode,
	int shifty)
{
	OBJECT	*target;

	for (target = CO; target; target = target->next) {
		if (!(target->shflags & SH_BEHIT))
			continue;
		if ((EX->radar&R_INTELCC) && !(target->flags&F_CC))
			continue;
		if (!(st.flags1&SF_EXTVIEW) && target == p)
			continue;
		show_intel (view, pov, p, target, sx, sy, orgx, orgy,
			maxx, maxy, ss, mode, shifty);
	}
}

LOCAL_FUNC void NEAR
show_intel (VIEW *view, OBJECT *pov, OBJECT *p, OBJECT *target, int sx,
	int sy, int orgx, int orgy, int maxx, int maxy, int ss, int map,
	int shifty)
{
	VECT	RR;
	int	D[2], off_screen, size, dx, dy, tti, speed, c, s, t;
	long	dist;

	if (map == HDT_MAP)
		map = 1;
	else if (map == HDT_RADAR)
		map = 2;
	else
		map = 0;

	if (map) {
		if (CC == target)
			dist = CC->R[Z]/VONE;
		else
			dist = (target->R[Z] - CC->R[Z])/VONE;
		tti = 0;
	} else {
		dist = ldist3d (p->R, target->R)/VONE;
#if 0
		Vcopy (V, p->V);
		Vdec (V, EX->tspeed);
		speed = est_hyp (V[X], V[Y], V[Z])/VONE; /* closure speed */
#else
		speed = iabs(p->speed/VONE);		/* linear speed */
#endif
		if (dist/100 >= speed)
			tti = 999;
		else if (dist > VONE)
			tti = (int)(dist*10/speed);
		else
			tti = muldiv ((int)dist, 10, speed);
		if (dist > 31999)
			dist = 31999;
	}

	objects_show (1, view, pov, target->R, RR);

	if (map || tti == 999) {
		dx = fmul (sx, MAPBOX/4);
		dy = fmul (sy, MAPBOX/4);
	} else {
		size = muldiv (MAPBOX, 6*600-4*tti, 6*600);
		dx = fmul (sx, size);
		dy = fmul (sy, size);
	}
	
	off_screen = clip_to_screen (D, RR, maxx, maxy, maxx-dx, maxy-dy,
					shifty);
#if 1
	if (off_screen)
		return;
#endif
	D[X] = orgx + D[X];
	D[Y] = orgy - D[Y];
	show_box (p, target, D[X], D[Y], dx, dy, 1, off_screen,
		(int)dist, tti, 0, orgy+maxy+shifty, orgx+maxx, 0, 0, ss, 0);

/* show target horizontal speed vector.
*/
	if (map) {
		t = 25*VONE;		/* one boxfull for 100 knots */
		dx = muldiv (target->V[X], dx, t);
		dy = muldiv (target->V[Y], dy, t);
		if (dx || dy) {
			if (2 == map) {
				s = -pov->sinz;
				c =  pov->cosz;
				t  = fmul (dx, c) - fmul (dy, s);
				dy = fmul (dx, s) + fmul (dy, c);
				dx = t;
			}
			gr_color (ST_HFGI);
			add_segment (D[X], D[Y], D[X]+dx, D[Y]-dy,
				orgx, orgy, maxx, maxy, 0);
		}
	}
}

extern void FAR
show_data (OBJECT *p, int datax, int datay, int detail, int knots, int dist,
	int closure, char *name, int tti, int mode, int ss, Uint color)
{
	long	tt;

	if (detail) {
		if (knots)
			tt = 3L*dist + fmul (dist, 4601);	/* feet */
		else
			tt = dist;
		if (tt >= 10000L || tt <= -10000L)
			stroke_frac (datax, datay, tt/100, 0, 1, ss, color);
		else
			stroke_frac (datax, datay, tt/10, 3, 2, ss, color);
		datay += ss;

		if (!(1&mode)) {
			tt = closure;
			if (knots) {
				if (WE_M61 == EX->weapon)	/* knots */
					tt += fmul (closure, 15465);
				else				/* feet */
					tt = 3*tt + fmul (closure, 4601);
			}
			stroke_num (datax, datay, tt, ss, color);
			datay += ss;
			stroke_frac (datax, datay, (long)tti, 0, 1, ss, color);
			datay += ss;
		}

	}
	if (name)
		stroke_str (datax, datay, name, ss, color);
}

extern char * FAR
get_name (OBJECT *p, OBJECT *target, int mode)
{
	char		*name;
	static char	title[40];

	if (target->flags & F_CC) {
		if (target->flags & F_IMPORTED) {
			if (target->flags & F_FRIEND)
				name = target->rplayer->name;
			else {
				strcpy (title, target->rplayer->name);
				strcat (title, ":");
				strcat (title, target->rplayer->team);
				name = title;
			}
		} else
			name = st.nikname;
	} else if (mode != 1 || (EX->hud1 & HUD_INAME)) {
		title[0] = '*';
		if (IS_PLANE(target))
			strcpy (title+1, EEP(target)->name);
		else					/* object type */
			strcpy (title+1, TITLE(target));
		name = title;
	} else
		name = 0;
	return (name);
}

LOCAL_FUNC void NEAR
show_box (OBJECT *p, OBJECT *target, int x, int y, int dx, int dy, int mode,
	int off_screen, int dist, int tti, int closure, int bottom, int right,
	int hbottom, int hleft, int ss, int shifty)
{
	int	t, datax, datay, nlines, knots;
	char	*name;
	Uint	color;

	color = (1 == mode && (target->flags&F_CC))
		? (target->flags&F_FRIEND ? ST_FRIEND : ST_FOE)
		: ST_HFG;

	knots = EX->hud1 & HUD_KNOTS;
	if (NEWTGT(p))
		t = NEWTGT(p)--&1;
	else
		t = 1;

	if (t) {
		if (EX->target == target && (EX->radar&R_LOCK))
			show_rect (x, y, dx, dy, color, 0);
		else
			show_brect (x, y, dx, dy, FCON(.5), color, 0);
		if (off_screen == 1)			/* ahead */
			show_plus (x, y, dx, dy, color);
		else if (off_screen == 2)		/* behind */
			show_x (x, y, dx, dy, color);
	}

	if (EX->hud & HUD_DATA) {
		name = get_name (p, target, mode);
		nlines = ((1&mode) ? 1 : 3) + !!name;

		if (2&mode) {
			datax = hleft;
			datay = hbottom+shifty-(nlines-1)*ss;
		} else {
			if (y+dy+nlines*ss >= bottom+shifty)
				datay = y-dy-2-(nlines-1)*ss;
			else
				datay = y+dy+ss;
			datax = num_size (9L, ss) * (dist<0 ? 5 : 4);
			if (name) {
				t = stroke_size (name, ss);
				if (t < datax)
					t = datax;
			} else
				t = datax;

			t = right-t;
			datax = x-dx;
			if (datax > t)
				datax = t;
		}

		show_data (p, datax, datay, 1, knots, dist, closure, name,
				tti, mode, ss, color);
	}
}

extern int FAR
get_center (OBJECT *p, int orgy, int sy, int VVD[2])
{
	int	y, y0, hudtype;

	hudtype = EX->hud1 & HUD_TYPES;
	if (HUD_F16 == hudtype) {
		if (EX->equip & EQ_GEAR) {
			y0 = fmul (sy, F16CNTRG);
			y = -VVD[Y] - fmul (sy, F16HEADTOP);
			if (y0 < y)
				y0 = y;
		} else
			y0 = fmul (sy, F16CNTR);
	} else if (HUD_F15 == hudtype)
		y0 = fmul (sy, F15CNTR);
	else
		y0 = 0;
	y0 += orgy;
	return (y0);
}

/* radar range scale
*/

LOCAL_FUNC void NEAR
show_range (OBJECT *p, int dist, int closure, int orgx, int orgy, int sx,
	int sy, int tx, int ss, int VVD[2])
{
	int	i, x, y, y0, t, dm, range, ss2, hud1, hudtype;

	hud1 = EX->hud1;
	hudtype = hud1 & HUD_TYPES;

	ss2 = ss/2;
	if (hud1 & HUD_KNOTS) {
		dist = fmul (dist, FCON(0.54));		/* nm/1000 */
		if (WE_M61 == EX->weapon)		/* knots */
			closure = fmul (closure, FCON(1.943));
		else					/* feet/sec */
			closure = fmul (closure, FCON(3.28/4))*4;
	}
	gr_color (ST_HFG);
	if (hudtype == HUD_F15 || hudtype == HUD_F16 ||
	    hudtype == HUD_CLASSIC) {
		if (dist > 10000)			/* full scale range */
			range = 20000;
		else
			range = 10000;

		y0 = get_center (p, orgy, sy, VVD);
		if (HUD_F16 == hudtype) {
			dm = num_size (9L, ss);
			x = orgx + fmul (sx, F16ALT) - 3*dm;
			y = muldiv (sy, 8, 40/2);
			y0 += y/2 - 2*ss;
		} else if (HUD_F15 == hudtype) {
			x = orgx + fmul (sx, F15RDR);
			y = fmul (sy, F15RDRS);		/* scale size */
			y0 += muldiv (sy, 8, 15);
		} else {
			x = orgx + sx-2*ss-2*tx;
			y = muldiv (sy, 10, 15);
			y0 += muldiv (sy, 8, 15);
		}

		gr_move (x, y0);
		gr_draw (x, y0-y);
	    	t = range/1000;				/* radar range */
		stroke_num (x+tx+2, y0-y/2+ss2, (long)t/2, ss, ST_HFG);
		stroke_num (x-ss2,  y0-y-1,     (long)t,   ss, ST_HFG);
		for (i = 0; i < 5; ++i) {
			t = y0 - muldiv (i, y, 5-1);
			gr_move (x,    t);
			gr_draw (x+tx, t);
		}

		y = y0 - muldiv (y, dist, range);
	} else {
		x = orgx + fmul (sx, F15ALT);
		y = orgy + muldiv (sy, 8, 15);
	}
	gr_move (x-ss2, y-ss2);
	gr_draw (x,     y);
	gr_draw (x-ss2, y+ss2);

	closure = closure/10*10;
	dm = num_size ((long)closure, ss);
	stroke_num (x-dm-ss2, y+ss2, (long)closure, ss, ST_HFG);
}

LOCAL_FUNC void NEAR
check_shoot (OBJECT *p, int orgx, int orgy, int sx, int sy, int dx, int dy,
	int ty, int ss)
{
	int	t, x, y;

	if ((EX->radar&R_SHOOT)) {
		t = stroke_size ("SHOOT", ss)/2;
		x = orgx+dx;
		if (x-t < orgx-sx)
			x = orgx-sx;
		else if (x+t > orgx+sx)
			x = orgx+sx-t;
		else
			x -= t;
		t = ss;
		y = orgy-dy-ty;
		if (y-t < orgy-sy)
			y = orgy-sy+t;
		stroke_str (x, y, "SHOOT", ss, ST_HFGI);
	}
}

#if 0		/* keep this one for old times sake... */
LOCAL_FUNC void NEAR show_reticule (OBJECT *p, int x, int y, int tx, int ty,
	int orgx, int orgy, int sx, int sy, int off_screen, int c, int ss);

LOCAL_FUNC void NEAR
show_reticule (OBJECT *p, int x, int y, int tx, int ty, int orgx, int orgy,
	int sx, int sy, int off_screen, int c, int ss)
{
	int	dx, dy;

	check_shoot (p, orgx, orgy, sx, sy, x, y, ty, ss);

	x = orgx + x;
	y = orgy - y;

	dx = tx/2;
	dy = ty/2;

	gr_color (c);
	gr_move (x+tx,   y+dy-1);
	gr_draw (x+tx,   y+dy);
	gr_move (x+dx-1, y+ty);
	gr_draw (x+dx,   y+ty);
	gr_move (x-dx+1, y+ty);
	gr_draw (x-dx,   y+ty);
	gr_move (x-tx,   y+dy-1);
	gr_draw (x-tx,   y+dy);
	gr_move (x-tx,   y-dy+1);
	gr_draw (x-tx,   y-dy);
	gr_move (x-dx+1, y-ty);
	gr_draw (x-dx,   y-ty);
	gr_move (x+dx-1, y-ty);
	gr_draw (x+dx,   y-ty);
	gr_move (x+tx,   y-dy+1);
	gr_draw (x+tx,   y-dy);
	if (off_screen)
		show_x (x, y, tx, ty, c);
}
#endif

#define SEL3D	(D90/54)		/* 3.3 degrees wide circle */

LOCAL_FUNC int NEAR
show_select (VIEW *view, OBJECT *p, int radar, int orgx, int orgy, int sx,
	int sy, int maxx, int shifty)
{
	int	i, rx, ry, dx, dy, ox, oy;
	ANGLE	a;

	gr_color (ST_HFG);
	if (radar & R_SELECT3) {
		i = muldiv (VP->maxx, sx, maxx);
		a = ATAN (i, VP->z);
		if (a < SEL3D)
			i = FONE;
		else
			i = muldiv (FONE, SEL3D, a);

		dx = fmul (sx, i);
		dy = fmul (sy, i);

		gr_ellipse (orgx, orgy, dx, dy);
	} else if (radar & R_SELECT20) {
		orgy += shifty;
		rx = fmul (sx, RSELECT20);
		ry = fmul (sy, RSELECT20);

		ox = rx;
		oy = 0;
		for (a = 0, i = 0; i < 12*2; ++i) {
			a += D90/12;
			dx = fmul (COS (a), rx);
			dy = fmul (SIN (a), ry);
			gr_move (orgx+ox,        orgy-oy);
			gr_draw (orgx+(dx+ox)/2, orgy-(dy+oy)/2);
			gr_move (orgx-ox,        orgy+oy);
			gr_draw (orgx-(dx+ox)/2, orgy+(dy+oy)/2);
			ox = dx;
			oy = dy;
		}
		dy = sy/2;
	} else {
		orgy += shifty;
		dx = fmul (sx, RSELECT5);
		add_dash (orgx-dx, orgy, orgx-dx, orgy-sy, 16, FONE/2,
				orgx, orgy, sx, sy);
		add_dash (orgx-dx, orgy, orgx,    orgy,     4, FONE/2,
				orgx, orgy, sx, sy);
		add_dash (orgx+dx, orgy, orgx+dx, orgy-sy, 16, FONE/2,
				orgx, orgy, sx, sy);
		add_dash (orgx+dx, orgy, orgx,    orgy,     4, FONE/2,
				orgx, orgy, sx, sy);
		dy = 0;
	}
	return (dy);
}

#undef SEL3D

LOCAL_FUNC void NEAR
show_ip (VIEW *view, LVECT IP, OBJECT *pov, int orgx, int orgy, int dx,
	int dy, int maxx, int maxy, int clipx, int clipy, int shifty, int ss,
	int dt)
{
	int	toff;
	VECT	RR;
	int	DD[2];

	objects_show (1, view, pov, IP, RR);
	toff = clip_to_screen (DD, RR, maxx, maxy, clipx-dx, clipy-dy, shifty);
	if (!toff) {
		DD[X] = orgx + DD[X];
		DD[Y] = orgy - DD[Y];
		show_diamond (DD[X], DD[Y], dx, dy, ST_HFG, 0);
		keep_inside (DD+X, DD+Y, ss, ss*4, 0, -ss, orgx, orgy, clipx,
			clipy, shifty);
		stroke_frac (DD[X]+ss, DD[Y], dt, 0, 1, ss, ST_HFG);
	}
}

LOCAL_FUNC void NEAR
show_bombs (VIEW *view, OBJECT *p, OBJECT *pov, int orgx, int orgy, int sx,
	int sy, int maxx, int maxy, int clipx, int clipy, int shifty, int ss)
{
	int	dx, dy, dt;
	OBJECT	*b;
	LVECT	IP;

	dx = fmul (sx, IPDIAMOND);
	dy = fmul (sy, IPDIAMOND);

	for (b = CO; b; b = b->next) {
		if (b->owner != CC || ET_BOMB != b->e_type)
			continue;
		dt = (int)-(TIMEOUT (EBM(b)->timpact) / 100);
		if (dt < 0)
			dt = 0;
		show_ip (view, EBM(b)->IP, pov, orgx, orgy, dx, dy, maxx,
			maxy, clipx, clipy, shifty, ss, dt);
	}

	if (!(EX->radar & R_ON)) {
		dt = BombIP (p->R, p->V, 0L, IP)/10;
		show_ip (view, IP, pov, orgx, orgy, dx, dy, maxx, maxy,
			clipx, clipy, shifty, ss, dt);
	}
}

LOCAL_FUNC void NEAR
show_target (OBJECT *p, OBJECT *target, VECT RR, int dist, int closure,
	int tti, int orgx, int orgy, int maxx, int maxy, int ss)
{
	int	dd, dx, dy, x, i;
	long	tx, ty, tz;
	int	D[2];

	dd = num_size (9L, ss);
	dx = fmul (maxx, FCON(0.25));
	dy = fmul (maxy, FCON(0.25));
	if (!clip_to_screen (D, RR, maxx, maxy, maxx-dx, maxy-dy, 0))
#if 0
		show_bplus (orgx+D[X], orgy-D[Y], dx, dy, FCON(0.125), ST_HFG);
#else
		show_brect (orgx+D[X], orgy-D[Y], dx, dy, FCON(0.75), ST_HFG,
				0);
#endif

	x  = orgx - maxx + 2;
	dy = orgy + maxy - 2 - 3*ss;
	show_data (p, x, dy, 1, EX->hud1 & HUD_KNOTS,
		dist, closure, get_name (p, target, 0), tti, 0, ss, ST_HFG);

	tx = (target->R[X] - p->R[X])/VONE;
	ty = (target->R[Y] - p->R[Y])/VONE;
	tz = (target->R[Z] - p->R[Z])/VONE;
	if (labs(tx) > 25000L || labs(ty) > 25000L || labs(tz) > 25000L)
		return;				/* too far out */

	x  = orgx + maxx - 5*dd;
	dy = orgy + maxy - 2;

	dx = x;
	dx += stroke_str (dx, dy, "P ", ss, ST_HFG);
	i = ihypot2d ((int)tx, (int)ty);
	i = ANG2DEG(ATAN ((int)tz, i));
	dx += stroke_char (dx, dy, (i<0) ? '-' : ' ', ss, ST_HFG);
	stroke_frac (dx, dy, (long)iabs(i), 2, 0, ss, ST_HFG);

	dy -= ss;

	dx = x;
	dx += stroke_str (dx, dy, "H ", ss, ST_HFG);
	i = ANG2DEG(ATAN ((int)tx, (int)ty));
	if (i < 0)
		i += 360;
	stroke_frac (dx, dy, (long)i, 3, 0, ss, ST_HFG);
}
