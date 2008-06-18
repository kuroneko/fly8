#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

void
die (char *str, int exitcode)
{
	fprintf (stderr, "\r%s failed: %s   \n", str, strerror (errno));
	if (exitcode)
		exit (exitcode);
}

int
main (int argc, char *argv[])
{
	int	fd;
	char	fnme[40];
	struct JS_DATA_TYPE js;
	struct JS_DATA_TYPE_LONG jsl;
	int	xl, xh, yl, yh, x2l, x2h, y2l, y2h;
	long	t;

	if (argc == 2) {
		sprintf (fnme,"/dev/js%d", atoi (argv[1]));
		if ((fd = open (fnme,O_RDONLY)) < 0)
			die ("open", -1);
		t = 0;
		if (ioctl (fd, JS_SET_TIMELIMIT, &t) == -1)
			die ("Joystick ioctl JS_SET_TIMELIMIT failed", 0);
		if (ioctl (fd, JS_GET_CAL, &js) == -1)
			die ("Joystick ioctl JS_SET_CAL failed", 0);
		fprintf (stdout, "cal flags=%x x=%u y=%u\n",
			js.buttons, js.x, js.y);
		xl = yl = x2l = y2l = 30000;
		xh = yh = x2h = y2h = 0;
		while (1) {
			if (read (fd, &jsl, JS_RETURN_LONG) == JS_RETURN_LONG) {
				if (jsl.buttons) {
					xl = yl = x2l = y2l = 30000;
					xh = yh = x2h = y2h = 0;
				}
				if (xl  > jsl.x)
					xl  = jsl.x;
				if (yl  > jsl.y)
					yl  = jsl.y;
				if (x2l > jsl.x2)
					x2l = jsl.x2;
				if (y2l > jsl.y2)
					y2l = jsl.y2;

				if (xh  < jsl.x)
					xh  = jsl.x;
				if (yh  < jsl.y)
					yh  = jsl.y;
				if (x2h < jsl.x2)
					x2h = jsl.x2;
				if (y2h < jsl.y2)
					y2h = jsl.y2;
				fprintf (stdout,
				"\r%x %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d%*s",
					jsl.buttons,
					xl, jsl.x, xh, yl, jsl.y, yh,
					x2l, jsl.x2, x2h, y2l, jsl.y2, y2h,
					20, "");
			} else if (read (fd, &js, JS_RETURN) == JS_RETURN) {
				if (js.buttons) {
					xl = yl = 30000;
					xh = yh = 0;
				}
				if (xl  > js.x)
					xl  = js.x;
				if (yl  > js.y)
					yl  = js.y;

				if (xh  < js.x)
					xh  = js.x;
				if (yh  < js.y)
					yh  = js.y;
				fprintf (stdout,
				"\r%x %d/%d/%d %d/%d/%d%*s",
					js.buttons,
					xl, js.x, xh, yl, js.y, yh, 15, "");
			} else
				die ("read", 0);
			fflush (stdout);
		}
		close (fd);
	} else
		fprintf (stderr,"Usage: js <0|1>\n");

	exit (0);
	return (0);
}
