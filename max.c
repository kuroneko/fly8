/* --------------------------------- max.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Read a text macro definition. Directly replaces mac_read() of macros.c.
*/

#include "fly.h"
#include "keyname.h"


#define BUF_STR_LEN	1024

static Ulong	lineno = 1L, column = 0L, errs = 0L;
static int	eof = 1, pushback = 0;
static int	pushbuff[10] = {0};
static char	tabCTRL[] = "@abcdefghijklmnopqrstuvwxyz[\\]^_";


LOCAL_FUNC int NEAR
keyval (int c)
{
	if (c >= 0 && c < 32)
		c = tabCTRL[c] | K_CTRL;
	return (c);
}

LOCAL_FUNC int NEAR
nlfgetc (FILE *f)
{
	int	c;

	if (pushback) {
		c = pushbuff[--pushback];
		return (c);
	}

	if (eof)
		return (EOF);

	c = fgetc (f);
	++column;
	if ('\n' == c) {
		++lineno;
		column = 0;
	} else if (EOF == c)
		eof = 1;

	return (c);
}

LOCAL_FUNC Ulong NEAR
eatline (FILE *f)
{
	int	c;

	do {
		c = nlfgetc (f);
	} while (EOF != c && '\n' != c);
	return (EOF == c ? VK_EOF : (Ulong)c);
}

LOCAL_FUNC int NEAR
getbase (FILE *f, int base, int n, int c)
{
	int	i;

	for (i = 0;;) {
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'a' && c <= 'f')
			c -= 'a' - 10;
		else if (c >= 'A' && c <= 'F')
			c -= 'A' - 10;
		else {
			pushbuff[pushback++] = c;
			break;
		}
		if (c >= base)
			break;
		i = i * base + c;
		if (--n < 0)
			break;
		c = nlfgetc (f);
	}

	if (i >= 256) {
		++errs;
		MsgEPrintf (-50,
			"max: escaped value too big at %ld:%ld",
			lineno, column);
		i &= 0x00ff;
	}
	return (i);
}

LOCAL_FUNC int NEAR
getescape (FILE *f)
{
	int	c;

	c = nlfgetc (f);

	switch (c) {
	case 'a':
		c = 'g';
		break;
	case 'b':
		c = 'h';
		break;
	case 'e':
		c = '[';
		break;
	case 'f':
		c = 'l';
		break;
	case 'n':
		c = 'm';
		break;
	case 'r':
		c = 'j';
		break;
	case 't':
		c = 'i';
		break;
	case 'v':
		c = 'k';
		break;
	case '\n':
		c = -1;		/* traditional line break */
		goto raw;
	case '0':
		c = keyval (getbase (f, 8, 3, '0'));
		goto raw;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		c = keyval (getbase (f, 10, 2, c));
		goto raw;
	case 'x':
		c = keyval (getbase (f, 16, 2, '0'));
		goto raw;
	default:
		goto raw;
	}
	c |= K_CTRL;
raw:
	return (c);
}

LOCAL_FUNC int NEAR
getstring (FILE *f, Ushort *buf_str)
{
	int	c, n;

	for (n = 0;;) {
		c = nlfgetc (f);
		if (EOF == c) {
			++errs;
			MsgEPrintf (-50, "max: EOF inside string at %ld:%ld",
				lineno, column);
			n = -1;
			break;
		}
		if ('\\' == c) {
			c = getescape (f);
			if (-1 == c)
				continue;
		} else if ('"' == c)
			break;
		else if ('\n' == c)		/* simplified line break */
			continue;
		else
			c = keyval (c);
		if (n >= BUF_STR_LEN) {
			++errs;
			MsgEPrintf (-50, "max: string too long at %ld:%ld",
				lineno, column);
			continue;
		}
		buf_str[n++] = (Ushort)c;
	}
	return (n);
}

LOCAL_FUNC Ulong NEAR
getterm (FILE *f)
{
	char	keyname[32+1];
	int	n, i, c;

retry:
	for (n = 0;;) {
		c = nlfgetc (f);
		if (EOF == c) {
			if (n)
				break;
			return (VK_EOF);
		}
		if (isspace (c)) {
			if (n)
				break;
			continue;
		}
		if ('\\' == c) {
			if (n) {
				++errs;
				MsgEPrintf (-50, "max: bad escape at %ld:%ld",
					lineno, column);
				continue;
			}
			c = getescape (f);
			if (-1 == c)
				continue;
			return (c);
		}
		if (n < sizeof (keyname) - 1) {
			if (0 == n) {
				if ('"' == c)
					return (VK_STR);
				if ('#' == c)
					return (VK_COM);
			}
			keyname[n++] = (char)c;
		} else
			{/* truncate... */}
	}
	keyname[n] = '\0';

/* Look word up - is it a key?
*/
	for (i = 0; k_name[i].name; ++i) {
		if (!stricmp (k_name[i].name, keyname))
			return (k_name[i].value);
	}

/* Look word up - maybe a function name?
*/
	for (i = 0; f_name[i].name; ++i) {
		if (!stricmp (f_name[i].name, keyname))
			return (f_name[i].value);
	}

	if (1 == n)
		return (keyval(*(Uchar *)keyname));

	++errs;
	MsgEPrintf (-50, "max: bad token '%s' at %ld:%ld", keyname,
		lineno, column);
	if (EOF == c)
		return (VK_EOF);

	goto retry;
}

LOCAL_FUNC Ulong NEAR
maxkey (FILE *f)
{
	Ulong	c, key;

	for (key = 0L;;) {
		c = getterm (f);
		if (VK_EOF == c) {
			if (key) {
				++errs;
				MsgEPrintf (-50,
"max: unexpected end of file at %ld:%ld",
					lineno, column);
			}
			return (VK_EOF);
		} else if (VK_DEF == c) {
			if (key) {
				++errs;
				MsgEPrintf (-50,
"max: bad key preceding 'def' at %ld:%ld",
					lineno, column);
			}
			key = VK_DEF;
		} else if (VK_COM == c) {
			if (key) {
				++errs;
				MsgEPrintf (-50,
"max: bad key preceding comment at %ld:%ld",
					lineno, column);
			}
			key = 0L;
			c = eatline (f);
			if (VK_EOF == c)
				return (VK_EOF);
		} else if (VK_STR == c) {
			if (key) {
				++errs;
				MsgEPrintf (-50,
"max: bad key preceding string at %ld:%ld",
					lineno, column);
			}
			return (VK_STR);
		} else {
			key |= c;
			if (c & K_RAW)
				return (key);
		}
	}
	/* never reached */
}

#define MAXMACLEN	1024

extern int FAR
max_read (MACRO *Macros)
{
	FILE	*max = NULL;
	int	i, n, nMacros;
	Ulong	c;
	MACRO	*m;
	Ushort	*macbody = NULL;
	Ushort	*buf_str = NULL;

	macbody = (Ushort *)memory_calloc (MAXMACLEN, sizeof (*macbody));
	if (!macbody) {
		++errs;
		MsgEPrintf (-50, "max: no mem (1)");
		goto ret;
	}

	buf_str = (Ushort *)memory_calloc (BUF_STR_LEN, sizeof (*buf_str));
	if (!buf_str) {
		++errs;
		MsgEPrintf (-50, "max: no mem (2)");
		goto ret;
	}

	Sys->BuildFileName (st.filename, st.fdir, st.mname, MAX_EXT);
	max = fopen (st.filename, RTMODE);
	if (!max) {
		MsgEPrintf (-50, "max: open '%s' failed",
			st.filename);
		++errs;
		goto ret;
	}
	eof = 0;
	pushback = 0;
	LogPrintf ("Max      %s\n", st.filename);

	for (m = Macros, nMacros = 0;;) {
		c = maxkey (max);
		if (VK_EOF == c)
			break;
		if (!(VK_DEF & c)) {
			++errs;
			MsgEPrintf (-50, "max: 'def' not found at %ld:%ld",
				lineno, column);
			continue;
		}
def:
		if (nMacros > st.nMacros) {
			++errs;
			MsgEPrintf (-50, "max: too many macros at %ld:%ld",
				lineno, column);
			break;
		}
		m->name = (Ushort)c;
		for (n = 0;;) {
			c = maxkey (max);
			if (VK_EOF == c || (VK_DEF & c))
				break;
			if (VK_STR == c) {
				i = getstring (max, buf_str);
				if (i <= 0)
					continue;
				if (n+i > MAXMACLEN) {
					++errs;
					MsgEPrintf (-50,
"max: macro too long at %ld:%ld", lineno, column);
				} else {
					memcpy (macbody+n, buf_str,
						i * sizeof (Ushort));
					n += i;
				}
				continue;
			}
			if (n >= MAXMACLEN) {
				++errs;
				MsgEPrintf (-50,
					"max: macro too long at %ld:%ld",
					lineno, column);
				continue;
			}
			macbody[n++] = (Ushort)c;
		}
		if (0 == n) {
			++errs;
			MsgEPrintf (-50, "max: empty macro at %ld:%ld",
				lineno, column);
		}
		m->len = (Ushort)n;
		m->def = (Ushort *)memory_calloc (m->len, sizeof (*m->def));
		if (!m->def) {
			++errs;
			MsgEPrintf (-50, "max: no mem (3)");
			m->name = KEYUNUSED;
			goto ret;
		}
		memcpy (m->def, macbody, m->len * sizeof (*m->def));
		++m;
		++nMacros;
		if (VK_DEF & c)
			goto def;
		if (VK_EOF == c)
			break;
	}
ret:
	if (max) {
		fclose (max);
		max = NULL;
	}
	eof = 1;
	pushback = 0;
	if (macbody)
		macbody = memory_cfree (macbody, MAXMACLEN,
				sizeof (*macbody));
	if (buf_str)
		buf_str = memory_cfree (buf_str, BUF_STR_LEN,
				sizeof (*buf_str));
	return (0L != errs);
}

#undef MAXMACLEN
