/* --------------------------------- wave.h --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* System dependent configuration information: Microsoft WINDOWS, ms c8
 * Microsoft Windows  by Michael Taylor (miket@pcug.org.au)
*/

/* wave.h
 * Interface to simple tone generation function.
*/


#ifndef FLY8_MSWIN_WAVE_H
#define FLY8_MSWIN_WAVE_H

extern BOOL	FAR FInitSnd(void);
extern void	FAR CloseSnd(void);
extern void	FAR Fly8PlaySnd (int Freq);
extern void	FAR StopSnd(void);

#endif
