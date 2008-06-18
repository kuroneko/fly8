/* --------------------------------- mac2max.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Dump a macro file into text format. See 'max2mac'.
*/

#include "fly.h"
#include "keyname.h"


static int	raw = 0;


static int NEAR
getshort (int *i, FILE *f)
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

static int NEAR
keyname (int key)
{
	int	i;

	if (!raw) {
		for (i = 0; f_name[i].name; ++i) {
			if (f_name[i].value == key) {
				printf ("%s", f_name[i].name);
				return (1);
			}
		}
	}
	for (i = 0; k_name[i].name; ++i) {
		if (k_name[i].value == key) {
			printf ("%s", k_name[i].name);
			return (1);
		}
	}
	return (0);
}

static int NEAR
shiftname (int shift)
{
	int	i;

	for (i = 0; k_name[i].name; ++i) {
		if (k_name[i].value && F(k_name[i].value & K_RAW)
		    && (k_name[i].value & shift) == k_name[i].value) {
			printf ("%s", k_name[i].name);
			return ((int)k_name[i].value);
		}
	}
	return (0);
}

static void NEAR
putkey (int key)
{
	int	i, t;

	if (keyname (key))
		return;

	for (i = key & K_SHIFTS; i && T(t = shiftname (i)); i ^= t)
		printf (" ");
	if (i)
		printf ("SHX%02X ", ((Uint)i)>>8);	/* unknown shift */

	key &= K_RAW;
	if (isprint (key))
		printf ("%c", key);
	else
		printf ("\\%u", key);
}

static void NEAR
listmac (char	*mname)
{
	int	i, j, t, len;
	FILE	*mac;

	mac = fopen (mname, RBMODE);
	if (!mac) {
		printf ("open '%s' failed", mname);
		return;
	}

	for (i = 0;; ++i) {
		if (2 != getshort (&t, mac))
			break;
		if (2 != getshort (&len, mac)) {
			printf ("read '%s' failed (1)", mname);
			goto ret;
		}
		printf ("def ");
		putkey (t);
		printf ("\n");
		for (j = 0; j < len; ++j) {
			if (2 != getshort (&t, mac)) {
				printf ("read '%s' failed (2)", mname);
				goto ret;
			}
			printf ("    ");
			putkey (t);
			printf ("\n");
		}
	}
	if (!feof (mac))
		printf ("read '%s' failed (3)", mname);
ret:
	fclose (mac);
}

int
main (int argc, char *argv[])
{
	char	*mname;
	int	i;

	mname = NULL;
	for (i = 1; i < argc; ++i) {
		if (!strcmp ("-r", argv[i])) {
			raw = 1;
			continue;
		}

		if (NULL == mname && F(mname = argv[i])) {
			printf ("bad mac file name '%s'", argv[i]);
			exit (1);
		}

		printf ("bad parameter '%s'", argv[i]);
		exit (1);
	}
	if (NULL == mname)
		mname = "fly.mac";

	listmac (mname);

	return (0);
}
