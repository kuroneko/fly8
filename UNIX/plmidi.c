/* --------------------------------- plmidi.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Sound device driver for Linux MIDI.
*/

#include "fly.h"

#if HAVE_MIDI

#include "plsound.h"
#include "notes.h"


#define INITED		0x8000

#define FLY8_MIDI_DEV	"/dev/midi"

static FILE	*fmidi = NULL;
static char	*midi_dev = NULL;

static int	midi_channel = 0;
static int	midi_program = 80;
static int	midi_volume = 60;
static int	midi_note = -1;
static int	notes[] = {
	4*BN_/2,
	4*CN_, 4*CSH_, 4*DN_, 4*DSH_, 4*EN_, 4*FN_,
	4*FSH_, 4*GN_, 4*GSH_, 4*AN_, 4*ASH_, 4*BN_
};


LOCAL_FUNC void FAR
MidiStart (int n)
{
	int	i, j;

	if (!fmidi)
		return;

	if (n <= 0)
		n = 1;
	for (i = 10*12; n < (notes[1]+notes[0])/2; i -= 12)
		n <<= 1;
	for (j = rangeof (notes)-1; n < (notes[j]+notes[j-1])/2; --j)
		{}
	if ((midi_note = i+j) > 127)
		midi_note = 127;
	else if (midi_note < 0)
		midi_note = 0;

	fputc (0x90+midi_channel, fmidi);
	fputc (midi_note, fmidi);
	fputc (midi_volume, fmidi);
	fflush (fmidi);
}

LOCAL_FUNC void FAR
MidiStop (void)
{
	if (!fmidi || midi_note < 0)
		return;

	fputc (0x80+midi_channel, fmidi);
	fputc (midi_note, fmidi);
	fputc (0x7f, fmidi);
	fflush (fmidi);
	midi_note = -1;
}

LOCAL_FUNC void FAR
MidiTerm (void)
{
	if (fmidi) {
		fputc (0xfc, fmidi);
		fclose (fmidi);
		fmidi = NULL;
	}
	if (midi_dev) {
		STRfree (midi_dev);
		midi_dev = NULL;
	}

	if (Snd->flags & INITED) {
		PlsTerm();
		Snd->flags &= ~INITED;
		LogPrintf ("%s: term ok\n", Snd->name);
	}
}

LOCAL_FUNC int FAR
MidiInit (char *options)
{
	Ulong	l;

	Snd->flags &= ~INITED;

	if (F(midi_dev = get_sarg (options, "dev=")))
		midi_dev = STRdup (FLY8_MIDI_DEV);

	if (get_narg (options, "channel=", &l))
		midi_channel = 0;
	else if (l > 127) {
		LogPrintf ("%s: channel too large\n",
			Snd->name);
		midi_channel = 0;
	} else
		midi_channel = (int)l;

	if (get_narg (options, "program=", &l))
		midi_program = 80;
	else if (l > 127) {
		LogPrintf ("%s: program too large\n",
			Snd->name);
		midi_program = 80;
	} else
		midi_program = (int)l;

	if (get_narg (options, "volume=", &l))
		midi_volume = 100;
	else if (l > 127) {
		LogPrintf ("%s: volume too large\n",
			Snd->name);
		midi_volume = 100;
	} else
		midi_volume = (int)l;

	LogPrintf ("%s: midi device is %s\n",
		Snd->name, midi_dev);
	LogPrintf ("%s: channel %d\n",
		Snd->name, midi_channel);
	LogPrintf ("%s: program %d\n",
		Snd->name, midi_program);
	LogPrintf ("%s: volume  %d\n",
		Snd->name, midi_volume);

	if (F(fmidi = fopen (midi_dev, "w")))
		goto badret;

	fputc (0xff, fmidi);			/* system reset */
	fputc (0xfa, fmidi);			/* start */
	fputc (0xc0+midi_channel, fmidi);	/* program change */
	fputc (midi_program, fmidi);
	fflush (fmidi);

	if (PlsInit (options))
		goto badret;

	Snd->flags |= INITED;
	LogPrintf ("%s: init ok\n", Snd->name);
	return (0);

badret:
	MidiTerm ();
	return (1);
}


static struct plsextra FAR MidiExtra = {
	MidiStart,
	MidiStop,
	NULL,		/* beeps */
	0L,		/* lasttime */
	0L,		/* nexttime */
	0,		/* playing */
	0		/* nbeeps */
};

struct SndDriver NEAR SndPlMidi = {
	"PlMidi",
	0,
	&MidiExtra,
	MidiInit,
	MidiTerm,
	PlsPoll,
	PlsBeep,
	PlsEffect,
	PlsList
};
#endif /* if HAVE_MIDI */
