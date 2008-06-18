/* --------------------------------- bgr.h ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Header file for VGA graphics low level routines, bgr.c.
*/

#ifndef FLY8_BGR_H
#define FLY8_BGR_H

#ifndef BPTR
#define BPTR
#endif

/* bgr.c */
extern int	FAR bSetActive (char BPTR *base);
extern int	FAR bSetSize (Uint width, Uint height, Uint xbytes);
extern void	FAR bMoveTo (Uint x1, Uint y1);
extern void	FAR bDrawTo (Uint x2, Uint y2, Uint c);
extern void	FAR bDrawLine (Uint xa, Uint ya, Uint xb, Uint yb, Uint c);
extern int	FAR bSetWriteMode (int mode);
extern void	FAR bClear (Uint x, Uint y, Uint sizex, Uint sizey, 
	Uint color);
extern void	FAR bDrawEllipse (Uint xc, Uint yc, Uint a, Uint b, Uint c);
extern void	FAR bPolygon (int npoints, BUFLINE *points, Uint color);

/* bgrasm.x (asm line drawing option) */
#ifdef USE_BGRASM
extern void	FAR vgaset (char BPTR *va, int color, int length);
extern char BPTR * FAR ILoop (char BPTR *va, int dy, int dx, int dvx, int dvy,
	int color);
extern char BPTR * FAR SLoop (char BPTR *va, int count, int dva, int color);
#endif

/* grstat.c */
extern Ulong	FAR GrStats[2048];
extern void	FAR LogStats (void);

#endif	/* ifndef FLY8_BGR_H */
