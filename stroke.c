/* --------------------------------- stroke.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* text in graphics window - stroke painting.
*/

#include "fly.h"


static Uchar NEAR* NEAR* FAR StFonts[] = {
	(Uchar NEAR* NEAR*)StFont1,	/*  0 */
	(Uchar NEAR* NEAR*)StFont2,	/*  1 */
	(Uchar NEAR* NEAR*)StFont1,	/*  2 */
	(Uchar NEAR* NEAR*)StFont1,	/*  3 */
	(Uchar NEAR* NEAR*)StFont1,	/*  4 */
	(Uchar NEAR* NEAR*)StFont1,	/*  5 */
	(Uchar NEAR* NEAR*)StFont1,	/*  6 */
	(Uchar NEAR* NEAR*)StFont1,	/*  7 */
	(Uchar NEAR* NEAR*)StFont1,	/*  8 */
	(Uchar NEAR* NEAR*)StFont1	/*  9 */
};

#define	Font	st.StFontPtr

#define Sc(s)	{s*0/8, s*1/8, s*2/8, s*3/8, s*4/8, s*5/8, s*6/8, s*7/8}

static int NEAR Scales[][8] = {
Sc(0),  Sc(1),  Sc(2),  Sc(3),  Sc(4),  Sc(5),  Sc(6),  Sc(7),  Sc(8),  Sc(9),
Sc(10), Sc(11), Sc(12), Sc(13), Sc(14), Sc(15), Sc(16), Sc(17), Sc(18), Sc(19),
Sc(20), Sc(21), Sc(22), Sc(23), Sc(24), Sc(25), Sc(26), Sc(27), Sc(28), Sc(29),
Sc(30), Sc(31), Sc(32), Sc(33), Sc(34), Sc(35), Sc(36), Sc(37), Sc(38), Sc(39),
Sc(40), Sc(41), Sc(42), Sc(43), Sc(44), Sc(45), Sc(46), Sc(47), Sc(48), Sc(49),
Sc(50), Sc(51), Sc(52), Sc(53), Sc(54), Sc(55), Sc(56), Sc(57), Sc(58), Sc(59),
Sc(60), Sc(61), Sc(62), Sc(63), Sc(64), Sc(65), Sc(66), Sc(67), Sc(68), Sc(69),
Sc(70), Sc(71), Sc(72), Sc(73), Sc(74), Sc(75), Sc(76), Sc(77), Sc(78), Sc(79),
Sc(80), Sc(81), Sc(82), Sc(83), Sc(84), Sc(85), Sc(86), Sc(87), Sc(88), Sc(89),
Sc(90), Sc(91), Sc(92), Sc(93), Sc(94), Sc(95), Sc(96), Sc(97), Sc(98), Sc(99),
Sc(100), Sc(101), Sc(102), Sc(103), Sc(104), 
Sc(105), Sc(106), Sc(107), Sc(108), Sc(109),
Sc(110), Sc(111), Sc(112), Sc(113), Sc(114), 
Sc(115), Sc(116), Sc(117), Sc(118), Sc(119),
Sc(120), Sc(121), Sc(122), Sc(123), Sc(124),
Sc(125), Sc(126), Sc(127), Sc(128),
{0}};

#undef Sc

static ANGLE	StrokeAngle = 0;
static int	CosAngle = FONE, SinAngle = 0;
static int	StrokeX = 0, StrokeY = 0;
static int	StrokeDecChar = '.';

extern int FAR
font_set (int font)
{
	int	i;

	i = st.StFont;
	if (font >= 0) {
		st.StFont = font;
		st.StFontPtr = StFonts[st.StFont];
	}
	return (i);
}

extern void FAR
stroke_angle (ANGLE angle)
{
	StrokeAngle = angle;
	SinAngle = SIN (angle);
	CosAngle = COS (angle);
}

extern int FAR
stroke_decimal (int c)
{
	int	s;

	s = StrokeDecChar;
	StrokeDecChar = c;
	return (s);
}

extern int FAR
num_size (long num, int ss)
{
	int	size, dss, n;

	if (ss <= 0 || ss > rangeof (Scales))
		return (0);

	if (num < 0L) {
		num = -num;
		size = Scales[ss][0x0f&(Font['-'][0]>>4)];
	} else
		size = 0;
	dss = Scales[ss][0x0f&(Font['0'][0]>>4)];

	if (num < 10000L)
		if (num < 100L)
			if (num < 10L)
				n = 1;
			else
				n = 2;
		else
			if (num < 1000L)
				n = 3;
			else
				n = 4;
	else
		if (num < 1000000L)
			if (num < 100000L)
				n = 5;
			else
				n = 6;
		else
			if (num < 10000000L)
				n = 7;
			else if (num < 100000000L)
				n = 8;
			else
				n = 9;

	return (size+dss*n);
}

extern void FAR
num_extent (long num, int ss, int *exs, int *exc, int *eys, int *eyc)
{
	int	dx;

	dx = num_size (num, ss);
	*exs = fmul (dx, SinAngle);
	*exc = fmul (dx, CosAngle);
	*eys = fmul (ss, SinAngle);
	*eyc = fmul (ss, CosAngle);
}

extern int FAR
char_size (int Char, int size)
{
	register int	NEAR* scale;

	if (size <= 0 || size > rangeof (Scales))
		return (0);

	scale = Scales[size];
	return (Font[Char]
		? scale[0x0f&(Font[Char][0]>>4)]
		: scale[0x0f&(Font[0x100][0]>>4)]);
}

extern int FAR
stroke_size (char *s, int size)
{
	register int	NEAR* scale;
	register int	tsize, Char;

	if (!s || size <= 0 || size > rangeof (Scales))
		return (0);

	scale = Scales[size];
	for (tsize = 0; T(Char = *s++);) {
		if (Font[Char])
			tsize += scale[0x0f&(Font[Char][0]>>4)];
		else
			tsize += scale[0x0f&(Font[0x100][0]>>4)];
	}

	return (tsize);
}

#define stroke_line(scale,x,dx,y,dy,t) \
	if ('\x00' == (t)) \
		gr_draw ((x)+scale[dx], (y)-scale[dy]); \
	else \
		gr_move ((x)+scale[dx], (y)-scale[dy])

#define my_sign(x)	((x) < 0 ? -1 : 1)
#define my_round(x)	(((x) + my_sign (x)) >> 1)
	
LOCAL_FUNC void NEAR
stroke_rot (int	NEAR* scale, int x, int dx, int y, int dy, int t)
{
	int	xx, yy;

	dx = scale[dx] << 1;
	dy = scale[dy] << 1;
	xx = fmul (dx, CosAngle) - fmul (dy, SinAngle);
	yy = fmul (dx, SinAngle) + fmul (dy, CosAngle);

	x += my_round (xx);
	y -= my_round (yy);
	if ('\x00' == t)
		gr_draw (x, y);
	else
		gr_move (x, y);
}

LOCAL_FUNC int NEAR
stroke_c (int Char, int size)
{
	register int	NEAR* scale;
	register Uchar	NEAR* p;
	register int	t, xy;
	int		move;

	if (F(p = Font[Char]))
		p = Font[0x100];

	scale = Scales[size];
	if (StrokeAngle) {
		for (move = *p++; (t = *p++) < '\x02';) {
			xy = *p++;
			stroke_rot (scale, StrokeX, 0x0f&(xy>>4),
							StrokeY, 0x0f&xy, t);
		}
		move = scale[0x0f&(move>>4)];
		xy = fmul (move<<1, CosAngle);
		StrokeX += my_round (xy);
		xy = fmul (move<<1, SinAngle);
		StrokeY -= my_round (xy);
	} else {
		for (move = *p++; (t = *p++) < '\x02';) {
			xy = *p++;
			stroke_line (scale, StrokeX, 0x0f&(xy>>4),
							StrokeY, 0x0f&xy, t);
		}
		move = scale[0x0f&(move>>4)];
		StrokeX += move;
	}
	return (move);
}

extern int FAR
stroke_char (int x, int y, int Char, int size, int color)
{
	StrokeX = x;
	StrokeY = y;
	gr_color (color);

	if (size <= 0 || size > rangeof (Scales))
		return (0);

	return (stroke_c (Char, size));
}

extern int FAR
stroke_str (int x, int y, char *p, int size, int color)
{
	int	ch, numsize;

	StrokeX = x;
	StrokeY = y;
	gr_color (color);

	if (!p || !*p)
		return (0);

	if (size <= 0 || size > rangeof (Scales))
		return (0);

	for (numsize = 0; T(ch = *p); ++p)
		numsize += stroke_c (ch, size);
	return (numsize);
}

extern int FAR
stroke_num (int x, int y, long num, int size, int color)
{
	char	buf[15];

	sprintf (buf, "%ld", num);
	return (stroke_str (x, y, buf, size, color));
}

static Ulong magsize[] = {
	1UL, 10UL, 100UL, 1000UL, 10000UL, 100000UL,
	1000000UL, 10000000UL, 100000000UL, 1000000000UL,
};

extern int FAR
frac_size (long num, int digits, int frac, int ss)
{
	int	n, ndigits;

	if (ss <= 0 || ss > rangeof (Scales))
		return (0);

	n = Scales[ss][0x0f&(Font[StrokeDecChar][0]>>4)];
	if (num < 0) {
		n += Scales[ss][0x0f&(Font['-'][0]>>4)];
		num = -num;
	}
	if (digits > 0)
		ndigits = digits;
	else {
		if (frac >= rangeof(magsize))
			frac = rangeof(magsize) - 1;
		if ((Ulong)num >= magsize[frac]) {
			for (ndigits = frac; (Ulong)num >= magsize[++ndigits];)
				if (ndigits >= rangeof(magsize))
					break;
		} else {
			if (0 == digits)
				ndigits = frac + 1;
			else
				ndigits = frac;
		}
	}

	n += Scales[ss][0x0f&(Font['0'][0]>>4)] * ndigits;
	return (n);

}

extern int FAR
stroke_frac (int x, int y, long num, int digits, int frac, int size, int color)
/*
 * Show 't' in 'digits' positions and a decimal point 'frac' positions from
 * the right. If 'digits' is zero then the number of positions will be just as
 * many as will be needed with at least one integer digit. If 'digits' is -1
 * then fractions can start with the decimal point.
 * If 'digits' is less than 'frac' then the number will be truncated!
*/
{
	char	buf[15], *p;
	int	len, numsize;

	StrokeX = x;
	StrokeY = y;
	gr_color (color);

	if (size <= 0 || size > rangeof (Scales))
		return (0);

	if (num < 0) {
		numsize = stroke_c ('-', size);
		num = -num;
	} else
		numsize = 0;
	sprintf (buf, "%ld", num);
	len = strlen (p = buf);

	if (-1 == digits)
		digits = (len > frac) ? len : frac;
	else if (0 == digits)
		digits = (len > frac) ? len : frac+1;

	gr_color (color);
	for (; digits > 0; --digits) {
		if (digits == frac)
			numsize += stroke_c (StrokeDecChar, size);
		numsize += stroke_c ((digits > len) ? '0' : *p++, size);
	}
	return (numsize);
}

#undef Font
#undef stroke_line
#undef my_sign
#undef my_round

