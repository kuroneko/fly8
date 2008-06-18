/* --------------------------------- stick.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for the PC joystick as a pointing device. On port A It will
 * read the analog hat if it is present and will read all four buttons.
 * Use of options:
 *
 * ix=
 * iy=
 * ir=
 * it=
 * ih=
 *	idle zone for the analog channels (% of max)
 * count
 *	use counter instead of timer to read the joystick
 * rd=
 *	read n times
 * dly=
 *	delay n units between re-reading
 * rdr[-]
 *	we have a rudder on x2 ('-' means it's upside down)
 * hat
 *	we have a hat (default is analog on y2 as for FCS)
 * ttl[-]
 *	we have a throttle on y2 ('-' means it's upside down)
 * zero
 *	we have no buttons (default is two)
 * four
 *	we have four buttons
 * many
 *	we have many buttons (driver detemined)
 * chpro
 *	we have a CHpro
*/

#include "fly.h"

#if HAVE_JOYSTICK

#include "stick.h"


#define PO		p->opt
#define FA1D		PO[IFA1D]
#define FA1F		PO[IFA1F]
#define FA2D		PO[IFA2D]
#define FA2F		PO[IFA2F]
#define FA3D		PO[IFA3D]
#define FA3F		PO[IFA3F]
#define FA4D		PO[IFA4D]
#define FA4F		PO[IFA4F]
#define FA5D		PO[IFA5D]
#define FA5F		PO[IFA5F]

#define POPTS		PO[IPFREE+0]
#define FNREAD		PO[IPFREE+1]	/* user option: read n times */
#define FDELAY		PO[IPFREE+2]	/* user option: delay n units */
#define FA1IDLE		PO[IPFREE+3]	/* user option: idle value */
#define FA1BASE		PO[IPFREE+4]	/* calibrated low  value */
#define FA1TOP		PO[IPFREE+5]	/* calibrated high value */
#define FA2IDLE		PO[IPFREE+6]
#define FA2BASE		PO[IPFREE+7]
#define FA2TOP		PO[IPFREE+8]
#define FA3IDLE		PO[IPFREE+9]
#define FA3BASE		PO[IPFREE+10]
#define FA3TOP		PO[IPFREE+11]
#define FA4IDLE		PO[IPFREE+12]
#define FA4BASE		PO[IPFREE+13]
#define FA4TOP		PO[IPFREE+14]
#define FA5IDLE		PO[IPFREE+15]
#define FA5BASE		PO[IPFREE+16]
#define FA5TOP		PO[IPFREE+17]

#define	REF		100		/* expected full range */
#define	REFDETENT	75		/* Fly8 AB detent position */

/* Second level joystick input: apply 'nread' and 'ndelay'.
*/

LOCAL_FUNC int NEAR
JsInput (int which, STICK *s, int opts, Ushort mask, Ushort nread,
	Ushort delay)
{
	Uint	m, js, min0, min1, min2, min3, min4, min5, ntimes;
	int	i, t;

	min0 = min1 = min2 = min3 = min4 = min5 = 0xffffU;

	for (ntimes = 0;;) {
		m = readstick (which, s, mask, opts);
		if (min0 > s->a[0])
			min0 = s->a[0];
		if (min1 > s->a[1])
			min1 = s->a[1];
		if (min2 > s->a[2])
			min2 = s->a[2];
		if (min3 > s->a[3])
			min3 = s->a[3];
		if (min4 > s->a[4])
			min4 = s->a[4];
		if (min5 > s->a[5])
			min5 = s->a[5];

		if (++ntimes >= nread)		/* read more? */
			break;

		if (T(i = delay)) {		/* delay? */
			t = 1234;
			for (i *= 10; i-- > 0;)
				t *= 19;
		}
	}

	js = m | ~mask;
	s->a[0] = (Ushort)((js & JS_A0) ? 0 : min0);
	s->a[1] = (Ushort)((js & JS_A1) ? 0 : min1);
	s->a[2] = (Ushort)((js & JS_A2) ? 0 : min2);
	s->a[3] = (Ushort)((js & JS_A3) ? 0 : min3);
	s->a[4] = (Ushort)((js & JS_A4) ? 0 : min4);
	s->a[5] = (Ushort)((js & JS_A5) ? 0 : min5);
	
	return (m);
}


/* Third level joystick input: interpret the raw input and adjust for
 * calibration.
*/


/* Interpret the joystick reading as symm_etric -100...+100.
 * The 'base' parameter is interpreted as a fraction of the center reading.
*/
LOCAL_FUNC void NEAR
JsSymm (POINTER *p, int channel, int reading, int sign, int base, int top)
{
	int	center;
	int	play;

	center = p->c[channel];
	play   = p->play[channel];

	if (reading < (base += (play >> 1)))
		reading = -100;
	else if (reading < (center -= play))
		reading = muldiv (-100, center - reading, center - base);
	else if (reading < (center += play+play))
		reading = 0;
	else if (reading < (top -= (play >> 1)))
		reading = muldiv (100, reading - center, top - center);
	else
		reading = 100;

	reading *= sign;				/* orientation */

	p->a[channel] = (short)reading;
}
	
/* Interpret the joystick reading as 'hat' position.
 * The 'base' parameter is interpreted as a fraction of the center reading.
 * the reading is spread evenly from 0 to 100 and has the following states
 * in order: 0=up, 25=right, 50=down, 75=left, 100=center.
*/
LOCAL_FUNC void NEAR
JsHat (POINTER *p, int reading, int base, int top, char *btn)
{
	if (reading <= base)
		reading = 0;
	else if (reading >= top)
		reading = REF;
	else
		reading = muldiv (REF, reading - base, top - base);

	btn[4 + (reading + REF/8)/(REF/4)] = 1;
}

/* Interpret the joystick reading as a 0-100.
 * The 'base' parameter is interpreted as a fraction of the center reading.
*/
LOCAL_FUNC void NEAR
JsPos (POINTER *p, int channel, int reading, int sign, int base, int top)
{
	int	play;

	play = p->play[channel];

	if (reading <= (base += play))
		reading = 0;
	else if (reading >= (top -= play))
		reading = REF;
	else
		reading = muldiv (REF, reading - base, top - base);

	if (sign < 0)
		reading = REF - reading;
	p->a[channel] = (short)reading;
}

LOCAL_FUNC int FAR
JsaRead (POINTER *p)
{
	STICK	s[1];
	int	i, n;
	char	btn[NBTNS];

	n = JS_A0|JS_A1;
	if (POPTS & READA2)
		n |= JS_A2;
	if (POPTS & READA3)
		n |= JS_A3;
	if (JsInput (0, s, POPTS, (Ushort)n, (Ushort)FNREAD, (Ushort)FDELAY))
		return (1);			/* reading failed */

	if (p->low[0] > s->a[FA1F])
		p->low[0] = s->a[FA1F];
	if (p->high[0] < s->a[FA1F])
		p->high[0] = s->a[FA1F];
	if (p->low[1] > s->a[FA2F])
		p->low[1] = s->a[FA2F];
	if (p->high[1] < s->a[FA2F])
		p->high[1] = s->a[FA2F];
	if (POPTS & RUDDER) {
		if (p->low[2] > s->a[FA3F])
			p->low[2] = s->a[FA3F];
		if (p->high[2] < s->a[FA3F])
			p->high[2] = s->a[FA3F];
	}
	if (POPTS & THROTTLE) {
		if (p->low[3] > s->a[FA4F])
			p->low[3] = s->a[FA4F];
		if (p->high[3] < s->a[FA4F])
			p->high[3] = s->a[FA4F];
	}
	if (POPTS & HAT) {
		if (p->low[10] > s->a[FA5F])
			p->low[10] = s->a[FA5F];
		if (p->high[10] < s->a[FA5F])
			p->high[10] = s->a[FA5F];
	}

	if (!(POPTS & CALIBRATED))
		return (0);

	memset (btn, 0, sizeof (btn));
	if (POPTS & CHPRO) {
		if (s->b[0] && s->b[1])
			btn [4 + (T(s->b[3]) << 1) + T(s->b[2])] = 1;
		else {
			for (i = 0; i < 4; ++i)
				btn[i] = (char)T(s->b[i]);
		}
		n = 8;
	} else {
		if (POPTS & NBUTTONS)
			n = s->nbuttons;
		else if (POPTS & ZEROBUTTONS)
			n = 0;
		else if (POPTS & FOURBUTTONS)
			n = 4;
		else
			n = 2;

		for (i = 0; i < n; ++i)
			btn[i] = (char)T(s->b[i]);
	}

	JsSymm (p, 0, s->a[FA1F], -FA1D, FA1BASE, FA1TOP);
	JsSymm (p, 1, s->a[FA2F],  FA2D, FA2BASE, FA2TOP);
	if (POPTS & RUDDER)
		JsSymm (p, 2, s->a[FA3F], FA3D, FA3BASE, FA3TOP);
	if (POPTS & THROTTLE) {
		JsPos (p, 3, s->a[FA4F], FA4D, FA4BASE, FA4TOP);
		if (p->a[3] < (short)(p->c[3] - FA4IDLE))
			p->a[3] = (short)(muldiv (p->a[3],
						REFDETENT, p->c[3]));
		else if (p->a[3] < (short)(p->c[3] + FA4IDLE))
			p->a[3] = REFDETENT;
		else
			p->a[3] = (short)(REFDETENT
				+ muldiv (REF-REFDETENT,
				p->a[3] - p->c[3], REF - p->c[3]));
	}
	if (POPTS & HAT) {
		JsHat (p, s->a[FA5F], FA5BASE, FA5TOP, btn);
		if (n < 8) {

/* keep old value of non existing buttons, if any.
*/
			for (i = n; i < 4; ++i)
				btn[i] = (char)(p->btn[i] & 1);
			n = 8;	/* as set by JsHat() */
		}
	}

	do_btns (p, btn, n);

	return (0);
}

/* Calibrate joy-stick. Paddle and 'hat' must be at center!
*/
LOCAL_FUNC int FAR
JsaCal (POINTER *p)
{
	STICK	s[1];
	int	m;
	char	axes[6];

/* axes[]==1 read it
*/
	memset (axes, 0, sizeof (axes));
	axes[FA1F] = 1;
	axes[FA2F] = 1;
	if (POPTS & RUDDER)
		axes[FA3F] = 1;
	if (POPTS & THROTTLE)
		axes[FA4F] = 1;
	if (POPTS & HAT)
		axes[FA5F] = 1;

	m = 0;
	if (axes[0])
		m |= JS_A0;
	if (axes[1])
		m |= JS_A1;
	if (axes[2]) {
		m |= JS_A2;
		POPTS |= READA2;
	}
	if (axes[3]) {
		m |= JS_A3;
		POPTS |= READA3;
	}
	if (axes[4]) {
		m |= JS_A4;
		POPTS |= READA4;
	}
	if (axes[5]) {
		m |= JS_A5;
		POPTS |= READA5;
	}

	m = JsInput (0, s, POPTS, (Ushort)m, 10, 10);

/* axes[]==1 got it
*/
	if (m & JS_A0)
		axes[0] = 0;
	if (m & JS_A1)
		axes[1] = 0;
	if (m & JS_A2) {
		axes[2] = 0;
		POPTS &= ~READA2;
	}
	if (m & JS_A3) {
		axes[3] = 0;
		POPTS &= ~READA3;
	}
	if (m & JS_A4) {
		axes[4] = 0;
		POPTS &= ~READA4;
	}
	if (m & JS_A5) {
		axes[5] = 0;
		POPTS &= ~READA5;
	}

	if (!axes[FA1F])
		return (1);
	if (!s->a[FA1F])
		return (2);
	FA1BASE = p->low[0];
	FA1TOP  = p->high[0];
	p->c[0] = s->a[FA1F];
	p->play[0] = (Ushort)muldiv (FA1TOP, FA1IDLE, 100);
	p->a[0] = p->l[0] = 0;
	p->low [0] = 0x7fff;
	p->high[0] = 0x0000;

	if (!axes[FA2F])
		return (3);
	if (!s->a[FA2F])
		return (4);
	FA2BASE = p->low[1];
	FA2TOP  = p->high[1];
	p->c[1] = s->a[FA2F];
	p->play[1] = (Ushort)muldiv (FA2TOP, FA2IDLE, 100);
	p->a[1] = p->l[1] = 0;
	p->low [1] = 0x7fff;
	p->high[1] = 0x0000;

	if (POPTS & RUDDER) {
		if (!axes[FA3F] || !s->a[FA3F]) {
			MsgEPrintf (-50, "%s: no analog rudder",
				p->control->name);
			POPTS &= ~RUDDER;
		} else {
			MsgWPrintf (-50, "%s: using analog rudder",
				p->control->name);

			FA3BASE = p->low[2];
			FA3TOP  = p->high[2];
			p->c[2] = s->a[FA3F];
			p->play[2] = (Ushort)muldiv (FA3TOP, FA3IDLE, 100);

			p->a[2] = p->l[2] = 0;
			p->low [2] = 0x7fff;
			p->high[2] = 0x0000;
		}
	}
	if (POPTS & THROTTLE) {
		if (!axes[FA4F] || !s->a[FA4F]) {
			MsgEPrintf (-50, "%s: no analog throttle",
				p->control->name);
			POPTS &= ~THROTTLE;
		} else {
			MsgWPrintf (-50, "%s: using analog throttle",
				p->control->name);
			FA4BASE = p->low[3];
			FA4TOP  = p->high[3];
			p->play[3] = (Ushort)muldiv (FA4TOP, FA4IDLE, 100);
			p->low [3] = 0x7fff;
			p->high[3] = 0x0000;
			JsPos (p, 3, s->a[FA4F], FA4D, FA4BASE, FA4TOP);
			p->c[3] = p->a[3];
			p->a[3] = p->l[3] = 0;
		}
	}
	if (POPTS & HAT) {
		if (!axes[FA5F] || !s->a[FA5F]) {
			MsgEPrintf (-50, "%s: no analog hat",
				p->control->name);
			POPTS &= ~HAT;
		} else {
			MsgWPrintf (-50, "%s: using analog hat",
				p->control->name);
			FA5BASE = p->low[10];
			FA5TOP  = p->high[10];
			p->c[10] = s->a[FA5F];
			p->play[10] = (Ushort)muldiv (FA5TOP, FA5IDLE, 100);
			p->low [10] = 0x7fff;
			p->high[10] = 0x0000;
		}
	}

	POPTS |= CALIBRATED;
	return (0);
}

LOCAL_FUNC void NEAR
JsOptions (POINTER *p, char *options)
{
	long	l;

	if (get_parg (options, "count"))
		POPTS &= ~USETIMER;

	if (get_narg (options, "rd=", &l))
		FNREAD = 1;
	else
		FNREAD = (int)l;

	if (get_narg (options, "dly=", &l))
		FDELAY = 1;
	else
		FDELAY = (int)l;

	if (get_narg (options, "ix=", &l))
		FA1IDLE = 10;
	else
		FA1IDLE = (int)l;
	if (get_narg (options, "iy=", &l))
		FA2IDLE = 10;
	else
		FA2IDLE = (int)l;
}

LOCAL_FUNC int FAR
JsaInit (POINTER *p, char *options)
{
	long	l;
	int	ret;
	char	*o;

	POPTS &= ~(HAT|THROTTLE|FOURBUTTONS|ZEROBUTTONS|NBUTTONS);
	POPTS |= USETIMER;

	JsOptions (p, options);

	if (get_narg (options, "ir=", &l))
		FA3IDLE = 10;
	else
		FA3IDLE = (int)l;
	if (get_narg (options, "it=", &l))
		FA4IDLE = 10;
	else
		FA4IDLE = (int)l;
	if (get_narg (options, "ih=", &l))
		FA5IDLE = 10;
	else
		FA5IDLE = (int)l;

	if (T(o = get_parg (options, "rdr"))) {
		POPTS |= RUDDER;
		if ('-' == *o)
			FA3D = -1;
	}

	if (get_parg (options, "hat"))
		POPTS |= HAT;
	if (T(o = get_parg (options, "ttl"))) {
		POPTS |= THROTTLE;
		if ('-' == *o)
			FA4D = -1;
	}
	if ((POPTS & HAT) && (POPTS & THROTTLE) && FA4F == FA5F) {
		LogPrintf ("%s: HAT and THROTTLE conflict\n",
			p->control->name);
		return (1);
	}

	if (get_parg (options, "zero"))
		POPTS |= ZEROBUTTONS;
	else if (get_parg (options, "four"))
		POPTS |= FOURBUTTONS;
	else if (get_parg (options, "chpro"))
		POPTS |= CHPRO;
	else if (get_parg (options, "many"))
		POPTS |= NBUTTONS;


	if (T(ret = initstick (0, options, POPTS))) {
		LogPrintf ("%s: init failed %d\n",
			p->control->name, ret);
		return (ret);
	}

	JsaCal (p);		/* one for the road */
	ret = JsaCal (p);
	POPTS &= ~CALIBRATED;
	if (ret)
		LogPrintf ("%s: cal failed %d\n",
			p->control->name, ret);
	else {
		MsgWPrintf (200, "Move the stick to all 4 egdes,");
		MsgWPrintf (200, "   then center it.");
		if (POPTS & HAT) {
			MsgWPrintf (200, "Try all hat positions,");
			MsgWPrintf (200, "   then center it.");
		}
		if (POPTS & THROTTLE) {
			MsgWPrintf (200, "Move throttle to both ends,");
			MsgWPrintf (200, "   then set to MIL detent.");
		}
		if (POPTS & RUDDER) {
			MsgWPrintf (200, "Move rudders to both sides,");
			MsgWPrintf (200, "   then set to middle.");
		}
		MsgWPrintf (200, "now hit 'x' to callibrate.");
	}

	return (ret);
}

LOCAL_FUNC void FAR
JsaTerm (POINTER *p)
{
	termstick (0, POPTS);
}

LOCAL_FUNC int FAR
JsCenter (POINTER *p)
{
	p->a[FA1F] = p->a[FA2F] = 0;
	p->l[FA1F] = p->l[FA2F] = 0;

	return (0);
}

LOCAL_FUNC int FAR
JsbRead (POINTER *p)
{
	STICK	s[1];
	char	btn[2];

	if (JsInput (1, s, POPTS, JS_A0|JS_A1, (Ushort)FNREAD, (Ushort)FDELAY))
		return (1);			/* reading failed */

	if (p->low[0] > s->a[FA1F])
		p->low[0] = s->a[FA1F];
	if (p->high[0] < s->a[FA1F])
		p->high[0] = s->a[FA1F];
	if (p->low[1] > s->a[FA2F])
		p->low[1] = s->a[FA2F];
	if (p->high[1] < s->a[FA2F])
		p->high[1] = s->a[FA2F];

	if (!(POPTS & CALIBRATED))
		return (0);

	memset (btn, 0, sizeof (btn));

	JsSymm (p, 0, s->a[FA1F], -FA1D, FA1BASE, FA1TOP);
	JsSymm (p, 1, s->a[FA2F],  FA2D, FA2BASE, FA2TOP);

	btn[0] = (char)s->b[0];
	btn[1] = (char)s->b[1];
	do_btns (p, btn, rangeof (btn));

	return (0);
}

LOCAL_FUNC int FAR
JsbCal (POINTER *p)
{
	STICK	s[1];
	int	m;

	m = JS_A0|JS_A1;
	m = JsInput (1, s, POPTS, (Ushort)m, 10, 10);

	if (m & JS_A0)
		return (1);
	if (!s->a[0])
		return (2);
	FA1BASE = p->low[0];
	FA1TOP  = p->high[0];
	p->c[0] = s->a[FA1F];
	p->play[0] = (Ushort)muldiv (FA1TOP, FA1IDLE, 100);
	p->a[0] = p->l[0] = 0;
	p->low [0] = 0x7fff;
	p->high[0] = 0x0000;

	if (m & JS_A1)
		return (3);
	if (!s->a[1])
		return (4);
	FA2BASE = p->low[1];
	FA2TOP  = p->high[1];
	p->c[1] = s->a[FA2F];
	p->play[1] = (Ushort)muldiv (FA2TOP, FA2IDLE, 100);
	p->a[1] = p->l[1] = 0;
	p->low [1] = 0x7fff;
	p->high[1] = 0x0000;


	POPTS |= CALIBRATED;
	return (0);
}

LOCAL_FUNC int FAR
JsbInit (POINTER *p, char *options)
{
	int	ret;

	POPTS |= USETIMER;
	JsOptions (p, options);

	if (initstick (1, options, POPTS))
		return (1);

	JsbCal (p);		/* one for the road */
	POPTS &= ~CALIBRATED;
	ret = JsbCal (p);
	POPTS &= ~CALIBRATED;
	if (ret)
		LogPrintf ("%s: cal failed %d\n",
			p->control->name, ret);
	else {
		MsgWPrintf (200, "Move the stick to all 4 egdes,");
		MsgWPrintf (200, "   then center it.");
		MsgWPrintf (200, "now hit 'x' to callibrate.");
	}

	return (ret);
}

LOCAL_FUNC void FAR
JsbTerm (POINTER *p)
{
	termstick (1, POPTS);
}


struct PtrDriver NEAR PtrAstick = {
	"ASTICK",
	0,
	NULL,	/* extra */
	JsaInit,
	JsaTerm,
	JsaCal,
	JsCenter,
	JsaRead,
	std_key
};

struct PtrDriver NEAR PtrBstick = {
	"BSTICK",
	0,
	NULL,	/* extra */
	JsbInit,
	JsbTerm,
	JsbCal,
	JsCenter,
	JsbRead,
	std_key
};

#undef PO
#undef FA1D
#undef FA1F
#undef FA2D
#undef FA2F
#undef FNREAD
#undef FDELAY
#undef FA1IDLE
#undef FA1BASE
#undef FA1TOP
#undef FA2IDLE
#undef FA2BASE
#undef FA2TOP
#undef FA3IDLE
#undef FA3BASE
#undef FA3TOP
#undef FA4IDLE
#undef FA4BASE
#undef FA4TOP
#undef POPTS

#undef REF
#undef REFDETENT

#endif /* if HAVE_JOYSTICK */
