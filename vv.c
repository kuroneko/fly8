/* --------------------------------- vv.c ----------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Velocity Vector (Flight Path Marker) stuff.
*/

#include "plane.h"


extern int FAR
get_vv (OBJECT *p, VECT RR)
{
	int	tt;

	Vcopy (RR, p->vb);
	if (EX->hud3 & HUD_GVV) {
		tt = fmul (RR[X], p->cosy);
		RR[X] += fmul (tt, p->cosy);
		RR[Z] += fmul (tt, p->siny);
	} else
		tt = 0;
	return (tt);
}

LOCAL_FUNC void NEAR
do_fpm (HUD *h, int VV[2], int rx, int ry, int tx, int ty, int color, int ret,
	int type)
{
	if (VV[Y] > h->clipt-ty) {
		VV[Y] = h->clipt-ty;
		ret = 1;
	}
	if (1 == ret && (Uint)st.present % 250 > 125)	/* blink rate: 4Hz */
		return;

	show_fpm (h->orgx + VV[X], h->orgy - VV[Y], rx, ry, tx, ty,
		color, type);
}

/* Show the flight path marker and (optionally) the flight director.
*/
extern void FAR
show_vv (HUD *h, VIEW *view, OBJECT *p, int color)
{
	int	x, y, tx, ty, rx, ry, type, ret;
	int	sa, ca;
	ANGLE	a;
	VECT	RR;
	int	VV[2];

	if (!(EX->hud & HUD_VV) && !(EX->hud2 & HUD_DIRECTOR)) {
		VVDELAY = 0;
		return;
	}

	rx = fmul (h->sx, RVV);
	ry = fmul (h->sy, RVV);

	type = EX->hud1 & HUD_TYPES;
	if (type == HUD_F16) {
		tx = fmul (h->sx, EX->ldgap);
		ty = ry*2;
	} else if (type == HUD_F15) {
		tx = rx*2;
		ty = ry*2;
	} else {
		tx = fmul (h->sx, SVV);
		ty = fmul (h->sy, SVV*4/5);
	}

/* Show caged vv (optional).
*/
	if (p->speed <= 4*VONE || !(EX->hud3 & HUD_GVV))
		goto no_cvv;

	ca = get_vv (p, RR);
	a = ATAN (ca, RR[Y]);

	screen_coords (view, RR);
	ret = clip_to_screen (h->VV, RR, h->maxx, h->maxy,
			h->clipx-tx, h->clipy-ry, h->shifty);
	if (2 == ret)
		goto no_cvv;

	do_fpm (h, h->VV, rx, ry, tx, ty, color, ret,
		1 + (type != HUD_CLASSIC));

	if (iabs(a) < DEG(1))
		goto only_cvv;
no_cvv:

/* Show true vv.
*/
	if (p->speed > 4*VONE) {
		Vcopy (RR, p->vb);
		screen_coords (view, RR);
		ret = clip_to_screen (VV, RR, h->maxx, h->maxy,
				h->clipx-tx, h->clipy-ry, h->shifty);
		if (2 == ret)
			goto no_vv;
	} else {
		VV[X] = VV[Y] = 0;
		ret = 0;
	}

	do_fpm (h, VV, rx, ry, tx, ty, color, ret,
		(EX->hud3 & HUD_GVV) ? 0 : 1 + (type != HUD_CLASSIC));

no_vv:
	if (!(EX->hud3 & HUD_GVV)) {
		h->VV[X] = VV[X];
		h->VV[Y] = VV[Y];
	}
only_cvv:

	if (EX->hud2 & HUD_VW) {
		if (ret) {
			if ((VVDELAY += st.interval) > VVPERIOD)
				VVDELAY = VVPERIOD;
		} else if (VVDELAY > 0) {
			if ((VVDELAY -= st.interval) < 0)
				VVDELAY = 0;
		}
	} else
		VVDELAY = 0;

	if ((EX->hud2 & HUD_DIRECTOR) &&
	    (EX->misc[12] || EX->misc[13])) {
		x = rx;
		rx = tx;
		tx = x*8;

		y = ry;
		ry = ty;
		ty = y*8;

		sa = SIN(EX->misc[13]);		/* roll */
		ca = COS(EX->misc[13]);

		y = EX->misc[12];		/* pitch */
		if (y > DEG(30))
			y = DEG(30);
		else if (y < -DEG(30))
			y = -DEG(30);
		x = muldiv (h->sx/2, y, DEG(30));
		y = muldiv (h->sy/2, y, DEG(30));
		x = fmul (x, sa);
		y = fmul (y, ca);

		x += h->VV[X];
		y += h->VV[Y];
#if 0
		if (x > h->clipr-tx)
			x = h->clipr-tx;
		else if (x < h->clipl+tx)
			x = h->clipl+tx;
		if (y > h->clipt-ty)
			y = h->clipt-ty;
		else if (y < h->clipb+ty)
			y = h->clipb+ty;
#endif
		x = h->orgx + x;
		y = h->orgy - y;
		show_dir1 (x, y, rx, ry, tx, ty, sa, ca, h->fgi,
			h->orgx, h->orgy, h->sx, h->sy, h->shifty);
	}
}

/* Show the waterline mark.
*/
extern void FAR
show_wl (HUD *h, OBJECT *p, int color)
{
	int	tx, ty;

	if (EX->hud & HUD_PLUS)
		return;

	if (VVDELAY > 0 ||
	    ((HUD_FA18 == (EX->hud1 & HUD_TYPES)) && (EX->equip & EQ_GEAR))) {
		tx = fmul (h->sx, SVV);
		ty = fmul (h->sy, SVV);
		show_w (h->orgx, h->orgy, tx, ty, color);
	}
}
