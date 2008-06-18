/* --------------------------------- fly8str.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* This is the Fly8 driver for editstr(). Fly8 is running in the
 * background so it will display the partially edited string as long
 * as edit_str is active. The output callback is dummied out for this
 * reason (except for the beep).
*/

#include "fly.h"


/* These functions are call-backs from editstr().
*/

static void * FAR
e_malloc (int n)
{
	return (memory_alloc (n));
}

static void * FAR
e_free (void *p, int n)
{
	memory_free (p, n);
	return (NULL);
}

/* Put a character to the output device.
*/
static void FAR
e_put (int ch)
{
	if ('\a' == ch && st.quiet)
		Snd->Effect (EFF_BEEP, SND_ON);
}

/* Get a character from the input device.
*/
static int FAR
e_get (void)
{
	return (mgetch ());
}

/* Set cursor shape to indicate insert/overtype mode.
*/
static void FAR
e_show (int edit_mode)
{}




/* Called by Fly8 to initialise editstr().
*/
extern int FAR
edit_init (void)
{
	return (editset (e_malloc, e_free, e_put, e_get, e_show,
		3, st.maxrecall));
}

/* Called by Fly8 to terminate editstr().
*/
extern int FAR
edit_term (void)
{
	return (editstr (NULL, 0));
}

static char	FAR* Prompt = 0;

/* Called by Fly8 to access editstr().
*/
extern int FAR
edit_str (char *prompt, char FAR* str, int len)
{
	int	ret;

	Prompt = prompt;
	ret = editstr (str, len);
	Prompt = 0;
	return (ret);
}

/* display the prompt and the user input if editstr() is active.
*/
extern void FAR
edit_show (VIEW *view, int orgx, int orgy, int maxx, int maxy, int bss)
{
	int	i, x, dx, xl, xr, y, tick;
	char	*p;
	char	*str;
	int	len, pos, mode;

	if (!Prompt)
		return;

	dx = stroke_size ("x", bss);
	tick = bss/2;

	xl = orgx - maxx + bss + 2;
	xr = orgx + maxx - bss - 2;
	y = orgy + maxy - 4;

	gr_color (CC_LGRAY);
	gr_move (xl-2,   y+2);
	gr_draw (xr+bss, y+2);
	gr_draw (xr+bss, y-1-bss);
	gr_draw (xl-2,   y-1-bss);
	gr_draw (xl-2,   y+2);

	x = xl;
	if (*Prompt) {
		for (p = Prompt; *p; ++p)
			x += stroke_char (x, y, *p,  bss, CC_LGRAY);
		x += dx;
	}

	editget (&str, &len, &pos, &mode);

	for (i = 0;; ++i) {
		if (i == pos) {
			gr_color (CC_WHITE);
			gr_move (x, y+3);
			gr_draw (x, y-2-bss);
			if (mode) {
				gr_move (x-tick, y-2-bss-tick);
				gr_draw (x,      y-2-bss);
				gr_draw (x+tick, y-2-bss-tick);
			}
		}
		if (i >= len)
			break;
		if (x > xr)
			break;
		x += stroke_char (x, y, str[i], bss, CC_WHITE);
	}
}
