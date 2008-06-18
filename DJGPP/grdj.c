/* --------------------------------- grdj.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Vga graphics driver (direct tseng4000 access, 256 colors only).
*/

#include "fly.h"

#if 1 == SYS_DJGPP

#include <graphics.h>
#include <pc.h>
#include <dos.h>


#define INITED		0x1000

#define inp(p) 		inportb(p)
#define outp(p,b)	outportb(p,b)

#define GrOR	0x200

static int _GrWriteMode = 0;
static Uint x = 0, y = 0;

LOCAL_FUNC void FAR
DjDrawTo (Uint x2, Uint y2, Uint c)
{
	GrLine (x, y, x2, y2, _GrWriteMode|c);
	x = x2;
	y = y2;
}

LOCAL_FUNC void FAR
DjMoveTo (Uint x1, Uint y1)
{
	x = x1;
	y = y1;
}

LOCAL_FUNC int FAR
DjSetActive (int page)
{return (0);}

LOCAL_FUNC int FAR
DjSetVisual (int page)				/* done */
{return (0);}

LOCAL_FUNC int FAR
DjSetWriteMode (int mode)
{
	switch (mode) {
	default:
	case T_MSET:
		_GrWriteMode = 0;
		break;
	case T_MOR:
		_GrWriteMode = GrOR;
		break;
	case T_MXOR:
		_GrWriteMode = GrXOR;
		break;
	}
	return (0);
}

LOCAL_FUNC void FAR
DjClear (Uint x, Uint y, Uint sx, Uint sy, Uint color)
{
	for (sy += y, sx += x-1; y < sy; ++y)
		GrLine (x, y, sx, y, color);
}

LOCAL_FUNC int FAR
DjSetPalette (int index, long c)
{
	GrSetColor (index, C_RGB_R (c), C_RGB_G (c), C_RGB_B (c));
	return (index);
}

LOCAL_FUNC int FAR
DjSetBiosMode (int n)
{
	union REGS	regs;

	if (n >= 0x100) {
		regs.x.ax = 0x4f02;
		regs.x.bx = n;
	} else
		regs.x.ax = n;
	return (int86 (0x10, &regs, &regs));
}

LOCAL_FUNC int FAR
DjInit (DEVICE *dev, char *options)
{
	int	i;
	long	temp;

	if (dev->sizex == 0 || dev->sizey == 0)
		return (1);

	GrSetMode (GR_width_height_graphics, dev->sizex, dev->sizey);

#if 1
	if (dev->mode > 0)
		DjSetBiosMode (dev->mode);
#endif
	dev->npages = 1;
	for (i = 0; i++ < 256 && GrAllocCell () >= 0;)	/* get 2-255 */
		;

	DjSetWriteMode (T_MSET);
	st.colors[CC_BLACK] = DjSetPalette (CC_BLACK, C_BLACK);
	for (i = 0; i < dev->npages; ++i) {
		DjSetActive (i);
		DjClear (0, 0, dev->sizex, dev->sizey, st.colors[CC_BLACK]);
	}

	DjSetVisual (0);
	DjSetActive (0);

	if (get_narg (options, "shutters=", &temp))
		st.misc[7] = (int)temp;
	else
		st.misc[7] = 0;

	Gr->flags |= INITED;

	LogPrintf ("%s: init ok\n", Gr->name);
	return (0);
}

LOCAL_FUNC void FAR
DjTerm (DEVICE *dev)		/* done */
{
	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	GrSetMode (GR_default_text, 0, 0);
	DjSetBiosMode (0x03);

	LogPrintf ("%s: term ok\n", Gr->name);
}

LOCAL_FUNC void FAR
DjEllipse (register Uint x1, register Uint y1, Uint rx, Uint ry, 
	register Uint color)
{
	int	ax, bx, cx, dx, ay, by, cy, dy;

	ax = fmul ( 3196, rx);		/* sin (pi/16) */
	ay = fmul ( 3196, ry);
	bx = fmul ( 9102, rx);		/* sin (3*pi/16) */
	by = fmul ( 9102, ry);
	cx = fmul (13623, rx);		/* sin (5*pi/16) */
	cy = fmul (13623, ry);
	dx = fmul (16069, rx);		/* sin (7*pi/16) */
	dy = fmul (16069, ry);

	DjMoveTo (x1+dx, y1-ay);
	DjDrawTo (x1+cx, y1-by, color);
	DjDrawTo (x1+bx, y1-cy, color);
	DjDrawTo (x1+ax, y1-dy, color);
	DjDrawTo (x1-ax, y1-dy, color);
	DjDrawTo (x1-bx, y1-cy, color);
	DjDrawTo (x1-cx, y1-by, color);
	DjDrawTo (x1-dx, y1-ay, color);
	DjDrawTo (x1-dx, y1+ay, color);
	DjDrawTo (x1-cx, y1+by, color);
	DjDrawTo (x1-bx, y1+cy, color);
	DjDrawTo (x1-ax, y1+dy, color);
	DjDrawTo (x1+ax, y1+dy, color);
	DjDrawTo (x1+bx, y1+cy, color);
	DjDrawTo (x1+cx, y1+by, color);
	DjDrawTo (x1+dx, y1+ay, color);
	DjDrawTo (x1+dx, y1-ay, color);
}

LOCAL_FUNC int FAR
DjShutters (int eye)
{
	if (st.misc[7]) {
		if (eye >= 0)
			outp (st.misc[7]+4, 1+2*eye);
		else if (-1 == eye)
			outp (st.misc[7]+4, 1);		/* on */
		else if (-2 == eye)
			outp (st.misc[7]+4, 0);		/* off */
		return (0);				/* have shutters */
	} else
		return (1);				/* no shutters */
}

struct GrDriver NEAR GrDJ = {
	"GrDJ",
	0,
	NULL,		/* Extra */
	0,		/* Devices */
	DjInit,
	DjTerm,
	DjMoveTo,
	DjDrawTo,
	DjSetVisual,
	DjSetActive,
	DjClear,
	DjSetWriteMode,
	DjSetPalette,
	DjEllipse,
	0, 		/* Polygon */
	0,		/* Flush */
	DjShutters
};
#undef inp
#undef outp
#undef GrOR
#undef INITED
#endif
