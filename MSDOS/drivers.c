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


#ifdef FLY8_MSC
#if 0
extern struct GrDriver NEAR GrQc;
#endif
#endif
#ifdef FLY8_BC
extern struct GrDriver NEAR GrBGI;
#endif
extern struct GrDriver NEAR GrFast;
#ifdef INCLUDE_GRVESA
extern struct GrDriver NEAR GrVesa;
#endif

struct GrDriver NEAR* FAR GrDrivers[] = {
	&GrFast,	/* default */
#ifdef INCLUDE_GRVESA
	&GrVesa,
#endif
#ifdef FLY8_MSC
#if 0
	&GrQc,
#endif
#endif
#ifdef FLY8_BC
	&GrBGI,
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


extern struct PtrDriver NEAR PtrKeypad;
extern struct PtrDriver NEAR PtrMouse;
extern struct PtrDriver NEAR PtrAstick;
extern struct PtrDriver NEAR PtrBstick;
extern struct PtrDriver NEAR PtrRandom;

struct PtrDriver NEAR* FAR PtrDrivers[] = {
	&PtrKeypad,
	&PtrMouse,
	&PtrAstick,
	&PtrBstick,
	&PtrRandom,
0};


extern struct KbdDriver NEAR KbdConsole;

struct KbdDriver NEAR* FAR KbdDrivers[] = {
	&KbdConsole,
0};


extern struct NetDriver NEAR NetCom;
extern struct NetDriver NEAR NetPack;
extern struct NetDriver NEAR NetSlip;
extern struct NetDriver NEAR NetPcUDP;
#if HAVE_UDP
extern struct NetDriver NEAR NetUdp;
#endif

struct NetDriver NEAR* FAR NetDrivers[] = {
	&NetCom,
	&NetPack,
	&NetSlip,
	&NetPcUDP,
#if HAVE_UDP
	&NetUdp,
#endif
0};
