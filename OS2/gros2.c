/* --------------------------------- grOS2.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Graphics driver for OS/2.
 * OS/2 support by Michael Taylor miket@pcug.org.au
*/

#define INCL_DOS
#define INCL_GPI
#define INCL_WIN
#define INCL_GPIDEFAULTS
#define INCL_GPIPRIMITIVES  /* for parameter definitions */
#include <os2.h>

#include "fly.h"
#include "common.h"

#define INITED    0x8000

extern HAB     	hab;		/* PM anchor block handle              */
extern HMQ	hmq;		/* Message queue handle                */
HWND	ghWndMain;		/* Frame window handle                 */
HWND	hwndClient;		/* Client window handle                */
extern HPS	hps;
static HDC	hdc;
static RECTL	rect;
static ULONG	colors[256*2];
static int	height=0;
static HPAL    	hpal;
static HPAL    	oldhpal;

extern MRESULT EXPENTRY Fly8WndProc (HWND hwnd, ULONG msg, MPARAM mp1,
	MPARAM mp2);

#ifdef TRACK_ENDPOINT
static int	currx = 0, curry = 0, plotpt = 0;
#endif

static long	ncolors = 0;
static void GPIChangeSize (void);


static void
GPIChangeSize ()
{
        RECTL rect;

   	WinQueryWindowRect (hwndClient, &rect);
LogPrintf ("GrGPI: new Rect dimensions are: %d:%d:%d:%d\n",
rect.xLeft, rect.yBottom, rect.xRight, rect.yTop);
TRACE ();

	height = rect.yTop;
   	CS->sizex = (xshort)rect.xRight - (xshort)rect.xLeft;
   	CS->sizey = (xshort)rect.yTop - (xshort)rect.yBottom;
   	set_screen (CS->sizex, CS->sizey);

      	WinFillRect (hps, &rect, CLR_BLACK);

   	repaint ();
}

static LINEBUNDLE bunAttrs; /* Information for color    */

extern void
GPIResetPalette (void)
{
        int	n;
	ULONG	i;

        if (hpal) {
		GpiSelectPalette (hps, hpal);
		n = WinRealizePalette (hwndClient, hps, &i);
      		if (hps != hps) {                                                  /* SM$:M2 */
			GpiSelectPalette (hps, hpal);
			n = WinRealizePalette (hwndClient, hps, &i);
		}
	}
}

static int
GPISetActive (int page)
{
   	page = page;
   	return (0);
}

static int
GPISetVisual (int page)
{
   	page = page;
   	return (0);
}

static int
GPISetWriteMode (int mode)
{
       	switch (mode) {
       	default:
       	case T_MSET:
          	mode = FM_OVERPAINT;
          	break;
       	case T_MOR:
          	mode = FM_OR;
          	break;
       	case T_MXOR:
          	mode = FM_XOR;
          	break;
       	}
   	GpiSetMix (hps, mode);

   	return (0);
}

static long maxcolors=0;

static int
GPISetPalette (int n, long c)
{
	ULONG	i, x;
	ULONG	r, g, b;


   	if (ncolors < 16 && (CC_BLACK != n && CC_WHITE != n))
      		return (st.colors[CC_WHITE]);

	r = C_RGB_R (c);
	g = C_RGB_G (c);
	b = C_RGB_B (c);
	
	c = (r << 16) + (g << 8) + b;
	c = r * 65536 + g * 256 + b;
   	colors[n] = c;

	if (n > maxcolors)
	        maxcolors = n;
	
	if (oldhpal  != NULLHANDLE)
	        GpiSelectPalette (hps, oldhpal);
	if (hpal != NULLHANDLE)  
	        GpiDeletePalette (hpal);
	WinRealizePalette (hwndClient, hps, &i);
	WinRealizePalette (hwndClient, hps, &i);
	WinRealizePalette (hwndClient, hps, &i);

   	hpal = GpiCreatePalette (hab, 0L, LCOLF_CONSECRGB,
		maxcolors, colors);

	oldhpal = GpiSelectPalette (hps, hpal);

/*	GpiSetPaletteEntries (hpal, LCOLF_CONSECRGB, 20, maxcolors, colors);*/

	i = maxcolors;
	x = WinRealizePalette (hwndClient, hps, &i);
	
	return (n);
}

static Uint    last_color = 0xffff;

static void
GPIMoveTo (Uint x, Uint y)
{
	POINTL	pt;

#ifdef TRACK_ENDPOINT
   	if (plotpt) {
		pt.x = currx;
		pt.y = height - curry;
		plotpt = 0;
      		GpiSetPel (hps, &pt);
	}
   	currx = x;
   	curry = y;
#endif

	pt.x = x;
	pt.y = height - y;

	GpiMove (hps, &pt);
}

static void
GPIDrawTo (Uint x, Uint y, Uint c)
{
	POINTL	pt;

   	if (last_color != c) {
		GpiSetColor (hps, c);
	      	last_color = c;
	}

	pt.x = x;
	pt.y = height - y;

	GpiLine (hps, &pt);

#ifdef TRACK_ENDPOINT
   	currx = x;
   	curry = y;
   	plotpt = 1;
#endif
}

static void
GPIClear (Uint x, Uint y, Uint sx, Uint sy, Uint c)
{
   	rect.xLeft   = x;
   	rect.yTop    = y + sy;
   	rect.xRight  = x + sx;
   	rect.yBottom = y;

      	WinFillRect (hps, &rect, CLR_BLACK);

#ifdef TRACK_ENDPOINT
   	currx = x;
   	curry = y;
   	plotpt = 0;
#endif
}

static void
GPIPolygon (int npoints, BUFLINE *points, Uint c)
{
        POLYGON	p;
   	POINTL	wpoints[50];
   	int	i;

   	if (npoints > rangeof (wpoints))
      		return;

	p.ulPoints = npoints;
	p.aPointl  = wpoints;

   	for (i = 0; i < npoints; ++i) {
      		wpoints[i].x =  *points++;
      		wpoints[i].y =  height - *points++;
   	}
	bunAttrs.lColor = c;
	bunAttrs.lBackColor = c;
	GpiSetDefAttrs (hps, PRIM_AREA,
		ABB_BACK_COLOR | ABB_COLOR, &bunAttrs);
   	GpiPolygons (hps, npoints, &p, 0, POLYGON_EXCL);
}

static void
GPITerm (DEVICE *dev)
{
   	dev = dev;

	GpiSelectPalette (hps, oldhpal);
	GpiDeletePalette (hpal);
	/* Process for termination */
	GpiDestroyPS (hps);
	WinDestroyWindow ( ghWndMain );

   	hpal = 0;

      	hps = 0;
   	hps = 0;
      	ghWndMain = 0;

   	if (Gr->flags & INITED) {
      		Gr->flags &= ~INITED;
      		LogPrintf ("%s: term ok\n", Gr->name);
   	}
}

static int
GPIInit (DEVICE *dev, char * options)
{
   	int	r, g, b, n;
	int	x, y;
   	ULONG	i, flCreate;
  	SIZEL sizl = { 0L, 0L };

TRACE ();
   	if (Gr->flags & INITED) {
      		LogPrintf ("%s: init - already inited\n", Gr->name);
      		return (1);
   	}

   	if (dev->sizex == 0 || dev->sizey == 0) {
      		LogPrintf ("%s: init - bad window sizes\n", Gr->name);
      		return (1);
   	}

   	ghWndMain = 0;

TRACE ();
#define ID_WINDOW	256

   	/* Register a window class, and create a standard window. */
  	if (!WinRegisterClass(hab, (PSZ)"Fly8Window", (PFNWP)Fly8WndProc,
		     CS_SIZEREDRAW, 0)) {
           	WinMessageBox( HWND_DESKTOP, HWND_DESKTOP,
            		(PSZ)"GrOS2 - WinRegisterClass call failed",
            		(PSZ)"Fly8", 0, MB_OK | MB_INFORMATION );
           	WinDestroyMsgQueue ( hmq );
           	WinTerminate ( hab );
           	return ( 1 );
	}
TRACE ();
   	flCreate = FCF_STANDARD &
        	  ~FCF_SHELLPOSITION &
                  ~FCF_MENU;
  	if ((ghWndMain = WinCreateStdWindow(HWND_DESKTOP, 0, &flCreate,
               "Fly8Window", "", 0, (HMODULE)0L, ID_WINDOW, 
	       &hwndClient)) == 0L) {
           	WinMessageBox( HWND_DESKTOP, HWND_DESKTOP,
            		(PSZ)"GrOS2: WinCreateStdWindow call failed",
            		(PSZ)"Fly8", 0, MB_OK | MB_INFORMATION );
           	WinDestroyMsgQueue ( hmq );
           	WinTerminate ( hab );
           	return ( 1 );
	}
    	WinSetWindowText(ghWndMain, "Fly8 by Eyal Lebedinsky");

   	x = ( (LONG)WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN)
                       - dev->sizex ) / 2;
   	y = ( (LONG)WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN)
                       - dev->sizey ) / 2;
	/* Shows and activates frame    */
  	if (!WinSetWindowPos( ghWndMain,
        	        HWND_TOP,
                	x/*0*/, y/*0*/, dev->sizex, dev->sizey,
                   	SWP_SIZE | SWP_MOVE | SWP_ACTIVATE | SWP_SHOW)) {
           	WinMessageBox( HWND_DESKTOP, HWND_DESKTOP,
            		(PSZ)"GrOS2 - WinSetWindowPos call failed",
            		(PSZ)"Fly8", 0, MB_OK | MB_INFORMATION );
           	WinDestroyMsgQueue ( hmq );
           	WinTerminate ( hab );
           	return ( 1 );
	}

       	hdc = WinOpenWindowDC (hwndClient);
	sizl.cx = sizl.cy = 0;
        hps = GpiCreatePS (hab, hdc, &sizl,
                           PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);
   	WinQueryWindowRect (hwndClient, &rect);
LogPrintf ("GrGPI: Rect dimensions are: %d:%d:%d:%d\n",
rect.xLeft, rect.yBottom, rect.xRight, rect.yTop);
TRACE ();

	height = rect.yTop;
   	CS->sizex = (xshort)rect.xRight - (xshort)rect.xLeft;
   	CS->sizey = (xshort)rect.yTop - (xshort)rect.yBottom;
   	set_screen (CS->sizex, CS->sizey);

      	WinFillRect (hps, &rect, CLR_BLACK);

   	for (i = 0, r = 0, g = 127, b = 127; i < 16;
               i++, r += 1, g += 1, b += 1) {
		colors[i] = r * 65536 + g * 256 + b;
   	}
   	hpal = GpiCreatePalette (hab, LCOL_PURECOLOR, LCOLF_CONSECRGB,
		16, colors);

	oldhpal = GpiSelectPalette (hps, hpal);

	n = WinRealizePalette (hwndClient, hps, &i);

	DevQueryCaps (GpiQueryDevice(hps), CAPS_COLORS, 1L, (PLONG)&ncolors);
	dev->colors = ncolors;

	dev->npages = 1;

   	usingDive = 0;

   	Gr->flags |= INITED;

   	LogPrintf ("%s: init ok\n", Gr->name);
TRACE ();
   	return (0);

badret:
   	GPITerm (dev);
   	return (1);
}

static void
GPIFlush (void)
{
   	if (resetSSize) {
           	GPIChangeSize ();
           	resetSSize = 0;
   	}
}

struct GrDriver  GrOS2S = {
   "GrGPI",
   0,
   NULL,	/* Extra */
   0,		/* Devices */
   GPIInit,
   GPITerm,
   GPIMoveTo,
   GPIDrawTo,
   GPISetVisual,
   GPISetActive,
   GPIClear,
   GPISetWriteMode,
   GPISetPalette,
   0,		/* Ellipse */
   GPIPolygon,
   GPIFlush,
   0		/* Shutters */
};
#undef INITED
