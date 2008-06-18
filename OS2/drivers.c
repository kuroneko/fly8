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
 *  OS/2 support by Michael Taylor miket@pcug.org.au
*/

#include "fly.h"

extern struct GrDriver	GrOS2S;
extern struct GrDriver	GrBitMap;
#if HAVE_DIVE
extern struct GrDriver	GrDive;
#endif

struct GrDriver	*GrDrivers[] = {
	&GrOS2S,	/* flicker - single buffered      */
	&GrBitMap,	/* uses bitmap */
#if HAVE_DIVE
	&GrDive,	/* uses the OS/2 Dive functions */
#endif
0};

extern struct SndDriver	PlsMMSound;
extern struct SndDriver	NEAR SndNone;

struct SndDriver	*SndDrivers[] = {
#if 1
	&PlsMMSound,
#else
	&SndNone,
#endif
0};

extern struct PtrDriver	PtrMouse;
extern struct PtrDriver	PtrKeypad;
#if HAVE_JOYSTICK
extern struct PtrDriver	PtrAstick;
extern struct PtrDriver	PtrBstick;
#endif
extern struct PtrDriver	PtrRandom;

struct PtrDriver	*PtrDrivers[] = {
	&PtrKeypad,
	&PtrMouse,
#ifdef HAVE_JOYSTICK
	&PtrAstick,
	&PtrBstick,
#endif
	&PtrRandom,
0};

extern struct KbdDriver	KbdConsole;

struct KbdDriver	*KbdDrivers[] = {
	&KbdConsole,
0};

#if HAVE_UDP
extern struct NetDriver	NetUdp;
#endif
struct NetDriver	*NetDrivers[] = {
#if HAVE_UDP
	&NetUdp,
#endif
0};
