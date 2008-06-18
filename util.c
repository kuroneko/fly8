/* --------------------------------- util.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* General purpose utility functions.
*/

#include "fly.h"


extern int FAR
opt36 (int c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	if (c >= 'a' && c <= 'z')	/* Warning: ASCII only !!! */
		return (c - 'a' + 10);
	if (c >= 'A' && c <= 'Z')
		return (c - 'A' + 10);
	return (-1);
}

extern int FAR
get_long (char *p, long *lp)
{
	long	l;

	if (!p)
		return (1);

	if (p[0] == '0') {
		if (p[1] == 'x' || p[1] == 'X')
			if (1 != sscanf (p+2, "%lx", &l))
				return (1);
			else
				{}
		else if (1 != sscanf (p, "%lo", &l))
			return (1);
		else
			{}
	} else if (1 != sscanf (p, "%ld", &l))
		return (1);
	else
		{}
	*lp = l;
	return (0);
}

extern int FAR
get_int (char *p, int *li)
{
	long	l;

	if (get_long (p, &l) || l != (long)(int)l)
		return (1);
	if (l != (long)(int)l)
		return (1);
	*li = (int)l;
	return (0);
}

/* The following functions are used by the drivers to retrieve user options.
 *
 * The general format is ":[opt]:[opt]:[opt]..."
 *
 * A leading ':' is assumed to be in front of the 0-th element if it is
 * not actually present.
 *
 * Un-specified options are left out. Trailing ':::' not needed.
 *
 * An option is either "NameValue" or "Value". "Name" should include a
 * trailing "=" if you want a "Name=Value" format.
 *
 * The first two functions use the Name and return the Value (either a
 * pointer[non-terminated] or a long) while the last two (get_i*) return
 * the Value for the i-th (first is zero) option (no Name expected).
 * NULL/-1L pointer means not present.
 *
 *	get_parg()	returns a pointer to the option Value (by name)
 *	get_piarg()	returns a pointer to the option Value (by index)
 *	get_sarg()	returns a (string) copy of the option Value (by name)
 *	get_siarg()	returns a (string) copy of the option Value (by index)
 *	get_narg()	returns the Value as a long (by name)
 *	get_niarg()	returns the Value as a long (by index)
*/

extern char * FAR
get_parg (char *options, char *argname)
{
	char	*p;
	int	len;

	len = strlen (argname);

	for (p = options; p; p = strchr (p, ':')) {
		if (p != options || ':' == *p)
			++p;
		if (!strnicmp (p, argname, len)) {
			p += len;
			break;
		}
	}
	return (p);
}

extern char * FAR
get_piarg (char *options, int argno)
{
	char	*p;
	int	i;

	if (0 == argno) {
		if (T(p = options) && ':' == *p)
			++p;
	} else {
		for (i = 0, p = options; p; p = strchr (p, ':'), ++i) {
			++p;
			if (i == argno)
				break;
		}
	}
	return (p);
}

extern char * FAR
get_sarg (char *options, char *argname)
{
	char	*p, *q;
	int	t;

	if (F(p = get_parg (options, argname)))
		return (0);
	q = strchr (p, ':');
	if (!q)
		p = STRdup (p);
	else {
		t = *q;
		*q = '\0';
		p = STRdup (p);
		*q = (char)t;
	}
	return (p);
}

extern char * FAR
get_siarg (char *options, int argno)
{
	char	*p, *q;
	int	t;

	if (F(p = get_piarg (options, argno)))
		return (0);
	q = strchr (p, ':');
	if (!q)
		p = STRdup (p);
	else {
		t = *q;
		*q = '\0';
		p = STRdup (p);
		*q = (char)t;
	}
	return (p);
}

extern int FAR
get_narg (char *options, char *argname, long *lp)
{
	char	*p;
	long	l;

	if (F(p = get_parg (options, argname)) || get_long (p, &l))
		return (1);
	*lp = l;
	return (0);
}

extern int FAR
get_niarg (char *options, int argno, long *lp)
{
	char	*p;
	long	l;

	if (F(p = get_piarg (options, argno)) || get_long (p, &l))
		return (1);
	*lp = l;
	return (0);
}

extern int FAR			/* debug utility */
looping (int i)
{
	static int	n;

	if (!i)
		n = 0;
	else if (++n > i)
		return (1);

	return (0);
}

/* This one is from "Numerical Recipes in C"
*/

#define IA	16807
#define IM	2147483647UL
#define IQ	127773UL
#define IR	2836
#define IMASK	123459876UL

static long	rand_seed = 1;

extern int FAR
Frand (void)
{
	long	k;

	rand_seed ^= IMASK;
	k = rand_seed / IQ;
	rand_seed = IA * (rand_seed - k * IQ) - IR * k;
	if (rand_seed < 0)
		rand_seed += IM;
	rand_seed ^= IMASK;
	return ((int)(0x07fff & rand_seed));
}
#undef IA
#undef IM
#undef IQ
#undef IR
#undef IMASK

extern void FAR
Fsrand (Uint seed)
{
	rand_seed = seed;
}

extern void FAR
Frandomize (void)
{
	Fsrand (Tm->Hires ());
}


static char	FAR sline[4][40] = {{0}};
static int	FAR iline = 0;

LOCAL_FUNC char * NEAR
alloc_line (void)
{
	char	*line;

	line = sline[iline++];
	if (iline >= rangeof(sline))
		iline = 0;
	return (line);
}

LOCAL_FUNC Uchar * NEAR
edit_l (Ulong n, char *s, Ulong f)
{
#if 0
	int	m, k, u;
	char	*line;

	line = alloc_line ();

	u = (int)(n % 1000);		n /= 1000;
	k = (int)(n % 1000);		n /= 1000;
	m = (int)(n % 1000);		n /= 1000;
	if (n)
		sprintf (line, "%s%u,%03u,%03u,%03u",
				s, (int)n, m, k, u);
	else if (m)
		sprintf (line, "%s%u,%03u,%03u",
				s, m, k, u);
	else if (k)
		sprintf (line, "%s%u,%03u",
				s, k, u);
	else
		sprintf (line, "%s%u",
				s, u);
#else
	int	i, j;
	int	k[20];
	char	*line;

	line = alloc_line ();

/* break number into digits
*/
	for (i = 0; i < rangeof(k); ++i) {
		k[i] = (int)(n % 10);
		n /= 10;
	}

/* multiply by factor
*/
	if (f) {
		n = 0;
		for (i = 0; i < rangeof(k); ++i) {
			n += k[i] * f;
			k[i] = (int)(n % 10);
			n /= 10;
		}
	}

/* Print prefix
*/
	j = 0;
	if (s) {
		while (*s)
			line[j++] = *s++;
	}

/* Find highest digit
*/
	for (i = rangeof(k); --i > 0 && 0 == k[i];)
		;

/* Print number
*/
	for (; i >= 0; --i) {
		line[j++] = (char)('0' + k[i]);
		if (i && !(i%3))
			line[j++] = ',';
	}
	line[j] = '\0';
#endif
	return ((Uchar *)line);
}

extern Uchar * FAR
show_l (long l)
{
	Ulong	n;
	char	*s;

	if (l < 0) {
		n = (Ulong)-l;
		s = "-";
	} else {
		n = (Ulong)l;
		s = "";
	}


	return (edit_l (n, s, 0L));
}

extern Uchar * FAR
show_ul (Ulong l)
{
	return (edit_l (l, NULL, 0L));
}

extern Uchar * FAR
show_ulf (Ulong l, Ulong f)
{
	return (edit_l (l, NULL, f));
}

extern Uchar *FAR
show_time (char *title, Ulong tt)
{
	int	tn, ss, mm, hh;
	Ulong	t;
	char	*line;

	line = alloc_line ();

	t = tt / 100;
	tn = (int)(t % 10);		t /= 10;
	ss = (int)(t % 60);		t /= 60;
	mm = (int)(t % 60);		t /= 60;
	hh = (int)(t % 24);		t /= 24;
	if (t)
		sprintf (line, "%s%u+%02u:%02u:%02u.%01u",
				title, (int)t, hh, mm, ss, tn);
	else if (hh)
		sprintf (line, "%s%02u:%02u:%02u.%01u",
				title, hh, mm, ss, tn);
	else
		sprintf (line, "%s%02u:%02u.%01u",
				title, mm, ss, tn);
	return ((Uchar *)line);
}

/* A set of functions to store (P) or retrieve (G) Big (B) or Little (L)
 * endian, Words (w) or Longwords (l). Used in the low level networking
 * drivers.
*/

extern Uint FAR
ComGBw (Uchar *p)
{
	return (((Uint)p[0] << 8) + p[1]);
}

extern Uint FAR
ComGLw (Uchar *p)
{
	return (((Uint)p[1] << 8) + p[0]);
}

extern void FAR
ComPBw (Uchar *p, Uint w)
{
	p[0] = (Uchar)(0x0ff & (w >> 8));
	p[1] = (Uchar)(0x0ff & (w     ));
}

extern void FAR
ComPLw (Uchar *p, Uint w)
{
	p[0] = (Uchar)(0x0ff & (w     ));
	p[1] = (Uchar)(0x0ff & (w >> 8));
}

extern void FAR
ComPBl (Uchar *p, Ulong l)
{
	p[0] = (Uchar)(0x0ff & (l >> 24));
	p[1] = (Uchar)(0x0ff & (l >> 16));
	p[2] = (Uchar)(0x0ff & (l >>  8));
	p[3] = (Uchar)(0x0ff & (l      ));
}
