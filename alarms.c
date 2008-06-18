/* --------------------------------- alarms.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Set and show alarms/warnings/status the Head Up Display. It also arms
 * the warning lamps.
*/

#include "plane.h"


LOCAL_FUNC void NEAR show_xbreak (HUD *h, OBJECT *p, int color);

extern void FAR
alarm_set (int mode)
{
	if (!st.quiet)
		return;

	if ((mode & SS_ALARM) && !(st.sounds & SS_ALARM)) {
		st.sounds |= SS_ALARM;
		Snd->Effect (EFF_ALARM, SND_ON);
	} else if (!(mode & SS_ALARM) && (st.sounds & SS_ALARM)) {
		st.sounds &= ~SS_ALARM;
		Snd->Effect (EFF_ALARM, SND_OFF);
	}

	if ((mode & SS_WARN) && !(st.sounds & SS_WARN)) {
		st.sounds |= SS_WARN;
		Snd->Effect (EFF_WARN, SND_ON);
	} else if (!(mode & SS_WARN) && (st.sounds & SS_WARN)) {
		st.sounds &= ~SS_WARN;
		Snd->Effect (EFF_WARN, SND_OFF);
	}
}

LOCAL_FUNC void NEAR
show_xbreak (HUD *h, OBJECT *p, int color)
{
	int	dx, dy, x, y;

	dy = h->sy>>1;
	dx = h->sx>>1;
	y = h->orgy + h->shifty - h->sy + dy;

	if ((EX->hud3 & HUD_XVAR) && (int)EX->misc[17] > 0)
		x = fmul (h->sx>>1, (int)EX->misc[17]);
	else
		x = 0;

	if (x > 0 || ((Uint)st.present) % 400 < 200) {	/* blink rate: 2.5Hz */
		gr_color (color);
		gr_move (h->orgx-x-dx, y-dy);
		gr_draw (h->orgx-x,    y);
		gr_draw (h->orgx-x-dx, y+dy);

		gr_move (h->orgx+x+dx, y-dy);
		gr_draw (h->orgx+x,    y);
		gr_draw (h->orgx+x+dx, y+dy);
	}
}

/* produce audio/visual effects attached to the lamps panel.
*/
extern void FAR
hud_alarm (HUD *h, OBJECT *p, int color, int mode, int hon)
{
	int	alarm, valarm, balarm, blink, ss2, ss3, y;
	long	t;

	blink = ((int)st.present)&0x0080;	/* 128ms period */
	alarm = 0;
	ss2 = h->ss*2;
	ss3 = h->ss*3;
	y = h->orgy + h->shifty;

	if (IS_PLANE(p) && (p->gpflags & GPF_PILOT)) {
		valarm = ((EX->hud & HUD_ON) && (EX->hud1 & HUD_VALARM))
			 || HDT_HUD == mode;
		balarm = valarm && blink;

		if (EX->lamps[LAMP_GLIMIT] & LAMP_RED) {
			alarm = SS_WARN;
			if (balarm)
				stroke_str (h->orgx-h->dd*6, y-h->ss*5,
					"GLIMIT", ss2, color);
		}

		if (EX->lamps[LAMP_STALL] & LAMP_RED) {
			alarm = SS_WARN;
			if (balarm)
				stroke_str (h->orgx-h->dd*5, y-ss3, "STALL",
					ss2, color);
		}

		if (EX->lamps[LAMP_FUEL] & LAMP_BRED) {
			alarm = SS_WARN;
			if (balarm)
				stroke_str (h->orgx-h->dd*4,
					y+h->ss*5, "FUEL", ss2, color);
		}

		CL->next->flags &= ~F_VISIBLE;
		if (EX->lamps[LAMP_ALT] & LAMP_RED) {
			if (hon && (EX->hud2 & HUD_XBREAK))
				show_xbreak (h, p, color);
			if (EX->hud2 & HUD_XGRID)
				CL->next->flags |= F_VISIBLE;
			if (EX->lamps[LAMP_PULLUP] & LAMP_RED) {
				alarm = SS_ALARM;
				t = 1000L * EX->fuel / EP->fuel_capacity;
				t = (t/1000)*1000+1;
				if (valarm && (st.present%t) < 200)
					stroke_str (h->orgx-h->dd*10,
						y+ss3, "PULL UP", ss3, color);
			}
		}

		if (EX->lamps[LAMP_EJECT] & LAMP_RED) {
			alarm = SS_ALARM;
			if (balarm)
				stroke_str (h->orgx-h->dd*8, y, "EJECT",
					ss3, color);
		}

		if (!(EX->hud1 & HUD_AALARM))
			alarm = 0;
	}

	alarm_set (alarm);
}
