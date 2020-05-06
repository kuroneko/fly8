/* --------------------------------- drivers.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* User defined lists of drivers. This one is for UNIX/X11.
 *
 * Associated with the player we have one of each:
 *  Graphics Driver (output)
 *  Sound Driver (output)
 *  Keyboard Driver (input)
 *  Pointer Driver (input)
 *  Network Drivers (i/o)
*/

#include "fly.h"


extern struct GrDriver GrX;
extern struct GrDriver GrI;
#ifdef LINUX
extern struct GrDriver GrSVGA;
#endif

struct GrDriver *GrDrivers[] = {
	&GrX,		/* default */
	&GrI,
#ifdef LINUX
	&GrSVGA,
#endif
0};


struct SndDriver *SndDrivers[] = {
	0,
0};


extern struct PtrDriver PtrKeypad;
extern struct PtrDriver PtrMouse;
#ifdef HAVE_JOYSTICK
extern struct PtrDriver PtrAstick;
extern struct PtrDriver PtrBstick;
#endif
extern struct PtrDriver PtrRandom;

struct PtrDriver *PtrDrivers[] = {
	&PtrKeypad,
	&PtrMouse,
#ifdef HAVE_JOYSTICK
	&PtrAstick,
	&PtrBstick,
#endif
	&PtrRandom,
0};


extern struct KbdDriver KbdConsole;

struct KbdDriver *KbdDrivers[] = {
	&KbdConsole,
0};


#ifdef HAVE_FIFO
extern struct NetDriver NEAR NetFifo;
#endif
#ifdef HAVE_UDP
extern struct NetDriver NEAR NetUdp;
#endif

struct NetDriver *NetDrivers[] = {
#ifdef HAVE_FIFO
	&NetFifo,
#endif
#ifdef HAVE_UDP
	&NetUdp,
#endif
0};
