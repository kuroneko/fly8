/* --------------------------------- svga.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* SVGAlib control functions. The keyboard uses vga_getkey() and it
 * does some translations:
 *	del (127)	-> Ctrl-h
 *	Ctrl-j (10)	-> Ctrl-m
*/

#include "fly.h"

#if HAVE_SVGALIB

#include "svga.h"
#include <vgamouse.h>
#include <vgakeyboard.h>


extern int FAR
svga_palette (int color, int r, int g, int b)
{
	vga_setpalette (color, r>>2, g>>2, b>>2);
	return (color);
}

static int
svga_setvisual (Ulong base)
{
	vga_setdisplaystart (base);
	vga_waitretrace ();
	return (0);
}

static int
svga_setpage (int bank)
{
	vga_setpage (bank);
	return (0);
}

extern void FAR
svga_init (Uint mode, int width, int height, int xbytes)
{
	vga_init ();
	vga_lockvc ();
	vga_setmode (mode);		/* set mode */
	vga_setpage (0);

	vInit (graph_mem, width, height, xbytes, svga_setpage,
		svga_setvisual);

	mouse_init ("", vga_getmousetype (), 1);
	mouse_setxrange (0, width-1);
	mouse_setyrange (0, height-1);
#if 0
	keyboard_init ();
	keyboard_translatekeys (TRANSLATE_CURSORKEYS|TRANSLATE_DIAGONAL|
				TRANSLATE_KEYPADENTER);
#endif
}

extern void FAR
svga_term (void)
{
#if 0
	keyboard_close ();
#endif
	mouse_close ();
	vga_setmode (TEXT);
	vga_unlockvc ();
}


extern int
svga_GetMouse (int *win_x, int *win_y, char *btn, int *nbtn)
{
	int	b;

	mouse_update ();

	*win_x = mouse_getx ();
	*win_y = mouse_gety ();
	b      = mouse_getbutton ();
	btn[0] = (char)T(b & MOUSE_RIGHTBUTTON);
	btn[1] = (char)T(b & MOUSE_LEFTBUTTON);
	*nbtn = 2;

	return (0);
}

static Uchar	NEAR tabCTRL[] = "@abcdefghijklmnopqrstuvwxyz[\\]^_";

static int EscTab[] = {
/* 0*/	-1,		K_HOME,		K_INS,		K_DEL,		K_END,
/* 5*/	K_PGUP,		K_PGDN,		-1,		-1,		-1,
/*10*/	-1,		-1, 		-1, 		-1,		-1,
/*15*/	-1,		-1, 		K_F6, 		K_F7,		K_F8,
/*20*/	K_F9,		K_F10, 		K_F11, 		K_F12,		-1,
-1
};

LOCAL_FUNC void
svga_DumpKeys (char *title, int n1, int n2)
{
	int	c;

	LogPrintf (title, n1, n2);
	while (T(c = vga_getkey ()))
		LogPrintf ("<%d>", c);
	LogPrintf ("\n");
}

/* have seen "Esc [", now parse on.
*/
LOCAL_FUNC int
svga_GetBrack ()
{
	int	c;
	int	n;

	switch (c = vga_getkey ()) {
	case 0:
		svga_DumpKeys ("key=Esc[\n", 0, 0);
		c = -1;
		break;
	case '[':
		switch (c = vga_getkey ()) {
		case 0:
			svga_DumpKeys ("key=Esc[[\n", 0, 0);
			c = -1;
			break;
		case 'A':
			c = K_F1;
			break;
		case 'B':
			c = K_F2;
			break;
		case 'C':
			c = K_F3;
			break;
		case 'D':
			c = K_F4;
			break;
		case 'E':
			c = K_F5;
			break;
		default:
			svga_DumpKeys ("key=Esc[[<%d>", c, 0);
			c = -1;
			break;
		}
		break;
	case 'A':
		c = K_UP;
		break;
	case 'B':
		c = K_DOWN;
		break;
	case 'C':
		c = K_RIGHT;
		break;
	case 'D':
		c = K_LEFT;
		break;
	case 'G':
		c = K_CENTER;
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		n = 0;
		while (isdigit (c)) {
			n = n * 10 + (c - '0');
			c = vga_getkey ();
			if (0 == c) {
				svga_DumpKeys ("key=Esc[%d\n", n, 0);
				return (-1);
			}
		}
		if ('~' != c) {
			svga_DumpKeys ("key=Esc[%d<%d>\n", n, c);
			c = -1;
			break;
		}
		if (n >= rangeof (EscTab))
			c = -1;
		else
			c = EscTab[n];
		if (c < 0)
			svga_DumpKeys ("key=Esc[%d~\n", n, 0);
		break;
	default:
		svga_DumpKeys ("key=Esc[<%d>", c, 0);
		c = -1;
		break;
	}
	return (c);
}

extern int
svga_Kread (void)
{
	int	c;

again:
	c = vga_getkey ();
	if (0 == c)
		return (-1);

	if (0x1b == c) {		/* Esc */
		c = vga_getkey ();
		if (0 == c)
			c = K_ESC;
		else if ('[' == c)
			c = svga_GetBrack ();
		else
			c = c | K_ALT;
#if 0
		else if ( c >= 'a' && c < 'z')
			c = c | K_ALT;
		else {
			svga_DumpKeys ("key=Esc<%d>", c, 0);
			c = -1;
		}
#endif
		if (c < 0)
			goto again;
	} else if (c < 32) {
#if 0
		LogPrintf ("key=Ctrl-%c\n", tabCTRL[c]);
#endif
		c = tabCTRL[c] | K_CTRL;
		if (K_NL == c)
			c = K_ENTER;		/* xlate ^J  -> ^M */
	} else if (K_DEL == c)
			c = K_RUBOUT;		/* xlate del -> ^H */
#if 0
	else
		LogPrintf ("key=<%d>\n", c);
#endif

	return (c);
}
#endif
