/* --------------------------------- hudmenu.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* HUD Menus.
*/

#include "plane.h"


/* Select HUD type
*/

static MENU FAR	MenuHudType[] = {
	{'0', "Classic"},	/*  0 */
	{'1', "FA18"},		/*  1 */
	{'2', "F16"},		/*  2 */
	{'3', "F15"},		/*  3 */
	{'4', "Ether"},		/*  4 */
{'\0', 0}};

extern int FAR
menu_hudtype (void)
{
	int	sel;

	sel = (EE(CV)->hud1&HUD_TYPES)/HUD_TYPE;
	sel = menu_open (MenuHudType, sel);

	switch (sel) {
	case MENU_ABORTED:
	case MENU_FAILED:
		break;
	default:
		EE(CV)->hud1 = (EE(CV)->hud1 & ~HUD_TYPES) | (sel*HUD_TYPE);
		hud_setup (CV);
		break;
	}
	if (MENU_FAILED != sel)
		menu_close ();

	return (0);
}

/* Select ladder sizes;
*/

static MENU FAR	MenuLdSize[] = {
	{'g', "gap"},		/*  0 */
	{'s', "step"},		/*  1 */
	{'h', "horizon"},	/*  2 */
	{'l', "land"},		/*  3 */
	{'t', "tip"},		/*  4 */
	{'n', "ndash"},		/*  5 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_ldsize (void)
{
	int	sel, quit;

	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuLdSize, 0);
		switch (sel) {
		case MENU_ABORTED:
		default:
			quit = 1;
			break;
		case 0:
			get_num ("gap size", &EE(CV)->ldgap, MT_XSHORT,
					0, FONE, FONE/128);
			break;
		case 1:
			get_num ("step size", &EE(CV)->ldstep, MT_XSHORT,
					0, FONE, FONE/128);
			break;
		case 2:
			get_num ("horizon size", &EE(CV)->ldstep0, MT_XSHORT,
					0, FONE, FONE/128);
			break;
		case 3:
			get_num ("land size", &EE(CV)->ldstepg, MT_XSHORT,
					0, FONE, FONE/128);
			break;
		case 4:
			get_num ("tip size", &EE(CV)->ldtip, MT_XSHORT,
					0, FONE, FONE/128);
			break;
		case 5:
			get_num ("ndash", &EE(CV)->ldndash, MT_XSHORT,
					0, 8, 1);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}
	return (0);
}

/* Select ladder style
*/

static MENU FAR	MenuLadder[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
	{'l', "ladder"},	/*  3 */
	{'p', "pinned"},	/*  4 */
	{'r', "right"},		/*  5 */
	{'e', "erect"},		/*  6 */
	{'c', "color"},		/*  7 */
	{'f', "funnel"},	/*  8 */
	{'s', "slant"},		/*  9 */
	{'z', "zenith"},	/* 10 */
	{'u', "under"},		/* 11 */
	{'t', "tip0"},		/* 12 */
	{'h', "hold"},		/* 13 */
	{'R', "h roll"},	/* 14 */
	{'S', "sun"},		/* 15 */
	{'i', "negtip"},	/* 16 */
	{'x', "sizes"},		/* 17 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_ladder (void)
{
	int	sel, quit;

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuLadder, 0);
		switch (sel) {
		case MENU_ABORTED:
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			SetOption (&EE(CV)->hud, HUD_LADDER);
			break;
		case 4:
			SetOption (&EE(CV)->ladder, LD_FIXED);
			break;
		case 5:
			SetOption (&EE(CV)->ladder, LD_RIGHT);
			break;
		case 6:
			SetOption (&EE(CV)->ladder, LD_ERECT);
			break;
		case 7:
			SetOption (&EE(CV)->ladder, LD_COLOR);
			break;
		case 8:
			SetOption (&EE(CV)->ladder, LD_FUNNEL);
			break;
		case 9:
			SetOption (&EE(CV)->ladder, LD_SLANT);
			break;
		case 10:
			SetOption (&EE(CV)->ladder, LD_ZENITH);
			break;
		case 11:
			SetOption (&EE(CV)->ladder, LD_UNDER);
			break;
		case 12:
			SetOption (&EE(CV)->ladder, LD_TIP0);
			break;
		case 13:
			SetOption (&EE(CV)->ladder, LD_HOLD);
			break;
		case 14:
			SetOption (&EE(CV)->ladder, LD_HOLDROLL);
			break;
		case 15:
			SetOption (&EE(CV)->ladder, LD_SUN);
			break;
		case 16:
			SetOption (&EE(CV)->ladder, LD_NEGTIP);
			break;
		case 17:
			menu_ldsize ();
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}
	return (0);
}

/* Select HUD parts (x)
*/

static MENU FAR	MenuHudGround[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
	{'p', "gnd ptr"},	/*  3 */
	{'x', "xbreak"},	/*  4 */
	{'v', "xvar"},		/*  5 */
	{'g', "xgrid"},		/*  6 */
	{'u', "pullup"},	/*  7 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_hud_ground (void)
{
	int	sel, quit;

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuHudGround, 0);
		switch (sel) {
		case MENU_ABORTED:
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			SetOption (&EE(CV)->hud1, HUD_PENDULUM);
			break;
		case 4:
			SetOption (&EE(CV)->hud2, HUD_XBREAK);
			break;
		case 5:
			SetOption (&EE(CV)->hud3, HUD_XVAR);
			break;
		case 6:
			SetOption (&EE(CV)->hud2, HUD_XGRID);
			break;
		case 7:
			SetOption (&EE(CV)->hud3, HUD_CUE);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}

/* Select HUD parts
*/

static MENU FAR	MenuHudParts[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
	{'l', "ladder"},	/*  3 */
	{'a', "altitude"},	/*  4 */
	{'s', "speed"},		/*  5 */
	{'h', "heading"},	/*  6 */
	{'b', "border"},	/*  7 */
	{'v', "vv"},		/*  8 */
	{'w', "vw"},		/*  9 */
	{'+', "plus"},		/* 10 */
	{'P', "pointer"},	/* 11 */
	{'B', "beta"},		/* 12 */
	{'g', "ground"},	/* 13 */
	{'d', "director"},	/* 14 */
	{'y', "waypoint"},	/* 15 */
	{'t', "tracers"},	/* 16 */
	{'G', "ghost"},		/* 17 */
	{'T', "truehead"},	/* 18 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_hudparts (void)
{
	int	sel, quit;

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuHudParts, 0);
		switch (sel) {
		case MENU_ABORTED:
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			menu_ladder ();
			quit = 1;
			break;
		case 4:
			SetOption (&EE(CV)->hud2, HUD_ALTITUDE);
			break;
		case 5:
			SetOption (&EE(CV)->hud2, HUD_SPEED);
			break;
		case 6:
			SetOption (&EE(CV)->hud2, HUD_HEADING);
			break;
		case 7:
			SetOption (&EE(CV)->hud1, HUD_BORDER);
			break;
		case 8:
			SetOption (&EE(CV)->hud, HUD_VV);
			break;
		case 9:
			SetOption (&EE(CV)->hud2, HUD_VW);
			break;
		case 10:
			SetOption (&EE(CV)->hud, HUD_PLUS);
			break;
		case 11:
			SetOption (&EE(CV)->hud, HUD_CURSOR);
			break;
		case 12:
			SetOption (&EE(CV)->hud2, HUD_BETA);
			break;
		case 13:
			menu_hud_ground ();
			quit = 1;
			break;
		case 14:
			SetOption (&EE(CV)->hud2, HUD_DIRECTOR);
			break;
		case 15:
			SetOption (&EE(CV)->hud2, HUD_WAYPOINT);
			break;
		case 16:
			SetOption (&EE(CV)->hud2, HUD_BTRAIL);
			break;
		case 17:
			SetOption (&EE(CV)->hud3, HUD_GVV);
			break;
		case 18:
			SetOption (&EE(CV)->hud3, HUD_TRUEHEADING);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}

/* Select HUD options.
*/

static MENU FAR	MenuHudOptions1[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
	{'3', "heading"},	/*  3 */
	{'k', "knots"},		/*  4 */
	{'T', "top"},		/*  5 */
	{'f', "fine"},		/*  6 */
	{'x', "xfine"},		/*  7 */
	{'b', "big"},		/*  8 */
	{'l', "scale"},		/*  9 */
	{'a', "area"},		/* 10 */
	{'c', "cas"},		/* 11 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_hudoptions1 (void)
{
	int	sel, quit, ch;
	char	msg[80], prompt[80];

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuHudOptions1, 0);
		switch (sel) {
		case MENU_ABORTED:
		case MENU_FAILED:
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			SetOption (&EE(CV)->hud, HUD_FULLHEADING);
			break;
		case 4:
			SetOption (&EE(CV)->hud1, HUD_KNOTS);
			break;
		case 5:
			SetOption (&EE(CV)->hud1, HUD_TOP);
			break;
		case 6:
			SetOption (&EE(CV)->hud, HUD_FINE);
			if (EE(CV)->hud & HUD_FINE)
				EE(CV)->hud &= ~HUD_XFINE;
			break;
		case 7:
			SetOption (&EE(CV)->hud, HUD_XFINE);
			if (EE(CV)->hud & HUD_XFINE)
				EE(CV)->hud &= ~HUD_FINE;
			break;
		case 8:
			SetOption (&EE(CV)->hud, HUD_BIG);
			break;
		case 9:
			for (;;) {
				sprintf (prompt, "scale len(%d)[10-25]",
					(int)EE(CV)->tapelen);
				sprintf (msg, "%d", (int)EE(CV)->tapelen);
				edit_str (prompt, msg, sizeof (msg));
				if ('\0' == msg[0])
					break;
				if (1 == sscanf (msg, "%u", &ch) &&
				    ch >= 10 && ch <= 25) {
					EE(CV)->tapelen = (xshort)ch;
					break;
				}
			}
			break;
		case 10:
			for (;;) {
				sprintf (prompt, "hud area(%dDeg)[1-45]",
					(int)EE(CV)->hudarea);
				sprintf (msg, "%d", (int)EE(CV)->hudarea);
				edit_str (prompt, msg, sizeof (msg));
				if ('\0' == msg[0])
					break;
				if (1 == sscanf (msg, "%u", &ch) &&
				    ch >= 1 && ch <= 45) {
					EE(CV)->hudarea = (xshort)ch;
					break;
				}
			}
			break;
		case 11:
			SetOption (&EE(CV)->hud2, HUD_CALIBRATED);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}

static MENU FAR	MenuHudOptions2[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
	{'A', "a alarm"},	/*  3 */
	{'V', "v alarm"},	/*  4 */
	{'F', "font"},		/*  5 */
	{'S', "fontsize"},	/*  6 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_hudoptions2 (void)
{
	int	sel, quit, ch;
	char	msg[80], prompt[80];
	HMSG	*m;

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuHudOptions2, 0);
		switch (sel) {
		case MENU_ABORTED:
		case MENU_FAILED:
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			SetOption (&EE(CV)->hud1, HUD_AALARM);
			break;
		case 4:
			SetOption (&EE(CV)->hud1, HUD_VALARM);
			break;
		case 5:
			for (;;) {
				m = MsgPrintf (0, "StFont(%d)?[0-9]",
					(int)st.StFont);
				ch = mgetch ();
				msg_del (m);
				if (ch >= '0' && ch <= '9') {
					font_set (ch - '0');
					break;
				}
			}
			break;
		case 6:
			for (;;) {
				sprintf (prompt, "StFontSize(%d)[4-64]",
					(int)st.StFontSize);
				sprintf (msg, "%d", (int)st.StFontSize);
				edit_str (prompt, msg, sizeof (msg));
				if ('\0' == msg[0])
					break;
				if (1 == sscanf (msg, "%u", &ch) &&
				    ch >= 8/2 && ch <= 8*8) {
					st.StFontSize = ch;
					break;
				}
			}
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}

/* Select HUD radar options.
*/

static MENU FAR	MenuHudRadar[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
	{'C', "corner"},	/*  3 */
	{'d', "data"},		/*  4 */
	{'D', "distance"},	/*  5 */
	{'n', "name"},		/*  6 */
	{'a', "accvect"},	/*  7 */
	{'r', "reticle"},	/*  8 */
	{'t', "target"},	/*  9 */
	{'z', "ross"},		/* 10 */
	{'L', "limit"},		/* 11 */
	{'H', "thick"},		/* 12 */
	{'h', "hidetgt"},	/* 13 */
	{'p', "tpointer"},	/* 14 */
	{'v', "vpointer"},	/* 15 */
	{'l', "lead mode"},	/* 16 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_hudradar (void)
{
	int	sel, quit;
	int	i;

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuHudRadar, 0);
		switch (sel) {
		case MENU_ABORTED:
		case MENU_FAILED:
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			SetOption (&EE(CV)->hud1, HUD_CORNER);
			break;
		case 4:
			SetOption (&EE(CV)->hud, HUD_DATA);
			break;
		case 5:
			SetOption (&EE(CV)->hud1, HUD_IDIST);
			break;
		case 6:
			SetOption (&EE(CV)->hud1, HUD_INAME);
			break;
		case 7:
			SetOption (&EE(CV)->hud1, HUD_ACCVECT);
			break;
		case 8:
			SetOption (&EE(CV)->hud, HUD_RETICLE);
			break;
		case 9:
			SetOption (&EE(CV)->hud, HUD_TARGET);
			break;
		case 10:
			SetOption (&EE(CV)->hud, HUD_ROSS);
			break;
		case 11:
			SetOption (&EE(CV)->hud1, HUD_LIMIT);
			break;
		case 12:
			SetOption (&EE(CV)->hud1, HUD_THICK);
			break;
		case 13:
			SetOption (&EE(CV)->hud2, HUD_HIDETGT);
			break;
		case 14:
			SetOption (&EE(CV)->hud2, HUD_TPOINTER);
			break;
		case 15:
			SetOption (&EE(CV)->hud2, HUD_VPOINTER);
			break;
		case 16:
			i = (EE(CV)->radar & R_MODES) / R_MODE;
			i = (i + 1) % 5;
			EE(CV)->radar &= ~R_MODES;
			EE(CV)->radar |= i * R_MODE;
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}

/* Select HDD options.
*/

static MENU FAR	MenuHdd[] = {
	{'0', "off"},		/*  0 */
	{'1', "on"},		/*  1 */
	{'2', "toggle"},	/*  2 */
	{'i', "instruments"},	/*  3 */
	{'n', "nav"},		/*  4 */
	{'c', "compass"},	/*  5 */
	{'q', " square"},	/*  6 */
	{'o', " ortho"},	/*  7 */
	{'p', "panel"},		/*  8 */
{'\0', 0}};

LOCAL_FUNC int NEAR
menu_hdd (void)
{
	int	sel, quit;

	SetOption (NULL, 2);
	sel = 0;
	for (quit = 0; !quit;) {
		sel = menu_open (MenuHdd, 0);
		switch (sel) {
		case MENU_ABORTED:
		case MENU_FAILED:
		default:
			quit = 1;
			break;
		case 0:
		case 1:
		case 2:
			SetOption (NULL, (Ushort)sel);
			break;
		case 3:
			SetOption (&EE(CV)->hdd, HDD_INSTRUMENTS);
			break;
		case 4:
			SetOption (&EE(CV)->hdd, HDD_NAV);
			break;
		case 5:
			SetOption (&EE(CV)->hdd, HDD_COMPASS);
			break;
		case 6:
			SetOption (&EE(CV)->hdd, HDD_SQRCOMPASS);
			break;
		case 7:
			SetOption (&EE(CV)->hdd, HDD_ORTCOMPASS);
			break;
		case 8:
			SetOption (&EE(CV)->hdd, HDD_PANEL);
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	}

	return (0);
}

/* Select HUD sub-menu
*/

static MENU FAR	MenuHud[] = {
	{'0', "HUD off"},	/*  0 */
	{'u', "HUD on"},	/*  1 */
	{'t', "type"},		/*  2 */
	{'p', "parts"},		/*  3 */
	{'1', "options1"},	/*  4 */
	{'2', "options2"},	/*  5 */
	{'r', "radar"},		/*  6 */
	{'i', "ils"},		/*  7 */
	{'d', "hdd"},		/*  8 */
	{'h', "help"},		/*  9 */
{'\0', 0}};

extern int FAR
menu_hud (void)
{
	int	sel;
	Ushort	list = 0;

	if (!IS_PLANE(CV))
		return (0);

	if (st.flags & SF_VERBOSE) {
		list = set_lists (0);
		set_lists (SF_HUD);
	}

	sel = menu_open (MenuHud, 0);

	switch (sel) {
	case MENU_ABORTED:
	case MENU_FAILED:
	default:
		break;
	case 0:
		EE(CV)->hud &= ~HUD_ON;
		break;
	case 1:
		EE(CV)->hud |= HUD_ON;
		break;
	case 2:
		menu_hudtype ();
		break;
	case 3:
		menu_hudparts ();
		break;
	case 4:
		menu_hudoptions1 ();
		break;
	case 5:
		menu_hudoptions2 ();
		break;
	case 6:
		menu_hudradar ();
		break;
	case 7:
		menu_ils ();
		break;
	case 8:
		menu_hdd ();
		break;
	case 9:
		set_lists (SF_HUD);
		break;
	}
	if (MENU_FAILED != sel)
		menu_close ();

	if (st.flags & SF_VERBOSE)
		set_lists (list);
	return (0);
}

extern void FAR
hud_setup (OBJECT *p)
{
	int	font;

	EX->hud  &= HUD_ON|HUD_CURSOR|HUD_ROSS;
	EX->hud1 &= HUD_TYPES|HUD_INAME|HUD_IDIST;
	EX->hud2 &= HUD_ILS|HUD_XGRID;
	EX->hud3 &= HUD_GVV;
	EX->hdd  &= HDD_PANEL;
	EX->ladder = 0;

	EX->hud  |= HUD_DEFAULT|HUD_LADDER;
	EX->hud1 |= HUD_VALARM|HUD_AALARM|HUD_LIMIT|HUD_BORDER;
	EX->hud2 |= HUD_HEADING|HUD_ALTITUDE|HUD_SPEED|HUD_XBREAK;
	EX->hud3 |= HUD_CUE;

	switch (EX->hud1 & HUD_TYPES) {
	case HUD_F15:
		EX->hud  |= HUD_FINE|HUD_PLUS|HUD_DATA;
		EX->hud1 |= HUD_KNOTS|HUD_ACCVECT;
		EX->hud2 |= HUD_HIDETGT;
		EX->ladder |= LD_NEGTIP;
		EX->hudarea = 10;
		EX->hudshift = FCON (0.40);
		EX->ldgap = FCON(0.077);
		EX->ldstep = FCON(0.238);
		EX->ldstep0 = FCON(0.391);
		EX->ldstepg = FCON(0.61);	/* ??? */
		EX->ldtip = FCON (0.03);
		EX->ldndash = 5;
		font = 1;
		EX->hudFontSize = FCON(0.06);
		break;
	case HUD_F16:
		EX->hud  |= HUD_PLUS|HUD_DATA;
		EX->hud1 |= HUD_KNOTS|HUD_ACCVECT|HUD_PENDULUM|HUD_CORNER;
		EX->hud2 |= HUD_BTRAIL|HUD_TPOINTER;
		EX->hud3 |= HUD_XVAR;
		EX->ladder |= LD_NEGTIP;
		EX->hudarea = 11;
		EX->hudshift = FCON (0.50);
		EX->ldgap = FCON(0.085);
		EX->ldstep = FCON(0.220);
		EX->ldstep0 = FCON(0.58);
		EX->ldstepg = FCON(0.58);
		EX->ldtip = FCON (0.03);
		EX->ldndash = 4;
		font = 1;
		EX->hudFontSize = FCON(0.045);
		break;
	case HUD_FA18:
		EX->hud  |= HUD_FULLHEADING|HUD_DATA;
		EX->hud1 |= HUD_KNOTS|HUD_PENDULUM;
		EX->hud2 |= HUD_VW;
		EX->hud3 |= HUD_GVV;
		EX->ladder |= LD_SLANT|LD_ZENITH|LD_UNDER|LD_TIP0|LD_NEGTIP;
		EX->hudarea = 10;
		EX->hudshift = FCON (0.4);
		EX->ldgap = FCON(0.11);
		EX->ldstep = FCON(0.23);
		EX->ldstep0 = FCON(0.34);
		EX->ldstepg = FCON(0.96);
		EX->ldtip = FCON (0.06);
		EX->ldndash = 5;
		font = 1;
		EX->hudFontSize = FCON(0.06);
		break;
	case HUD_ETHER:
		EX->hud  |= HUD_BIG|HUD_XFINE|HUD_DATA|HUD_PLUS;
		EX->hud1 &= ~HUD_BORDER;
		EX->hud1 |= HUD_KNOTS|HUD_TOP;
		EX->hud2 |= HUD_VW|HUD_TPOINTER;
		EX->hud3 |= HUD_TRUEHEADING;
		EX->ladder |= LD_NEGTIP;
		EX->hudarea = 15;
		EX->tapelen = 16;		/* scales length */
		EX->hudshift = 0;
		EX->ldgap = FONE/32*5;
		EX->ldstep = FONE/32*12;
		EX->ldstep0 = FONE/32*18;
		EX->ldstepg= FONE/32*18;
		EX->ldtip = FCON (0.03);
		EX->ldndash = 5;
		font = 1;
		EX->hudFontSize = FCON(0.06);
		break;
	default:
		EX->hud  |= HUD_BIG|HUD_XFINE|HUD_PLUS|HUD_DATA;
		EX->hud1 |= HUD_KNOTS|HUD_TOP;
		EX->hud2 |= HUD_VW;
		EX->ladder |= LD_ERECT|LD_NEGTIP;
		EX->hudarea = 11;
		EX->tapelen = 16;		/* scales length */
		EX->hudshift = FCON (0.3);
		EX->ldgap = FONE/32*5;
		EX->ldstep = FONE/32*12;
		EX->ldstep0 = FONE/32*18;
		EX->ldstepg= FONE/32*18;
		EX->ldtip = FCON (0.03);
		EX->ldndash = 5;
		font = 0;
		EX->hudFontSize = FCON(0.05);
		break;
	}
	if (MODEL_CLASSIC != EP->opt[0])
		EX->hud  |= HUD_VV;

	if (!CC || CC == p)
		font_set (font);
}

extern void FAR
cc_setup (void)
{
	if (!IS_PLANE (CC))
		return;
	if (st.flags & SF_BLANKER) {
		EE(CC)->hdd &= ~HDD_PANEL;
		EE(CC)->radar = R_ON | R_LOCK | (3*R_MODE);
		EE(CC)->flags |= PF_CHASE | PF_KILL;
		EE(CC)->weapon = WE_M61;
	} else if (WIN_FULL == st.windows)
		EE(CC)->hdd |= HDD_PANEL;

	if (WIN_ETHER == st.windows) {
		EE(CC)->hud1 = (EE(CC)->hud1 & ~HUD_TYPES) | HUD_ETHER;
		hud_setup (CC);
	}
}

extern void FAR
win_setup (void)
{
	if (WIN_ETHER == st.windows && CC && IS_PLANE (CC))
		EE(CC)->hdd |= HDD_COMPASS | HDD_SQRCOMPASS | HDD_ORTCOMPASS;
}
