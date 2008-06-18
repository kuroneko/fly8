/* --------------------------------- banktool.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* FM Bank tool for FLY8
 *
 * author: Chris Collins (ccollins@pcug.org.au)
 *
 * The bank tools will let me build new banks, and assign new instruments
 * into the banks.
*/

#include <stdio.h>
#include <stdlib.h>


typedef struct inst {
	char	AnyBloodyThing[12];
} inst;

static inst	instbank[20];

static void
usage (void)
{
	printf ("banktool: Tool for bank manipulation\n");
	printf ("banktool [filename] will build a new bank of default.ins\n");
	printf ("banktool [infilename] [patchnumber] [patchname] [outfilename]\n");
	printf ("         will replace the instrument at [patchnumber]\n");
}

static void
buildbank (char *outname)
{
	FILE	*fout;
	FILE	*fin;
	inst	t_inst;
	int	ctr;

	fout = fopen (outname, "wb");
	fin = fopen ("default.ins", "rb");
	fread (&t_inst, 12, 1, fin);
	for (ctr = 0; ctr < 20; ctr++) {
		fwrite (&t_inst, 12, 1, fout);
	}
	fclose (fin);
	fclose (fout);
}

static int
replaceinst (char *inbankname, int instnum, char *patchname, char *outbankname)
{
	FILE	*fbin = NULL;
	FILE	*fin = NULL;
	FILE	*fout = NULL;
	inst	t_inst;
	int	ctr;
	int	ret = 0;
	
	if (NULL == (fbin = fopen (inbankname, "rb"))) {
		ret = -1;
		goto done;
	}
	if (NULL == (fout = fopen (outbankname, "wb"))) {
		ret = -2;
		goto done;
	}
	if (NULL == (fin = fopen (patchname, "rb"))) {
		ret = -3;
		goto done;
	}
	for (ctr = 0; ctr < 20; ++ctr) {
		if (1 != fread (&t_inst, 12, 1, fbin)) {
			ret = -4;
			break;
		}
		if (ctr == instnum) {
			if (1 != fread (&t_inst, 12, 1, fin)) {
				ret = -5;
				break;
			}
		}
		if (1 != fwrite (&t_inst, 12, 1, fout)) {
			ret = -6;
			break;
		}
	}
done:
	if (fin)
		fclose (fin);
	if (fout)
		fclose (fout);
	if (fbin)
		fclose (fbin);

	return (ret);
}
	

int
main (int argc, char **argv)
{
	int	ctr;
	
	if (2 == argc)
		buildbank (argv[1]);
	else if (5 == argc) {
		ctr = atoi (argv[2]);
		ctr = replaceinst (argv[1], ctr, argv[3], argv[4]);
		if (ctr)
			fprintf (stderr, "replaceinst failed %d\n", ctr);
	} else {
		usage ();
		exit (1);
	}

	return (0);
}
