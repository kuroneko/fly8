/* --------------------------------- plsound.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* A generic play-list Sound device driver. You need to supply your own
 * start/stop functions.
*/

#include "fly.h"
#include "plsound.h"

#define	QSIZE	32
#define PLSE	((struct plsextra FAR *)Snd->extra)

LOCAL_FUNC int NEAR
PlsCommand (BEEP *b)
{
	int	l, ret;

again:
	for (ret = 1; ret && (l = b->list[0]) < 0;) {
		switch (l) {
		default:
		case -1:			/* END */
			ret = 0;
			break;
		case -2:			/* GOTO,address */
			b->list += 2*b->list[1];
			break;
		case -3:			/* REPEAT,address,n,cnt */
			if (b->list[3]) {
				if (--b->list[3])	/* Nth time */
					b->list += 2*b->list[1];
				else			/* done */
					b->list += 2*2;
			} else {			/* start */
				b->list[3] = b->list[2];
				b->list += 2*b->list[1];
			}
			break;
		}
	}
	if (ret) {
		b->freq = *b->list++;
		b->milli = *b->list++/4;
		if (!b->milli)
			goto again;
	}

	return (ret);
}

extern void FAR
PlsPoll (int force)
{
	int	i, diff, highest, next;
	BEEP	*b;

	if (!PLSE->beeps)
		return;

	if (!PLSE->nbeeps) {
		PLSE->lasttime = st.lasttime;
		return;
	}
	if (!force && PLSE->nexttime > st.lasttime)
		return;

	diff = (int)(st.lasttime - PLSE->lasttime);
	PLSE->lasttime = st.lasttime;
	
	highest = 0;
	next = 0x7fff;
	for (i = 0; i < QSIZE; ++i) {
		b = PLSE->beeps+i;
		if (!b->milli)
			continue;
		if (b->milli <= diff) {
			if (b->list && PlsCommand (b))
				goto use_it;
			b->milli = 0;
			--PLSE->nbeeps;
			continue;
		}
		b->milli -= diff;
use_it:
		if (b->freq > highest)
			highest = b->freq;
		if (next > b->milli)
			next = b->milli;
	}
	if (highest) {
		if (highest != PLSE->playing) {
			if (PLSE->playing)
				PLSE->Stop ();
			PLSE->playing = highest;
			PLSE->Start (PLSE->playing);
		}
	} else if (PLSE->playing) {
		PLSE->Stop ();
		PLSE->playing = 0;
	}
	PLSE->nexttime = st.lasttime + next;
}

LOCAL_FUNC int NEAR
PlsAlloc (int f, int milli, int *list)
{
	int	i;
	BEEP	*b;

	if (!PLSE->beeps)
		return (-1);

	if (f <= 0)
		f = 440;

	PlsPoll (0);
	for (i = QSIZE; --i >= 0;) {
		b = PLSE->beeps+i;
		if (b->milli)
			continue;
		b->freq = f;
		b->milli = milli;
		if (NULL != (b->list_id = list))
			b->list = list+2;
		else
			b->list = NULL;
		++PLSE->nbeeps;
		break;
	}
	if (i >= 0)
		PlsPoll (1);
	return (i);
}

extern int FAR
PlsBeep (int f, int milli)
{
	if (!PLSE->beeps)
		return (-1);
	return (PlsAlloc (f, milli, 0));
}

extern int FAR
PlsList (int *list, int command)
{
	int	i;
	BEEP	*b;

	if (!PLSE->beeps)
		return (-1);
	if (SND_OFF == command) {
		for (i = 0; i < QSIZE; ++i) {
			b = PLSE->beeps+i;
			if (!b->milli)
				continue;
			if (list && list != b->list_id)
				continue;
			b->list = b->list_id = 0;
		}
		PlsPoll (1);
		return (0);
	}
	if (*list < 0 || !(list[1]/4))
		return (-1);
	return (PlsAlloc (list[0], list[1]/4, list));
}

extern int FAR
PlsEffect (int eff, int command, ...)
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

extern int FAR
PlsInit (char *options)
{
	if (!PLSE->beeps) {
		if (F(PLSE->beeps = (BEEP *)memory_calloc (QSIZE,
			sizeof (*(PLSE->beeps)))))
				return (-1);
	}
	memset (PLSE->beeps, 0, QSIZE * sizeof (*(PLSE->beeps)));
	PLSE->playing = 0;
	PLSE->nbeeps = 0;
	PLSE->lasttime = PLSE->nexttime = st.lasttime;

	PLSE->Stop ();
	return (0);
}

extern void FAR
PlsTerm (void)
{
	PLSE->beeps = memory_cfree (PLSE->beeps, QSIZE,
						sizeof (*(PLSE->beeps)));
	PLSE->Stop ();
};

#undef QSIZE
#undef PLSE
