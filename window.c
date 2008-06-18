/* --------------------------------- window.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Manage the different windows.
*/

#include "fly.h"


extern void FAR
windows_term (void)
{
	int	i;

	for (i = 0; i < NHDD; ++i) {
		if (!(st.hdd[i].flags & HDF_ON))
			continue;
		st.hdd[i].type = HDT_NONE;
		st.hdd[i].flags = 0;
		DEL0 (st.hdd[i].view.window);
		DEL0 (st.hdd[i].view.viewport);
		st.hdd[i].view.screen = 0;
	}
}

LOCAL_FUNC void NEAR
window_set (int i, int type, int orgx, int orgy, int maxx, int maxy)
{
	WINDOW	*w;

	if (F(w = NEW (st.hdd[i].view.window))) {
		MsgEPrintf (-50, "hdd alloc failed");
		if (0 == i)
			die ();
		return;
	}
	st.hdd[i].type = type;
	st.hdd[i].flags = HDF_ON;
	st.hdd[i].view.viewport = 0;
	st.hdd[i].view.screen = CS;
	st.hdd[i].FgColor = CC_WHITE;
	st.hdd[i].BgColor = CC_BLACK;
	st.hdd[i].BoColor = CC_LGRAY;

	w->orgx = orgx;
	w->orgy = orgy;
	w->maxx = maxx;
	w->maxy = maxy;

	if (0 == i) {
		CW->orgx = w->orgx;
		CW->orgy = w->orgy;
		CW->maxx = w->maxx;
		CW->maxy = w->maxy;
		if (VIS_STEREOSCOPIC == st.stereo)
			set_small_frame ();
	}
}

extern void FAR
windows_set (void)
{
	windows_term ();

	switch (st.windows) {
	case WIN_LANDSCAPE:
		window_set (0, HDT_FRONT, FONE/2,   FONE/3,   FONE/2, FONE/3);
		window_set (1, HDT_STORES,FONE/8,   FONE/6*5, FONE/8, FONE/6);
		window_set (2, HDT_MIRROR,FONE/2,   FONE/6*5, FONE/4, FONE/6);
		window_set (3, HDT_RADAR, FONE/8*7, FONE/6*5, FONE/8, FONE/6);
		break;
	case WIN_PORTRATE:
		window_set (0, HDT_FRONT, FONE/3,   FONE/2,   FONE/3, FONE/2);
		window_set (1, HDT_REAR,  FONE/6*5, FONE/3,   FONE/6, FONE/3);
		window_set (2, HDT_RADAR, FONE/6*5, FONE/6*5, FONE/6, FONE/6);
		break;
	case WIN_SQUARE:
		window_set (0, HDT_FRONT, FONE/8*3, FONE/2, FONE/8*3, FONE/2);
		window_set (1, HDT_REAR,  FONE/8*7, FONE/6,   FONE/8, FONE/6);
		window_set (2, HDT_PAN,   FONE/8*7, FONE/6*3, FONE/8, FONE/6);
		window_set (3, HDT_RADAR, FONE/8*7, FONE/6*5, FONE/8, FONE/6);
		break;
	case WIN_WIDE:
		window_set (0, HDT_FRONT, FONE/2,   FONE/3,   FONE/2, FONE/3);
		window_set (1, HDT_REAR,  FONE/8,   FONE/6*5, FONE/8, FONE/6);
		window_set (2, HDT_STORES,FONE/8*3, FONE/6*5, FONE/8, FONE/6);
		window_set (3, HDT_PAN,   FONE/8*5, FONE/6*5, FONE/8, FONE/6);
		window_set (4, HDT_RADAR, FONE/8*7, FONE/6*5, FONE/8, FONE/6);
		break;
	case WIN_PANORAMA:
		window_set (0, HDT_FRONT, FONE/6*3, FONE/3,   FONE/6, FONE/3);
		window_set (1, HDT_LEFT,  FONE/6*1, FONE/3,   FONE/6, FONE/3);
		window_set (2, HDT_RIGHT, FONE/6*5, FONE/3,   FONE/6, FONE/3);
		window_set (3, HDT_REAR,  FONE/8,   FONE/6*5, FONE/8, FONE/6);
		window_set (4, HDT_STORES,FONE/8*3, FONE/6*5, FONE/8, FONE/6);
		window_set (5, HDT_PAN,   FONE/8*5, FONE/6*5, FONE/8, FONE/6);
		window_set (6, HDT_RADAR, FONE/8*7, FONE/6*5, FONE/8, FONE/6);
		break;
	case WIN_ETHER:
		window_set (0, HDT_FRONT,	FCON(.50),	FCON(.30),
						FCON(0.485),	FCON(.28));
		window_set (1, HDT_STORES,	FCON(0.17),	FCON(.80),
						FCON(0.17),	FCON(.20));
		window_set (2, HDT_RADAR,	FCON(0.51),	FCON(.80),
						FCON(0.17),	FCON(.20));
		window_set (3, HDT_PAN,		FCON(0.84),	FCON(.87),
						FCON(0.16),	FCON(.13));
		window_set (4, HDT_LAMPS,	FCON(0.84),	FCON(.67),
						FCON(0.16),	FCON(.07));
		window_set (5, HDT_NONE,	FCON(0.50),	FCON(.30),
						FCON(0.50),	FCON(.30));
		break;
	case WIN_FULL:
	default:
		window_set (0, HDT_FRONT,	FONE/2,		FONE/2,
						FONE/2,		FONE/2);
		break;
	}
	win_setup ();
}

/* As of now, the device and the screen cover the same area, so a resize
 * affects both. It would be better if we would regard the device as the
 * full available drawing area (the full screen) and use the screen to
 * describe that rectangle which the program is currently using. A resize
 * then will only affect the screen.
*/
extern void FAR
set_screen (int sizex, int sizey)
{
	LogPrintf ("set screen to %dx%d\n", sizex, sizey);

/* adjust the device parameters.
*/
	if (CD->sizex && CD->sizey) {
		CD->lengx = muldiv (CD->lengx, sizex, CD->sizex);
		CD->lengy = muldiv (CD->lengy, sizey, CD->sizey);
	}

/* leave 'left' and 'top' unchanged.
*/
	CD->sizex  = sizex;
	CD->right  = CD->left + CD->sizex - 1;
	CD->sizey  = sizey;
	CD->bottom = CD->top  + CD->sizey - 1;

/* adjust the screen to use the full device. Since many 2D functions draw
 * directly to pixel values, we will allocate a symmetrical screen so that
 * a pixel can be exactly at the center of the screen.
*/
	CS->sizex = (CD->sizex - 1)/2*2;	/* round down */
	CS->left  = CD->left;

	CS->sizey = (CD->sizey - 1)/2*2;
	CS->top   = CD->top;

	repaint ();
}

LOCAL_FUNC void NEAR
adjust_viewports (void)
{
	int		t;
	OBJECT		*v;
	VIEWPORT	*vp;

/* Adjust all viewports to main window's aspect ratio. This adjustment is
 * necessary because the main window and the main viewport are correctly
 * adjusted at all times. When selecting an external-view the viewports are
 * swapped, and if the new viewport has the wrong aspect ratio then the
 * main view is distorted. Note that HDD displays dynamicaly adjust the
 * aspect ratios just before using it [views.c: show_view()].
*/
	for (v = CO; v; v = v->next) {
		if (T(vp = v->viewport) && vp != CP) {
			vp->maxx  = CP->maxx;
			vp->maxy  = CP->maxy;
			vp->distz = CP->distz;
			vp->x     = CP->x;
			vp->y     = CP->y;
			t = vp->zoom - CP->zoom;
			vp->zoom  = CP->zoom;
			zoom (vp, t);
		}
	}
}

/* Set the viewport parameters. These are always relative to the main
 * screen/window.
*/
LOCAL_FUNC void NEAR
set_viewport (VIEWPORT *vp)
{
	int	tx, ty;

	if (CD->lengx == 0 || CD->lengy == 0) {
		LogPrintf ("null viewport\n");
		die ();
	}
	tx = fmul (CS->sizex, CW->maxx);
	tx = muldiv (FONE, tx, CD->sizex);
	ty = fmul (CS->sizey, CW->maxy);
	ty = muldiv (FONE, ty, CD->sizey);
	if (CD->lengx > CD->lengy)
		ty = muldiv (ty, CD->lengy, CD->lengx);
	else
		tx = muldiv (tx, CD->lengx, CD->lengy);

	vp->maxx = tx;			/* maps world to viewport */
	vp->maxy = ty;
	vp->distz = 1*VONE*VONE;
#if 0
	if (tx < ty)
		vp->z = 2*tx;		/* 45 degrees view in x */
	else
		vp->z = 2*ty;		/* 45 degrees view in y */
#else
	vp->z = 2*tx;			/* always 45 degrees view in x */
#endif
	vp->x = 0*FONE;			/* viewer at center */
	vp->y = 0*FONE;
	vp->zoom = 0;
}

extern void FAR
set_main (void)
{
	int	i;

	set_screen (CD->sizex, CD->sizey);
	windows_set ();
	i = CP->zoom;
	set_viewport (CP);
	zoom (CP, i);
	adjust_viewports ();
}

extern void FAR
set_small_frame (void)
{
	CW->orgx -= CW->maxx;		/* left frame */
	CW->maxx = muldiv (CW->maxx, st.gap-1, 2*st.gap);
	CW->orgx += CW->maxx;

	CP->maxx = muldiv (CP->maxx, st.gap-1, 2*st.gap);
	CP->z	 = muldiv (CP->z, st.gap-1, 2*st.gap);

	adjust_viewports ();
}

extern void FAR
set_large_frame (void)
{
	CW->orgx -= CW->maxx;
	if (CW->maxx > FONE/2)
		CW->maxx = FONE;
	else
		CW->maxx = muldiv (CW->maxx, 2*st.gap, st.gap-1);
	CW->orgx += CW->maxx;

	if (CP->maxx > FONE/2)
		CP->maxx = FONE;
	else
		CP->maxx = muldiv (CP->maxx, 2*st.gap, st.gap-1);
	if (CP->z > FONE/2)
		CP->z = FONE;
	else
		CP->z = muldiv (CP->z, 2*st.gap, st.gap-1);

	adjust_viewports ();
}


#define	ZOOMIN		FCON(1.26)	/* cubic root of 2.0 */
#define	ZOOMOUT		FCON(1/1.26)

extern void FAR
zoom (VIEWPORT *vp, int zoom)
{
	if (!vp || !zoom)
		return;

	zoom += vp->zoom;
	set_viewport (vp);

	for (; zoom > 0; ++vp->zoom, --zoom) {
		if (vp->z < ZOOMOUT && vp->distz < VMAX/2) {
			vp->z = fmul (vp->z, ZOOMIN);
			vp->distz = fmul (vp->distz, ZOOMIN);
		} else if (vp->maxx > FONE/128 && vp->maxy > FONE/128) {
			vp->maxx = fmul (vp->maxx, ZOOMOUT);
			vp->maxy = fmul (vp->maxy, ZOOMOUT);
		} else
			break;
	}

	for (; zoom < 0; --vp->zoom, ++zoom) {
		if (vp->maxx < ZOOMOUT && vp->maxy < ZOOMOUT) {
			vp->maxx = fmul (vp->maxx, ZOOMIN);
			vp->maxy = fmul (vp->maxy, ZOOMIN);
		} else if (vp->z > FONE/128 &&  vp->distz > VONE) {
			vp->z = fmul (vp->z, ZOOMOUT);
			vp->distz = fmul (vp->distz, ZOOMOUT);
		} else
			break;
	}
}

#undef ZOOMIN
#undef ZOOMOUT

extern int FAR
window_select (void)
{
	int	ch;
	int	save_flags1;
	HMSG	*m;

	save_flags1 = st.flags1 & SF_IDENT;
	st.flags1 |= SF_IDENT;
	for (;;) {
		m = MsgWPrintf (0, "Which window [x for extview] ?");
		ch = mgetch ();
		msg_del (m);
		if (KF_ESC == ch || K_ENTER == ch) {
			ch = -1;
			break;
		}
		if ('x' == ch)
			break;
		ch -= '0';
		if (ch >= 0 && ch <= NHDD &&
		    (!ch || st.hdd[ch].flags & HDF_ON))
			break;
	}
	st.flags1 &= ~SF_IDENT;
	st.flags1 |= save_flags1;
	return (ch);
}

static MENU MenuWin[] = {
	{'c', "configure"},	/*  0 */
	{'b', "bg color"},	/*  1 */
	{'f', "fg color"},	/*  2 */
	{'o', "bo color"},	/*  3 */
	{'1', "full"},		/*  4 */
	{'2', "landscape"},	/*  5 */
	{'3', "portrait"},	/*  6 */
	{'4', "square"},	/*  7 */
	{'5', "wide"},		/*  8 */
	{'6', "panorama"},	/*  9 */
	{'e', "ether"},		/* 10 */
{'\0', 0}};

extern int FAR
menu_windows (void)
{
	int	sel, i, ret;
	int	hdd;
	Ushort	*item;

	for (ret = 0; !ret;) {
		sel = menu_open (MenuWin, st.windows+3);

		switch (sel) {
		case MENU_ABORTED:
		case MENU_FAILED:
			ret = 1;
			break;
		case 0:
		case 1:
		case 2:
		case 3:
		    for (;;) {
			hdd = window_select ();
			if (hdd < 0)
				break;
			if (0 == sel) {
				if ('x' == hdd) {
					if (st.flags1 & SF_EXTVIEW)
						swap_extview ();
					i = st.extview;
				} else
					i = st.hdd[hdd].type;
				i = menu_view (i);

				if (i >= 0) {
					if ('x' == hdd)
						st.extview = i;
					else
						st.hdd[hdd].type = i;
				}
			} else {
				if ('x' == hdd) {
					MsgEPrintf (5, "select a real window");
					continue;
				}
				switch (sel) {
				default:
				case 1:
					item = hdd ? &st.hdd[hdd].BgColor
								: &CS->BgColor;
					break;
				case 2:
					item = hdd ? &st.hdd[hdd].FgColor
								: &CS->FgColor;
					break;
				case 3:
					item = hdd ? &st.hdd[hdd].BoColor
								: &CS->BoColor;
					break;
				}
				color_assign (item);
			}
			sim_set ();
			screen_empty ();
			screen_start ();
			sim_reset ();
		    }
		    ret = 1;
		    break;
		default:
			sim_set ();
			screen_empty ();
			st.windows = sel - 3;
			set_main ();
			screen_start ();
			sim_reset ();
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}
