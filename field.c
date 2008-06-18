/* --------------------------------- field.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Read a field from a file, used for reading all input files.
*/

#include "fly.h"


extern int FAR
field_find (FILE *ifile, char *line)
{
	do {
		fgets (line, 256, ifile);
		if (ferror (ifile)) {
			LogPrintf ("%s %ld: nav read error\n",
				st.filename, st.lineno);
			return (-1);
		}
		++st.lineno;
	} while ('\n' == line[0] || '#' == line[0]);
	return (0);
}

extern int FAR
field_long (FILE *ifile, char *line, long *value)
{
	long	l;

	if (field_find (ifile, line))
		return (-1);
	if (1 != sscanf (line, "%ld", &l)) {
		LogPrintf ("bad number at line %ld: %s\n",
			st.lineno, st.filename);
		return (-1);
	}
	*value = l;
	return (0);
}

extern int FAR
field_read (FILE *ifile, struct FldTab FAR *fld, char * line)
{
	long	t;
	char	s[80];

	if (field_find (ifile, line))
		return (-1);

	if (READ_S & fld->type) {		/* string */
		if (1 != sscanf (line, "\"%[^\"]", s)) {
			LogPrintf ("%s %ld: bad field data\n",
				st.filename, st.lineno);
			return (-1);
		}
		*(char **)(fld->p) = STRdup (s);
		return (0);
	}

	if (1 != sscanf (line, "%ld", &t)) {
		LogPrintf ("%s %ld: bad field data\n",
			st.filename, st.lineno);
		return (-1);
	}

	if (READ_I+4 == fld->type)
		*(long *)(fld->p) = t;
	else if (READ_I+2 == fld->type)
		*(short *)(fld->p) = (short)t;
	else if (READ_I+1 == fld->type)
		*(short *)(fld->p) = (short)t;
	else
		return (-1);
	return (0);
}
