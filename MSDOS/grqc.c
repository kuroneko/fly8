/* --------------------------------- grqc.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Vga graphics driver (uses quickC graphics library).
*/

#include "fly.h"

#ifdef FLY8_MSC

#include <conio.h>

#ifdef C7GRAPH
#include <c7graph.h>
#else
#include <graph.h>
#endif


#define DOSYNC		0x0001
#define BIGPAGE		0x0006
#define POSTSYNC	0x0008
#define INITED		0x8000

#define MAX_SYNC_WAIT	1000L	/* 1 second is long enough */

static struct videoconfig	vc[1] = {0};
static Uint			LastColor = (Uint)-1;


LOCAL_FUNC int FAR
QcSetVisual (int page)
{
	int	port;
	Ulong	lasttime;

	if (2 == CD->colors)
		port = 0x3ba;		/* monochrome */
	else
		port = 0x3da;		/* colour */
	lasttime = st.lasttime;

	if (CD->flags & DOSYNC) {
		while (inp (port) & 0x01) {	/* wait for Display Enabled */
			sys_poll (20);
			if (st.lasttime - lasttime > MAX_SYNC_WAIT) {
				LogPrintf ("%s: sync timed out\n", Gr->name);
				die ();
			}
		}

	}
	_setvisualpage (page);
	if (CD->flags & (DOSYNC|POSTSYNC)) {
		while (inp (port) & 0x08) {	/* wait for Vert Sync start */
			sys_poll (21);
			if (st.lasttime - lasttime > MAX_SYNC_WAIT) {
				LogPrintf ("%s: sync timed out\n", Gr->name);
				die ();
			}

		}
		while (!(inp (port) & 0x08)) {	/* wait for Vert Sync end */
			sys_poll (22);
			if (st.lasttime - lasttime > MAX_SYNC_WAIT) {
				LogPrintf ("%s: sync timed out\n", Gr->name);
				die ();
			}

		}
	}
	return (0);
}

LOCAL_FUNC int FAR
QcSetActive (int page)
{
	_setactivepage (page);
	return (0);
}

LOCAL_FUNC void FAR
QcDrawTo (Uint x, Uint y, Uint color)
{
	if (color != LastColor) {
		_setcolor (color);
		LastColor = color;
	}
	_lineto (x, y);
}

LOCAL_FUNC int FAR
QcWriteMode (int mode)
{
	switch (mode) {
	case T_MSET:
		_setwritemode (_GPSET);
		break;
	case T_MOR:
		_setwritemode (_GOR);
		break;
	case T_MXOR:
		_setwritemode (_GXOR);
		break;
	}
	return (0);
}

LOCAL_FUNC int FAR
QcSetPalette (int index, long c)
{
	int	r, g, b;

	r = C_RGB_R (c);
	g = C_RGB_G (c);
	b = C_RGB_B (c);

	c = (((0x03fL&(b>>2))<<16) + ((0x03fL&(g>>2))<<8) + (0x03fL&(r>>2)));

	_remappalette (index, c);
	return (index);
}
		
LOCAL_FUNC int FAR
QcInit (DEVICE *dev, char *options)
{
	long		temp;

	if (_setvideomode (dev->mode) == 0)
		return (1);

	_getvideoconfig (vc);
	if (vc->numxpixels == 0)
		return (1);
	dev->npages = vc->numvideopages;

	if (get_narg (options, "shutters=", &temp))
		st.misc[7] = (int)temp;
	else
		st.misc[7] = 0;

	Gr->flags |= INITED;

	LogPrintf ("%s: init ok\n", Gr->name);
	return (0);
}

LOCAL_FUNC void FAR
QcTerm (DEVICE *dev)
{
	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	_setvideomode (_DEFAULTMODE);

	LogPrintf ("%s: term ok\n", Gr->name);
}

LOCAL_FUNC void FAR
QcEllipse (Uint x1, Uint y1, Uint rx, Uint ry, Uint color)
{
	if (color != LastColor) {
		_setcolor (color);
		LastColor = color;
	}
	_ellipse (_GBORDER, x1-rx, y1+ry, x1+rx, y1-ry);
}

LOCAL_FUNC int FAR
QcShutters (int eye)
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

struct GrDriver NEAR GrQc = {
	"GrQC",
	0,
	NULL,		/* Extra */
	0,		/* Devices */
	QcInit,
	QcTerm,
	_moveto,
	QcDrawTo,
	QcSetVisual,
	QcSetActive,
	0,	 	/* Clear */
	QcWriteMode,
	QcSetPalette,
	QcEllipse,
	0, 		/* Polygon */
	0,		/* Flush */
	QcShutters
};
#undef DOSYNC
#undef BIGPAGE
#undef POSTSYNC
#undef INITED
#undef MAX_SYNC_WAIT

#endif /* ifdef FLY8_MSC */
