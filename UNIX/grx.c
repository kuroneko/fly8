/* --------------------------------- grx.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Graphics driver for X11.
 *
 * Support for double buffering added August 19, 1993
 * by Niclas Wiberg (nicwi@isy.liu.se).
*/

#include "fly.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#if HAVE_XPM
#include <X11/xpm.h>
#include "fly8icon.h"
#endif

#include "grx.h"
#include "xkeys.h"

#define GRX_SINGLE	0x0001
#define GRX_DIRECT	0x0002
#define INITED		0x8000

#define APPNAME		"fly8 GrX"
#define NOCOLOR		-1

static Display		*TheDisplay;
static GC		TheGC;
static Window		TheWindow, TheRoot;
static int		TheScreen;
static Colormap		TheColormap;
static unsigned long	FgColor, BgColor;
static int		width = 0, height = 0;
static int		ncolors = 256;
static int		xold = 0, yold = 0;
static int		last_color = -1;

static Pixmap		ThePixmaps[2];
static Drawable		TheDrawable;
static XWindowAttributes	attr;
#if HAVE_XPM
static Pixmap		TheIcon;
#endif

LOCAL_FUNC void FAR
GrxClearWin (Drawable drawable)
{
	XSetForeground (TheDisplay, TheGC, BgColor);
	XFillRectangle (TheDisplay, drawable, TheGC, 0, 0,
		width, height);
	XSetForeground (TheDisplay, TheGC, FgColor);
	last_color = -1;
	repaint ();
}

LOCAL_FUNC void FAR
GrxClear (Uint x, Uint y, Uint sx, Uint sy, Uint color)
{
	XSetForeground (TheDisplay, TheGC, color);
	XFillRectangle (TheDisplay, TheDrawable, TheGC, x, y, sx, sy);
	XSetForeground (TheDisplay, TheGC, FgColor);
	last_color = -1;
}

LOCAL_FUNC void
GrxMoveTo (Uint x, Uint y)
{
	xold = x;
	yold = y;
}

LOCAL_FUNC void
GrxDrawTo (Uint x, Uint y, Uint c)
{
	if (last_color != c) {
		last_color = c;
		XSetForeground (TheDisplay, TheGC, c);
	}
	XDrawLine (TheDisplay, TheDrawable, TheGC, xold, yold, x, y);
	xold = x;
	yold = y;
}

LOCAL_FUNC int FAR
GrxSetActive (int page)
{
	if (Gr->flags & (GRX_DIRECT|GRX_SINGLE))
		return (0);
	TheDrawable = ThePixmaps[page];
	last_color = -1;
	return (0);
}
  
LOCAL_FUNC int FAR
GrxSetVisual (int page)
{
	if (Gr->flags & GRX_DIRECT)
		return (0);
	XCopyArea (TheDisplay,
		ThePixmaps[page],
		TheWindow,
		TheGC,
		0, 0, width, height, 0, 0);
	last_color = -1;
	return (0);
}
  
LOCAL_FUNC int FAR
GrxSetWriteMode (int mode)
{
	switch (mode) {
	default:
	case T_MSET:
		mode = GXcopy;
		break;
	case T_MOR:
		mode = GXor;
		break;
	case T_MXOR:
		mode = GXxor;
		break;
	}
	XSetFunction (TheDisplay, TheGC, mode);
	return (0);
}

LOCAL_FUNC int FAR
GrxSetPalette (int n, long c)
{
	XColor	color;

	if (ncolors < 16) {
		if (CC_BLACK != n)
			n = CC_WHITE;
		return (st.colors[n]);
	}
	if (NOCOLOR != st.colors[n])
		{}	/* FreeThisColor */
	color.red   = C_RGB_R (c) << 8;
	color.green = C_RGB_G (c) << 8;
	color.blue  = C_RGB_B (c) << 8;
	XAllocColor (TheDisplay, TheColormap, &color);
	return (color.pixel);
}

LOCAL_FUNC void FAR
GrxEllipse (Uint x, Uint y, Uint rx, Uint ry, Uint c)
{
	if (last_color != c) {
		last_color = c;
		XSetForeground (TheDisplay, TheGC, c);
	}
	XDrawArc (TheDisplay, TheDrawable, TheGC, x-rx, y-ry, 2*rx, 2*ry,
			0, 360*64);
}

LOCAL_FUNC void FAR
GrxDestroyImage (void)
{
	if (Gr->flags & GRX_DIRECT)
		return;

	XFreePixmap (TheDisplay, ThePixmaps[0]);
	if (!(Gr->flags & GRX_SINGLE))
		XFreePixmap (TheDisplay, ThePixmaps[1]);
}

LOCAL_FUNC void FAR
GrxCreateImage (void)
{
	if (Gr->flags & GRX_DIRECT) {
		TheDrawable = TheWindow;
		GrxClearWin (TheDrawable);
	} else {
		ThePixmaps[0] =
			XCreatePixmap (
				TheDisplay,
				RootWindow (TheDisplay, TheScreen),
				width, height,
				DisplayPlanes (TheDisplay, TheScreen));
		GrxClearWin (ThePixmaps[0]);
		if (!(Gr->flags & GRX_SINGLE)) {
			ThePixmaps[1] =
				XCreatePixmap (
					TheDisplay,
					RootWindow (TheDisplay, TheScreen),
					width, height,
					DisplayPlanes (TheDisplay, TheScreen));
			GrxClearWin (ThePixmaps[1]);
		}
		TheDrawable = ThePixmaps[0];
	}
}

LOCAL_FUNC int FAR
GrxInit (DEVICE *dev, char *options)
{
	XEvent	event;

	Gr->flags = 0;
	if (get_parg (options, "direct")) {
		Gr->flags |= GRX_DIRECT;
		MsgWPrintf (-50, "%s: direct mode", Gr->name);
	} else if (get_parg (options, "single")) {
		Gr->flags |= GRX_SINGLE;
		MsgWPrintf (-50, "%s: single buffering", Gr->name);
	}

	if (dev->sizex == 0 || dev->sizey == 0)
		return (1);
	width  = dev->sizex;
	height = dev->sizey;

	TheDisplay = XOpenDisplay (""); 
	if (!TheDisplay) {
		LogPrintf ("%s: can't open display\n", Gr->name);
		die ();
	}

	TheScreen = DefaultScreen (TheDisplay);
	TheRoot = DefaultRootWindow (TheDisplay);
	st.colors[CC_WHITE] = FgColor = WhitePixel (TheDisplay, TheScreen);
	st.colors[CC_BLACK] = BgColor = BlackPixel (TheDisplay, TheScreen);
	TheWindow = XCreateSimpleWindow (TheDisplay, TheRoot,
		0, 0, width, height, 0, FgColor, BgColor);

#if HAVE_XPM
	XpmCreatePixmapFromData (TheDisplay, TheWindow, fly8_xpm, &TheIcon,
		NULL, NULL);
#endif

	XSelectInput (TheDisplay, TheWindow, 
		ExposureMask|KeyPressMask|StructureNotifyMask);
	XMapRaised (TheDisplay, TheWindow);
	XSetStandardProperties (TheDisplay, TheWindow, APPNAME, APPNAME,
#if HAVE_XPM
		TheIcon,
#else
		NULL,
#endif
		NULL, 0, NULL);
	TheGC = DefaultGC (TheDisplay, TheScreen);

	do {
		XNextEvent (TheDisplay, &event);
	} while (event.type != Expose);

/* Assign colors
*/
	TheColormap = DefaultColormap (TheDisplay, DefaultScreen (TheDisplay));

	XGetWindowAttributes (TheDisplay, DefaultRootWindow (TheDisplay),
		&attr);
	LogPrintf ("%s: Image depth %d\n", Gr->name, attr.depth);

	ncolors = XDisplayCells (TheDisplay, TheScreen);
	if (ncolors < 16)
		ncolors = 2;
	dev->colors = ncolors;
	LogPrintf ("%s: Using %d colors\n", Gr->name, ncolors);

	if (Gr->flags & (GRX_DIRECT|GRX_SINGLE))
		dev->npages = 1;
	else
		dev->npages = 2;

	GrxCreateImage ();

	GrxSetVisual (0);
	GrxSetActive (0);

	Gr->flags |= INITED;

	LogPrintf ("%s: init ok\n", Gr->name);
	return (0);
}

LOCAL_FUNC void FAR
GrxTerm (DEVICE *dev)		/* done */
{
	int	i;

	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	GrxDestroyImage ();

	for (i = 0; i < rangeof (st.colors); ++i)
		if (NOCOLOR == st.colors[i])
			{}	/* FreeThisColor */

	XDestroyWindow (TheDisplay, TheWindow);
	XCloseDisplay (TheDisplay);

	LogPrintf ("%s: term ok\n", Gr->name);
}

LOCAL_FUNC void FAR
GrxFlush (void)
{
	XFlush (TheDisplay);
	XSync  (TheDisplay, 0);
	(void) XEventsQueued (TheDisplay, QueuedAfterFlush);
}

LOCAL_FUNC void FAR
GrxPolygon (int npoints, BUFLINE *points, Uint color)
{
	XPoint	xpoints[50];
	int	i;

	if (npoints > rangeof (xpoints))
		return;

	for (i = 0; i < npoints; ++i) {
		xpoints[i].x =  *points++;
		xpoints[i].y =  *points++;
	}
	XSetForeground (TheDisplay, TheGC, color);
	XFillPolygon (TheDisplay, TheDrawable, TheGC, 
		xpoints, npoints, Convex, CoordModeOrigin);
}

LOCAL_FUNC int FAR
GrxGetMouse (int *win_x, int *win_y, char *btn, int *nbtn)
{
	Window		root, child;
	int		ret, root_x, root_y;
	unsigned int	keys_buttons;

	ret = XQueryPointer (TheDisplay, TheWindow, &root, &child, &root_x,
		&root_y, win_x, win_y, &keys_buttons);

	btn[0] = (char)T(keys_buttons & Button3Mask);
	btn[1] = (char)T(keys_buttons & Button1Mask);
	btn[2] = (char)T(keys_buttons & Button2Mask);
	*nbtn = 3;
	return (0);
}

LOCAL_FUNC void FAR
GrxResize (XConfigureEvent *cf)
{
	sim_set ();
	screen_empty ();

	GrxDestroyImage ();

	set_screen (cf->width, cf->height);

	GrxCreateImage ();

	screen_start ();
	sim_reset ();
}

LOCAL_FUNC int FAR
GrxKread (void)
{
	XEvent	event;
	int	n;

	for (n = -1; n < 0;) {
		if (!XEventsQueued (TheDisplay, QueuedAlready))
			return (-1);
		XNextEvent (TheDisplay, &event);
		switch (event.type) {
 		case  KeyPress:
			n = xGetKey (&event.xkey);
			break;
		case ConfigureRequest:
		case ConfigureNotify:
			GrxResize (&event.xconfigure);
			break;
		default:
			GrxClearWin (TheDrawable);
			break;
		}
	}

	return (n);
}

struct GrxExtra GrxExtra = {
	GrxGetMouse,
	GrxKread
};

struct GrDriver GrX = {
	"GrX",
	0,
	&GrxExtra,
	0,		/* Devices */
	GrxInit,
	GrxTerm,
	GrxMoveTo,
	GrxDrawTo,
	GrxSetVisual,
	GrxSetActive,
	GrxClear,
	GrxSetWriteMode,
	GrxSetPalette,
	GrxEllipse,
	GrxPolygon,
	GrxFlush,
	0		/* Shutters */
};

#undef GRX_SINGLE
#undef GRX_DIRECT
#undef INITED
#undef APPNAME
#undef NOCOLOR
