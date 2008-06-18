/* --------------------------------- timer.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* High resolution timer services.
 * Amiga version by Michael Taylor.
*/

#include "fly.h"
#include "amigainc.h"

#include <sys/types.h>
#include <sys/timeb.h>
#include <time.h>

static struct timerequest tr;
static struct MsgPort *tport=NULL;
/*static struct Message *msg;*/

static Ulong	seconds = 0, ticks = 0;

#define XTAL	1000000L		/* ticks resolution */

static void NEAR
timer_get (void)
{
#if 1
/* Issue the command and wait for it to finish, then get the reply
*/
	DoIO((struct IORequest *) &tr);

/* Get the results and close the timer device
*/
	ticks = tr.tr_time.tv_micro;
	seconds = tr.tr_time.tv_secs;
#else
	CurrentTime (&seconds, &ticks);
#endif
}

static Ulong FAR
timer_milli (void)
{
	timer_get ();
	return (ticks / 1000 + seconds * 1000);
}

static int FAR
timer_hires (void)		/* get fastest timer available */
{return (1);}

static int FAR
timer_init (void)
{
   LONG error;
   Ulong days,hrs,secs,mins,mics;

   /* Open the MICROHZ timer device */
   error = OpenDevice(TIMERNAME,UNIT_MICROHZ,(struct IORequest *) &tr,0);
   if(error) return 1;/* If the timer will not open then just return */

   tport=(struct MsgPort *)CreatePort(0,0);
   /* If we can't get a reply port then just quit */
   if(!tport)
        {
        CloseDevice((struct IORequest *) &tr );
        return 1;
        }

   /* Fill in the IO block with command data */
   tr.tr_node.io_Message.mn_Node.ln_Type = NT_MESSAGE;
   tr.tr_node.io_Message.mn_Node.ln_Pri  = 0;
   tr.tr_node.io_Message.mn_Node.ln_Name = NULL;
   tr.tr_node.io_Message.mn_ReplyPort    = tport;
   tr.tr_node.io_Flags			 = IOF_QUICK;
   tr.tr_node.io_Command                 = TR_GETSYSTIME;

   return (0);
}

static void FAR
timer_term (void)
{
	if (tport) {
		DeletePort(tport);
		CloseDevice((struct IORequest *) &tr);
	}
}

static char * FAR
timer_ctime (void)
{
	struct timeb	tm;

	ftime (&tm);
	return (ctime (&tm.time));
}

#define NINTS		10

static Ulong FAR
timer_interval (int mode, Ulong res)
{
	static Ulong	last_seconds[NINTS], last_ticks[NINTS];
	static int	n = -1;
	Ulong		t = 0;

	if (mode & TMR_PUSH) {
		++n;
		if (n >= NINTS) {
			LogPrintf ("timer: too many PUSHes... aborting\n");
			die ();
		}
	} else if (n < 0) {
		LogPrintf ("timer: too many POPs... aborting\n");
		die ();
	}

	if (mode & (TMR_READ|TMR_SET))
		timer_get ();

	if (mode & TMR_READ) {
		if (res > XTAL)
			res = 0;
		if (res > 0x7fffffffL/XTAL) {
			Ulong	tt;

			tt = res/(0x7fffffffL/XTAL) + 1;
			t = (seconds-last_seconds[n]) * res;
			if (ticks > last_ticks[n])
				t += ((ticks-last_ticks[n]) * (res/tt))
					/ (XTAL/tt);
			else
				t -= ((last_ticks[n]-ticks) * (res/tt))
					/ (XTAL/tt);
		} else if (res)
			t = (seconds-last_seconds[n]) * res
				+ (long)((ticks-last_ticks[n]) * res) / XTAL;
		else
			t = (seconds-last_seconds[n]) * XTAL
				+ (long)(ticks-last_ticks[n]);
	}

	if (mode & TMR_SET) {
		last_seconds[n] = seconds;
		last_ticks[n]   = ticks;
	}

	if (mode & TMR_POP)
		--n;
	return (t);
}

extern struct TmDriver TmDriver = {
	"timer.device",
	0,
	timer_init,
	timer_term,
	timer_milli,
	timer_hires,
	timer_ctime,
	timer_interval
};

#undef XTAL
