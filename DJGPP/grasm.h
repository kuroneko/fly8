/* --------------------------------- grasm.h -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Vga graphics low level routines.
*/

#ifndef FLY8_DJGPP_GRASM_H
#define FLY8_DJGPP_GRASM_H

#include "djgpp.h"
#include "bgr.h"

#define VGA_PAGE	((char BPTR *)0xd0000000)

#define GRL_TYPES	0x000f
#define GRL_TYPE_T4K	0x0000
#define GRL_TYPE_S3	0x0001
#define GRL_TYPE_VESA	0x0002
#define GRL_STATS	(GRL_TYPES+1)
#define INITED		0x8000

#define DOSYNC		0x0001
#define BIGPAGE		0x0006

/* low.c */
extern int	FAR GrlSetActiveBase (int page);
extern int	FAR GrlSetVisualBase (int page);
extern int	FAR GrlInit (int mode, int verbose, int *sizex, int *sizey,
	int *npages);
extern int	FAR GrlTerm (void);

#endif
