/* --------------------------------- gameport.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* read a JOYSTICK using <linux/game.h>.
*/

#include "fly.h"
#include "ustick.h"

#ifdef USE_GAME

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/game.h>


static int      fjs = -1;


static int
initgame (int which, char *options, int opts)
{
	long    l;
	int     i;
	int     ret;
	char    *dev;

	if ((fjs = open (dev = which ? "/dev/js1" : "/dev/js0",
						O_RDONLY)) < 0) {
		LogPrintf ("game.%d: \"%s\" open failed %d\n",
			which, dev, errno);
		return (-1);
	}

#if 0           /* don't do it - game.c has a bug here... */
	i = 0;
	if ((ret = ioctl (fjs, GAMEIOSCAL, &i)) < 0) {
		LogPrintf ("game.%d: 'cal' set failed %d\n", which, ret);
		return (ret);
	}
#endif
	l = 0L;
	if ((ret = ioctl (fjs, GAMEIOSTMLIM, &l)) < 0) {
		LogPrintf ("game.%d: 'timelimit' set failed %d\n", which, ret);
		return (ret);
	}
	l = which ? GAME_MSTICK : GAME_MRAW;
	if ((ret = ioctl (fjs, GAMEIOSMODE, &l)) < 0) {
		LogPrintf ("game.%d: 'mode' set failed %d\n", which, ret);
		return (ret);
	}
	return (0);
}

static Uint
readgame (int which, STICK *j, int mask, int opts)
{
	if (fjs < 0)
		return (0x0ff);

	if (which) {
		struct joystick jsdata;

		if (read (fjs, (void *)&jsdata, sizeof(jsdata)) != sizeof(jsdata))
			return (0xff);
		j->a[0] = jsdata.x;
		j->a[1] = jsdata.y;
		j->b[0] = (char)T(jsdata.buttons & GAME_BT_A);
		j->b[1] = (char)T(jsdata.buttons & GAME_BT_B);
		j->nbuttons = 2;
	} else {
		struct game     jsdata;

		if (read (fjs, (void *)&jsdata, sizeof(jsdata)) != sizeof(jsdata))
			return (0xff);
		j->a[0] = jsdata.paddle_a;
		j->a[1] = jsdata.paddle_b;
		j->a[2] = jsdata.paddle_c;
		j->a[3] = jsdata.paddle_d;
		j->b[0] = (char)T(jsdata.buttons & GAME_RBT_A);
		j->b[1] = (char)T(jsdata.buttons & GAME_RBT_B);
		j->b[2] = (char)T(jsdata.buttons & GAME_RBT_C);
		j->b[3] = (char)T(jsdata.buttons & GAME_RBT_D);
		j->nbuttons = 4;
	}


	return (0);
}
#else
static int
initgame (int which, char *options, int opts)
{return (-1);}

static Uint
readgame (int which, STICK *j, int mask, int opts)
{return (0x0ff);}
#endif

static int
termgame (int which, int opts)
{return(0);}


USTICK gameport = {
	"game",
	initgame,
	termgame,
	readgame
};
