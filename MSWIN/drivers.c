/* --------------------------------- drivers.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* User defined lists of drivers.
 *
 * Associated with the player we have one of each:
 *  Graphics Driver (output)
 *  Sound Driver (output)
 *  Keyboard Driver (input)
 *  Pointer Driver (input)
 *  Network Drivers (i/o)
 *
 *  Windows support by Michael Taylor miket@pcug.org.au
*/

#include "fly.h"

extern struct GrDriver NEAR GrSDL;

struct GrDriver NEAR * FAR GrDrivers[] = {
    &GrSDL,
    0};


#if HAVE_WAVE
extern struct SndDriver NEAR SndPlWave;
#endif
#if HAVE_MIDI
extern struct SndDriver NEAR SndPlMidi;
#endif

struct SndDriver NEAR * FAR SndDrivers[] = {
#if HAVE_WAVE
	&SndPlWave,
#endif
#if HAVE_MIDI
	&SndPlMidi,
#endif
0};


extern struct PtrDriver NEAR PtrMouse;
extern struct PtrDriver NEAR PtrKeypad;
extern struct PtrDriver NEAR PtrRandom;
extern struct PtrDriver NEAR PtrSdlStick;

struct PtrDriver NEAR * FAR PtrDrivers[] = {
	&PtrKeypad,
	&PtrMouse,
	&PtrSdlStick,
	&PtrRandom,
0};


extern struct KbdDriver NEAR KbdConsole;

struct KbdDriver NEAR * FAR KbdDrivers[] = {
	&KbdConsole,
0};


#if HAVE_UDP
extern struct NetDriver NEAR NetUdp;
#endif

struct NetDriver NEAR * FAR NetDrivers[] = {
#if HAVE_UDP
	&NetUdp,
#endif
0};
