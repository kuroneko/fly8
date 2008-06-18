/* --------------------------------- grwing.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Graphics driver for Microsoft Windows Wing.
 * Windows support by Michael Taylor miket@pcug.org.au
*/

#include <windows.h>

#include "fly.h"

#ifdef USE_WING

#include <wing.h>

#include "common.h"
#include "bgr.h"
#include "vgr.h"

extern int FAR ResetPalette (void);

#define GRG_STATS	0x0001
#define GRG_VGR		0x0002
#define GRG_INVERTED	0x0004
#define INITED		0x8000

#define COLOR_PLANE	16

static HDC		memhdc = 0;
static HBITMAP		mybm1 = 0, oldbm = 0;
static RECT		rect;
static COLORREF		*colors = 0;
static HPALETTE		hpal = 0;
static RGBQUAD		*aPaletteRGB = 0;
static void FAR 	*p1SurfaceBits = 0;
static struct Info {
	BITMAPINFOHEADER	InfoHeader;
	RGBQUAD			ColorTable[256];
}			*Info;
static PALETTEENTRY	*aPalette = 0;
static LOGPALETTE	*plgpl;
static Uint		rcolor[256];
static int		height = 0;


LOCAL_FUNC void FAR
WinbMoveTo (Uint x, Uint y)
{
	bMoveTo (x, height - y);
}

LOCAL_FUNC void FAR
WinbDrawTo (Uint x, Uint y, Uint color)
{
	bDrawTo (x, height - y, color);
}

LOCAL_FUNC void FAR
WinbEllipse (Uint x, Uint y, Uint rx, Uint ry, Uint c)
{
	bDrawEllipse (x, height - y, rx, ry, c);
}

LOCAL_FUNC void FAR
WinbClear (Uint x, Uint y, Uint sx, Uint sy, Uint c)
{
	bClear (x, (Gr->flags & GRG_INVERTED) ? height - (y + sy) : y,
		sx, sy, c);
}

LOCAL_FUNC void FAR
WinbPolygon (int npoints, BUFLINE *points, Uint color)
{
	BUFLINE	lpoints[50], *lp;
	int	i;

	for (lp = lpoints, i = 0; i < npoints; ++i) {
		*lp++ = *points++;
		*lp++ = (BUFLINE)(height - *points++);
	}

	bPolygon (npoints, lpoints, color);
}


LOCAL_FUNC void FAR
WinvMoveTo (Uint x, Uint y)
{
	vMoveTo (x, height - y);
}

LOCAL_FUNC void FAR
WinvDrawTo (Uint x, Uint y, Uint color)
{
	vDrawTo (x, height - y, color);
}

LOCAL_FUNC void FAR
WinvEllipse (Uint x, Uint y, Uint rx, Uint ry, Uint c)
{
	vEllipse (x, height - y, rx, ry, c);
}

LOCAL_FUNC void FAR
WinvClear (Uint x, Uint y, Uint sx, Uint sy, Uint c)
{
	vClear (x, (Gr->flags & GRG_INVERTED) ? height - (y + sy) : y,
		sx, sy, c);
}

LOCAL_FUNC void FAR
WinvPolygon (int npoints, BUFLINE *points, Uint color)
{
	BUFLINE	lpoints[50], *lp;
	int	i;

	for (lp = lpoints, i = 0; i < npoints; ++i) {
		*lp++ = *points++;
		*lp++ = (BUFLINE)(height - *points++);
	}

	vPolygon (npoints, lpoints, color);
}

LOCAL_FUNC int FAR
grvw_bank (int bank)
{
	vSetMem ((Uchar FAR *)((char BPTR *)p1SurfaceBits
							+ bank * 0x10000UL));
	return (0);
}


LOCAL_FUNC void FAR
set_inverted (void)
{
	if (Gr->flags & GRG_INVERTED) {
		Gr->MoveTo = (Gr->flags & GRG_VGR) ? WinvMoveTo : WinbMoveTo;
		Gr->DrawTo = (Gr->flags & GRG_VGR) ? WinvDrawTo : WinbDrawTo;
		Gr->Ellipse= (Gr->flags & GRG_VGR) ? WinvEllipse : WinbEllipse;
		Gr->Polygon= (Gr->flags & GRG_VGR) ? WinvPolygon : WinbPolygon;
	} else {
		Gr->MoveTo = (Gr->flags & GRG_VGR) ? vMoveTo : bMoveTo;
		Gr->DrawTo = (Gr->flags & GRG_VGR) ? vDrawTo : bDrawTo;
		Gr->Ellipse= (Gr->flags & GRG_VGR) ? vEllipse : bDrawEllipse;
		Gr->Polygon= (Gr->flags & GRG_VGR) ? vPolygon : bPolygon;
	}
}

static void FAR
WinGChangeSize ()
{
	HBITMAP	newbm;
	RECT	rect;
	RECT	wrect;
	int	wx, wy, newx;

	sim_set ();
	screen_empty ();

	GetClientRect (ghWndMain, &rect);
	GetWindowRect (ghWndMain, &wrect);

	wx = wrect.right - wrect.left - rect.right;	/* extra size of borders */
	wy = wrect.bottom - wrect.top - rect.bottom;	/* extra size of caption etc */
	newx = wx+((unsigned)((8*rect.right+31)&(~31))/8);

	if (newx != wrect.right - wrect.left) {
		/* make sure window size is a multiple of 8 */
		SetWindowPos (ghWndMain, HWND_TOP, 0, 0, newx, rect.bottom,
			SWP_NOMOVE);
		GetClientRect (ghWndMain, &rect);
	}

/* Initialise Info structure
*/
	WinGRecommendDIBFormat ((BITMAPINFO FAR *)Info);

/* Set the width and height of the DIB but preserve the
 * sign of biHeight in case top-down DIBs are faster
*/
	if (Info->InfoHeader.biHeight >= 0)
		Gr->flags |= GRG_INVERTED;
	else
		Gr->flags &= ~GRG_INVERTED;
	Info->InfoHeader.biHeight *= rect.bottom;
	Info->InfoHeader.biWidth = rect.right;
	if (T(newbm = WinGCreateBitmap (memhdc, (BITMAPINFO FAR *)Info,
					 &p1SurfaceBits))) {
		mybm1 = SelectObject (memhdc, newbm);
		DeleteObject (mybm1);
		mybm1 = newbm;
	}
	set_inverted ();

	height = rect.bottom;
	ResetPalette ();

	if (Gr->flags & GRG_VGR) {
		vSetMem ((Uchar FAR *)p1SurfaceBits);
		vSetSize (rect.right, rect.bottom, rect.right);
	} else {
		bSetActive ((char BPTR *)p1SurfaceBits);
		bSetSize (rect.right, rect.bottom, rect.right);
	}

  	set_screen (rect.right, rect.bottom);
	screen_start ();
	sim_reset ();
}

#if 0
static void ClearSystemPalette(void);
#endif

static HPALETTE CreateIdentityPalette(RGBQUAD aRGB[], int nColors);

static int maxused = 0;

extern int FAR
ResetPalette (void)
{
	Uint	i;
	Ulong	c;

	if (hpal) {
		for (i = COLOR_PLANE; i <= (Uint)maxused; ++i) {
			c = colors[i];
			aPaletteRGB[i-COLOR_PLANE].rgbRed      = GetRValue (c);
			aPaletteRGB[i-COLOR_PLANE].rgbGreen    = GetGValue (c);
			aPaletteRGB[i-COLOR_PLANE].rgbBlue     = GetBValue (c);
			aPaletteRGB[i-COLOR_PLANE].rgbReserved = 0;
		}
		if (hpal)
			DeleteObject (hpal);

#if 0
	  	ClearSystemPalette();
#endif
		hpal = CreateIdentityPalette(aPaletteRGB, maxused+1);

		UnrealizeObject (hpal);
		SelectPalette (hdc, hpal, FALSE);
		RealizePalette (hdc);
		
/* match the DIB color table to the
 * current palette to match the animated palette
 * Alas, palette entries are r-g-b, rgbquads are b-g-r
*/
		GetPaletteEntries (hpal, 0, 256, aPalette);
		for (i = 0; i < 256; ++i) {
			aPaletteRGB[i].rgbRed      = aPalette[i].peRed;
			aPaletteRGB[i].rgbGreen    = aPalette[i].peGreen;
			aPaletteRGB[i].rgbBlue     = aPalette[i].peBlue;
			aPaletteRGB[i].rgbReserved = 0;
		}
		WinGSetDIBColorTable (memhdc, 0, 256, aPaletteRGB);

/* convert to real position
*/
		for (i = 0; i <= (Uint)maxused; ++i)
			rcolor[i] = GetNearestPaletteIndex (hpal, colors[i]);
	}
	return (0);
}

LOCAL_FUNC int FAR
WinGSetActive (int page)
{
	page = page;
	return (0);
}

LOCAL_FUNC int FAR
WinGSetVisual (int page)
{
	page = page;
#if 0
	if (resetSSize) {
		WinGChangeSize ();
		resetSSize = 0;
	} else

/* copy the WinG Bitmap to the display
*/
		WinGBitBlt (hdc, 0, 0, CS->sizex, CS->sizey, memhdc, 0, 0);
#endif
	return (0);
}

LOCAL_FUNC int FAR
WinGSetPalette (int n, long c)
{
	n += COLOR_PLANE;
	if (n > maxused)
		maxused = n;
	colors[n] = RGB (C_RGB_R (c), C_RGB_G (c), C_RGB_B (c));

	ResetPalette ();
	return (n);
}

LOCAL_FUNC void FAR
WinGTerm (DEVICE *dev)
{
	SetSystemPaletteUse (hdc, SYSPAL_STATIC);

	if (hpal) {
		DeleteObject (hpal);
		hpal = 0;
	}
	
	if (ghWndMain) {
		ReleaseDC (ghWndMain, hdc);
		ghWndMain = 0;
	}

	if (oldbm) {
		SelectObject (memhdc, oldbm);
		DeleteObject (mybm1);
		mybm1 = 0;
		DeleteDC (memhdc);
		memhdc = 0;
		oldbm = 0;
	}
	
	hdc = 0;

	if (colors) {
		free (colors);
		colors = NULL;
	}
	if (aPalette) {
		free (aPalette);
		aPalette = NULL;
	}
	if (aPaletteRGB) {
		free (aPaletteRGB);
		aPaletteRGB = NULL;
	}
	if (Info) {
		free (Info);
		Info = NULL;
	}

	dev = dev;

	if (!(Gr->flags & INITED))
		return;
	Gr->flags &= ~INITED;

	if (Gr->flags & GRG_STATS)
		LogStats ();

	LogPrintf ("%s: term ok\n", Gr->name);
}

LOCAL_FUNC int FAR
WinGInit (DEVICE *dev, char * options)
{
	int i;
	
	Gr->flags &= GRG_VGR;
	if (get_parg (options, "stats"))
		Gr->flags |= GRG_STATS;

	if (dev->sizex == 0 || dev->sizey == 0)
		goto badret;
  	
/* Refuse to run if this is a non-palettized device or it is not 256
 * color mode.
*/

	if (0 != (hdc = GetDC (0))) {
		if (0 == GetDeviceCaps (hdc, RASTERCAPS) & RC_PALETTE) {
			ReleaseDC (0, hdc);
			hdc = 0;
			LogPrintf ("%s: display device not palettized\n",
				Gr->name);
			goto badret;
		}

		if (8 != GetDeviceCaps (hdc, BITSPIXEL)
					* GetDeviceCaps (hdc, PLANES)) {
			ReleaseDC (0, hdc);
			hdc = 0;
			LogPrintf ("%s: display device pixel depth not 8\n",
				Gr->name);
			goto badret;
		}

		ReleaseDC (0, hdc);
		hdc = 0;
	}

/* allocate all large data structures
*/
	if (F(colors = (COLORREF *)malloc (256 * sizeof (COLORREF))))
		goto badret;
	if (F(aPalette = (PALETTEENTRY *)malloc (256 * sizeof (PALETTEENTRY))))
		goto badret;
	if (F(aPaletteRGB = (RGBQUAD *)malloc (256 * sizeof (RGBQUAD))))
		goto badret;
	if (F(Info = (struct Info *)malloc (sizeof (struct Info))))
		goto badret;

	ghWndMain = CreateWindow (
			(LPSTR)Fly8AppName,
			(LPSTR)Fly8Message,
			WS_OVERLAPPEDWINDOW,
			0,			/*  x  */
			0,			/*  y  */
			dev->sizex,		/* cx  */
			dev->sizey,		/* cy  */
			(HWND)0,		/* no parent */
			(HMENU)0,		/* use class menu */
			(HANDLE)Fly8Instance,	/* handle to window instance */
			(LPSTR)0		/* no params to pass on */
			);

/* Make window visible
*/
	ShowWindow (ghWndMain, SW_SHOW );
	UpdateWindow (ghWndMain);

	hdc = GetDC (ghWndMain);
	GetClientRect (ghWndMain, &rect);

	height = rect.bottom;
  	set_screen (rect.right, rect.bottom);

/* Set up an optimal bitmap
*/
	if (WinGRecommendDIBFormat ((BITMAPINFO FAR *)Info) == FALSE) {
		MessageBox((HWND)0, (LPCSTR)"Cannot use WinG",
				    (LPCSTR)"Fly8 Message", MB_OK);
		goto badret;
	}

/* Set the width and height of the DIB but preserve the
 * sign of biHeight in case top-down DIBs are faster
*/
	if (Info->InfoHeader.biHeight >= 0)
		Gr->flags |= GRG_INVERTED;
	else
		Gr->flags &= ~GRG_INVERTED;
	Info->InfoHeader.biHeight *= rect.bottom;
	Info->InfoHeader.biWidth = rect.right;

	set_inverted ();

	if ((GetDeviceCaps (hdc, RASTERCAPS) & RC_BITBLT) &&
	    T(memhdc = WinGCreateDC ()) &&
	    T(mybm1  = WinGCreateBitmap (memhdc, (BITMAPINFO FAR *)Info,
					 &p1SurfaceBits))) {
		oldbm = SelectObject (memhdc, mybm1);
		dev->npages = 1; /* WinG double-bufferred using WinGBitBlt! */
	} else {
		MessageBox ((HWND)0, (LPCSTR)"Cannot use WinG",
				    (LPCSTR)"Fly8 Message", MB_OK);
		goto badret;
	}

	dev->colors = 256;

	SetROP2 (memhdc, R2_COPYPEN);
	
	GetSystemPaletteEntries (hdc, 0, 256, aPalette);

/* Alas, palette entries are r-g-b, rgbquads are b-g-r
*/
	for (i = 0; i < 256; ++i)
		colors[i] = 0;
	
	for (i = 0; i < 256; ++i) {
		aPaletteRGB[i].rgbRed      = aPalette[i].peRed;
		aPaletteRGB[i].rgbGreen    = aPalette[i].peGreen;
		aPaletteRGB[i].rgbBlue     = aPalette[i].peBlue;
		aPaletteRGB[i].rgbReserved = 0;
	}
#if 0
	ClearSystemPalette ();
#endif
	hpal = CreateIdentityPalette (aPaletteRGB, 256);

	if (Gr->flags & GRG_VGR) {
		vInit ((Uchar FAR *)p1SurfaceBits, rect.right, rect.bottom,
			rect.right, grvw_bank, NULL);
		vSetWriteMode (T_MSET);
	} else {
		bSetSize (rect.right, rect.bottom, rect.right);
		bSetActive ((char BPTR *)p1SurfaceBits);
		bSetWriteMode (T_MSET);
	}

	usingWinG = 1;

	Gr->flags |= INITED;

	LogPrintf ("%s: init ok\n", Gr->name);
	return (0);

badret:
	WinGTerm (dev);
	return (1);
}


LOCAL_FUNC void FAR
WinGFlush (void)
{
#if 1
	if (resetSSize) {
		WinGChangeSize ();
		resetSSize = 0;
	} else

/* copy the WinG Bitmap to the display
*/
		WinGBitBlt (hdc, 0, 0, CS->sizex, CS->sizey, memhdc, 0, 0);
#endif
}

struct GrDriver NEAR GrWing = {
	"GrWing",
	0,
	NULL,		/* Extra */
	0,		/* Devices */
	WinGInit,
	WinGTerm,
	bMoveTo,
	bDrawTo,
	WinGSetVisual,
	WinGSetActive,
	WinbClear,
	bSetWriteMode,
	WinGSetPalette,
	bDrawEllipse,
	bPolygon,
	WinGFlush,
	0		/* Shutters */
};

struct GrDriver NEAR GrvWing = {
	"GrvWing",
	GRG_VGR,
	NULL,		/* Extra */
	0,		/* Devices */
	WinGInit,
	WinGTerm,
	vMoveTo,
	vDrawTo,
	WinGSetVisual,
	WinGSetActive,
	WinvClear,
	vSetWriteMode,
	WinGSetPalette,
	vEllipse,
	vPolygon,
	WinGFlush,
	0		/* Shutters */
};

static HPALETTE
CreateIdentityPalette(RGBQUAD aRGB[], int nColors)
{
	int	nStaticColors;
	int	nUsableColors;
	int	j, i;
	struct {
		WORD		Version;
		WORD		NumberOfEntries;
		PALETTEENTRY	aEntries[256];
	}	Palette;
	HDC	hdc;

	Palette.Version = 0x300;
	Palette.NumberOfEntries = 256;

/* Just use the screen DC where we need it
*/
	hdc = GetDC(NULL);

	SetSystemPaletteUse (hdc, SYSPAL_NOSTATIC);

/* get the twenty static colors into
 * the array, then fill in the empty spaces with the
 * given color table
*/

/* Get the static colors from the system palette
*/
	nStaticColors = GetDeviceCaps(hdc, NUMCOLORS);
	GetSystemPaletteEntries(hdc, 0, 256, Palette.aEntries);

	for (i = 0; i < 10; i++)
		Palette.aEntries[i].peFlags = 0;
	for (i = 10; i < 246; i++)
		Palette.aEntries[i].peFlags = PC_RESERVED;
	for (i = 246; i < 256; i++)
		Palette.aEntries[i].peFlags = 0;

/* Fill in the entries from the given color table
*/
	nUsableColors = nColors;
	if (nUsableColors > 256 - nStaticColors)
		nUsableColors = 256 - nStaticColors;
	for (i = COLOR_PLANE, j = 0; j < nUsableColors; i++, j++) {
		Palette.aEntries[i].peRed   = aRGB[j].rgbRed;
		Palette.aEntries[i].peGreen = aRGB[j].rgbGreen;
		Palette.aEntries[i].peBlue  = aRGB[j].rgbBlue;
		Palette.aEntries[i].peFlags = PC_RESERVED;
	}

/* Remember to release the DC!
*/
	ReleaseDC(NULL, hdc);
#if 0
	SetSystemPaletteUse (hdc, SYSPAL_STATIC);
#endif

/* Return the palette
*/
	return CreatePalette((LOGPALETTE *)&Palette);
}

#if 0
static void
ClearSystemPalette(void)
{
/* A dummy palette setup
*/
	struct {
		WORD Version;
		WORD NumberOfEntries;
		PALETTEENTRY aEntries[256];
	}		Palette;
	HPALETTE	ScreenPalette = 0;
	HDC		ScreenDC;
	int		Counter;

/* Reset everything in the system palette to black
*/
	Palette.Version = 0x300;
	Palette.NumberOfEntries = 256;
	for(Counter = 0; Counter < 256; Counter++) {
		Palette.aEntries[Counter].peRed = 0;
		Palette.aEntries[Counter].peGreen = 0;
		Palette.aEntries[Counter].peBlue = 0;
		Palette.aEntries[Counter].peFlags = PC_NOCOLLAPSE;
	}

/* Create, select, realize, deselect, and delete the palette
*/
	ScreenDC = GetDC(NULL);
	ScreenPalette = CreatePalette((LOGPALETTE *)&Palette);
	if (ScreenPalette) {
		ScreenPalette = SelectPalette(ScreenDC,ScreenPalette,FALSE);
		RealizePalette(ScreenDC);
		ScreenPalette = SelectPalette(ScreenDC,ScreenPalette,FALSE);
		DeleteObject(ScreenPalette);
	}
	ReleaseDC(NULL, ScreenDC);
}
#endif
#undef GRG_STATS
#undef GRG_VGR
#undef GRG_INVERTED
#undef INITED
#endif /* #ifdef USE_WING */
