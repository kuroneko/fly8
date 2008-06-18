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
*/

#include "fly.h"


extern struct GrDriver GrAmiga;

struct GrDriver *GrDrivers[] = {
	&GrAmiga,		/* default */
0};

struct SndDriver *SndDrivers[] = {
	0,
0};


extern struct PtrDriver PtrKeypad;
extern struct PtrDriver PtrMouse;
extern struct PtrDriver PtrAstick;
extern struct PtrDriver PtrBstick;
extern struct PtrDriver PtrRandom;

struct PtrDriver *PtrDrivers[] = {
	&PtrKeypad,
	&PtrMouse,
	&PtrAstick,
	&PtrBstick,
	&PtrRandom,
0};


extern struct KbdDriver KbdConsole;

struct KbdDriver *KbdDrivers[] = {
	&KbdConsole,
0};


struct NetDriver *NetDrivers[] = {
0};


extern Uchar *StFont1[];
extern Uchar *StFont2[];

Uchar **StFonts[] = {
	StFont1,
	StFont2,
	StFont1,
	StFont1,
	StFont1,
	StFont1,
	StFont1,
	StFont1,
	StFont1,
	StFont1
};
