/* --------------------------------- menu.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Menu management.
*/

#include "fly.h"


char	FAR menuch[] = "0123456789abcdefghijklmnopqrstuvwxyz";

#define NMENUS	10
static struct {
	MENU	*menu;
	int	selected;
}	menus[NMENUS] = {{0}};

static int	nmenu = 0;

LOCAL_FUNC void NEAR
oops (void)
{
	if (st.quiet)
		Snd->Effect (EFF_BEEP, SND_ON);
}

extern int FAR
menu_open (MENU *menu, int selected)
{
	int	n, ch, c, ret, l;

	if (NMENUS == nmenu)
		return (MENU_FAILED);		/* open failed */

	for (l = 0; menu[l].letter; ++l)
		;
	menus[nmenu++].menu = menu;
	for (ret = 0; !ret;) {
		menus[nmenu-1].selected = selected;

		switch (ch = mgetch ()) {
		case KF_ESC:			/* user ABORTEDed */
			selected = MENU_ABORTED;
			ret = 1;
			break;
		case K_ENTER:			/* confirm selection */
			ret = 1;
			break;
		case K_UP:			/* prev option */
		case K_LEFT:
			if (selected > 0)
				--selected;
			else
				oops ();
			break;
		case K_DOWN:			/* next option */
		case K_RIGHT:
			++selected;
			if (!menu[selected].letter) {
				--selected;
				oops ();
			}
			break;
		case K_PGUP:			/* first option */
			selected = 0;
			break;
		case K_PGDN:			/* last option */
			while (menu[selected].letter)
				++selected;
			--selected;
			break;
		case K_F1|K_CTRL:
		case K_F2|K_CTRL:
		case K_F3|K_CTRL:
		case K_F4|K_CTRL:
		case K_F5|K_CTRL:
		case K_F6|K_CTRL:
		case K_F7|K_CTRL:
		case K_F8|K_CTRL:
		case K_F9|K_CTRL:
		case K_F10|K_CTRL:
			selected = ch - (K_F1|K_CTRL);
			if (selected > l) {
				selected = 0;
				oops ();
			}
			ret = 1;
			break;
		case K_F1|K_ALT:
		case K_F2|K_ALT:
		case K_F3|K_ALT:
		case K_F4|K_ALT:
		case K_F5|K_ALT:
		case K_F6|K_ALT:
		case K_F7|K_ALT:
		case K_F8|K_ALT:
		case K_F9|K_ALT:
		case K_F10|K_ALT:
			selected = ch - (K_F1|K_ALT) + 10;
			if (selected > l) {
				selected = 0;
				oops ();
			}
			ret = 1;
			break;
		default:			/* letter option */
			for (n = 0; T(c = menu[n].letter) && c != ch; ++n)
				;
			if (c) {
				selected = n;
				ret = 1;
			} else
				oops ();
			break;
		}
	}

	menus[nmenu-1].selected = selected;
	return (selected);
}

extern void FAR
menu_close (void)
{
	--nmenu;
}

LOCAL_FUNC void NEAR
show_option (int x, int y, int letter, char *text, int bss, Uint color)
{
	x += stroke_char (x, y, letter, bss, color);
	x += stroke_char (x, y, ' ', bss, color);
	x += stroke_str (x, y, text, bss, color);
}

#define	NHDDMENU	10

extern int FAR
hdd_menu (VIEW *view, int orgx, int orgy, int maxx, int maxy)
{
	int	ss, x, y, dx, dy, n, i, font, sel, color;
	MENU	*m;

	if (!nmenu)
		return (1);

	font = font_set (0);

	ss = maxx/12;
	if (ss > 128)
		ss = 128;
	else if (ss < 8)
		ss = 8;

	dy = maxy / (NHDDMENU/2);
	if (dy < ss)
		dy = ss;

	dx = stroke_size ("A", ss);

	x = orgx - maxx;
	y = orgy - maxy;

	m = menus[nmenu-1].menu;
	sel = menus[nmenu-1].selected;
	for (n = 0; m[n].letter; ++n) {
		color = (n == sel) ? ST_MENUH : ST_MENU;
		if (NHDDMENU == n) {
			x = orgx+maxx-dx;
			y = orgy - maxy;
		}
		y += dy;
		if (n >= NHDDMENU) {
			i = n - NHDDMENU + 1;
			if (i >= 10)
				i -= 10;
			stroke_char (x, y, '0'+i, ss, color);
			stroke_str  (orgx, y, m[n].text, ss, color);
		} else {
			i = n + 1;
			if (i >= 10)
				i -= 10;
			stroke_char (x, y, '0'+i, ss, color);
			stroke_str  (x+dx*2, y, m[n].text, ss, color);
		}
	}
	font = font_set (font);
	return (0);
}

extern void FAR
show_menu (VIEW *view, int orgx, int orgy, int maxx, int maxy, int bss)
{
	int	x, y, x0, y0, dx, dy, i, n, color;
	MENU	*m;

	if (!nmenu)
		return;

	dx = dy = bss;

	x0 = orgx - maxx;
	y0 = orgy - maxy;

	x = x0 + dx;
	y = y0 + dy;
	for (i = 0;; ++i) {
		m = menus[i].menu;
		if (i < nmenu-1) {
			n = menus[i].selected;
			color = ST_MENUH;
			y += dy;
			show_option (x, y, m[n].letter, m[n].text, dx, color);
			x += 2*dx;
			continue;
		}
		for (n = 0; m[n].letter; ++n) {
			color = (n == menus[i].selected) ? ST_MENUH : ST_MENU;
			y += dy;
			show_option (x, y, m[n].letter, m[n].text, dx, color);
		}
		break;
	}
}

static MENU MenuNum[] = {
	{'+', "Increase"},	/*  0 */
	{'-', "Decrease"},	/*  1 */
	{'=', "New"},		/*  2 */
	{'*', "Restore"},	/*  3 */
{'\0', 0}};

extern void FAR
get_num (char *name, void *value, int type, long vmin, long vmax, long vinc)
{
	int	sel;
	long	l, vold, vnew;
	char	in[16], msg[80];
	HMSG	*m;

	if (MT_CHAR == type)
		vold = *(char *)value;
	else if (MT_SHORT == type)
		vold = *(short *)value;
	else if (MT_INT == type)
		vold = *(int *)value;
	else if (MT_LONG == type)
		vold = *(long *)value;
	else {
		MsgEPrintf (50, "get_num> Bad type");
		return;
	}
	vnew = vold;

	sprintf (msg, "%s %9ld", name, vnew);
	m = MsgWPrintf (0, msg);

	sel = 3;
	do {
		if (m)
			sprintf (m->text, "%s %9ld", name, vnew);

		sel = menu_open (MenuNum, sel);
		switch (sel) {
		default:
		case MENU_ABORTED:
		case MENU_FAILED:
			break;
		case 0:
			vnew += vinc;
			if (vnew > vmax)
				vnew = vmax;
			break;
		case 1:
			vnew -= vinc;
			if (vnew < vmin)
				vnew = vmin;
			break;
		case 2:
			sprintf (msg, "%s from %ld to %ld", name, vmin, vmax);
			sprintf (in, "%ld", vnew);
			edit_str (msg, in, sizeof (in));
			if (1 == sscanf (in, "%ld", &l) &&
			    l <= vmax && l >= vmin)
				vnew = l;
			else
				MsgEPrintf (50, "Bad value");
			break;
		case 3:
			vnew = vold;
			break;
		}
		if (MENU_FAILED != sel)
			menu_close ();

		if (MT_CHAR == type)
			*(char *)value = (char)vnew;
		else if (MT_SHORT == type)
			*(short *)value = (short)vnew;
		else if (MT_INT == type)
			*(int *)value = (int)vnew;
		else if (MT_LONG == type)
			*(long *)value = vnew;
	} while (sel >= 0);

	msg_del (m);
}

extern void FAR
SetOption (Ushort *i, Ushort mask)
{
	static int	mode = 2;

	if (NULL == i) {
		mode = (int)mask;
		return;
	}
	switch (mode) {
	case 0:
		*i &= ~mask;
		break;
	case 1:
		*i |= mask;
		break;
	case 2:
		*i ^= mask;
		break;
	}
}

#undef NHDDMENU
#undef NMENUS
