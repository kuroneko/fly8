/* --------------------------------- gpport.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* read a JOYSTICK using Colorado Spectrum GamePort (Workstation).
*/

#include "fly.h"
#include "ustick.h"

#ifdef USE_GP

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifndef NO_TERMIOS
#include <termios.h>
#endif


static int      fjs = -1;
static Uchar    cmd[6];
static Uchar    last[6];
static int      have = 0;
static struct termios   old;

static int
initgp (int which, char *options, int opts)
{
	char    *dev;
#ifndef NO_TERMIOS
	struct termios  gp;
#endif

	if ((fjs = open (dev = which ? "/dev/gp1" : "/dev/gp0", 
				FLY8_NONBLOCK | O_RDONLY)) < 0) {
		LogPrintf ("GamePort.%d: \"%s\" open failed %d\n",
			which, dev, errno);
		return (-1);
	}

#ifndef NO_TERMIOS
	tcgetattr (fjs, &old);
	gp = old;

	gp.c_iflag &= ~(
		BRKINT |
		IGNBRK |
		IGNPAR |
		PARMRK |
		INPCK |
		ISTRIP |
		INLCR |
		IGNCR |
		IXON |
		IXOFF);

	gp.c_cflag &= ~(
		PARENB |
		CSIZE);
	gp.c_cflag |= (
		CREAD |
		CS8 |
		CSTOPB
#ifdef CRTSCTS
		| CRTSCTS
#endif
		);       /* portable? */

	gp.c_lflag &= ~(
		ECHO |
		ECHOE |
		ECHOK |
		ECHONL |
		ICANON |
		ISIG);

	cfsetispeed (&gp, B9600);
	cfsetospeed (&gp, B9600);

/* Any portable way to set DTR and CTS?
*/
	tcsetattr (fjs, TCSANOW, &gp);
#endif

	have = 0;

	last[0] = 0x000;
	last[1] = 0x000;
	last[2] = 0x080;
	last[3] = 0x080;
	last[4] = 0x080;
	last[5] = 0x080;

	return (0);
}

static int
termgp (int which, int opts)
{
	if (fjs >= 0) {
#ifndef NO_TERMIOS
		tcsetattr (fjs, TCSANOW, &old);
#endif
		close (fjs);
		fjs = -1;
	}

	return (0);
}

static Uint
readgp (int which, STICK *j, int mask, int opts)
{
	if (fjs < 0)
		return (0x0ff);

/* Process GP events.
*/
	while (1 == read (fjs, cmd+have, 1)) {
		switch (have) {
		case 0:
			if (!cmd[0])    /* wait for zero byte */
				++have;
			break;
		case 1:
			if (cmd[1])     /* wait for non zero byte */
				++have;
			break;
		case 2:
		case 3:
		case 4:
			++have;
			break;
		case 5:
			memcpy (last, cmd, sizeof (last));
		default:        /* should never happen... */
			have = 0;
			break;
		}
	}

/* Return last event data.
*/
	if (which) {
		j->a[0] = last[4];
		j->a[1] = last[5];
		j->b[0] = (char)T(last[1] & 0x40);
		j->b[1] = (char)T(last[1] & 0x80);
		j->nbuttons = 2;
	} else {
		j->a[0] = last[2];
		j->a[1] = last[3];
		j->a[2] = last[4];
		j->a[3] = last[5];
		j->b[0] = (char)T(last[1] & 0x10);
		j->b[1] = (char)T(last[1] & 0x20);
		j->b[2] = (char)T(last[1] & 0x40);
		j->b[3] = (char)T(last[1] & 0x80);
		j->nbuttons = 4;
	}

	return (0);
}
#else
static int
termgp (int which, int opts)
{return (0);}

static int
initgp (int which, char *options, int opts)
{return (-1);}

static Uint
readgp (int which, STICK *j, int mask, int opts)
{return (0x0ff);}
#endif


USTICK gpport = {
	"GamePort",
	initgp,
	termgp,
	readgp
};
