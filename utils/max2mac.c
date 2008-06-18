/* --------------------------------- max2mac.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Read a text macro definition and build a macro file. See 'mac2max'.
*/

#include "fly.h"
#include "keyname.h"


/* This is needed since "fly.h" defines it.
*/
struct status	NEAR st = {0};

static Ulong lineno = 1L, column = 0L, errs = 0L;
static int	eof = 1, pushback = 0;
static int	pushbuff[10] = {0};
static char tabCTRL[] = "@abcdefghijklmnopqrstuvwxyz[\\]^_";
static Ushort buf_str[1024] = {0};

static int NEAR
putshort (short i, FILE *f)
{
	if (errs)
		return (0);

	fputc (0x00ff&(i>>8), f);
	if (ferror (f)) {
		++errs;
		fprintf (stderr, "write error\n");
		return (1);
	}

	fputc (0x00ff&i, f);
	if (ferror (f)) {
		++errs;
		fprintf (stderr, "write error\n");
		return (1);
	}

	return (0);
}

static int NEAR
keyval (int c)
{
	if (c >= 0 && c < 32)
		c = tabCTRL[c] | K_CTRL;
	return (c);
}

static int NEAR
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

static Ulong NEAR
eatline (FILE *f)
{
	int	c;

	do {
		c = nlfgetc (f);
	} while (EOF != c && '\n' != c);
	return (EOF == c ? VK_EOF : (Ulong)c);
}

static int NEAR
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
		printf ("escaped value too big at %ld:%ld\n",
			lineno, column);
		i &= 0x00ff;
	}
	return (i);
}

static int NEAR
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

static int NEAR
getstring (FILE *f)
{
	int	c, n;

	for (n = 0;;) {
		c = nlfgetc (f);
		if (EOF == c) {
			++errs;
			printf ("EOF inside string at %ld:%ld\n",
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
		if (n >= rangeof (buf_str)) {
			++errs;
			printf ("string too long at %ld:%ld\n",
				lineno, column);
			continue;
		}
		buf_str[n++] = (Ushort)c;
	}
	return (n);
}

static Ulong NEAR
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
				printf ("bad escape at %ld:%ld\n",
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

/* Look word up - is it a function?
*/
	for (i = 0; f_name[i].name; ++i) {
		if (!stricmp (f_name[i].name, keyname))
			return (f_name[i].value);
	}

	if (1 == n)
		return (keyval(*(Uchar *)keyname));

	++errs;
	printf ("bad token '%s' at %ld:%ld\n", keyname, lineno, column);
	if (EOF == c)
		return (VK_EOF);

	goto retry;
}

static Ulong NEAR
getkey (FILE *f)
{
	Ulong	c, key;

	for (key = 0L;;) {
		c = getterm (f);
		if (VK_EOF == c) {
			if (key) {
				++errs;
				printf ("unexpected end of file at %ld:%ld\n",
					lineno, column);
			}
			return (VK_EOF);
		} else if (VK_DEF == c) {
			if (key) {
				++errs;
				printf ("bad key preceding 'def' at %ld:%ld\n",
					lineno, column);
			}
			key = VK_DEF;
		} else if (VK_COM == c) {
			if (key) {
				++errs;
				printf ("bad key preceding '##' %ld:%ld\n",
					lineno, column);
			}
			key = 0L;
			c = eatline (f);
			if (VK_EOF == c)
				return (VK_EOF);
		} else if (VK_STR == c) {
			if (key) {
				++errs;
				printf ("bad key preceding '#\"' %ld:%ld\n",
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

static Ushort definition[1024] = {0};

static void NEAR
buildmac (char *mname)
{
	FILE	*mac;
	int	i, n;
	Ulong	c;

	mac = fopen (mname, WBMODE);
	if (!mac) {
		++errs;
		printf ("open '%s' failed", mname);
		return;
	}
	eof = 0;
	pushback = 0;

	for (;;) {
		c = getkey (stdin);
		if (VK_EOF == c)
			break;
		if (!(VK_DEF & c)) {
			++errs;
			printf ("'def' not found at %ld:%ld\n", lineno, column);
			continue;
		}
def:
		if (putshort ((short)c, mac))	/* macro name */
			break;
		for (n = 0;;) {
			c = getkey (stdin);
			if (VK_EOF == c || (VK_DEF & c))
				break;
			if (VK_STR == c) {
				i = getstring (stdin);
				if (i <= 0)
					continue;
				if (n+i > rangeof (definition)) {
					++errs;
					printf ("macro too long at %ld:%ld\n",
						lineno, column);
				} else {
					memcpy (definition+n, buf_str, 2*i);
					n += i;
				}
				continue;
			}
			if (n >= rangeof (definition)) {
				++errs;
				printf ("macro too long at %ld:%ld\n",
					lineno, column);
				continue;
			}
			definition[n++] = (Ushort)c;
		}
		if (0 == n) {
			++errs;
			printf ("empty macro at %ld:%ld\n", lineno, column);
		}
		if (putshort ((short)n, mac))	/* macro length */
			break;
		for (i = 0; i < n; ++i) {	/* macro body */
			if (putshort (definition[i], mac))
				goto ret;
		}
		if (VK_DEF & c)
			goto def;
		if (VK_EOF == c)
			break;
	}
ret:
	fclose (mac);
	eof = 1;
	pushback = 0;
}

int
main (int argc, char *argv[])
{
	char	*mname;

	if (argc < 2 || F(mname = argv[1]))
		mname = "fly.mac";

	buildmac (mname);

	exit (0);
	return (0);
}
