/* --------------------------------- log.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* log file stuff.
*/

#include "fly.h"


#define NOTHINGTOLOG	(1000UL*60UL*60UL)	/* 1 hour */

static FILE *flog = 0;

extern int FAR
log_init (void)
{
	char	*lname;

	st.LogFlush_next = st.present + NOTHINGTOLOG;

	if (F(lname = st.lname))
		lname = LOGFILE;

	if (F(flog = fopen (lname, ATMODE))) {
#ifndef NOSTDERR
		fprintf (stderr, "open failed for log file '%s'\n", lname);
#endif
		return (1);
	}
	return (0);
}

extern void FAR
log_term (void)
{
	if (flog) {
		fclose (flog);
		flog = 0;
	}
}

extern int FAR
log_flush (int now)
{
	if (!now && TIMEOUT (st.LogFlush_next) < 0)
		return (0);
	st.LogFlush_next = st.present + NOTHINGTOLOG;
	if (flog) {
		log_term ();
		if (log_init())
			return (-1);
	}
	return (0);
}

extern int FAR
LogPrintf (const char *fmt, ...)
{
	va_list		ap;
	int		i;

	if (!flog) {
#ifdef NOSTDERR
		return (-1);
#else
		va_start (ap, fmt);
		i = vfprintf (stderr, fmt, ap);
		fflush (stderr);
		va_end (ap);
		return (i);
#endif
	}

	va_start (ap, fmt);
	i = vfprintf (flog, fmt, ap);
	va_end (ap);

	st.LogFlush_next = st.present +
			(!(st.debug & DF_TRACE) && st.LogFlush_rate > 0
				? st.LogFlush_rate : 0);
	log_flush (0);

	return (i);
}

extern void FAR
LogDumpHex (char *title, Uchar *h, int hlen)
{
	int	i;

	LogPrintf ("%s:", title);
	for (i = 0; i < hlen; ++i) {
		if (!(i%32))
			LogPrintf ("\n  ");
		else if (!(i%16))
			LogPrintf ("   ");
		else if (!(i%4))
			LogPrintf (" ");
		LogPrintf ("%02x", h[i]);
	}
	LogPrintf ("\n");
}

extern int FAR
LogTrace (const char *file, int lineno)
{
	return (st.debug & DF_TRACE
		? LogPrintf (">>%s:%d\n", file, lineno)
		: 0);
}
