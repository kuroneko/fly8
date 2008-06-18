/* --------------------------------- grx.h ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Header for Unix video drivers (grx.c and gri.c).
*/

#ifndef FLY8_UNIX_GRX_H
#define FLY8_UNIX_GRX_H

struct GrxExtra {
	int	(* GetMouse) (int *win_x, int *win_y, char *btn, int *nbtn);
	int	(* Kread) (void);
};

#endif
