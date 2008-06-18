/* -------------------------------- plfm.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au)
*/

/* Sound device driver for msdos FM for SoundBlasters (PlFm)
 * Only uses 1 channel, and would work with adlib, IF
 * it didn't use Sound blaster ports....  I'll fix that
 * sometime.......
 *
 * FM code "borrowed" from the Sound Blaster Freedom Project
 * and modified by Chris Collins (ccollins@pcug.org.au) to function
 * with fly8.
 *
 * Instrument Banking also by Chris Collins (ccollins@pcug.org.au)
*/

#include "fly.h"

#include "plsound.h"
#include "notes.h"

#if SYS_DJGPP
#include <pc.h>

#define inp(p) 		inportb (p)
#define outp(p,b)	outportb ((p), (b))

#else
#include <conio.h>
#endif

#include "plfm.h"


#define INITED		0x8000

static int		SbIOaddr = 0x220;
static FM_Instrument	patches[20];
static int		cvoice = 10;


LOCAL_FUNC void NEAR
delayit (int n, int addr)
{
	while (n-- > 0)
		inp (addr);
}

LOCAL_FUNC void NEAR
WriteFM (int chip, int addr, int data)
{
	int	ChipAddr;

	ChipAddr = SbIOaddr + ((chip) ? RIGHT_FM_ADDRESS : LEFT_FM_ADDRESS);

	outp (ChipAddr, 0x0ff & addr);
	delayit (6, ChipAddr);

	outp (ChipAddr+1, 0x0ff & data);
	delayit (35, ChipAddr);
}

LOCAL_FUNC void NEAR
Sb_FM_Reset (void)
{
	WriteFM (0, 1, 0);
	WriteFM (1, 1, 0);
}

LOCAL_FUNC void NEAR
Sb_FM_Key_Off (int voice)
{
/* turn voice off 
*/
	WriteFM (0x01 & (voice / 11), 0x0b0 + voice % 11, 0);
}

LOCAL_FUNC void NEAR
Sb_FM_Key_On (int voice, int freq, int octave)
{
	int	chip;

	chip = 0x01 & (voice / 11);
	voice %= 11;

	WriteFM (chip, 0x0a0 + voice, freq);			/* F number */
	WriteFM (chip, 0x0b0 + voice, ((freq & 0x0300) >> 8)	/* F number */
				| ((octave & 0x07) << 2)	/* block */
				| 0x020);			/* key */
}

LOCAL_FUNC void NEAR
Sb_FM_Voice_Volume (int voice, int vol)
{
	WriteFM (0x01 & (voice / 11), 0x040 + voice % 11, vol);
}

LOCAL_FUNC void NEAR
Sb_FM_Set_Voice (int voice, FM_Instrument *ins)
{
	int	op_cell_num;
	int	cell_offset;
	int	chip;

	chip = 0x01 & (voice / 11);
	voice %= 11;

/* check on voice range
*/
	cell_offset = voice % 3 + ((voice / 3) << 3);

/* set sound characteristic
*/
	op_cell_num = 0x020 + cell_offset;

	WriteFM (chip, op_cell_num, ins->SoundCharacteristic[0]);
	op_cell_num += 3;
	WriteFM (chip, op_cell_num, ins->SoundCharacteristic[1]);

/* set level/output
*/
	op_cell_num = 0x040 + cell_offset;
	WriteFM (chip, op_cell_num, ins->Level[0]);
	op_cell_num += 3;
	WriteFM (chip, op_cell_num, ins->Level[1]);

/* set Attack/Decay
*/
	op_cell_num = 0x060 + cell_offset;
	WriteFM (chip, op_cell_num, ins->AttackDecay[0]);
	op_cell_num += 3;
	WriteFM (chip, op_cell_num, ins->AttackDecay[1]);

/* set Sustain/Release
*/
	op_cell_num = 0x080 + cell_offset;
	WriteFM (chip, op_cell_num, ins->SustainRelease[0]);
	op_cell_num += 3;
	WriteFM (chip, op_cell_num, ins->SustainRelease[1]);

/* set Wave Select
*/
	op_cell_num = 0x0E0 + cell_offset;
	WriteFM (chip, op_cell_num, ins->WaveSelect[0]);
	op_cell_num += 3;
	WriteFM (chip, op_cell_num, ins->WaveSelect[1]);

/* set Feedback/Selectivity
*/
	op_cell_num = 0x0C0 + voice;
	WriteFM (chip, op_cell_num, ins->Feedback[0]);
}

/* Instruments are in a HSC compatible format. 12 bytes each.
*/
LOCAL_FUNC int NEAR
FmReadInstruments (char *filename)
{
	FILE	*fin = NULL;
	int	ctr;
	int	ret = 0;

	if (NULL == (fin = fopen (filename, RBMODE))) {
		ret = -1;
		goto done;
	}

	for (ctr = 0; ctr < 20; ctr++) {
		if (1 != fread (patches[ctr].SoundCharacteristic, 2, 1, fin))
			break;
		if (1 != fread (patches[ctr].Level, 2, 1, fin))
			break;
		if (1 != fread (patches[ctr].AttackDecay, 2, 1, fin))
			break;
		if (1 != fread (patches[ctr].SustainRelease, 2, 1, fin))
			break;
		if (1 != fread (patches[ctr].Feedback, 1, 1, fin))
			break;
		if (1 != fread (patches[ctr].WaveSelect, 2, 1, fin))
			break;
		if (1 != fread (patches[ctr].filler, 1, 1, fin))
			break;
	}
	if (ctr < 20)
		ret = -2;
done:
	if (fin)
		fclose (fin);
	return (ret);
}

LOCAL_FUNC void FAR
FmStart (int n)
{
	int	octave;
	Ulong	fnum;

	if (!(Snd->flags & INITED))
		return;

	fnum = (Uint)n * (0x100000UL>>4) / (50000UL>>4);
	for (octave = 0; fnum > 0x03ff; ++octave)
		fnum >>= 1;
	Sb_FM_Key_On (0, (int)fnum, octave);
}

LOCAL_FUNC void FAR
FmStop (void)
{
	if (!(Snd->flags & INITED))
		return;
	Sb_FM_Key_Off (0);
}

LOCAL_FUNC void FAR
FmTerm (void)
{
	int	ctr;

	for (ctr = 0; ctr < 20; ctr ++)
		Sb_FM_Key_Off (ctr);

	if (Snd->flags & INITED) {
		Sb_FM_Reset();
		PlsTerm ();
		Snd->flags &= ~INITED;
		LogPrintf ("%s: term ok\n", Snd->name);
	}
}

LOCAL_FUNC int FAR
FmInit (char *options)
{
	int	volume;
	char	*ctmp = NULL;
	long	l;
	int	i;
	int	ret;

	Snd->flags &= ~INITED;

	if (get_narg (options, "base=", &l))
		SbIOaddr = 0x220;
	else
		SbIOaddr = (int)l;
	if (get_narg (options, "volume=", &l))
		volume = 100;
	else if (l > 127) {
		LogPrintf ("%s: volume too high\n",
			Snd->name);
		volume = 100;
	} else
		volume = (int)l;
	ctmp = get_sarg (options, "bank=");
	if (NULL == ctmp) {
		ctmp = STRdup ("default.f8b");
		if (NULL == ctmp) {
			ret = -3;
			goto badret;
		}
	}
		
	LogPrintf ("%s: base    0x%x\n",
		Snd->name, SbIOaddr);
	LogPrintf ("%s: volume  %d\n",
		Snd->name, volume);
	LogPrintf ("%s: bank    %s\n",
		Snd->name, ctmp);
	
	Sb_FM_Reset ();

	for (i = 0; i < 20; i++)
		Sb_FM_Voice_Volume (i, volume);

	if (T(ret = FmReadInstruments (ctmp)))
		goto badret;

	Sb_FM_Set_Voice (0, &patches[10]);
	Sb_FM_Set_Voice (0, &patches[EFF_ENGINE]);

	if (PlsInit (options)) {
		ret = -4;
		goto badret;
	}

	Snd->flags |= INITED;

	LogPrintf ("%s: init ok\n", Snd->name);
	STRfree (ctmp);
	return (0);

badret:
	if (ctmp)
		STRfree (ctmp);

	LogPrintf ("%s: init failed %d\n", Snd->name, ret);
	FmTerm ();

	return (ret);
}


static struct plsextra FAR FmExtra = {
	FmStart,
	FmStop,
	NULL,		/* beeps */
	0L,		/* lasttime */
	0L,		/* nexttime */
	0,		/* playing */
	0		/* nbeeps */
};

LOCAL_FUNC int FAR
FmEffect (int eff, int command, ...)
{
	va_list		ap;
	int		arg;
	int		ret;

	ret = -1;

	if (SND_PARMS == command) {
		va_start (ap, command);
		switch (eff) {
		case EFF_ENGINE:
			arg = va_arg (ap, int);
			TnEngine[0] = arg;
			ret = 0;
			break;
		default:
			break;
		}
		va_end (ap);
	} else if (SND_ON == command || SND_OFF == command) {
		if (eff >= 0 && eff < 20 && eff != cvoice)
			Sb_FM_Set_Voice(0, &patches[eff]);
		switch (eff) {
		case EFF_HIT:
			ret = PlsBeep (1414, 50);
			break;
		case EFF_M61_SHOOT:
			ret = PlsBeep (1000, 10);
			break;
		case EFF_MK82_EXPLODE:
			ret = PlsBeep (1414, 50);
			break;
		case EFF_MK82_SHOOT:
			ret = PlsBeep (1000, 10);
			break;
		case EFF_NO_POINTER:
			ret = PlsBeep (440*3, 500);
			break;
		case EFF_BEEP:
			ret = PlsBeep (880, 100);
			break;
		case EFF_MSG:
			ret = PlsList (TnMsg, command);
			break;
		case EFF_ENGINE:
			ret = PlsList (TnEngine, command);
			break;
		case EFF_GONE:
			ret = PlsList (TnGone, command);
			break;
		case EFF_HELLO:
			ret = PlsList (TnHello, command);
			break;
		case EFF_NOTICE:
			ret = PlsList (TnNotice, command);
			break;
		case EFF_GEAR:
			ret = PlsList (TnGear, command);
			break;
		case EFF_ALARM:
			ret = PlsList (TnAlarm, command);
			break;
		case EFF_WARN:
			ret = PlsList (TnWarn, command);
			break;
		case EFF_DAMAGE:
			ret = PlsList (TnDamage, command);
			break;
		default:
			break;
		}
	}
	return (ret);
}

struct SndDriver NEAR SndPlFm = {
	"PlFm",
	0,
	&FmExtra,
	FmInit,
	FmTerm,
	PlsPoll,
	PlsBeep,
	FmEffect,
	PlsList
};
