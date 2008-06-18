/* -------------------------------- grBitMap.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Graphics driver for OS/2. using off-screen BitMap and BitBlt.
 * Supports DIVE and GPI BitBlt.
 * OS/2 support by Michael Taylor miket@pcug.org.au
*/

#ifdef HAVE_DIVE

#define INCL_DOS
#define INCL_GPI
#define INCL_WIN
#define INCL_GPIDEFAULTS
#define INCL_GPIPRIMITIVES  /* for parameter definitions */
#include <os2.h>
#define  _MEERROR_H_
#include <mmioos2.h>        /* It is from MMPM toolkit           */
#include <dive.h>
#include <fourcc.h>

/* suppress some WIN31/WIN32 variables from common.h */
#define DIVE1

#include "fly.h"
#include "common.h"
#include "bgr.h"

#define GRG_STATS	0x0001
#define GRG_VGR		0x0002
#define INITED		0x8000

static HPS	memhps = 0;
static HDC	hdc = 0;
static HDC	memhdc = 0;
static ULONG	colors[256];
static ULONG	xcolors[256];
static int	height = 0;
static HPAL    	hpal = 0;
static HPAL    	oldhpal = 0;
static LONG	index[256];
static SIZEL	sizl;
static HBITMAP	hbm = 0;
static HBITMAP	oldhbm = 0;
static BITMAPINFOHEADER2 bmih;
static BITMAPINFOHEADER2 bmp2data;
static PBITMAPINFO2 pbmi = 0;
static BYTE	*Bitmap = NULL;
static POINTL	aptl[3] = {{0u, 0u}, {800, 600}, {0u, 0u}};

static DIVE_CAPS DiveCaps = {0};
static FOURCC	fccFormats[100] = {0};
static ULONG	bufNum = 0;
static HDIVE  	hDive = 0;		/* DIVE handle */
static FOURCC 	fccColorFormat = 0;	/* Bitmap color format */
static ULONG	ulScanLineBytes;	/* output for number of bytes a scan line */
static ULONG	ulScanLines;		/* output for height of the buffer */
static SETUP_BLITTER SetupBlitter;	/* structure for DiveSetupBlitter */

static long	ncolors = 0;
static long 	maxcolors = 0;
static long 	bufLineLen = 0;

extern MRESULT EXPENTRY Fly8WndProc ( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );

LOCAL_FUNC void
BMbMoveTo (Uint x, Uint y)
{
	bMoveTo (x, height - y);
}

LOCAL_FUNC void
BMbDrawTo (Uint x, Uint y, Uint c)
{
	bDrawTo (x, height - y, c);
}

LOCAL_FUNC void
BMbEllipse (Uint x, Uint y, Uint rx, Uint ry, Uint c)
{
	bDrawEllipse (x, height - y, rx, ry, c);
}

LOCAL_FUNC void
BMbPolygon (int npoints, BUFLINE *points, Uint c)
{
   	BUFLINE p[200];
   	int	i;
   	if (npoints > 100)
      		return;

   	for (i = 0; i < npoints; ++i) {
      		p[i*2] =  *points++;
      		p[i*2+1] =  height - *points++;
   	}
   	bPolygon (npoints, p, c);
}

LOCAL_FUNC void
BMbClear (Uint x, Uint y, Uint sx, Uint sy, Uint c)
{
	bClear (x, height - (y + sy), sx, sy, c);
}


void DiveResetPalette (void);

static int
BMChangeSize (void)
{
        RECTL 	rect;
   	SWP     swp;
   	RECTL   rcl;
   	POINTL  pointl;
	int	moveonly=0;

   	WinQueryWindowRect (hwndClient, &rect);

   	if ((newx == (xshort)rect.xRight - (xshort)rect.xLeft) &&
   	    (newy == (xshort)rect.yTop - (xshort)rect.yBottom)) {
        		if (hDive)
			        moveonly = 1;
			else
		        	return 0; /* no change */
	}

LogPrintf ("%s: Rect dimensions are: %ld:%ld:%ld:%ld\n", Gr->name,
rect.xLeft, rect.yBottom, rect.xRight, rect.yTop);
TRACE ();

	newx = rect.xRight - rect.xLeft;
   	height = newy = rect.yTop - rect.yBottom;

      	WinFillRect (hps, &rect, CLR_BLACK);
	if (hDive) {
   		HRGN    hrgn;
   		RGNRECT rgnCtl;
     		RECTL   rctls[50];

     		hrgn = GpiCreateRegion( hps, 0, NULL);
		if (hrgn) {
     			WinQueryVisibleRegion( ghWndMain, hrgn);
               		rgnCtl.ircStart     = 0;
               		rgnCtl.crc          = 50;
               		rgnCtl.ulDirection  = 1;
     			GpiQueryRegionRects( hps, hrgn, NULL, &rgnCtl, rctls);
     			SetupBlitter.ulNumDstRects = rgnCtl.crcReturned;
     			SetupBlitter.pVisDstRects = rctls;
               		GpiDestroyRegion( hps, hrgn );
		} else {
        		rcl.xLeft   = 0;
        		rcl.yBottom = 0;
               		rcl.xRight  = newx;
               		rcl.yTop    = newy;
            		SetupBlitter.ulNumDstRects = 1;
               		SetupBlitter.pVisDstRects = &rcl;
		}

        	/* Now find the window position and size, relative to parent. */
        	WinQueryWindowPos (hwndClient, &swp );

        	/* Convert the point to offset from desktop lower left. */
        	pointl.x = swp.x;
        	pointl.y = swp.y;

        	WinMapWindowPoints (ghWndMain, HWND_DESKTOP, &pointl, 1);
/*
LogPrintf ("%s: Screen Pos is: %ld:%ld\n", Gr->name, pointl.x, pointl.y);
LogPrintf ("%s: Screen width/height= %ld:%ld\n", Gr->name, swp.cx, swp.cy);
TRACE ();
*/
		fccColorFormat = FOURCC_LUT8;

        	/* Tell DIVE about the new settings. */
        	SetupBlitter.ulStructLen = sizeof ( SETUP_BLITTER );
        	SetupBlitter.fccSrcColorFormat = fccColorFormat;
        	SetupBlitter.ulSrcWidth = newx;
        	SetupBlitter.ulSrcHeight = newy;
        	SetupBlitter.ulSrcPosX = 0;
        	SetupBlitter.ulSrcPosY = 0;
        	SetupBlitter.ulDitherType = 1;
        	SetupBlitter.fInvert = FALSE;
        	SetupBlitter.lDstPosX = 0;
        	SetupBlitter.lDstPosY = 0;
        	SetupBlitter.fccDstColorFormat = FOURCC_SCRN;
        	SetupBlitter.lScreenPosX = pointl.x;
        	SetupBlitter.lScreenPosY = pointl.y;
        	SetupBlitter.ulDstWidth = swp.cx;
        	SetupBlitter.ulDstHeight = swp.cy;

        	DiveSetupBlitter ( hDive, &SetupBlitter );

		if (moveonly && bufNum != 0)
		        goto finsetup;

	        if (bufNum) {
		      	DiveEndImageBufferAccess ( hDive, bufNum );
		       	DiveFreeImageBuffer ( hDive, bufNum );
		}
		bufNum = 0;

   		/* Allocate a buffer for image data */
   		if ( DiveAllocImageBuffer ( hDive, &bufNum, fccColorFormat,
                                 newx, newy, 0, 0 )) {
			LogPrintf ("GrDive Init: Allocation of buffer failed\n");
			return( 1 );
      		}

   		if ( DiveBeginImageBufferAccess ( hDive, bufNum, &Bitmap,
                                       &ulScanLineBytes, &ulScanLines )) {
      			DiveFreeImageBuffer ( hDive, bufNum );
			LogPrintf ("GrDive Init: could not get access to image buffer\n");
      			return( 1 );
      		}
		bufLineLen = ulScanLineBytes;
/*		LogPrintf ("GrDive Init: Bufnum=%ld, bufLineLen=%ld\n",
			(long)bufNum, bufLineLen);*/
		LogPrintf ("BitMap at %p\n", (char  *)Bitmap);
TRACE ();
	} else {
TRACE ();
		bufLineLen = newx;
		if (oldhbm)
       			GpiSetBitmap (memhps, oldhbm);
		if (hbm)
			GpiDeleteBitmap (hbm);
		/* Create bitmap for memory image of screen. */
        	memset(&bmih, 0, sizeof(bmih));
        	bmih.cbFix = sizeof(bmih);
        	bmih.cx = newx;
        	bmih.cy = newy;
        	bmih.cPlanes = 1;
        	bmih.cBitCount = 8;
        	hbm = GpiCreateBitmap (memhps, &bmih, 0L, NULL, NULL);
        	oldhbm = GpiSetBitmap (memhps, hbm);

		if (Bitmap)
	        	DosFreeMem (Bitmap);
		Bitmap = 0;
		if (DosAllocMem ((PPVOID)&Bitmap, newx * newy,
			PAG_COMMIT | PAG_READ | PAG_WRITE)) {
           		WinMessageBox( HWND_DESKTOP, HWND_DESKTOP,
            			(PSZ)"GrBitMap - DosAllocMem call failed",
            			(PSZ)"Fly8", 0, MB_OK | MB_INFORMATION );
           		return ( 1 );
		}
		LogPrintf ("BitMap at %p\n", (char *)Bitmap);
TRACE ();
	        aptl[0].x = 0;
	        aptl[0].y = 0;
	        aptl[1].x = newx;
	        aptl[1].y = newy;
	        aptl[2].x = 0;
	        aptl[2].y = 0;
	        memset (Bitmap, 0, newx * newy);
TRACE ();

  	        bmp2data.cbFix = 16L;
  	        GpiQueryBitmapInfoHeader(hbm, &bmp2data);
TRACE ();
		if (pbmi)
	        	DosFreeMem (pbmi);
		pbmi = 0;
  	        DosAllocMem((PPVOID)&pbmi, sizeof(BITMAPINFO2) +
		        (sizeof(RGB2) * (1 << bmp2data.cPlanes) *
                        (1 << bmp2data.cBitCount)),
                        PAG_COMMIT | PAG_READ | PAG_WRITE);
TRACE ();
  	        pbmi->cbFix = bmp2data.cbFix;
  	        pbmi->cx = bmp2data.cx;
  	        pbmi->cy = bmp2data.cy;
  	        pbmi->cPlanes = bmp2data.cPlanes;
  	        pbmi->cBitCount = bmp2data.cBitCount;
  	        pbmi->ulCompression = bmp2data.ulCompression;
  	        pbmi->cbImage = bmp2data.cbImage;
  	        pbmi->cxResolution = bmp2data.cxResolution;
  	        pbmi->cyResolution = bmp2data.cyResolution;
  	        pbmi->cclrUsed = bmp2data.cclrUsed;
  	        pbmi->cclrImportant = bmp2data.cclrImportant;
  	        pbmi->usUnits = bmp2data.usUnits;
  	        pbmi->usReserved = bmp2data.usReserved;
  	        pbmi->usRecording = bmp2data.usRecording;
  	        pbmi->usRendering = bmp2data.usRendering;
  	        pbmi->cSize1 = bmp2data.cSize1;
  	        pbmi->cSize2 = bmp2data.cSize2;
  	        pbmi->ulColorEncoding = bmp2data.ulColorEncoding;
  	        pbmi->ulIdentifier = bmp2data.ulIdentifier;

		DiveResetPalette ();
TRACE ();
      	}
finsetup:
	        
	bSetSize (newx, newy, bufLineLen);
	bSetActive ((char *)Bitmap);
	bSetWriteMode (T_MSET);
TRACE ();

   	set_screen (newx, newy);

   	repaint ();

TRACE ();
	return (0);
}

static int
BMSetActive (int page)
{
   	page = page;
   	return (0);
}

static int
BMSetVisual (int page)
{
   	page = page;
   	return (0);
}

static int
BMSetPalette (int n, long c)
{
	ULONG	i, x;
	ULONG	r, g, b;
  	POINTL 	coords;

   	if (ncolors < 16 && (CC_BLACK != n && CC_WHITE != n))
      		return (st.colors[CC_WHITE]);

	r = C_RGB_R (c);
	g = C_RGB_G (c);
	b = C_RGB_B (c);
	
	c = r * 65536 + g * 256 + b;
   	colors[n] = c;

	if (n >= maxcolors)
	        maxcolors = n + 1;
	
	if (oldhpal  != NULLHANDLE) {
	        GpiSelectPalette (hps, oldhpal);
	}
	if (hpal != NULLHANDLE)  
	        GpiDeletePalette (hpal);
	WinRealizePalette (hwndClient, hps, &i);
	WinRealizePalette (hwndClient, hps, &i);
	WinRealizePalette (hwndClient, hps, &i);

   	hpal = GpiCreatePalette (hab, 0L, LCOLF_CONSECRGB, maxcolors, colors);

	oldhpal = GpiSelectPalette (hps, hpal);

	i = maxcolors;
	x = WinRealizePalette (hwndClient, hps, &i);

	GpiCreateLogColorTable(memhps, (ULONG) LCOL_PURECOLOR,
		(LONG) LCOLF_CONSECRGB, (LONG) 0L, (LONG) 33L, (PLONG) colors);
      	GpiSetBackMix(memhps, BM_OVERPAINT);

  	/* Determine mapping from logical color value to bitmap color table
     	   index.  Anybody know a more direct way??? */
  	for (i = 0; i < maxcolors; i++) {
      		GpiSetColor(memhps, (LONG) i);
      		coords.x = i;
      		coords.y = 0;
      		GpiSetPel(memhps, &coords);
  	}
  	GpiQueryBitmapBits(memhps, 0L, CS->sizey, &Bitmap[0], pbmi);
  	for (i = 0; i < maxcolors; i++) {
	    	index[i] = (unsigned char)Bitmap[i];
#if 0
LogPrintf ("%s: color %d is %ld\n", Gr->name, i, index[i]);
#endif
TRACE ();
	}

	return ((int)index[n]);
}

static void
BMTerm (DEVICE *dev)
{
   	dev = dev;

TRACE ();
	if (oldhpal != NULLHANDLE)
	        GpiSelectPalette (hps, oldhpal);

	if (hpal != NULLHANDLE)
		GpiDeletePalette (hpal);
	
       	GpiSetBitmap (memhps, oldhbm);
	GpiDeleteBitmap (hbm);
	/* Process for termination */
	if (memhps != NULLHANDLE)
		GpiDestroyPS (memhps);
	if (hps != NULLHANDLE)
		GpiDestroyPS (hps);
	if (ghWndMain != NULLHANDLE)
		WinDestroyWindow ( ghWndMain );

	if (Bitmap)
	        DosFreeMem (Bitmap);
	if (pbmi)
        	DosFreeMem (pbmi);
	pbmi = 0;
	Bitmap = 0;
   	hpal = 0;
      	memhps = 0;
   	hps = 0;
      	ghWndMain = 0;
	hbm = oldhbm = 0;

	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	if (Gr->flags & GRG_STATS)
		LogStats ();

   	if (Gr->flags & INITED) {
      		Gr->flags &= ~INITED;
      		LogPrintf ("%s: term ok\n", Gr->name);
   	}
}

int init (DEVICE *dev, char * options)
{
   	int	r, g, b, n;
	int	x, y;
   	ULONG	i, flCreate;
  	SIZEL sizl = { 0L, 0L };

TRACE ();
	if (get_parg (options, "stats"))
		Gr->flags |= GRG_STATS;

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
   	/* Register a window class, and create a standard window. */
  	if (!WinRegisterClass(hab, (PSZ)"Fly8Window", (PFNWP)Fly8WndProc,
		     CS_SIZEREDRAW | CS_MOVENOTIFY, 0)) {
           	WinMessageBox( HWND_DESKTOP, HWND_DESKTOP,
            		(PSZ)"GrOS2 - WinRegisterClass call failed",
            		(PSZ)"Fly8", 0, MB_OK | MB_INFORMATION );
           	return ( 1 );
	}
TRACE ();
#define ID_WINDOW	256
   	flCreate = FCF_STANDARD &
        	  ~FCF_SHELLPOSITION &
                  ~FCF_MENU;
  	if ((ghWndMain = WinCreateStdWindow(HWND_DESKTOP, WS_VISIBLE, &flCreate,
               "Fly8Window", "", WS_VISIBLE, (HMODULE)0L, ID_WINDOW,
	       &hwndClient)) == 0L) {
           	WinMessageBox( HWND_DESKTOP, HWND_DESKTOP,
            		(PSZ)"GrOS2: WinCreateStdWindow call failed",
            		(PSZ)"Fly8", 0, MB_OK | MB_INFORMATION );
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
                	x/*0*/, y/*0*/, dev->sizex,
			dev->sizey,
                   	SWP_SIZE | SWP_MOVE | SWP_ACTIVATE | SWP_SHOW)) {
           	WinMessageBox( HWND_DESKTOP, HWND_DESKTOP,
            		(PSZ)"GrOS2 - WinSetWindowPos call failed",
            		(PSZ)"Fly8", 0, MB_OK | MB_INFORMATION );
           	return ( 1 );
	}

     	hdc = WinOpenWindowDC (hwndClient);
	sizl.cx = sizl.cy = 0;
        hps = GpiCreatePS (hab, hdc, &sizl,
                           PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);

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

TRACE ();
	return (0);
}

static int
BMInit (DEVICE *dev, char * options)
{
        /* Create presentation space for memory image of client window. */
        memhdc = DevOpenDC(hab, OD_MEMORY, (PSZ) "*", 0L, 0L, hdc);
        sizl.cx = 0;
        sizl.cy = 0;
        memhps = GpiCreatePS(hab, memhdc, &sizl,
		        PU_PELS | GPIT_MICRO | GPIA_ASSOC | GPIF_DEFAULT);

        /* do common initialisations - create window etc */
        if (init (dev, options))
	        return (1);

	if (BMChangeSize ())
	        return (1);

TRACE ();
   	usingDive = 1;
   	Gr->flags |= INITED;

   	LogPrintf ("%s: init ok\n", Gr->name);
TRACE ();
   	return (0);

badret:
   	BMTerm (dev);
   	return (1);
}

/* both GrBitMap and GrDive share this reset palette routine */
void
DiveResetPalette (void)
{
        int	n;
	ULONG	i;
  	POINTL 	coords;

        if (hpal) {
		if (oldhpal  != NULLHANDLE) {
	        	GpiSelectPalette (hps, oldhpal);
		}
		if (hpal != NULLHANDLE)
	        	GpiDeletePalette (hpal);
		WinRealizePalette (hwndClient, hps, &i);
		WinRealizePalette (hwndClient, hps, &i);
		WinRealizePalette (hwndClient, hps, &i);

   		hpal = GpiCreatePalette (hab, 0L, LCOLF_CONSECRGB, maxcolors, colors);

		oldhpal = GpiSelectPalette (hps, hpal);

		i = maxcolors;
		n = WinRealizePalette (hwndClient, hps, &i);

		GpiCreateLogColorTable(memhps, (ULONG) LCOL_PURECOLOR,
			(LONG) LCOLF_CONSECRGB, (LONG) 0L, 
			(LONG) 33L, (PLONG) colors);
      		GpiSetBackMix(memhps, BM_OVERPAINT);

  		/* Determine mapping from logical color value to bitmap color table
     	   	index.  Anybody know a more direct way??? */
  		for (i = 0; i < maxcolors; i++) {
      			GpiSetColor(memhps, (LONG) i);
      			coords.x = i;
      			coords.y = 0;
      			GpiSetPel(memhps, &coords);
  		}
  		GpiQueryBitmapBits(memhps, 0L, CS->sizey, &Bitmap[0], pbmi);
  		for (i = 0; i < maxcolors; i++) {
	    		index[i] = Bitmap[i];
		}
	}
	if (hDive) {
		DiveSetSourcePalette ( hDive, 0, maxcolors, (BYTE *)colors );

     		/* Query the physical palette from PM */
     		GpiQueryRealColors (hps, 0, 0, 256, (PLONG)xcolors);

     		/* Pass it to DIVE */
     		DiveSetDestinationPalette( hDive, 0, 256, (PBYTE)xcolors);
	}
}

static int
DiveSetPalette (int n, long c)
{
	ULONG	i, x;
	ULONG	r, g, b;

   	if (ncolors < 16 && (CC_BLACK != n && CC_WHITE != n))
      		return (st.colors[CC_WHITE]);

	r = C_RGB_R (c);
	g = C_RGB_G (c);
	b = C_RGB_B (c);
	
	c = r * 65536 + g * 256 + b;
   	colors[n] = c;

	if (n >= maxcolors)
	        maxcolors = n + 1;
	
	if (oldhpal  != NULLHANDLE) {
	        GpiSelectPalette (hps, oldhpal);
	}
	if (hpal != NULLHANDLE)  
	        GpiDeletePalette (hpal);
	WinRealizePalette (hwndClient, hps, &i);
	WinRealizePalette (hwndClient, hps, &i);
	WinRealizePalette (hwndClient, hps, &i);

   	hpal = GpiCreatePalette (hab, 0L, LCOLF_CONSECRGB, maxcolors, colors);

	oldhpal = GpiSelectPalette (hps, hpal);

	i = maxcolors;
	x = WinRealizePalette (hwndClient, hps, &i);

	DiveResetPalette ();

	return (n);
}

LOCAL_FUNC void
DiveTerm (DEVICE *dev)
{
TRACE ();
      	WinSetVisibleRegionNotify (hwndClient, FALSE);

      	DiveEndImageBufferAccess ( hDive, bufNum );
   	
   	/* Free the buffers allocated by DIVE and close DIVE */
       	DiveFreeImageBuffer ( hDive, bufNum );
	DiveClose ( hDive );

TRACE ();
   	/* Process for termination */
	if (oldhpal != NULLHANDLE)
	        GpiSelectPalette (hps, oldhpal);

	if (hpal != NULLHANDLE)
		GpiDeletePalette (hpal);
	
	if (hps != NULLHANDLE)
		GpiDestroyPS (hps);

TRACE ();
/*
	if (ghWndMain != NULLHANDLE)
		WinDestroyWindow ( ghWndMain );
*/
TRACE ();

	dev = dev;
	bufNum = 0;
	hDive = 0;
TRACE ();
	hps = memhps = 0;
/*	ghWndMain = 0;*/
	hpal = 0;

TRACE ();
	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	if (Gr->flags & GRG_STATS)
		LogStats ();

	LogPrintf ("%s: term ok\n", Gr->name);
TRACE ();
}

LOCAL_FUNC int
DiveInit (DEVICE *dev, char * options)
{
        /* Get the screen capabilities */
        DiveCaps.pFormatData = fccFormats;
        DiveCaps.ulFormatLength = 120;
        DiveCaps.ulStructLen = sizeof(DIVE_CAPS);

        if ( DiveQueryCaps ( &DiveCaps, DIVE_BUFFER_SCREEN )) {
        	WinMessageBox( HWND_DESKTOP, HWND_DESKTOP,
            		(PSZ)"Cannot use GrDive - DIVE not supported",
            		(PSZ)"Fly8", 0, MB_OK | MB_INFORMATION );
        	return ( 1 );
        }

        /* Get an instance of DIVE APIs. */
        if ( DiveOpen ( &hDive, FALSE, 0 )) {
           	WinMessageBox( HWND_DESKTOP, HWND_DESKTOP,
            		(PSZ)"GrDive - DiveOpen call failed",
            		(PSZ)"Fly8", 0, MB_OK | MB_INFORMATION );
           	return ( 1 );
	}
	
        /* do common initialisations - create window etc */
        if (init (dev, options))
	        return (1);
	memhps = hps;

   	/* Turn on visible region notification. */
   	WinSetVisibleRegionNotify (hwndClient, TRUE);

	newx = newy = -1;
	if (BMChangeSize ())
	        return (1);

	usingDive = 1;
   	Gr->flags |= INITED;
   	LogPrintf ("%s: init ok\n", Gr->name);
TRACE ();

	return (0);

badret:
	DiveTerm (dev);
	return (1);
}

static void
BMFlush (void)
{
   	if (resetSSize) {
           	BMChangeSize ();
           	resetSSize = 0;
   	} else {
		GpiSetBitmapBits(memhps, 0L, (LONG) height, Bitmap, pbmi);
		GpiBitBlt(hps, memhps, 3L, aptl, ROP_SRCCOPY, BBO_AND);
	}
}

static void
DiveFlush (void)
{
	if (0 == resetSSize) {
	      	DiveBlitImage (hDive, bufNum, DIVE_BUFFER_SCREEN);
/* Let other threads and processes have some time.*/
      		DosSleep (0);
	        return;
   	} else if (2 == resetSSize) { /* do nothing */
	        return;
	} else if (resetSSize < 0) {
            	DiveSetupBlitter ( hDive, 0 );
	        resetSSize = 2;
	        return;
	} else if (1 == resetSSize) {
           	BMChangeSize ();
           	resetSSize = 0;
	        return;
   	}
}

struct GrDriver  GrBitMap = {
   	"GrBitMap",
   	0,
   	NULL,			/* Extra */
   	0,			/* Devices */
   	BMInit,
   	BMTerm,
	BMbMoveTo,
	BMbDrawTo,
   	BMSetVisual,
   	BMSetActive,
   	BMbClear,
   	bSetWriteMode,
   	BMSetPalette,
   	BMbEllipse,		/* Ellipse */
   	BMbPolygon,
   	BMFlush,
   	0			/* Shutters */
};

struct GrDriver  GrDive = {
	"GrDive",
	0,
	NULL,		/* Extra */
	0,		/* Devices */
	DiveInit,
	DiveTerm,
	bMoveTo,
	bDrawTo,
	BMSetVisual,
	BMSetActive,
	bClear,
	bSetWriteMode,
	DiveSetPalette,
	bDrawEllipse,
	bPolygon,
	DiveFlush,
	0		/* Shutters */
};

#undef INITED
#endif
