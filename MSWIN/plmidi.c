/* --------------------------------- plmidi.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Sound device driver for mswim MIDI.
*/

#include "config.h"
#if HAVE_MIDI
#include <windows.h>
#include <mmsystem.h>

#include "fly.h"
#include "plsound.h"
#include "notes.h"


#define INITED		0x8000

static HMIDIOUT	mout = 0;
static int	midi_channel = 0;
static int	midi_program = 80;
static int	midi_volume = 60;
static int	midi_note = -1;
static int	notes[] = {
	4*BN_/2,
	4*CN_, 4*CSH_, 4*DN_, 4*DSH_, 4*EN_, 4*FN_,
	4*FSH_, 4*GN_, 4*GSH_, 4*AN_, 4*ASH_, 4*BN_
};


LOCAL_FUNC void NEAR
MidiOut (int a, int b, int c)
{
	Ulong	l;

	l = 0x0ff & c;
	l <<= 8;	l |= 0x0ff & b;
	l <<= 8;	l |= 0x0ff & a;
	midiOutShortMsg (mout, l);
}

LOCAL_FUNC void FAR
MidiStart (int n)
{
	int	i, j;

	if (!mout)
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

	MidiOut (0x090+midi_channel, midi_note, midi_volume);
}

LOCAL_FUNC void FAR
MidiStop (void)
{
	if (!mout || midi_note < 0)
		return;
	MidiOut (0x080+midi_channel, midi_note, 0x7f);
	midi_note = -1;
}

LOCAL_FUNC void FAR
MidiTerm (void)
{
	if (mout) {
		MidiOut (0xfc, 0, 0);
		midiOutClose (mout);
		mout = 0;
	}

	if (Snd->flags & INITED) {
		PlsTerm ();
		Snd->flags &= ~INITED;
		LogPrintf ("%s: term ok\n", Snd->name);
	}
}

LOCAL_FUNC int FAR
MidiInit (char *options)
{
	long	l;

	Snd->flags &= ~INITED;

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

	LogPrintf ("%s: channel %d\n",
		Snd->name, midi_channel);
	LogPrintf ("%s: program %d\n",
		Snd->name, midi_program);
	LogPrintf ("%s: volume  %d\n",
		Snd->name, midi_volume);

	if (midiOutOpen (&mout, MIDI_MAPPER, 0, 0, 0)) {
		LogPrintf ("%s: open MIDI failed\n",
			Snd->name);
		goto badret;
	}

	MidiOut (0x0ff, 0, 0);			/* system reset */
	MidiOut (0x0fa, 0, 0);			/* start */
	MidiOut (0x0c0+midi_channel,
			midi_program, 0);	/* program change */

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
#endif /* #if HAVE_MIDI */
