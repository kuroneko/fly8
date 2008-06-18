/* --------------------------------- views.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Show all visual displays
*/

#include "plane.h"


static MENU FAR MenuView[] = {
	{'f', "front"},		/*  0 */
	{'n', "none"},		/*  1 */
	{'a', "rear"},		/*  2 */
	{'m', "map"},		/*  3 */
	{'r', "radar"},		/*  4 */
	{'t', "target"},	/*  5 */
	{'p', "pan"},		/*  6 */
	{'g', "gaze"},		/*  7 */
	{'c', "chase"},		/*  8 */
	{'o', "follow"},	/*  9 */
	{'h', "hud"},		/* 10 */
	{'u', "up-front"},	/* 11 */
	{'P', "panel"},		/* 12 */
	{'R', "right"},		/* 13 */
	{'L', "left"},		/* 14 */
	{'s', "stores"},	/* 15 */
	{'l', "lamps"},		/* 16 */
	{'M', "mirror"},	/* 17 */
{'\0', 0}};

/* Select external view mode
*/

extern int FAR
menu_view (int sel)
{
	sel = menu_open (MenuView, sel);
	if (MENU_FAILED != sel)
		menu_close ();
	return (sel);
}

LOCAL_FUNC void NEAR
ident (HDD *hdd, int orgx, int orgy, int maxx, int maxy, int ss)
{
	int	bss, i, l;
	char	*p;

	if (st.flags1 & SF_IDENT) {
		p = MenuView[hdd->type].text;
		l = strlen (p);
		bss = ss*4;
		if (bss > 64)
			bss = 64;
		if (bss > maxx/l)
			bss = maxx/l;
		i = hdd - (HDD *)st.hdd;
		stroke_num (orgx-bss*l/2, orgy, (long)i, bss, ST_IFG);
		stroke_str (orgx-bss*l/2, orgy+bss, p, bss, ST_IFG);
	} else if (!(st.flags & SF_BLANKER) && (st.flags1 & SF_WIDENT)) {
		if (!(st.flags & SF_MAIN)) {
			p = MenuView[hdd->type].text;
			l = stroke_size (p, ss);
			stroke_str (orgx+maxx-l, orgy-maxy+ss, p, ss, ST_HFG);
		}
	}
}

/* Get window dimentions.
*/
extern void FAR
get_area (VIEW *view, int *orgx, int *orgy, int *maxx, int *maxy)
{
	int		px;
	VIEWPORT	*vp;

/* Calculate paralax.
*/
	if (F(vp = VP))
		vp = CVIEW->viewport;
	px = muldiv (vp->z, vp->shift, vp->distz);	/* normalized */
	px = muldiv (VS->sizex, px, vp->maxx) / 2;	/* pixels */
	px = fmul (px, VW->maxx) + (px<0);

	if (orgx)
		*orgx = fmul (VW->orgx, VS->sizex) + VS->left + px;
	if (orgy)
		*orgy = fmul (VW->orgy, VS->sizey) + VS->top;
	if (maxx)
		*maxx = fmul (VW->maxx, VS->sizex) - 1 - iabs(px);
	if (maxy)
		*maxy = fmul (VW->maxy, VS->sizey) - 1;
}

/* Get largest square that fits in window.
*/
extern void FAR
get_square (VIEW *view, int maxx, int maxy, int *sizex, int *sizey)
{
	int	sx, sy;

	if (!VD)
		view = CVIEW;

	sx = muldiv (VD->lengx, maxx, VD->sizex);
	sy = muldiv (VD->lengy, maxy, VD->sizey);

	if (sx > sy) {		/* wide window */
		sx = muldiv (maxx, sy, sx);
		sy = maxy;
	} else {		/* high window */
		sy = muldiv (maxy, sx, sy);
		sx = maxx;
	}
	if (sizex)
		*sizex = sx;
	if (sizey)
		*sizey = sy;
}

extern int FAR
scenery (int type)
{
	return (HDT_NONE != type && HDT_HUD != type && HDT_UPFRONT != type &&
		HDT_PANEL != type && HDT_LAMPS != type && HDT_STORES != type);
}

LOCAL_FUNC void NEAR
swap_view (short old, short new)
{
	int	i;

	st.hdd[0].type = old;
	for (i = 1; i < NHDD; ++i) {
		if ((st.hdd[i].flags & HDF_ON) && st.hdd[i].type == old) {
			st.hdd[i].type = new;
			break;
		}
	}
}

extern OBJECT * FAR
get_viewer (int type)
{
	OBJECT	*pov;

	for (pov = CO; pov; pov = pov->next) {
		if (pov->name == O_VIEWER && pov->misc[0] == type)
			break;
	}
	return (pov);
}

extern void FAR
save_viewport (OBJECT *p)
{
	if (!p->viewport) {
		if (F(NEW (p->viewport)))
			return;
	}
	memcpy (p->viewport, CP, sizeof (*p->viewport));
}

extern void FAR
get_viewport (OBJECT *p)
{
	if (p->viewport)
		memcpy (CP, p->viewport, sizeof (*CP));
	else {
		CP->flags = 0;
		CP->x     = 0*FONE;		/* viewer at center */
		CP->y     = 0*FONE;
		CP->shift = 0;
		CP->eyex  = 0;
		CP->eyey  = 0;
		CP->eyez  = 0;
		CP->rotx  = 0;
		CP->roty  = 0;
		CP->rotz  = 0;
	}
}

extern void FAR
update_viewer (void)
{
	OBJECT *p;

	if (T(p = get_viewer (0)))
		save_viewport (p);
	save_viewport (CV);
}

extern void FAR
swap_extview (void)
{
	OBJECT		*p;

	if (scenery (st.extview)) {
		if (F(p = get_viewer (st.extview))) {
			MsgWPrintf (50, "No viewer");
			return;
		}
	} else
		p = 0;

	sim_set ();
	screen_empty ();

	if (st.flags1 & SF_EXTVIEW) {
		if (p) {
			save_viewport (p);
			get_viewport (CV);
		}
		swap_view (HDT_FRONT, st.extview);
	} else {
		if (p) {
			save_viewport (CV);
			get_viewport (p);
		}
		swap_view (st.extview, HDT_FRONT);
	}

	st.flags1 ^= SF_EXTVIEW;
	screen_start ();
	sim_reset ();
}


#define SIF_BG		0x0001
#define SIF_FG		0x0002
#define SIF_HUD		0x0004
#define SIF_CLEAR	0x0008
#define SIF_ALL		(SIF_CLEAR|SIF_HUD|SIF_FG|SIF_BG)

LOCAL_FUNC void NEAR
show_hdd (HDD *hdd, VIEW *view, OBJECT *pov, int flags)
{
	E_PLANE		*ecv = NULL;
	VIEWPORT	*save_vp = NULL;
	OBJECT		*p;
	int		vr, wr, smaxx = 0, smaxy = 0;
	int		orgx, orgy, maxx, maxy, ss;
	int		type, scene, clear, solid;
	Ushort		saved = 0, shud = 0, shud1 = 0, shdd = 0;

	if (IS_PLANE(CV))
		p = CV;
	else
		p = CC;
	if (!p)
		return;

/* adjust viewport aspect ratio. turn off HUD for auxiliay HDDs.
*/
	do /* once */ {
		if (st.flags & SF_MAIN)
			break;
		if (HDT_HUD == hdd->type) {
			pov = p;
			save_vp = VP;
			VP = CP;
			saved = 1;
			break;
		}
		if (F(pov = get_viewer ((HDT_HUD == hdd->type)
						? HDT_FRONT : hdd->type)))
			break;
		if (!pov->viewport) {
			pov = NULL;
			break;
		}

		saved = 2;
		ecv = IS_PLANE(CV) ? EE(CV) : 0;
		VP = pov->viewport;
		smaxx = VP->maxx;
		smaxy = VP->maxy;
		if (ecv) {
			shud  = ecv->hud;
			shud1 = ecv->hud1;
			shdd  = ecv->hdd;
			ecv->hud  &= ~(HUD_ON|HUD_CURSOR|HUD_RETICLE);
			ecv->hud1 &= ~HUD_VALARM;
			ecv->hdd  &= ~HDD_PANEL;
		}

/* Adjust aspect ratio. All viewports use the same aspect ratio as the main
 * window, now adjust the viewport for the actual window.
 * The 1/4 scaling is done to allow handling high aspect ratios.
*/
		vr = muldiv (FONE/4, VP->maxx, VP->maxy);
		wr = muldiv (FONE/4, VW->maxx, VW->maxy);
		wr = muldiv (wr, VD->lengx, VD->lengy);
#if 0
		if (vr > wr)	/* viewport too wide */
			VP->maxx = muldiv (VP->maxx, wr, vr);
		else		/* viewport too hight */
			VP->maxy = muldiv (VP->maxy, vr, wr);
#else
		if (vr > wr)	/* viewport wider than window */
			VP->maxy = muldiv (VP->maxy, vr, wr);
		else		/* viewport higher than window */
			VP->maxy = muldiv (VP->maxy, vr, wr);
#endif
	} while (0);

	get_area (view, &orgx, &orgy, &maxx, &maxy);
	if (maxx <= 0 || maxy <= 0)
		return;

/* establish a good font size
*/
	ss = fmul (maxx, FCON (0.04));
	ss = muldiv (ss, st.StFontSize, 8);
	ss = (ss+1)&~1;			/* make it even */
	if (ss < 8)
		ss = 8;

	type = hdd->type;
	scene = scenery (type);
	solid = (st.flags & SF_SKY) && (st.flags1 & SF_SOLIDSKY) && scene;
	clear = st.flags2 & (SF_CLEAR1 << st.which_buffer);

/* show generic components, first clear the window if necessary
*/
	if (flags & SIF_BG) {
		if ((flags & SIF_CLEAR) && !solid
					&& (clear || (st.flags&SF_USECLEAR))) {
			gr_op (T_MPUSH);
			gr_op (T_MSET);
			gr_color (hdd->BgColor);
			gr_clear (orgx-maxx, orgy-maxy, 2*maxx+1, 2*maxy+1);
			gr_op (T_MPOP);
		}

/* show/clear border if necessary
*/
		if (clear) {
			gr_op (T_NOERASE);
			gr_op (T_MPUSH);
			gr_op (T_MSET);
			gr_color ((st.flags & SF_BLANKER)
					 ? hdd->BgColor : hdd->BoColor);
			gr_move (orgx - maxx-1, orgy - maxy-1);
			gr_draw (orgx + maxx+1, orgy - maxy-1);
			gr_draw (orgx + maxx+1, orgy + maxy+1);
			gr_draw (orgx - maxx-1, orgy + maxy+1);
			gr_draw (orgx - maxx-1, orgy - maxy-1);
			gr_op (T_MPOP);
			gr_op (T_ERASE);
		}

		if (pov && scene) {
			show_sky (view, pov);
			objects_show (0, view, pov, 0, 0);
		}
	}

/* HUD, radar, intel etc.
*/
	if (pov && (flags & SIF_HUD))
		show_hud (view, pov, p, orgx, orgy, maxx, maxy, type);

/* overlaid displays.
*/
	if (st.flags & SF_MAIN) {
		if ((flags & SIF_FG) && IS_PLANE (p)) {
			if (EX->hdd & HDD_INSTRUMENTS)
				show_inst (view, p);
			if (EX->hdd & HDD_PANEL)
				show_panel (view, p, maxx, maxy, orgx, orgy,
					ss);
		}
	}

/* status info. This text is shown over the whole screen. It will not show if
 * there is an active UPFRONT HDD.
*/
	if (flags & SIF_FG) {
		screen_info (view, orgx, orgy, maxx, maxy, ss, type);
		ident (hdd, orgx, orgy, maxx, maxy, ss);
	}

/* restore viewport.
*/
	switch (saved) {
	case 1:
		VP = save_vp;
		break;
	case 2:
		VP->maxx = smaxx;
		VP->maxy = smaxy;
		if (ecv) {
			ecv->hud  = shud;
			ecv->hud1 = shud1;
			ecv->hdd  = shdd;
		}
		VP = NULL;
		break;
	default:
		break;
	}

/* show specifics
*/
	switch (hdd->type) {
	case HDT_NONE:
		break;
	case HDT_UPFRONT:
		if (hdd_menu (view, orgx, orgy, maxx, maxy))
			screen_info (view, orgx, orgy, maxx, maxy, ss, 
				hdd->type);
		break;
	case HDT_PANEL:
		show_panel (view, CC, maxx, maxy, orgx, orgy, ss);
		break;
	case HDT_STORES:
		show_stores (view, CC, maxx, maxy, orgx, orgy, ss);
		break;
	case HDT_LAMPS:
		show_lamps (view, CC, maxx, maxy, orgx, orgy, ss);
		break;
	default:
		break;
	}
}

LOCAL_FUNC void NEAR
render_bg (OBJECT *pov, int frame, int hudinfront)
{
	show_hdd (st.hdd, CVIEW, pov, SIF_BG
				| ((frame && !hudinfront) ? SIF_HUD : 0)
				| ((2 != frame ) ? SIF_CLEAR : 0));
}

LOCAL_FUNC void NEAR
render_fg (OBJECT *pov, int frame, int hudinfront)
{
	show_hdd (st.hdd, CVIEW, pov,
			SIF_FG | ((frame && !hudinfront) ? 0: SIF_HUD));
}

LOCAL_FUNC void NEAR FASTCALL
show_main (HDD *hdd)
{
	ANGLE	cross;				/* stereo cross-eyed angle */
	int	rev, shift, rotz, scene, hudinfront;
	int	sscopic_shift = 0;
	int	frame;				/* 0=mono 1=left 2=right */
	OBJECT	*pov;
	int	save_colors[NCOLORS];
	int	i;
	int	color;

	st.flags |= SF_MAIN;

#if 1
	i = (HDT_HUD == hdd->type);
	scene = scenery (hdd->type);
	if (st.flags1 & SF_EXTVIEW) {		/* find viewer */
		if (scene || i) {
			pov = get_viewer (i ? HDT_FRONT : hdd->type);
			if (!pov)
				pov = CV;
		} else
			pov = CV;
	} else {
		pov = CV;
		scene = 1;
	}
#else
	pov = CV;
	scene = scenery (hdd->type);
#endif

	cross = 0;	/* ASIN (fdiv (st.paralax, st.focus)); */

	if (st.stereo) {
		rev = st.flags1 & SF_STEREOREV;
		if (!scene)
			hudinfront = 1;
		else
			hudinfront = st.flags1 & SF_HUDINFRONT;
		if (VIS_ALTERNATING == st.stereo && st.which_buffer)
			rev = !rev;
		if (rev) {
			shift = -st.paralax;	/* transverse: right eye */
			rotz  = -cross;
		} else {
			shift = st.paralax;	/* parallel:   left eye */
			rotz  = cross;
		}
		CP->shift -= shift;		/* left eye */
		CP->rotz  += rotz;
	} else {
		shift = rotz = 0;		/* avoid compiler warning */
		hudinfront = 0;
	}

	if (VIS_REDBLUE == st.stereo) {
		memcpy (save_colors, st.colors, sizeof (save_colors));
		gr_op (T_MOR);
	} else
		gr_op (T_MSET);

/* First we show the left eye view. This is also used to show the single view
 * (mono or alternating stereo).
*/
	frame = VIS_REDBLUE == st.stereo ? 1 : 0;	/* mono/left frame */
	if (frame) {
		color = save_colors[ST_SLEFT];
		for (i = 0; i < rangeof (st.colors); ++i)
			st.colors[i] = color;
		st.colors[CC_BLACK] = save_colors[CC_BLACK];
	}

	if (scene)
		render_bg (pov, frame, hudinfront);

	if (st.stereo && hudinfront) {
		CP->shift += shift;			/* back to center */
		CP->rotz  -= rotz;
	}

#if 0
	if (VIS_REDBLUE != st.stereo)
#endif
		render_fg (pov, frame, hudinfront);

	if (st.stereo && !hudinfront) {
		CP->shift += shift;			/* back to center */
		CP->rotz  -= rotz;
	}
		
/* Second we show the right eye view (for non-alternating stereo).
*/
	if (st.stereo && VIS_ALTERNATING != st.stereo) {
		if (VIS_STEREOSCOPIC == st.stereo) {
			sscopic_shift = muldiv (CW->maxx, 2*st.gap+2, st.gap-1);
			CW->orgx += sscopic_shift;
		}

		CP->shift += shift;			/* right eye */
		CP->rotz  -= rotz;

		frame = VIS_REDBLUE == st.stereo ? 2 : 0; /* right frame */
		if (frame) {
			color = save_colors[ST_SRIGHT];
			for (i = 0; i < rangeof (st.colors); ++i)
				st.colors[i] = color;
			st.colors[CC_BLACK] = save_colors[CC_BLACK];
		}
		if (scene)
			render_bg (pov, frame, hudinfront);

		if (hudinfront) {
			CP->shift -= shift;		/* back to center */
			CP->rotz  += rotz;
			if (VIS_REDBLUE == st.stereo) {
				color = save_colors[ST_SBOTH];
				for (i = 0; i < rangeof (st.colors); ++i)
					st.colors[i] = color;
				st.colors[CC_BLACK] = save_colors[CC_BLACK];
			}
		}

		render_fg (pov, frame, hudinfront);

		if (!hudinfront) {
			CP->shift -= shift;		/* back to center */
			CP->rotz  += rotz;
		}

		if (VIS_STEREOSCOPIC == st.stereo)
			CW->orgx -= sscopic_shift;
	}

	if (VIS_REDBLUE == st.stereo) {
		color = save_colors[ST_SBOTH];
		for (i = 0; i < rangeof (st.colors); ++i)
			st.colors[i] = color;
		st.colors[CC_BLACK] = save_colors[CC_BLACK];
	}

	if (VIS_REDBLUE == st.stereo)
		memcpy (st.colors, save_colors, sizeof (st.colors));

	st.flags &= ~SF_MAIN;
}

extern void FAR
render_picture (void)
{
	int	i;
	long	t27;

	t27 = STATS_TIME3D;
	Tm->Interval (TMR_START, 0L);

	if (st.flags2 & (SF_CLEAR1 << st.which_buffer)) {
		gr_op (T_NOERASE);
		gr_op (T_MPUSH);
		gr_op (T_MSET);
		gr_color (CS->BgColor);
		gr_clear (CS->left, CS->top, CS->sizex, CS->sizey);
		gr_op (T_MPOP);
		gr_op (T_ERASE);
	}

	for (i = 0; i < NHDD; ++i) {
		if (!(st.hdd[i].flags & HDF_ON))
			continue;

		if (0 == i)
			show_main (&st.hdd[i]);
		else
			show_hdd (&st.hdd[i], &st.hdd[i].view, NULL,
				SIF_ALL);

		buffer_close ();
		st.hdd[i].bufs[1-st.which_buffer] = st.buf[HEAD];
		st.buf[HEAD] = st.buf[TAIL] = 0;
	}

	st.flags2 &= ~(SF_CLEAR1 << st.which_buffer);

	STATS_TIMEHDD += Tm->Interval (TMR_STOP, 10000L)
						- (STATS_TIME3D - t27);
	STATS_DISPLAYLISTSIZE = buffers_size (1-st.which_buffer);
}
