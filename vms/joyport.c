/* --------------------------------- joyport.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* read a JOYSTICK using <linux/joystick.h>.
*/

#include "fly.h"
#include "ustick.h"

#ifdef USE_JOY

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>


static int      fjs = -1;

static int
initjoy (int which, char *options, int opts)
{
	struct JS_DATA_TYPE     jsdata;
	long                    l;
	char    *dev;

	if ((fjs = open (dev = which ? "/dev/js1" : "/dev/js0",
						O_RDONLY)) < 0) {
		LogPrintf ("%s open failed %d\n", dev, errno);
		return (-1);
	}

	l = -1L;        /* get around possible joystick driver bug */
	if (ioctl (fjs, JS_SET_TIMELIMIT, &l) < 0)
		return (1);

	l = 5000L;
	if (ioctl (fjs, JS_SET_TIMEOUT, &l) < 0)
		return (2);

	jsdata.x = jsdata.y = 0;
	if (ioctl (fjs, JS_SET_CAL, &jsdata) < 0)
		return (3);
	return (0);
}

static Uint
readjoy (int which, STICK *j, int mask, int opts)
{
	struct JS_DATA_TYPE_LONG jsdata;
	int     len;

	if (fjs < 0)
		return (0x0ff);

	len = (opts & READA3) ? JS_RETURN_LONG : JS_RETURN;
	if (read (fjs, (void *)&jsdata, len) != len)
		return (1);

	j->a[0] = jsdata.x;
	j->a[1] = jsdata.y;
	j->a[2] = jsdata.x2;
	j->a[3] = jsdata.y2;
	j->b[0] = (char)T(jsdata.buttons & 0x0001);
	j->b[1] = (char)T(jsdata.buttons & 0x0002);
	j->b[2] = (char)T(jsdata.buttons & 0x0004);
	j->b[3] = (char)T(jsdata.buttons & 0x0008);
	j->nbuttons = 4;

	return (0);
}
#else
static int
initjoy (int which, char *options, int opts)
{return (-1);}

static Uint
readjoy (int which, STICK *j, int mask, int opts)
{return (0x0ff);}
#endif

static int
termjoy (int which, int opts)
{return (0);}



USTICK joyport = {
	"joystick",
	initjoy,
	termjoy,
	readjoy
};
