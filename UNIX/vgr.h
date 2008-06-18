/* --------------------------------- vgr.h ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* header for linear buffer graphics routines, vgr.c.
*/

#ifndef FLY8_VGR_H
#define FLY8_VGR_H

/* vgr.c */
extern int	FAR vSetWriteMode (int mode);
extern int	FAR vSetActive (int page);
extern int	FAR vSetVisual (int page);
extern int	FAR vSetMem (Uchar *vmem);
extern int	FAR vSetSize (Uint width, Uint height, Uint xbytes);
extern void	FAR vInit (Uchar *vmem, Uint width, Uint height, Uint xbytes,
	int (FAR *setbank) (int bank),
	int (FAR *setbase) (Ulong base));
extern void	FAR vMoveTo (Uint x1, Uint y1);
extern void	FAR vDrawTo (Uint x2, Uint y2, Uint c);
extern void	FAR vClear (Uint x, Uint y, Uint sizex, Uint sizey, Uint color);
extern void	FAR vEllipse (Uint xc, Uint yc, Uint a, Uint b, Uint c);
extern void	FAR vPolygon (int npoints, BUFLINE *points, Uint color);
extern Ulong	FAR vStats[2048];

/* grstat.c */
extern Ulong	FAR GrStats[2048];
extern void	FAR LogStats (void);

#endif
