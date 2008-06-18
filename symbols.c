/* --------------------------------- symbols.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Paint basic HUD symbols.
*/

#include "fly.h"


extern void FAR
show_w (int x, int y, int tx, int ty, int color)
{
	int	dx, dy;

	dx = tx/4;
	dy = ty/2;

	gr_color (color);
	gr_move (x - dx*3, y);
	gr_draw (x - dx*2, y);
	gr_draw (x - dx,   y+dy);
	gr_draw (x,        y);
	gr_draw (x + dx,   y+dy);
	gr_draw (x + dx*2, y);
	gr_draw (x + dx*3, y);
}

extern void FAR
show_x (int x, int y, int tx, int ty, int color)
{
	gr_color (color);
	gr_move (x + tx, y + ty);
	gr_draw (x - tx, y - ty);
	gr_move (x + tx, y - ty);
	gr_draw (x - tx, y + ty);
}

extern void FAR
show_rect (int x, int y, int tx, int ty, int color, int showx)
{
	gr_color (color);
	gr_move (x + tx, y + ty);
	gr_draw (x + tx, y - ty);
	gr_draw (x - tx, y - ty);
	gr_draw (x - tx, y + ty);
	gr_draw (x + tx, y + ty);
	if (showx)
		show_x (x, y, tx, ty, color);
}

extern void FAR
show_brect (int x, int y, int tx, int ty, int ratio, int color, int showx)
{
	int	dx, dy;

	dx = fmul (tx, ratio);
	dy = fmul (ty, ratio);

	gr_color (color);
	gr_move (x+dx, y+ty);
	gr_draw (x+tx, y+ty);
	gr_draw (x+tx, y+dy);
	gr_move (x+tx, y-dy);
	gr_draw (x+tx, y-ty);
	gr_draw (x+dx, y-ty);
	gr_move (x-dx, y-ty);
	gr_draw (x-tx, y-ty);
	gr_draw (x-tx, y-dy);
	gr_move (x-tx, y+dy);
	gr_draw (x-tx, y+ty);
	gr_draw (x-dx, y+ty);
	if (showx)
		show_x (x, y, tx, ty, color);
}

extern void FAR
show_diamond (int x, int y, int tx, int ty, int color, int showx)
{
	gr_color (color);
	gr_move (x-tx, y);
	gr_draw (x,    y-ty);
	gr_draw (x+tx, y);
	gr_draw (x,    y+ty);
	gr_draw (x-tx, y);
	if (showx)
		show_x (x, y, tx, ty, color);
}

extern void FAR
show_plus (int x, int y, int tx, int ty, int color)
{
	gr_color (color);
	gr_move (x - tx, y);
	gr_draw (x + tx, y);
	gr_move (x,      y - ty);
	gr_draw (x,      y + ty);
}

extern void FAR
show_bplus (int x, int y, int tx, int ty, int ratio, int color)
{
	int	dx, dy;

	dx = fmul (tx, ratio);
	dy = fmul (ty, ratio);

	gr_color (color);
	gr_move (x - tx, y);
	gr_draw (x - dx, y);
	gr_move (x + dx, y);
	gr_draw (x + tx, y);
	gr_move (x,      y - ty);
	gr_draw (x,      y - dy);
	gr_move (x,      y + dy);
	gr_draw (x,      y + ty);
}

extern void FAR
show_fpm (int x, int y, int rx, int ry, int tx, int ty, int color, int shape)
{
	gr_color (color);
	if (2 == shape)					/* round */
		gr_ellipse (x, y, rx, ry);
	else if (shape)					/* square */
		show_diamond (x, y, rx, ry, color, 0);

	gr_move (x,    y-ry);
	gr_draw (x,    y-ty);
	gr_move (x+rx, y);
	gr_draw (x+tx, y);
	gr_move (x-rx, y);
	gr_draw (x-tx, y);
}

extern void FAR
show_dir (int x, int y, int rx, int ry, int tx, int ty, int color)
{
	gr_color (color);
	gr_move (x+rx, y);
	gr_draw (x+tx, y);
	gr_move (x-rx, y);
	gr_draw (x-tx, y);
	gr_move (x,    y-ry);
	gr_draw (x,    y-ty);
}

extern void FAR
show_dir1 (int x, int y, int rx, int ry, int tx, int ty, int sa, int ca,
	int color,
	int orgx, int orgy, int sx, int sy, int shifty)
{
	int	x0, y0, x1, y1, x2, y2, x3, y3;

	x0 = fmul (rx, ca);		/* wing inside */
	y0 = fmul (rx, sa);
	x1 = fmul (tx, ca);		/* wing outside */
	y1 = fmul (tx, sa);
	x2 = fmul (rx, sa);		/* tail inside */
	y2 = fmul (rx, ca);
	x3 = fmul (tx, sa);		/* tail outside */
	y3 = fmul (tx, ca);
	x3 -= x3>>2;			/* shorter tail? */
	y3 -= y3>>2;

	gr_color (color);
#if 0
	gr_move (x+x0,      y+y0);	/* right wing */
	gr_draw (x+x1,      y+y1);
	gr_draw (x+x1-x2/4, y+y1+y2/4);	/* tip */

	gr_move (x-x0,      y-y0);	/* left wing */
	gr_draw (x-x1,      y-y1);
	gr_draw (x-x1-x2/4, y-y1+y2/4);	/* tip */

	gr_move (x+x2,      y-y2);	/* tail */
	gr_draw (x+x3,      y-y3);
#else
	add_segment (x+x0, y+y0, x+x1,      y+y1,
		orgx, orgy, sx, sy, shifty);			/* right wing */
	add_segment (x+x1, y+y1, x+x1-x2/4, y+y1+y2/4,
		orgx, orgy, sx, sy, shifty);			/* tip */
	add_segment (x-x0, y-y0, x-x1,      y-y1,
		orgx, orgy, sx, sy, shifty);			/* left wing */
	add_segment (x-x1, y-y1, x-x1-x2/4, y-y1+y2/4,
		orgx, orgy, sx, sy, shifty);			/* tip */
	add_segment (x+x2, y-y2, x+x3,      y-y3,
		orgx, orgy, sx, sy, shifty);			/* tail */
#endif
}

extern void FAR
show_ptr (int x, int y, int rx, int ry, int tx, int ty, int color, int round)
{
	gr_color (color);
	if (round)
		gr_ellipse (x, y, rx, ry);
	else
		show_rect (x, y, rx, ry, color, 0);
	gr_move (x+tx/4, y-ty/4);
	gr_draw (x+tx,   y-ty);
}

extern void FAR
show_trig (int x, int y, int dx, int dy, int color)
{
	gr_color (color);
	gr_move (x-dx, y);
	gr_draw (x+dx, y);
	gr_draw (x,    y-dy);
	gr_draw (x-dx, y);
}
