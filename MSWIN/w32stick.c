/* --------------------------------- w32stick.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* joystick reading for the Win32 Interface
 * Driver by Chris Collins (ccollins@pcug.org.au)
*/

#include <windows.h>
#include <mmsystem.h>

#include "fly.h"

#if HAVE_JOYSTICK

#include "stick.h"


#define JNAME		"W32stick"


/* First (lowest) level PC joystick reading. Acquires hardware dependent
 * data as is.
*/

/* Internal Flags for slight optimisation
*/

/* Axes
*/
#define NTJOY_X1	0x0001
#define NTJOY_Y1	0x0002
#define NTJOY_Z1	0x0004
#define NTJOY_R1	0x0008
#define NTJOY_U1	0x0010
#define NTJOY_V1	0x0020

#define NTJOY_POV	0x0040

/* Buttons
*/
#define NTJOY_BA1	0x0100
#define NTJOY_BB1	0x0200
#define NTJOY_BC1	0x0400
#define NTJOY_BD1	0x0800

static short	_ntjoy_availdevs = 0;
static short	_ntjoy_numsticks = 0;
static short	_ntjoy_extrabuttons = 0;


extern int FAR
initstick (int which, char *options, int opts)
{
	JOYCAPS		jcaps;
	MMRESULT	retval;

	_ntjoy_numsticks = (short)joyGetNumDevs ();
	if (!_ntjoy_numsticks) {
		MsgEPrintf (-50, "%s: no joystick driver found", JNAME);
		return (1);
	}
	LogPrintf ("%s: %d Devices Found\n", JNAME, _ntjoy_numsticks);

/* now build an internal "Description" for our joystick device
*/
	retval = joyGetDevCaps (JOYSTICKID1, &jcaps, sizeof (jcaps));
	if (retval != JOYERR_NOERROR) {
		switch (retval) {
			case MMSYSERR_NODRIVER:
				LogPrintf (
					"%s: Driver reported, but not found\n",
					JNAME);
				return (2);
				break;
			case MMSYSERR_INVALPARAM:
				LogPrintf (
					"%s: joyGetDecCaps: Invalid Param\n",
					JNAME);
				return (3);
				break;
			default:
				LogPrintf (
					"%s: Unknown Error (%d) During Init\n",
					JNAME, retval);
				return (4);
				break;
		}
	}
	LogPrintf ("%s: Joystick 1 is a \"%s\"\n", JNAME, jcaps.szPname);

	LogPrintf ("%s: %d axes %d buttons",
		JNAME, jcaps.wNumAxes, jcaps.wNumButtons);
	if (jcaps.wCaps & JOYCAPS_HASZ)
		LogPrintf (" HASZ");
	if (jcaps.wCaps & JOYCAPS_HASR)
		LogPrintf (" HASR");
	if (jcaps.wCaps & JOYCAPS_HASU)
		LogPrintf (" HASU");
	if (jcaps.wCaps & JOYCAPS_HASV)
		LogPrintf (" HASV");
	if (jcaps.wCaps & JOYCAPS_HASPOV)
		LogPrintf (" HASPOV");
	if (jcaps.wCaps & JOYCAPS_POV4DIR)
		LogPrintf (" POV4DIR");
	if (jcaps.wCaps & JOYCAPS_POVCTS)
		LogPrintf (" POVCTS");
	LogPrintf ("\n");

	if (jcaps.wNumAxes > 0)
		_ntjoy_availdevs |= NTJOY_X1 | NTJOY_Y1;
	if (jcaps.wCaps & JOYCAPS_HASZ)
		_ntjoy_availdevs |= NTJOY_Z1;
	if (jcaps.wCaps & JOYCAPS_HASR)
		_ntjoy_availdevs |= NTJOY_R1;
	if (jcaps.wCaps & JOYCAPS_HASU)
		_ntjoy_availdevs |= NTJOY_U1;
	if (jcaps.wCaps & JOYCAPS_HASV)
		_ntjoy_availdevs |= NTJOY_V1;

	if (jcaps.wCaps & (JOYCAPS_HASPOV|JOYCAPS_POV4DIR|JOYCAPS_POVCTS))
		_ntjoy_availdevs |= NTJOY_POV;

	switch (jcaps.wNumButtons) {
		default:
			_ntjoy_availdevs |= NTJOY_BD1;
		case 3:
			_ntjoy_availdevs |= NTJOY_BC1;
		case 2:
			_ntjoy_availdevs |= NTJOY_BB1;
		case 1:
			_ntjoy_availdevs |= NTJOY_BA1;
		case 0:
			break;
	}

/* buttons above 4 are mapped to buttons 11-NBTNS
*/
	if (jcaps.wNumButtons > 4) {
		_ntjoy_extrabuttons = jcaps.wNumButtons - 4;
		if (_ntjoy_extrabuttons > NBTNS-10)
			_ntjoy_extrabuttons = NBTNS-10;
	} else
		_ntjoy_extrabuttons = 0;

	return (0);
}

extern int FAR
termstick (int which, int opts)
{
	return (0);
}

extern Uint FAR
readstick (int which, STICK *s, int mask, int opts)
{
	JOYINFOEX	stick1;
	MMRESULT	retval;
	static int	baderr = 0;

	memset (&stick1, 0, sizeof (JOYINFOEX));
	stick1.dwSize = sizeof (JOYINFOEX);
	stick1.dwFlags = JOY_RETURNBUTTONS;
	if (_ntjoy_availdevs & NTJOY_POV)
		stick1.dwFlags |= JOY_RETURNPOV;
	if (mask & JS_A0)
		stick1.dwFlags |= JOY_RETURNX;
	if (mask & JS_A1)
		stick1.dwFlags |= JOY_RETURNY;
	if (mask & JS_A2)
		stick1.dwFlags |= JOY_RETURNR;
	if (mask & JS_A3)
		stick1.dwFlags |= JOY_RETURNZ;
	if (mask & JS_A4)
		stick1.dwFlags |= JOY_RETURNU;
	if (mask & JS_A5)
		stick1.dwFlags |= JOY_RETURNV;
		
#if 0
stick1.dwFlags |= JOY_RETURNR;	/* testing NBUTTONS */
#endif
	retval = joyGetPosEx (JOYSTICKID1, &stick1);
	switch (retval) {
		case JOYERR_NOERROR:
			baderr &= ~0x04;
			break;
		case MMSYSERR_NODRIVER:
			/* normally bad, just ignore it for now. */
			break;
		case MMSYSERR_INVALPARAM:
			if (baderr & 0x01)
				break;
			MsgEPrintf (-50,
				"%s: joyGetPosEx reports Invalid Param",
				JNAME);
			baderr |= 0x1;
			break;
		case MMSYSERR_BADDEVICEID:
			if (baderr & 0x02)
				break;
			MsgEPrintf (-50,
				"%s: joyGetPosEx reports Bad Device ID",
				JNAME);
			baderr |= 0x2;
			break;
		case JOYERR_UNPLUGGED:
			if (baderr & 0x04)
				break;
			MsgEPrintf (50,
				"%s: Joystick 1 is unplugged!",
				JNAME);
			baderr |= 0x4;
			break;
		default:
			if (baderr & 0x08)
				break;
			MsgEPrintf (-50,
				"%s: joyGetPosEx unknown error %d",
				JNAME, retval);
			baderr |= 0x08;
			break;
	}
			
	memset (s->b, 0, sizeof (s->b));

	if (which) {
		if (mask & JS_A0)
			s->a[0] = _ntjoy_availdevs & NTJOY_Z1
					? (Ushort)stick1.dwZpos
					: 0;
		if (mask & JS_A1)
			s->a[1] = _ntjoy_availdevs & NTJOY_R1
					? (Ushort)stick1.dwRpos
					: 0;
		s->b[0] = _ntjoy_availdevs & NTJOY_BC1
				? (char)T(stick1.dwButtons & JOY_BUTTON3)
				: 0;
		s->b[1] = _ntjoy_availdevs & NTJOY_BD1
				? (char)T(stick1.dwButtons & JOY_BUTTON4)
				: 0;
		s->nbuttons = 2;
	} else {
		if (mask & JS_A0)
			s->a[0] = _ntjoy_availdevs & NTJOY_X1
					? (Ushort)stick1.dwXpos
					: 0;
		if (mask & JS_A1)
			s->a[1] = _ntjoy_availdevs & NTJOY_Y1
					? (Ushort)stick1.dwYpos
					: 0;
		if (mask & JS_A2)
			s->a[2] = _ntjoy_availdevs & NTJOY_R1
					? (Ushort)stick1.dwRpos
					: 0;
		if (mask & JS_A3)
			s->a[3] = _ntjoy_availdevs & NTJOY_Z1
					? (Ushort)stick1.dwZpos
					: 0;
		if (mask & JS_A4)
			s->a[4] = _ntjoy_availdevs & NTJOY_U1
					? (Ushort)stick1.dwUpos
					: 0;
		if (mask & JS_A5)
			s->a[5] = _ntjoy_availdevs & NTJOY_V1
					? (Ushort)stick1.dwVpos
					: 0;
		s->b[0] = (char)T(stick1.dwButtons & (1UL << 0));
		s->b[1] = (char)T(stick1.dwButtons & (1UL << 1));
		s->b[2] = _ntjoy_availdevs & NTJOY_BC1
				? (char)T(stick1.dwButtons & (1UL << 2))
				: 0;
		s->b[3] = _ntjoy_availdevs & NTJOY_BD1
				? (char)T(stick1.dwButtons & (1UL << 3))
				: 0;
		s->nbuttons = 4;
	}

/* Read POV state
*/
	if (_ntjoy_availdevs & NTJOY_POV) {
		switch (stick1.dwPOV) {
		case JOY_POVFORWARD:
			s->b[4] = 1;
			break;
		case JOY_POVRIGHT:
			s->b[5] = 1;
			break;
		case JOY_POVBACKWARD:
			s->b[6] = 1;
			break;
		case JOY_POVLEFT:
			s->b[7] = 1;
			break;
		case JOY_POVCENTERED:
		default:
			break;
		}
	}

#if 0	/* testing NBUTTONS */
#define	REF		100		/* expected full range */
{
	Ushort		pov;
	static Ushort	FA4BASE = 0xffffU;
	static Ushort	FA4TOP = 0;

	pov = (Ushort)stick1.dwRpos;
	if (FA4BASE > pov)
		FA4BASE = pov;
	if (FA4TOP < pov)
		FA4TOP = pov;

	if (FA4TOP != FA4BASE) {
		pov = muldiv (REF, pov - FA4BASE, FA4TOP - FA4BASE);
		pov = (pov + REF/8)/(REF/4);
		s->b[4+pov] = 1;
	}
	s->nbuttons = 8;
}
#endif

/* Read extra buttons
*/
	if (_ntjoy_extrabuttons > 0) {
		int	i;

		for (i = 0; i < _ntjoy_extrabuttons; ++i)
			s->b[i+10] = (char)T(stick1.dwButtons & (1UL << (4+i)));
		s->nbuttons = 10+_ntjoy_extrabuttons;
	}

#if 0
	MsgPrintf (1, "x1=%5u y1=%5u x2=%5u y2=%5u",
		(Uint)s->a[0], (Uint)s->a[1], (Uint)s->a[2], (Uint)s->a[3]);
#endif
	return (0);
}
#undef JNAME
#undef NTJOY_X1
#undef NTJOY_Y1
#undef NTJOY_Z1
#undef NTJOY_R1
#undef NTJOY_BA1
#undef NTJOY_BB1
#undef NTJOY_BC1
#undef NTJOY_BD1
#undef NTJOY_POV

#endif /* if HAVE_JOYSTICK */
