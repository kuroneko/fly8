#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>

extern int errno;

void
die (s1, s2)
char *s1, *s2;
{
	printf ("%s %s %d %s\n", s1, s2, errno, strerror (errno));
	exit (-1);
}

void
usage ()
{
	die ("Usage: jscal <0|1>\n", "");
}

int
main (argc, argv)
int	argc;
char	*argv[];
{
	int			fd, jsn, tmp;
	long			tmpl;
	struct JS_DATA_TYPE	js;
	struct JS_DATA_TYPE_LONG	jsl;
	char			file[20];

	if (argc != 2)
		usage ();
	if ((jsn = atoi (argv[1])) != 0 && jsn != 1)
		usage ();
	sprintf (file, "/dev/js%d", jsn);
	printf ("Trying %s\n", file);
	if ((fd = open (file, O_RDONLY)) < 0)
		die ("open error", file);

	tmp = 30000;
	printf ("Setting Timeout to %d\n", tmp);
	if (ioctl (fd, JS_SET_TIMEOUT, &tmp) == -1)
		die ("Joystick ioctl failed", "");
	if (ioctl (fd, JS_GET_TIMEOUT, &tmp) == -1)
		die ("Joystick ioctl failed", "");
	printf ("Timeout value= %d\n", tmp);

	if (ioctl (fd, JS_GET_TIMELIMIT, &tmpl) == -1)
		die ("Joystick ioctl failed", "");
	printf ("Timelimit value=%ld\n", tmpl);
	tmpl = 1;
	printf ("Setting Timelimit= %ld\n", tmpl);
	if (ioctl (fd, JS_SET_TIMELIMIT, &tmpl) == -1)
		die ("Joystick ioctl failed","");

	if (ioctl (fd, JS_GET_CAL, &js) == -1)
		die ("Joystick ioctl failed", "");
	printf ("Current correction: %5u, %5u\n", js.x, js.y);
	js.x = js.y = 0;
	printf ("Setting correction to %5u, %5u\n", js.x, js.y);
	if (ioctl (fd, JS_SET_CAL, &js) == -1)
		die ("Joystick ioctl failed", "");

	printf ("Move joystick to lower right and press either button\n");
	while ((read (fd, &jsl, JS_RETURN_LONG) > 0) && jsl.buttons == 0x00)
		printf ("\r%5u %5u %5u %5u ", jsl.x, jsl.y, jsl.x2, jsl.y2);
	for (tmp = 0; jsl.x > 0xff; tmp++, jsl.x = jsl.x >> 1)
		;
	js.x = tmp;
	for (tmp = 0; jsl.y > 0xff; tmp++, jsl.y = jsl.y >> 1)
		;
	js.y = tmp;
	printf ("Setting correction: %5u, %5u\n", js.x, js.y);
	if (ioctl (fd, JS_SET_CAL, &js) == -1)
		die ("Joystick ioctl failed", "");
	if (ioctl (fd, JS_GET_CAL, &js) == -1)
		die ("Joystick ioctl failed", "");
	printf ("Verify Correction: %5u, %5u\n", js.x, js.y);

	while (1) {
		if (read (fd, &jsl, JS_RETURN_LONG) == JS_RETURN_LONG) {
			printf ("\r%x %5u %5u %5u %5u ",
				jsl.buttons, jsl.x, jsl.y, jsl.x2, jsl.y2);
			fflush (stdout);
		} else if (read (fd, &js, JS_RETURN) == JS_RETURN) {
			printf ("\r%x %5u %5u ",
				js.buttons, js.x, js.y);
			fflush (stdout);
		} else
			printf ("\rread failed %d %s  ", errno,
				strerror (errno));
	}
	(void) close (fd);
	exit (0);
	return (0);
}
