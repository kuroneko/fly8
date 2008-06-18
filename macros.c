/* --------------------------------- macros.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handle keyboard macros.
 *
 * This module manages the input data stream. Keys->Keys hold keystrokes that
 * are processed before the keyboard device is interrogated for more. It is
 * filled with data by calling mac_interpret().
 *
 * Another subject is key macros. Some keys are bound to a definition which
 * is a list of keystokes. When such a character is encountered it is this
 * list that gets placed in the KeysBuffer.
*/

#include "fly.h"


#define RECBUFFERSIZE	1024

struct KeysStack {
	Ushort	*Keys;
	int	Size;		/* original length */
	int	Count;		/* next available */
};
#define KEYSSTACKDEPTH		256
static struct KeysStack *KeysStack= NULL;
static struct KeysStack *Keys = NULL;

static Ushort	*RecordBuffer = 0;	/* macro recording buffer */
static Ushort	*RecordPtr = 0;		/* position in RecordBuffer */
static int	Quoting = 0;		/* mread ignores special keys */
static MACRO	*Macros = 0;		/* macro definitions */
static MACRO	*Recording = 0;		/* Macro being recorded */
static HMSG	*RecordingMsg = 0;

LOCAL_FUNC int	NEAR record_char (int ch);
LOCAL_FUNC int	NEAR mac_expand (int name);
LOCAL_FUNC int	NEAR mac_hot (int name);
LOCAL_FUNC int	NEAR mac_record (void);
LOCAL_FUNC int	NEAR mac_play (void);
LOCAL_FUNC int	NEAR mac_write (void);
LOCAL_FUNC int	NEAR mac_read (void);

LOCAL_FUNC int NEAR
keys_push (Ushort *keys, int size)
{
	if (NULL == Keys)
		Keys = &KeysStack[0];
	else if (&KeysStack[KEYSSTACKDEPTH] == ++Keys) {
		--Keys;
		MsgEPrintf (-50, "Macro Nesting overflow");
		return (1);
	}
	if (F(Keys->Keys = mem_alloc (size * sizeof (*Keys->Keys)))) {
		MsgEPrintf (-50, "keys_push no mem");
		return (2);
	}
	memcpy (Keys->Keys, keys, size * sizeof (*Keys->Keys));
	Keys->Size = size;
	Keys->Count = 0;

	return (0);
}

LOCAL_FUNC int NEAR
keys_get (void)
{
	int	ch;

	if (NULL == Keys)
		return (-1);

	ch = (int)Keys->Keys[Keys->Count++];

	for (; Keys->Count >= Keys->Size; --Keys) {
		mem_free (Keys->Keys, Keys->Size * sizeof (*Keys->Keys));
		if (&KeysStack[0] == Keys) {
			Keys = NULL;
			break;
		}
	}

	return (ch);
}

extern int FAR
mread (void)				/* non-blocking read */
{
	int	ch;

ReRead:
	if (-1 == (ch = keys_get ())) {
		for (;;) {
			ch = Kbd->Read ();
			if (-1 != ch && Recording) {
				ch = record_char (ch);
				if (ch == -1)
					continue;
			}
			break;
		}
	}

	if (-1 == ch)
		return (ch);

	if (mac_hot (ch))
		goto ReRead;


	if (!Quoting) {
		if (KF_MACRECORD == ch) {
			mac_record ();
			goto ReRead;
		}

		if (KF_MACPLAY == ch) {
			mac_play ();
			goto ReRead;
		}
	}

	return (ch);
}

extern int FAR
mgetch (void)				/* blocking read */
{
	int	ch;

	while (-1 == (ch = mread ()))
		sys_poll (1);
	return (ch);
}

extern int FAR
mac_interpret (Ushort *keys, int len)
{
	return (keys_push (keys, len));
}

extern void FAR
mac_flush (void)
{
	while (NULL != Keys)
		keys_get ();
	Recording = 0;
}

extern int FAR
mac_init (void)
{
	int	i;
	MACRO	*m;

	RecordBuffer = (Ushort *)memory_calloc (RECBUFFERSIZE,
						sizeof (*RecordBuffer));
	if (!RecordBuffer) {
		MsgEPrintf (-50, "macros: no mem (2)");
		return (1);
	}

	mac_flush ();

	Macros = (MACRO *)memory_calloc (st.nMacros, sizeof (*Macros));
	if (!Macros) {
		MsgEPrintf (-50, "macros: no mem (3)");
		return (1);
	}

	for (m = Macros, i = 0; i < st.nMacros; ++i, ++m) {
		m->name = KEYUNUSED;
		m->len  = 0;
		m->def  = 0;
	}

	KeysStack = (struct KeysStack *)memory_calloc (KEYSSTACKDEPTH,
				sizeof (*KeysStack));
	if (!KeysStack) {
		MsgEPrintf (-50, "macros: no mem (4)");
		return (1);
	}

	mac_read ();

	return (0);
}

extern void FAR
mac_term (void)
{
	int	i, j;
	MACRO	*m;

	mac_flush ();

	RecordBuffer = memory_cfree (RecordBuffer, RECBUFFERSIZE,
						sizeof (*RecordBuffer));

	if (Macros) {
		mac_write ();

		for (m = Macros, j = i = 0; i < st.nMacros; ++i, ++m) {
			if (KEYUNUSED != m->name) {
				m->def  = memory_cfree (m->def,
						m->len, sizeof (*m->def));
				m->name = KEYUNUSED;
				m->len  = 0;
				++j;
			}
		}

		Macros = memory_cfree (Macros, st.nMacros, sizeof (*Macros));
		LogPrintf ("nMacros  %d\n", j);
	}

	KeysStack = memory_cfree (KeysStack, KEYSSTACKDEPTH,
			sizeof (*KeysStack));
}

LOCAL_FUNC MACRO * NEAR
mac_find (int name)
{
	int	i;
	MACRO	*m;

	if (!Macros)
		return (0);

	for (m = Macros, i = 0; i < st.nMacros; ++i, ++m)
		if (m->name == (Ushort)name)
			return (m);
	return (0);
}

LOCAL_FUNC void NEAR
record_add (int ch)
{
	if (!RecordBuffer)
		MsgEPrintf (50, "No Recording");
	else if (RecordPtr == RecordBuffer+RECBUFFERSIZE)
		MsgEPrintf (50, "Recording overflow");
	else
		*RecordPtr++ = (Ushort)ch;
}

LOCAL_FUNC int NEAR
record_char (int ch)
/*
 * return (-1): control char used up, re-read.
 * else		use this char.
*/
{
	MACRO	*m;
	int	new, l;
	HMSG	*p;
	Ushort	sflags;

/* Check for redefined control keys
*/
	if (T(m = mac_find (ch)) && 1 == m->len &&
			(KF_MACPLAY == m->def[0] || KF_MACRECORD == m->def[0]))
		ch = m->def[0];

	if (ch == KF_MACPLAY) {
		m = Recording;
		Recording = 0;

		p = MsgWPrintf (0, "Abort/Cont/Quote?");
		sflags = st.flags;	st.flags |= SF_INTERACTIVE;
		ch = mgetch ();
		st.flags = sflags;
		msg_del (p);
		if (ch == KF_MACPLAY) {			/* abort */
			if (m->def == 0)
				m->name = KEYUNUSED;
			MsgPrintf (50, "Macro aborted");
			msg_del (RecordingMsg);
			--Quoting;
			return (-1);
		} else if (ch == KF_MACRECORD) {	/* cont */
			Recording = m;
			return (-1);
		}
		Recording = m;
		record_add (KF_MACPLAY);
		record_add (ch);
		if (!mac_expand (ch)) {
			MsgPrintf (10, "Macro undefined");
			return (ch);
		}
		return (-1);
	} else if (ch == KF_MACRECORD) {
		msg_del (RecordingMsg);
		RecordingMsg = 0;
		--Quoting;

		m = Recording;
		Recording = 0;

		new = (m->def == 0);
		l = RecordPtr - RecordBuffer;

		if (new && !l) {			/* aborted */
			MsgPrintf (50, "Macro aborted");
			m->name = KEYUNUSED;
			return (-1);
		}
		if (!new)				/* delete old */
			m->def = memory_cfree (m->def,
						m->len, sizeof (*m->def));
		m->len = 0;

		if (!l) {				/* just deleting */
			MsgPrintf (50, "Macro deleted");
			m->name = KEYUNUSED;
			return (-1);
		}

		m->def = (Ushort *)memory_calloc (l, sizeof (*m->def));
		if (!m->def) {
			MsgPrintf (100, "macros: no mem (4)");
			m->name = KEYUNUSED;
			return (-1);
		}
		memcpy (m->def, RecordBuffer, l * sizeof (Ushort));
		m->len = (Ushort)l;

		MsgPrintf (50, "Macro %sdefined", new ? "" : "re");
		return (-1);
	}

	record_add (ch);
	return (ch);
}

LOCAL_FUNC int NEAR
mac_record (void)
{
	int	name;
	MACRO	*m;
	HMSG	*p;
	Ushort	sflags;

	if (!Macros || !RecordBuffer) {
		MsgPrintf (50, "No Macros");
		return (-1);
	}

	p = MsgWPrintf (0, "Macro Key ?");
	sflags = st.flags;	st.flags |= SF_INTERACTIVE;
	name = Kbd->Getch ();			/* get macro name */
	st.flags = sflags;
	msg_del (p);
	if (name == KF_MACRECORD || name == KF_MACPLAY) {
		MsgPrintf (50, "Macro aborted");
		return (0);
	}

	m = mac_find (name);
	if (!m) {
		m = mac_find (KEYUNUSED);		/* new macro */
		if (!m) {
			MsgWPrintf (100, "All macros used");
			return (1);		/* all macros defined */
		}
		m->name = (Ushort)name;
		m->len = 0;
		m->def = 0;
	} else
		MsgWPrintf (100, "Warn ReDefining");

	Recording = m;
	RecordPtr = RecordBuffer;
	RecordingMsg = MsgWPrintf (0, "Recording...");
	++Quoting;

	return (0);
}

/* return 1 if hot (and then expand it too).
*/
LOCAL_FUNC int NEAR
mac_hot (int name)
{
	int	r, s;

	r = name & K_RAW;
	s = name & K_SHIFTS;
	if (((r >= 'a' && r <= 'z') || (r >= '0' && r <= '9'))
	    && (K_CTRL == s || K_ALT == s || (K_BTN & s))) {
		if (mac_expand (name))
			return (1);
	}
	return (0);
}

/* return 1 if a macro (and then expand it too).
*/
LOCAL_FUNC int NEAR
mac_expand (int name)
{
	MACRO		*m;
	static int	depth = 0;

	m = mac_find (name);
	if (!m || !m->def)
		return (0);

	if (depth > 16) {
		MsgEPrintf (10, "Macro nesting > 16");
		return (1);
	}
	++depth;
	mac_interpret (m->def, (int)m->len);
	--depth;

	return (1);
}

LOCAL_FUNC int NEAR
mac_play (void)
{
	int	name;
	HMSG	*p;
	Ushort	sflags;

	if (!Macros || !RecordBuffer) {
		MsgPrintf (100, "No Macros");
		return (-1);
	}

	p = MsgWPrintf (0, "Play/Abort?");
	++Quoting;
	sflags = st.flags;	st.flags |= SF_INTERACTIVE;
	name = Kbd->Getch ();			/* get macro name */
	st.flags = sflags;
	--Quoting;
	msg_del (p);
	if (name == KF_MACRECORD || name == KF_MACPLAY) {
		MsgPrintf (100, "Macro aborted");
		return (-1);
	}

	if (mac_expand (name))			/* play */
		return (0);

	MsgPrintf (10, "Macro undefined");
	return (-1);
}

LOCAL_FUNC int NEAR
write_short (Ushort n, FILE *f)
{
	fputc (0x0ff&(n>>8), f);
	if (ferror (f))
		return (0);
	fputc (0x0ff&n, f);
	if (ferror (f))
		return (1);
	return (2);
}

LOCAL_FUNC int NEAR
read_short (int *i, FILE *f)
{
	int	n;

	n = fgetc (f);
	if (ferror (f) || feof (f))
		return (0);
	n = (n<<8) + fgetc (f);
	if (ferror (f) || feof (f))
		return (1);
	*i = n;
	return (2);
}

LOCAL_FUNC int NEAR
mac_write (void)
{
	int	i;
	Ushort	j;
	MACRO	*m;
	FILE	*mac;

	if (!Macros || !st.mname)
		return (1);

	Sys->BuildFileName (st.filename, st.fdir, st.mname, MAC_EXT);
	mac = fopen (st.filename, WBMODE);
	if (!mac) {
#if 1
		MsgEPrintf (-50, "macros: open %s failed (1)", st.filename);
#endif
		return (1);
	}

	for (m = Macros, i = 0; i < st.nMacros; ++i, ++m) {
		if (!m->def)
			continue;
		if (2 != write_short (m->name, mac)) {
			MsgEPrintf (-50, "macros: write %s failed (1)",
				st.filename);
			goto ret;
		}
		if (2 != write_short (m->len, mac)) {
			MsgEPrintf (-50, "macros: write %s failed (2)",
				st.filename);
			goto ret;
		}
		for (j = 0; j < m->len; ++j) {
			if (2 != write_short (m->def[j], mac)) {
				MsgEPrintf (-50, "macros: write %s failed (3)",
					st.filename);
				goto ret;
			}
		}
	}
ret:
	fclose (mac);
	return (0);
}

LOCAL_FUNC int NEAR
mac_read (void)
{
	int	i, t;
	Ushort	j;
	MACRO	*m;
	FILE	*mac;

	if (!st.mname)
		st.mname = STRdup ("fly");

	if (!Macros || !st.mname) {
		MsgPrintf (-50, "No Macros");
		return (1);
	}

	i = max_read (Macros);
	if (i >= 0)
		return (i);

	Sys->BuildFileName (st.filename, st.fdir, st.mname, MAC_EXT);
	mac = fopen (st.filename, RBMODE);
	if (!mac) {
		MsgEPrintf (-50, "macros: open %s failed (2)", st.filename);
		return (1);
	}
	LogPrintf ("Macros   %s\n", st.filename);

	for (i = 0, m = Macros;; ++i, ++m) {
		if (2 != read_short (&t, mac))
			break;
		if (i >= st.nMacros) {
			MsgEPrintf (-50, "too many macros");
			goto ret;
		}
		m->name = (Ushort)t;
		if (2 != read_short (&t, mac)) {
			MsgEPrintf (-50, "macros: read %s failed (1)",
				st.filename);
			m->name = KEYUNUSED;
			goto ret;
		}
		m->len = (Ushort)t;
		m->def = (Ushort *)memory_calloc (m->len, sizeof (*m->def));
		if (!m->def) {
			MsgPrintf (-50, "macros: no mem (5)");
			m->name = KEYUNUSED;
			goto ret;
		}
		for (j = 0; j < m->len; ++j) {
			if (2 != read_short (&t, mac)) {
				MsgEPrintf (-50, "macros: read %s failed (2)",
					st.filename);
				m->name = KEYUNUSED;
				m->def = memory_cfree (m->def,
						m->len, sizeof (*m->def));
				goto ret;
			}
			m->def[j] = (Ushort)t;
		}
	}
	if (!feof (mac))
		MsgEPrintf (-50, "read %s failed (3)", st.filename);
ret:
	fclose (mac);
	return (0);
}

#undef RECBUFFERSIZE
