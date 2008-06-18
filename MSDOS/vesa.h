/* --------------------------------- vesa.h --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* header for vesa.c.
*/

#ifndef FLY8_MSDOS_VESA_H
#define FLY8_MSDOS_VESA_H

#include "vgr.h"


/* vesa.c */
extern void	FAR vesa_palette (int color, int r, int g, int b);
extern void	FAR vesa_init (Uint mode, int width, int height, int xbytes);
extern void	FAR vesa_term (void);
extern Ulong	FAR vesa_BankSwitches;

#endif
