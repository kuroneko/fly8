/* --------------------------------- need.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Often missing common functions.
*/

#include "fly.h"


#if NEED_STRICMP
extern int FAR
stricmp (const char *a, const char *b)
{
	int	t;

	for (; 0 == (t = toupper(*(Uchar*)a) - toupper(*(Uchar*)b)); ++a,++b) {
		if ('\0' == *a)
			return (0);
	}
	return (t > 0 ? 1 : -1);
}
#endif

#if NEED_STRNICMP
extern int FAR
strnicmp (const char *a, const char *b, size_t n)
{
	int	t;

	for (; 0 < n; --n, ++a, ++b) {
		if (0 != (t = toupper (*(Uchar *)a) - toupper (*(Uchar *)b)))
			return (t > 0 ? 1 : -1);
		else if ('\0' == *a)
			return (0);
	}
	return (0);
}
#endif

#if NEED_STRDUP
extern char * FAR
strdup (register const char *s)
{
	char	*p;
	int	len;

	if (F(s))
		return (NULL);
	
	if (T(p = malloc (len = strlen (s) + 1)))
		memcpy (p, s, len);
	return (p);
}
#endif

#if NEED_STRERROR
extern char * FAR
strerror (int n)
{
	static char	buf[30];

	sprintf (buf, "error is %d", n);
	return (buf);
}
#endif

#if NEED_LABS
extern long FAR
labs (long x)
{
	return (x<0 ? -x : x);
}
#endif
