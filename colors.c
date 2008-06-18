/* --------------------------------- colors.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* color handling.
*/

#include "fly.h"


LOCAL_FUNC long NEAR
color_show (Ulong c)
{
	return (((Ulong)C_RGB_R (c) << 16) |
		(       C_RGB_G (c) <<  8) |
		(       C_RGB_B (c)      ));
}

#if 0
LOCAL_FUNC long NEAR
color_saturation (long color)			/* get intensity */
{
	int	r, g, b, w;

	r = C_RGB_R (color);
	g = C_RGB_G (color);
	b = C_RGB_B (color);

	w = (r*67+g*21+b*15)/(67+21+15);

	return (C_RGB (w, w, w));
}
#endif

LOCAL_FUNC long NEAR
color_intensify (long color)			/* highlight a color */
{
	Uint	r, g, b, m;

	r = C_RGB_R (color);
	g = C_RGB_G (color);
	b = C_RGB_B (color);

	m = (r > g) ? r : g;
	if (b > m)
		m = b;
	if (!m)
		return (color);
	
	r = (Uint)muldiv (r, 0x0ff, m);
	g = (Uint)muldiv (g, 0x0ff, m);
	b = (Uint)muldiv (b, 0x0ff, m);

	return (C_RGB (r, g, b));
}

#define COLOR_OFFSET	8
#define COLOR_VALUE	6
#define COLOR_SIZE	(COLOR_OFFSET+COLOR_VALUE+1)
#define	COLOR_STEP	8

static char FAR names[][COLOR_SIZE] = {
	"Black   xxxxxx",	/*  0 */
	"Red     xxxxxx",	/*  1 */
	"Blue    xxxxxx",	/*  2 */
	"Magenta xxxxxx",	/*  3 */
	"Green   xxxxxx",	/*  4 */
	"Brown   xxxxxx",	/*  5 */
	"Gray    xxxxxx",	/*  6 */
	"dYellow xxxxxx",	/*  7 */
	"Yellow  xxxxxx",	/*  8 */
	"lRed    xxxxxx",	/*  9 */
	"lBlue   xxxxxx",	/* 10 */
	"lGray   xxxxxx",	/* 11 */
	"dBrown  xxxxxx",	/* 12 */
	"skyBlue xxxxxx",	/* 13 */
	"dGreen  xxxxxx",	/* 14 */
	"White   xxxxxx"	/* 15 */
};

static MENU MenuPalette[] = {
	{'k', names[ 0]},	/*  0 */
	{'r', names[ 1]},	/*  1 */
	{'b', names[ 2]},	/*  2 */
	{'m', names[ 3]},	/*  3 */
	{'g', names[ 4]},	/*  4 */
	{'n', names[ 5]},	/*  5 */
	{'a', names[ 6]},	/*  6 */
	{'h', names[ 7]},	/*  7 */
	{'H', names[ 8]},	/*  8 */
	{'o', names[ 9]},	/*  9 */
	{'f', names[10]},	/* 10 */
	{'A', names[11]},	/* 11 */
	{'B', names[12]},	/* 12 */
	{'s', names[13]},	/* 13 */
	{'d', names[14]},	/* 14 */
	{'w', names[15]},	/* 15 */
{'\0', 0}};

extern char * FAR
color_name (int color)
{
	static char	name[COLOR_OFFSET];

	memcpy (name, names[color], COLOR_OFFSET-1);
	name[COLOR_OFFSET-1] = '\0';

	return (name);
}

extern char * FAR
color_rgb (int color)
{
	static char	value[COLOR_VALUE+1];

	memcpy (value, names[color]+COLOR_OFFSET, COLOR_VALUE);
	value[COLOR_VALUE] = '\0';

	return (value);
}

extern int FAR
set_rrggbb (int letter, Ulong value)
{
	int	c;

	for (c = rangeof (MenuPalette); --c >= 0;) {
		if (letter == MenuPalette[c].letter)
			break;
	}

	if (c < 0 || c >= rangeof (st.palette))
		return (1);

	value = C_RGB (0x0ff&(Uint)(value>>16), 0x0ff&(Uint)(value>>8),
			0x0ff&(Uint)value);
	st.palette[c] = value;
	sprintf (names[c]+COLOR_OFFSET, "%06lx", color_show (value));
	if (ST_HFG == (Uint)c) {
		value = color_intensify (value);
		st.palette[ST_HFGI] = value;
		sprintf (names[ST_HFGI]+COLOR_OFFSET, "%06lx",
							color_show (value));
	}
	return (0);
}

extern void FAR
set_palette (void)
{
	int	i;
	int	c;

	for (i = 0; i < rangeof (st.colors); ++i) {
		sprintf (names[i]+COLOR_OFFSET, "%06lx",
						color_show (st.palette[i]));
		if (Gr->SetPalette) {
			c = Gr->SetPalette (i, st.palette[i]);
			if (c < 0) {
				LogPrintf ("set_palette (%d, %.8x) failed\n",
					i, st.palette[i]);
				if (-1 == st.colors[i])
					die ();
			} else
				st.colors[i] = c;
		} else
			st.colors[i] = i;
	}
}

extern int FAR
color_assign (Ushort *item)
{
	int	color;

	color = *item;
	color = menu_open (MenuPalette, color);
	if (MENU_FAILED != color)
		menu_close ();		/* close MenuPalette */
	if (color >= 0) {
		*item = color;
		repaint ();
	}
	return (color);
}

static MENU MenuGetColor[] = {
	{'+', "Brighter"},	/*  0 */
	{'-', "Darker"},	/*  1 */
	{'R', "+red"},		/*  2 */
	{'r', "-red"},		/*  3 */
	{'G', "+green"},	/*  4 */
	{'g', "-green"},	/*  5 */
	{'B', "+blue"},		/*  6 */
	{'b', "-blue"},		/*  7 */
	{'=', "New"},		/*  8 */
	{'*', "Restore"},	/*  9 */
{'\0', 0}};

LOCAL_FUNC void NEAR
color_adjust (int c)
{
	Uint	r, g, b;
	Ulong	newc;
	int	sel;
	int	cc;
	long	l;
	char	msg[7];
	HMSG	*m;

	if (!Gr->SetPalette)
		MsgWPrintf (50, "no set palette function");

	newc = st.palette[c];

	m = MsgWPrintf (0, names[c]);

	sel = 8;	/* new */
	do {
		if (m)
			sprintf (m->text+COLOR_OFFSET, "%06lx",
				color_show (newc));
		if (Gr->SetPalette) {
			cc = Gr->SetPalette (c, newc);
			if (cc < 0) {
				MsgEPrintf (50,
					"set_palette (%d, %.8x) failed\n",
					c, newc);
			} else {
				st.colors[c] = cc;
				repaint ();
			}
		} else {
			MsgWPrintf (50, "no set palette function");
			st.colors[c] = c;
		}

		r = C_RGB_R (newc);
		g = C_RGB_G (newc);
		b = C_RGB_B (newc);

		sel = menu_open (MenuGetColor, sel);

		switch (sel) {
		default:
		case MENU_ABORTED:
		case MENU_FAILED:
			break;
		case 0:
			if ((r += (r+COLOR_STEP-1)/COLOR_STEP) > 0x0ff)
				r = 0x0ff;
			if ((g += (g+COLOR_STEP-1)/COLOR_STEP) > 0x0ff)
				g = 0x0ff;
			if ((b += (b+COLOR_STEP-1)/COLOR_STEP) > 0x0ff)
				b = 0x0ff;
set_newc:
			newc = C_RGB (r, g, b);
			break;
		case 1:
			r -= (r+COLOR_STEP-1)/COLOR_STEP;
			g -= (g+COLOR_STEP-1)/COLOR_STEP;
			b -= (b+COLOR_STEP-1)/COLOR_STEP;
			goto set_newc;
		case 2:
			if ((r += (r+COLOR_STEP-1)/COLOR_STEP) > 0x0ff)
				r = 0x0ff;
			goto set_newc;
		case 3:
			r -= (r+COLOR_STEP-1)/COLOR_STEP;
			goto set_newc;
		case 4:
			if ((g += (g+COLOR_STEP-1)/COLOR_STEP) > 0x0ff)
				g = 0x0ff;
			goto set_newc;
		case 5:
			g -= (g+COLOR_STEP-1)/COLOR_STEP;
			goto set_newc;
		case 6:
			if ((b += (b+COLOR_STEP-1)/COLOR_STEP) > 0x0ff)
				b = 0x0ff;
			goto set_newc;
		case 7:
			b -= (b+COLOR_STEP-1)/COLOR_STEP;
			goto set_newc;
		case 8:
			sprintf (msg, "%06lx", color_show (newc));
			edit_str ("color RRGGBB", msg, sizeof (msg));
			if (1 == sscanf (msg, "%lx", &l)) {
				b = 0x0ff & (Uint)(l      );
				g = 0x0ff & (Uint)(l >> 8 );
				r = 0x0ff & (Uint)(l >> 16);
				goto set_newc;
			} else
				MsgEPrintf (50, "*** Bad color");
			break;
		case 9:
			newc = st.palette[c];
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();
	} while (sel >= 0);

	st.palette[c] = newc;
	memcpy (names[c], m->text, COLOR_SIZE);

	m = msg_del (m);
}
#undef COLOR_OFFSET
#undef COLOR_STEP

/* this menu is used for programming the palette.
*/
extern int FAR
menu_palette (void)
{
	int	sel;

	for (sel = 0; MENU_ABORTED != sel;) {
		sel = menu_open (MenuPalette, sel);
		if (sel >= 0)
			color_adjust (sel);
		if (MENU_FAILED != sel)
			menu_close ();
	}
	return (0);
}

/* This menu allows assigning colors to image items.
*/

static MENU FAR MenuColors[] = {
	{'a', "info"},		/*  0 */
	{'b', "mfg"},	 	/*  1 */
	{'c', "wfg"},		/*  2 */
	{'d', "cfg"},	 	/*  3 */
	{'e', "hud fg"},	/*  4 */
	{'f', "hud fgi"}, 	/*  5 */
	{'g', "hud bo"}, 	/*  6 */
	{'h', "s left"},	/*  7 */
	{'i', "s right"}, 	/*  8 */
	{'j', "s both"}, 	/*  9 */
	{'k', "ground"}, 	/* 10 */
	{'l', "dull"},	 	/* 11 */
	{'m', "faint"}, 	/* 12 */
	{'n', "sky"}, 		/* 13 */
	{'o', "friend"}, 	/* 14 */
	{'p', "foe"}, 		/* 15 */
	{'q', "help"}, 		/* 16 */
	{'r', "fire1"}, 	/* 17 */
	{'s', "fire2"}, 	/* 18 */
	{'t', "menu"},  	/* 19 */
	{'u', "menuh"}, 	/* 20 */
	{'v', "sun"},	 	/* 21 */
	{'w', "earth"}, 	/* 22 */
	{'x', "compass"}, 	/* 23 */
	{'y', "msg err"}, 	/* 24 */
	{'z', "msg warn"}, 	/* 25 */
	{'A', "lamp off"}, 	/* 26 */
	{'B', "lamp ok"}, 	/* 27 */
	{'C', "lamp err"}, 	/* 28 */
{'\0', 0}};

extern int FAR
menu_colors (void)
{
	int	sel;

	for (sel = 0; sel >= 0;) {
		sel = menu_open (MenuColors, sel);
		if (MENU_FAILED == sel)
			break;
		if (sel >= 0)
			color_assign (&st.assign[sel]);
		menu_close ();				/* close MenuColors */
	}

	return (MENU_FAILED == sel);
}
