/* ------------------------------ os2sound.h -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* System dependent configuration information: OS/2
 * IBm OS/2  by Michael Taylor (miket@pcug.org.au)
*/

#ifndef OS2SOUND_H
#define OS2SOUND_H

extern BOOL	FAR FInitSnd(void);
extern void	FAR CloseSnd(void);
extern void	FAR Fly8PlaySnd (int Freq);
extern void	FAR StopSnd(void);

#endif
