/* --------------------------------- svga.h --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* header for svga.c.
*/

#ifndef FLY8_UNIX_SVGA_H
#define FLY8_UNIX_SVGA_H

#include <vga.h>
#include "vgr.h"


/* svga.c */
extern int	svga_palette (int color, int r, int g, int b);
extern void	svga_init (Uint mode, int width, int height, int xbytes);
extern void	svga_term (void);
extern int	svga_GetMouse (int *win_x, int *win_y, char *btn, int *nbtn);
extern int	svga_Kread (void);

#endif
