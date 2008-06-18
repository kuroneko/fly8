/* --------------------------------- message.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handle message queue for the display.
*/

#include "fly.h"

static int	MsgFlags = 0;

#define MSG_INITED	0x0001

LOCAL_FUNC HMSG * NEAR
msg_add (const char *fmt, va_list ap, int ttl, int flags)
{
	HMSG	*q;
	static char buf[256];

	vsprintf (buf, fmt, ap);

	if (ttl < 0) {
		LogPrintf ("%s\n", buf);
		ttl = -ttl;
	}

	if (!(MsgFlags & MSG_INITED))
		return (0);

	if (F(NEW (q)))
		return (0);

	q->text = (char *)memory_alloc (strlen (buf) + 1);
	if (!q->text) {
		DEL0 (q);
		return (0);
	}
	strcpy (q->text, buf);
	q->timeout = (ttl ? st.present + 100L*ttl : 0);
	q->flags = flags;
	q->next = st.msg;
	st.msg = q;

	if (st.quiet)
		Snd->Effect (EFF_MSG, SND_ON);

	return (q);
}

extern HMSG * FAR
msg_del (const HMSG *p)
{
	HMSG	*q, *prev;

	if (!p)
		return (NULL);

	for (prev = 0, q = st.msg; q && q != p; q = q->next)
		prev = q;

	if (!q)
		return (NULL);

	if (prev)
		prev->next = q->next;
	else
		st.msg = q->next;
	memory_free (q->text, strlen (q->text) + 1);
	DEL (q);
	return (NULL);
}

extern HMSG *FAR
MsgPrintf (int ttl, const char *fmt, ...)
{
	va_list	ap;
	HMSG 	FAR *msg;

	va_start (ap, fmt);
	msg = msg_add (fmt, ap, ttl, 0);
	va_end (ap);
	return (msg);
}

extern HMSG *FAR
MsgEPrintf (int ttl, const char *fmt, ...)
{
	va_list	ap;
	HMSG	FAR *msg;

	va_start (ap, fmt);
	msg = msg_add (fmt, ap, ttl, MSG_ERR);
	va_end (ap);
	return (msg);
}

extern HMSG *FAR
MsgWPrintf (int ttl, const char *fmt, ...)
{
	va_list	ap;
	HMSG	FAR *msg;

	va_start (ap, fmt);
	msg = msg_add (fmt, ap, ttl, MSG_WARN);
	va_end (ap);
	return (msg);
}

extern void FAR
msg_show (int orgx, int orgy, int maxx, int maxy, int bss)
{
	HMSG	*q, *next;
	int	c, x, y, x0, y0, y1, dx, dy;

	dx = dy = bss;

	x0 = orgx - maxx;
	y0 = orgy + maxy;
	y1 = orgy - maxy + dy;

	x = x0 + dx;
	y = y0 - 2*dy;

	for (q = st.msg; q; q = next) {
		next = q->next;
		if (y < y1 || (q->timeout && TIMEOUT (q->timeout) > 0))
			q = msg_del (q);
		else {
			if (VIS_REDBLUE == st.stereo)
				c = (q->flags & MSG_ERR) ? CC_WHITE
				  : (q->flags & MSG_WARN) ? CC_WHITE
				  : CC_LGRAY;
			else
				c = (q->flags & MSG_ERR) ? ST_MSGERR
				  : (q->flags & MSG_WARN) ? ST_MSGWARN
				  : ST_MFG;
			stroke_str (x, y, q->text, bss, c);
			y -= dy;
		}
	}
}

extern void FAR
msg_clear (int hard)
{
	HMSG	*q, *next;

	for (q = st.msg; q; q = next) {
		next = q->next;
		if (hard || q->timeout)
			q = msg_del (q);
	}
}

extern int FAR
msg_init (void)
{
	msg_clear (1);
	MsgFlags |= MSG_INITED;
	return (0);
}

extern void FAR
msg_term (void)
{
	MsgFlags &= ~MSG_INITED;
	msg_clear (1);
}

#undef MSG_INITED
