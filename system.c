/* --------------------------------- system.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Common system-services.
*/

#include "fly.h"

static Ushort simstack[10] = {0};
static Ushort simindex = 0;

extern void FAR
sim_set (void)
{
	simstack[simindex++] = st.flags & SF_SIMULATING;
	st.flags |= SF_SIMULATING;
}

extern void FAR
sim_reset (void)
{
	st.flags = (st.flags & ~SF_SIMULATING) | simstack[--simindex];
}

LOCAL_FUNC void NEAR
idle_loop (void)
{
	Ulong	t;

	if ((st.flags & SF_SIMULATING) || !(st.flags & SF_INITED))
		return;
	st.flags |= SF_SIMULATING;

	t = (Tm->Milli () - st.big_bang) - st.present;
	if (t >= st.frame_rate) {
		active_loop (t);		/* do one video frame */
		log_flush (0);
	}

	st.flags &= ~SF_SIMULATING;
}

extern void FAR
sys_poll (int id)
{
	Ulong		thistime;

	if (st.lasttime != (thistime = Tm->Milli () - st.big_bang)) {
		st.lasttime = thistime;
		if (Sys->Poll)
			Sys->Poll ();
		if (Snd->Poll)
			Snd->Poll (0);
		if (st.network & NET_INITED)
			netports_poll ();
		idle_loop ();
	}
}

extern void FAR
iefbr14 (void)	/* used for very short busy delays */
{}
