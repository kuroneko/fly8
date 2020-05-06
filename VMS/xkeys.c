/* --------------------------------- xkeys.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Graphics driver for X11: parse key presses.
*/

#include "fly.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "grx.h"
#include "xkeys.h"
#include "mouse.h"


LOCAL_FUNC int
xGetSpecial (XKeyEvent *xkey, KeySym keysym)
{
	int	k;

	switch (keysym) {
	default:
		return (-1);
	case XK_Right:
		k = K_RIGHT;
		break;
	case XK_Left:
		k = K_LEFT;
		break;
	case XK_Up:
		k = K_UP;
		break;
	case XK_Down:
		k = K_DOWN;
		break;
	case XK_Begin:
		k = K_CENTER;
		break;
	case XK_Home:
		k = K_HOME;
		break;
	case XK_End:
		k = K_END;
		break;
	case XK_Prior:
		k = K_PGUP;
		break;
	case XK_Next:
		k = K_PGDN;
		break;
	case XK_Insert:
		k = K_INS;
		break;
	case XK_F1:         /* Warning: a hack! */
	case XK_F2:
	case XK_F3:
	case XK_F4:
	case XK_F5:
	case XK_F6:
	case XK_F7:
	case XK_F8:
	case XK_F9:
	case XK_F10:
	case XK_F11:
	case XK_F12:
		k = K_F1 + (keysym - XK_F1);
		break;
	}

	k |= K_SPECIAL;

	if (xkey->state & ShiftMask)
		k |= K_SHIFT;
	if (xkey->state & ControlMask)
	k |= K_CTRL;
	if (xkey->state & (Mod1Mask|Mod5Mask))
	k |= K_ALT;

	return (k);
}

static Uchar tabCTRL[] = "@abcdefghijklmnopqrstuvwxyz[\\]^_";

extern int
xGetKey (XKeyEvent *xkey)
{
	KeySym		keysym;
	char		buffer[20];
	XComposeStatus	cs;
	int		c;

	if (XLookupString (xkey, buffer, sizeof(buffer)-1, &keysym, &cs)) {
		c = 0x0ff & buffer[0];
		if (c < 32)
			c = tabCTRL[c] | K_CTRL;
	} else
		c = xGetSpecial (xkey, keysym);
	return (c);
}

extern int
GetMouse (int *win_x, int *win_y, char *btn, int *nbtn)
{
	if (Gr && Gr->extra)
		return (((struct GrxExtra *)(Gr->extra))->GetMouse 
					(win_x, win_y, btn, nbtn));
	else
		return (-1);
}
