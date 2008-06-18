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


extern struct GrDriver GrASM;
extern struct GrDriver GrVBE;
#if 1 == SYS_DJGPP
extern struct GrDriver GrDJ;
#endif

struct GrDriver *GrDrivers[] = {
	&GrASM,		/* default */
	&GrVBE,
#if 1 == SYS_DJGPP
	&GrDJ,
#endif
0};


extern struct SndDriver NEAR SndPlSpeaker;
extern struct SndDriver NEAR SndPlMidi;
extern struct SndDriver NEAR SndPlFm;
extern struct SndDriver NEAR SndPlAdlib;

struct SndDriver NEAR* FAR SndDrivers[] = {
	&SndPlSpeaker,
	&SndPlMidi,
	&SndPlFm,
	&SndPlAdlib,
0};

extern struct PtrDriver PtrKeypad;
extern struct PtrDriver PtrRandom;
extern struct PtrDriver PtrMouse;
extern struct PtrDriver PtrAstick;
extern struct PtrDriver PtrBstick;

struct PtrDriver *PtrDrivers[] = {
	&PtrKeypad,
	&PtrAstick,
	&PtrBstick,
	&PtrMouse,
	&PtrRandom,
0};


extern struct KbdDriver KbdConsole;

struct KbdDriver *KbdDrivers[] = {
	&KbdConsole,
0};


extern struct NetDriver NEAR NetPcUDP;
extern struct NetDriver NEAR NetPKT;
#if HAVE_UDP
extern struct NetDriver NEAR NetUdp;
#endif

struct NetDriver *NetDrivers[] = {
	&NetPcUDP,
	&NetPKT,
#if HAVE_UDP
	&NetUdp,
#endif
0};
