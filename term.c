/* --------------------------------- term.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* shutdown code.
*/

#include "fly.h"


#define LFRAC_RES	1000L

LOCAL_FUNC char * NEAR
lfrac (long a, long b, long ldiv)
{
	int		s, f;
	long		l;
	static char	buf[20];

#if 1
	if (a < 0x7fffffffL/ldiv)
		l = a * ldiv / b;
	else if (b >= ldiv)
		l = a / (b / ldiv);
	else
		return ("999.999");			/* see LFRAC_RES */
#else
	l = a / (0x7fffffffL / ldiv);
	l += (l < 0) ? -1 : 1;
	a /= l;
	b /= l;
	l = a * ldiv / b;
	if (l > 999999)
		return ("999.999");			/* see LFRAC_RES */
#endif
	if (l) {
		if (T(s = l < 0)) {
			f = (int)((-l)%LFRAC_RES);
			l = -((-l)/LFRAC_RES);
		} else {
			f = (int)(l%LFRAC_RES);
			l = l/LFRAC_RES;
		}
		if (s && !l)
			sprintf (buf, "  -0.%03d", f);	/* see LFRAC_RES */
		else
			sprintf (buf, "%4ld.%03d", l, f); /* see LFRAC_RES */
		return (buf);
	} else
		return ("0");
}
#undef LFRAC_RES

extern void FAR
terminate (int StackUsed)
{
	Ulong		lt;
	struct netname	*nn;
	static int	depth = 0;

	if (++depth > 4)	/* must be looping, can't tell the world :-( */
		return;

	LogPrintf ("Shutdown started: %s\n", Tm->Ctime ());
	log_flush (1);

	st.flags &= ~SF_INITED;
	if (Tm) {
		lt = st.present / 1000L;
		LogPrintf ("Time   %7lu:%02lu\n", lt/60L, lt%60L);
	} else
		lt = 0;

	if (STATS_FRAMESCOUNT) {
		LogPrintf ("nFrames %9s\n", show_l (STATS_FRAMESCOUNT));
		if (T(lt))
			LogPrintf ("frm/sec  %s\n",
				lfrac (STATS_FRAMESCOUNT, lt, 1000L));
		lt = STATS_TIMETOTAL / 1000L;
		LogPrintf ("           ms/frm\n");
		LogPrintf ("Total    %s\n",
			lfrac (STATS_TIMETOTAL, STATS_FRAMESCOUNT, 100L));
		LogPrintf ("Video    %s\n",
			lfrac (STATS_TTIMEVIDEO, STATS_FRAMESCOUNT, 100L));
		LogPrintf ("3D       %s\n",
			lfrac (STATS_TTIME3D, STATS_FRAMESCOUNT, 100L));
		LogPrintf ("2D       %s\n",
			lfrac (STATS_TTIMEHDD, STATS_FRAMESCOUNT, 100L));
		LogPrintf ("Simulate %s\n",
			lfrac (STATS_TTIMESIM, STATS_FRAMESCOUNT, 100L));
		LogPrintf ("PageFlip %s\n",
			lfrac (STATS_TTIMESYNC, STATS_FRAMESCOUNT, 100L));
		LogPrintf ("Balance  %s\n",
			lfrac (STATS_TIMETOTAL - STATS_TTIMEVIDEO -
					STATS_TTIME3D -STATS_TTIMESIM -
					STATS_TTIMESYNC - STATS_TTIMEHDD,
				STATS_FRAMESCOUNT, 100L));
		if (T(lt))
			LogPrintf ("fps      %s\n",
				lfrac (STATS_FRAMESCOUNT, lt, 10000L));
	}

	stats_show ();

	nav_term ();

	remote_term ();

	if (CC)
		LogPrintf ("score    %u\n", (int)CC->score);
	CV = CC = 0;

	list_clear (&CO);

	land_term ();

	LogPrintf ("bullets  %s\n", show_l (st.nbullets));
	LogPrintf ("last obj %s\n", show_l (st.object_id));

	bodies_term ();

	buffers_term ();

	pointers_term ();

	funcs_term ();

	if (Snd) {
		Snd->Term ();
		sound_term ();
		Snd = &SndNone;
	}

	mac_term ();

	if (Kbd) {
		Kbd->Term ();
		kbrd_term ();
		Kbd = &KbdNone;
	}

	DEL0 (CW);
	DEL0 (CP);
	if (CS) {
		if (CD) {
			if (Gr) {
				if (Gr->Shutters)
					Gr->Shutters (-2);	/* turn off */
				Gr->Term (CD);
			}
			CD = 0;
		}
		DEL0 (CS);
	}

	windows_term ();

	devices_release ();
	devices_term ();

	if (st.flags & SF_SKY)
		sky_term ();

	edit_term ();

	msg_term ();

	st.iname	= STRfree (st.iname);
	st.lname	= STRfree (st.lname);
	st.mname	= STRfree (st.mname);
	st.vmdname	= STRfree (st.vmdname);
	st.navname	= STRfree (st.navname);
	st.lndname	= STRfree (st.lndname);
	st.fdir		= STRfree (st.fdir);
	st.ptrname	= STRfree (st.ptrname);
	st.grname	= STRfree (st.grname);
	st.grmname	= STRfree (st.grmname);
	st.sndname	= STRfree (st.sndname);
	st.kbdname	= STRfree (st.kbdname);
	st.nikname	= STRfree (st.nikname);
	st.teamname	= STRfree (st.teamname);
	st.homename	= STRfree (st.homename);
	st.timeropts	= STRfree (st.timeropts);
	st.ptype	= STRfree (st.ptype);
	st.dtype	= STRfree (st.dtype);
	st.initkeys	= STRfree (st.initkeys);

	for (nn = st.netnames; nn;) {
		STRfree (nn->name);
		nn = DEL (nn);
	}

	mem_stats ();

	LogPrintf ("stack used: %d bytes\n", StackUsed);
	LogPrintf ("Fly8 end: %s\n", Tm->Ctime ());

	log_term ();

	if (Tm) {
		if (Tm->Term)
			Tm->Term ();
		Tm = &TmNone;
	}

	if (Sys) {
		if (Sys->Term)
			Sys->Term ();
		Sys = &SysNone;
	}

	mem_term ();

	--depth;
}

extern void FAR
die (void)
{
	st.flags &= ~(SF_INITED|SF_INTERACTIVE);
	LogPrintf ("Aborting...\n");
	terminate (0);
	exit (1);
}
