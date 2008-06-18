/* --------------------------------- loop.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* This is the main 'busy-loop' of the simulation.
*/

#include "plane.h"


LOCAL_FUNC int	NEAR FASTCALL simulate_world (void);
LOCAL_FUNC void	NEAR FASTCALL show_picture (void);

extern void FAR
active_loop (Ulong t)
{
	st.present += t;
	st.ObjectTimeout = st.present + OBJECT_TIMEOUT;
	st.PlayerTimeout = st.present + PLAYER_TIMEOUT;

	if (t > 1000L)			/* ignore long pauses */
		st.interval = 1000;
	else
		st.interval = (int)t;

	st.dither = Frand () % 1000;

	Tm->Interval (TMR_START, 0L);

	if (!(st.flags & SF_PAUSED)) {
		if (simulate_world ())
			die ();

		render_picture();	/* build picture into display list */
	}

	show_picture ();		/* draw display list on screen */

/* Gather stats.
*/
	st.misc[0] = (int)Tm->Interval (TMR_STOP, 10000L);
	st.misc[1] = (int)STATS_TIMEVIDEO;
	st.misc[2] = (int)STATS_TIME3D;
	st.misc[3] = (int)STATS_TIMESIM;
	st.misc[4] = (int)STATS_TIMESYNC;
	st.misc[5] = (int)STATS_TIMEHDD;

	++STATS_FRAMESCOUNT;
	STATS_TIMETOTAL += st.misc[0];

	STATS_TTIME3D += STATS_TIME3D;
	STATS_TTIMESIM += STATS_TIMESIM;
	STATS_TTIMESYNC += STATS_TIMESYNC;
	STATS_TTIMEHDD += STATS_TIMEHDD;
	STATS_TTIMEVIDEO += STATS_TIMEVIDEO;

	STATS_TIME3D = 0L;
	STATS_TIMESIM = 0L;
	STATS_TIMESYNC = 0L;
	STATS_TIMEHDD = 0L;
	STATS_TIMEVIDEO = 0L;
}

LOCAL_FUNC int NEAR FASTCALL
simulate_world (void)
/*
 * Do the global housekeeping. This routine is called repeatedly from the
 * main program just before the objects simulation.
*/
{
	if (st.quiet >= 2 && IS_PLANE (CC)) {		/* engine noise */
#if 0
		Snd->Effect (EFF_ENGINE, SND_PARMS, iabs (CC->speed/4));
		Snd->Effect (EFF_ENGINE, SND_PARMS, iabs (EE(CC)->thrust/4));
#endif
		Snd->Effect (EFF_ENGINE, SND_PARMS, iabs (EE(CC)->power/16));
	}

	if (objects_dynamics ())
		return (1);

	if (land_update (CV))
		return (1);

	return (0);
}

/* initiate the screen drawing structures.
*/
extern void FAR
screen_start (void)
{
	if (st.flags1 & SF_DBUFFERING) {
		Gr->SetVisual (st.which_buffer);
		Gr->SetActive (1-st.which_buffer);
	} else {
		if (Gr->SetVisual)
			Gr->SetVisual (0);
		if (Gr->SetActive)
			Gr->SetActive (0);
	}
}

/* empty all screen related structures, drain display lists.
*/
extern void FAR
screen_empty (void)
{
	buffers_free (0);
	buffers_free (1);
	repaint ();
}

extern void FAR
double_buffer (int mode)
{
	if (!((mode ^ st.flags1) & SF_DBUFFERING))
		return;
	if (CD->npages > 1) {
		screen_empty ();
		st.flags1 ^= SF_DBUFFERING;
		screen_start ();
		MsgPrintf (50, "%s buffering",
			st.flags1 & SF_DBUFFERING ? "double" : "single");
	} else
		MsgPrintf (50, "no double buffering");
}

extern void FAR
repaint (void)
{
	st.flags2 |= SF_CLEAR;
}

/* st.which_buffer indicates which buffer is now visible.
 * bufold is the currently active (invisible) page
 *	it is drawn,  then becomes visible (inactive)
 * bufnew is the cuurently visible (inactive) page
 *	it is erased, then becomes active (invisible)
*/

LOCAL_FUNC void NEAR FASTCALL
show_picture (void)
{
	int	bufold, bufnew;

	if ((st.flags & SF_PAUSED) && (VIS_ALTERNATING != st.stereo))
		goto ret;

	Tm->Interval (TMR_START, 0L);

	bufnew = st.which_buffer;
	st.which_buffer = (st.which_buffer+1)%NBUFS;
	bufold = st.which_buffer;

	if (st.flags1 & SF_DBUFFERING) {
		Gr->SetActive (bufold);			/* access new page */
		buffers_show (bufold);			/* draw new page */

		Tm->Interval (TMR_START, 0L);
		Gr->SetVisual (bufold);			/* show new page */
		STATS_TIMESYNC = Tm->Interval (TMR_STOP, 10000L);

		if (VIS_ALTERNATING == st.stereo)
			Gr->Shutters (bufold);

		Gr->SetActive (bufnew);			/* access old page */
		if (!(st.flags & SF_USECLEAR))
			buffers_erase (bufnew);		/* erase old page */
		buffers_free (bufnew);
	} else {
		if (!(st.flags & SF_USECLEAR))
			buffers_erase (bufnew);		/* erase old page */
		buffers_free (bufnew);
		if (VIS_ALTERNATING == st.stereo)
			Gr->Shutters (bufold);

		buffers_show (bufold);			/* show new page */

		if (Gr->SetVisual) {
			Tm->Interval (TMR_START, 0L);
			Gr->SetVisual (0);		/* show page */
			STATS_TIMESYNC = Tm->Interval (TMR_STOP, 10000L);
		}
	}
	STATS_TIMEVIDEO += (int)(Tm->Interval (TMR_STOP, 10000L)
							- STATS_TIMESYNC);
ret:
	if (Gr->Flush) {
		Tm->Interval (TMR_START, 0L);
		Gr->Flush ();
		STATS_TIMESYNC = Tm->Interval (TMR_STOP, 10000L);
	}
}

static BUFFER 	*pause_buf = NULL;

extern void FAR
pause_set (Ushort mode)
{
	int	orgx, orgy, sizex, sizey;
	BUFFER 	*save_buf[2] = {NULL, NULL};
	short	save_avail;

	if (!((mode ^ st.flags) & SF_PAUSED))
		return;

	if ((mode & SF_PAUSED) && (st.network & NET_INITED)) {
		MsgEPrintf (30, "cannot pause on net");
		return;
	}

	if (!(st.flags & SF_PAUSEMSG)) {
		st.flags = (st.flags & ~SF_PAUSED) | mode;
		return;
	}

	sim_set ();		/* we want immediate response */

/* Get 2D display area parameters.
*/
	get_area (CVIEW, &orgx, &orgy, &sizex, &sizey);

	if (mode & SF_PAUSED) {

/* save current buf
*/
		save_buf[HEAD] = st.buf[HEAD];
		save_buf[TAIL] = st.buf[TAIL];
		save_avail     = st.buf_avail;
		buffer_close ();
		st.buf[HEAD] = st.buf[TAIL] = 0;

/* create the 'Paused' buf
*/
		gr_op (T_MXOR);
		stroke_str (orgx - sizex + 2, orgy - sizey + 2 + st.StFontSize,
			"Paused", st.StFontSize, ST_IFG);

/* save it
*/
		buffer_close ();
		pause_buf = st.buf[HEAD];

/* restore original buf
*/
		st.buf[HEAD] = save_buf[HEAD];
		st.buf[TAIL] = save_buf[TAIL];
		st.buf_p     = st.buf[TAIL] ? st.buf[TAIL]->p : NULL;
		st.buf_avail = save_avail;

/* show the 'Paused' buf
*/
		if (st.flags1 & SF_DBUFFERING) {
			Gr->SetActive (st.which_buffer);
			buffer_show (pause_buf);
			Gr->SetActive (1-st.which_buffer);
		}
		buffer_show (pause_buf);

		st.flags |= SF_PAUSED;
	} else {
		st.flags &= ~SF_PAUSED;

/* erase the 'Paused' buf, then free it
*/
		if (st.flags1 & SF_DBUFFERING) {
			Gr->SetActive (st.which_buffer);
			buffer_show (pause_buf);
			Gr->SetActive (1-st.which_buffer);
		}
		buffer_show (pause_buf);
		buffer_free (pause_buf);
	}

	sim_reset ();
}
