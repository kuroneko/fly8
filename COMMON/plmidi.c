/* --------------------------------- plmidi.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Sound device driver for msdos MIDI.
*/

#include "fly.h"

#if HAVE_MIDI

#include "plsound.h"
#include "notes.h"

#if SYS_DJGPP
#include <pc.h>

#define inp(p) 		inportb (p)
#define outp(p,b)	outportb ((p), (b))

#else
#include <conio.h>
#endif


#define INITED		0x8000

static int	midi_channel = 0;
static int	midi_program = 80;
static int	midi_volume = 60;
static int	midi_note = -1;
static int	notes[] = {
	4*BN_/2,
	4*CN_, 4*CSH_, 4*DN_, 4*DSH_, 4*EN_, 4*FN_,
	4*FSH_, 4*GN_, 4*GSH_, 4*AN_, 4*ASH_, 4*BN_
};

#define MPU_BASE	0x330
#define MPU		mpu_port

/* Port Addresses
*/
#define MPU_DATA	(MPU+0)
#define MPU_COMD	(MPU+1)
#define MPU_STAT	(MPU+1)

/* Status Port Bits
*/
#define NOWRITE		0x040		/* write buffer empty */
#define NOREAD		0x080		/* read buffer full */

/* MPU commands
*/
#define MPU_RESET	0x0FF		/* reset */
#define MPU_UART	0x03F		/* UART (polled) */

/* MPU access functions
*/
#define mpu_cmd(b)	outp (MPU_COMD, b)
#define mpu_read()	inp (MPU_DATA)
#define mpu_write(b)	outp (MPU_DATA, b)
#define mpu_rstat()	inp (MPU_STAT)
#define mpu_wstat()	inp (MPU_STAT)
#define input_ready()	(!(mpu_rstat () & NOREAD))
#define output_ready()	(!(mpu_wstat () & NOWRITE))

static int		mpu_port = MPU_BASE;


LOCAL_FUNC int NEAR
mpu_wait_out (void)
{
	long	timeout;

	for (timeout = 30000L; !output_ready (); timeout--) {
		if (timeout <= 0)
			return (-1);
	}
	return (0);
}

LOCAL_FUNC int NEAR
mpu_out (int c)
{
	if (mpu_wait_out ())
		return (-1);
	mpu_write (c);
	return (0);
}

LOCAL_FUNC int NEAR
mpu_wait_in (void)
{
	long	timeout;

	for (timeout = 30000L; !input_ready (); timeout--) {
		if (timeout <= 0)
			return (-1);
	}
	return (0);
}

LOCAL_FUNC int NEAR
mpu_in (void)
{
	if (mpu_wait_in ())
		return (-1);
	return (mpu_read ());
}


LOCAL_FUNC void FAR
MidiStart (int n)
{
	int	i, j;

	if (!(Snd->flags & INITED))
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

	mpu_out (0x090+midi_channel);
	mpu_out (midi_note);
	mpu_out (midi_volume);
}

LOCAL_FUNC void FAR
MidiStop (void)
{
	if (!(Snd->flags & INITED) || midi_note < 0)
		return;

	mpu_out (0x080+midi_channel);
	mpu_out (midi_note);
	mpu_out (0x07f);
	midi_note = -1;
}

LOCAL_FUNC void FAR
MidiTerm (void)
{
	if (Snd->flags & INITED) {
		mpu_out (0x0fc);		/* stop */
		mpu_out (0x0ff);	/* system reset */
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

	if (get_narg (options, "base=", &l))
		mpu_port = MPU_BASE;
	else
		mpu_port = (int)l;

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

	LogPrintf ("%s: base    0x%x\n",
		Snd->name, mpu_port);
	LogPrintf ("%s: channel %d\n",
		Snd->name, midi_channel);
	LogPrintf ("%s: program %d\n",
		Snd->name, midi_program);
	LogPrintf ("%s: volume  %d\n",
		Snd->name, midi_volume);

	mpu_cmd (MPU_RESET);
	mpu_in ();
	mpu_cmd (MPU_UART);
	mpu_in ();

	mpu_out (0x0ff);		/* system reset */
	mpu_out (0x0fa);		/* start */
	mpu_out (0x0b0);		/* Local Control on */
	mpu_out (0x07a);
	mpu_out (0x07f);
	mpu_out (0x0c0+midi_channel);	/* program change */
	mpu_out (midi_program);

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
