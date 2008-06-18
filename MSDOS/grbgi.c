/* --------------------------------- grbgi.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Vga graphics driver (uses Borland BGI graphics library).
*/

#include "fly.h"

#ifdef FLY8_BC

#include <conio.h>
#include <graphics.h>


#define DOSYNC		0x0001
#define BIGPAGE		0x0006
#define POSTSYNC	0x0008
#define INITED		0x8000

#define MAX_SYNC_WAIT	1000L	/* 1 second is long enough */

static Uint	LastColor = (Uint)-1;

LOCAL_FUNC void FAR
BGIClear (Uint x, Uint y, Uint sx, Uint sy, Uint color)
{
	setbkcolor (color);
	cleardevice ();
}

LOCAL_FUNC int FAR
BGISetVisual (int page)
{
	Ulong	lasttime;
	int	port;

	if (2 == CD->colors)
		port = 0x3ba;		/* monochrome */
	else
		port = 0x3da;		/* colour */
	lasttime = st.lasttime;

	if (CD->flags & DOSYNC) {
		while (inp (port) & 0x01) {	/* wait for Display Enabled */
			sys_poll (26);
			if (st.lasttime - lasttime > MAX_SYNC_WAIT) {
				LogPrintf ("%s: sync timed out\n", Gr->name);
				die ();
			}
		}
	}
	setvisualpage(page);
	if (CD->flags & (DOSYNC|POSTSYNC)) {
		while (inp (port) & 0x08) {	/* wait for Vert Sync start */
			sys_poll (27);
			if (st.lasttime - lasttime > MAX_SYNC_WAIT) {
				LogPrintf ("%s: sync timed out\n", Gr->name);
				die ();
			}
		}
		while (!(inp (port) & 0x08)) {	/* wait for Vert Sync end */
			sys_poll (28);
			if (st.lasttime - lasttime > MAX_SYNC_WAIT) {
				LogPrintf ("%s: sync timed out\n", Gr->name);
				die ();
			}
		}
	}
	return (0);
}

LOCAL_FUNC int FAR
BGISetActive (int page)
{
	setactivepage (page);
	return (0);
}

LOCAL_FUNC void FAR
BGIDrawTo (Uint x, Uint y, Uint color)
{
	if (color != LastColor) {
		setcolor (color);
		LastColor = color;
	}
	lineto (x, y);
}

LOCAL_FUNC int FAR
BGIWriteMode (int mode)
{
	switch (mode) {
	case T_MSET:
		setwritemode (COPY_PUT);
		break;
	case T_MOR:
		exit(99);
		break;
	case T_MXOR:
		setwritemode (XOR_PUT);
		break;
	}
	return (0);
}

LOCAL_FUNC int FAR
BGISetPalette (int index, long c)
{
	setrgbpalette (index, C_RGB_R (c), C_RGB_G (c), C_RGB_B (c));
	return (index);
}

LOCAL_FUNC int FAR
BGIInit (DEVICE *dev, char *options)
{
	long	temp;
	int	gdriver = DETECT;	/* auto detection */
	int	gmode;
	int	errorcode;

/* initialize graphics mode
*/
	initgraph (&gdriver, &gmode, "");
	if (grOk != (errorcode = graphresult ())) {
		LogPrintf ("%s: GrBGI Graphics init error: %s\n", Gr->name,
			grapherrormsg (errorcode));
		return (1);		/* return with error code */
	}

	dev->sizex = getmaxx () +1;
	dev->sizey = getmaxy () +1;

	dev->npages = 1;

	if (get_narg (options, "shutters=", &temp))
		st.misc[7] = (int)temp;
	else
		st.misc[7] = 0;

	Gr->flags |= INITED;

	LogPrintf ("%s: init ok\n", Gr->name);
	return (0);
}

LOCAL_FUNC void FAR
BGITerm (DEVICE *dev)
{
	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	dev = dev;
	closegraph ();

	LogPrintf ("%s: term ok\n", Gr->name);
}

LOCAL_FUNC void FAR
BGIEllipse (Uint x1, Uint y1, Uint rx, Uint ry, Uint color)
{
	if (color != LastColor) {
		setcolor (color);
		LastColor = color;
	}
	ellipse (x1, y1, 0, 0, rx, ry);
}

LOCAL_FUNC int FAR
BGIShutters (int eye)
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

struct GrDriver NEAR GrBGI = {
	"GrBGI",
	0,
	NULL,		/* Extra */
	0,		/* Devices */
	BGIInit,
	BGITerm,
	moveto,
	BGIDrawTo,
	BGISetVisual,
	BGISetActive,
	BGIClear,
	BGIWriteMode,
	BGISetPalette,
	BGIEllipse,
	0, 		/* Polygon */
	0,		/* Flush */
	BGIShutters
};

#undef DOSYNC
#undef BIGPAGE
#undef POSTSYNC
#undef INITED
#undef MAX_SYNC_WAIT

#endif /* ifdef FLY8_BC */
