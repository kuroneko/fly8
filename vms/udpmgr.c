/* --------------------------------- udpmgr.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* This client talks to the fly8udp server and can be used to shut it down
 * with the 'shutdown' request.
 * You can request 'stats' from the server too.
 * To quit use 'end'.
*/

#include "config.h"

#ifdef HAVE_UDP

#include <stdio.h>
#define PHEAD		msg
#include "fly8udp.h"


static char	admin[LADDRESS] = ADMIN_ADDR;
static char	*pname = 0;

#ifdef MSDOS
static char	FAR inbuf[1500] = "", *pbuf = inbuf;
#endif

static char	FAR msg[1500];			/* message buffer */

int
main (int argc, char *argv[])
{
	int			fd;
	struct sockaddr_in	svr, cli;
	char			*protoname;
	struct protoent		*proto;
	long			nin;		/* incomming msgs counter */
	int			n;		/* incomming msg length */
	unsigned long		srv_addr;	/* server address */
	struct hostent		*hostptr;
	int			len;
	char			*servname;

	pname = argv[0];

	if (argc < 2)
		servname = "localhost";
	else
		servname = argv[1];

#ifdef WATTCP
	sock_init ();

	if ((fd = socket (AF_INET, SOCK_FLY8, IPPROTO_UDP)) < 0) {
#else
	protoname = "udp";
	if ((proto = getprotobyname (protoname)) == NULL) {
		printf ("%s: getprotobyname(%s) failed: %s\n", 
			pname, protoname, strerror (errno));
		exit (1);
	}

	if ((fd = socket (AF_INET, SOCK_FLY8, proto->p_proto)) < 0) {
#endif
		printf ("%s: socket() failed: %s\n", pname, strerror (errno));
		exit (1);
	}

#ifndef MSDOS
	n = fcntl (fd, F_GETFL);
	if (fcntl (fd, F_SETFL, n|FLY8_NONBLOCK) < 0) {
		printf ("%s: fcntl(socket) failed: %s\n",
			pname, strerror (errno));
		exit (1);
	}
#endif

/* Set up server (our output) address.
*/
	if ((hostptr = gethostbyname (servname)) == NULL) {
		printf ("%s: gethostbyname(%s) failed: %s\n", 
			pname, servname, strerror (errno));
		exit (1);
	}

	if (hostptr->h_addrtype != AF_INET) {
		printf ("%s: not AF_INET address\n", 
			pname);
		exit (1);
	}

	srv_addr = ((struct in_addr *)hostptr->h_addr_list[0])->s_addr;

	memset (&svr, 0, sizeof (svr));
	svr.sin_family      = AF_INET;
	svr.sin_addr.s_addr = srv_addr;
	svr.sin_port        = htons (IPPORT_FLY8);

	printf ("server address is %08lx:%04x\n", 
		ntohl (svr.sin_addr.s_addr), ntohs (svr.sin_port));
	fflush (stdout);

/* Set up client (our input) address.
*/
	memset (&cli, 0, sizeof (cli));
	cli.sin_family      = AF_INET;
	cli.sin_addr.s_addr = htonl (INADDR_ANY);
	cli.sin_port        = htons (0);

	if (bind (fd, (struct sockaddr *) &cli, sizeof (cli)) < 0) {
		printf ("%s: bind() failed: %s\n", pname, strerror (errno));
		exit (1);
	}

	printf ("my     address is %08lx:%04x\n", 
		ntohl (cli.sin_addr.s_addr), ntohs (cli.sin_port));
	fflush (stdout);

/* Set up stdin for non-blocking.
*/
#ifndef MSDOS
	n = fcntl (STDIN_FILENO, F_GETFL);
	if (fcntl (STDIN_FILENO, F_SETFL, n|FLY8_NONBLOCK) < 0) {
		printf ("%s: fcntl(stdin) failed: %s\n", pname, strerror (errno));
		exit (1);
	}
#endif

	for (nin = 0;;) {

/* Accept replies from server.
*/
		len = sizeof (cli);
		n = recvfrom (fd, msg, sizeof (msg), 0,
			(struct sockaddr *)&cli, (int *)&len);
		if (n < 0) {
#ifndef WATTCP
			if (EWOULDBLOCK != errno) {
				printf ("%s: recvfrom() failed: %s\n", 
					pname, strerror (errno));
				break;
			}
#endif
#ifdef WATTCP
		} else if (n > 0) {
#else
		} else {
#endif
			printf ("%s %3u bytes from server: \"%s\"\n",
				pname, n, msg+PHSIZE);
			fflush (stdout);
		}

/* Get user input.
*/
#ifdef MSDOS
		n = 0;
		while (kbhit ()) {
			n = getche ();
			if ('\r' == n)
				break;
			*pbuf++ = (char)n;
		}
		if ('\r' != n)
			continue;
		putch ('\n');
		*pbuf++ = '\n';
		*pbuf++ = '\0';
		strncpy (PHDATA, inbuf, sizeof(msg)-PHSIZE);
		pbuf = inbuf;
#else
		if (NULL == fgets (PHDATA, sizeof(msg)-PHSIZE, stdin)) {
			if (EWOULDBLOCK == errno)
				continue;
			break;
		}
#endif

/* Check for quit message. The FLY8_NONBLOCK seems to make it impossible to
 * get a good EOF on a ^D.
*/
		if (!strcmp (PHDATA, "end\n") ||
		    !strcmp (PHDATA, "exit\n") ||
		    !strcmp (PHDATA, "quit\n"))
			break;

/* Identify message as admin type.
*/
		memset (msg, 0, PHSIZE);
		memcpy (PHFROM, admin, LADDRESS);
		n = strlen (PHDATA) + 1;
		PHLEN[0] = (unsigned char)(0x0ff & (len >> 8));
		PHLEN[1] = (unsigned char)(0x0ff & (len     ));

		n += PHSIZE;
		if (n != sendto (fd, msg, n, 0, (struct sockaddr *)&svr,
							sizeof (svr))) {
#ifndef WATTCP
			if (EWOULDBLOCK == errno) {
				printf ("%s: WOULDBLOCK\n", 
					pname);
				fflush (stdout);
				continue;
			}
#endif
			printf ("%s: sendto() failed: %s\n", 
				pname, strerror (errno));
			break;
		}
		printf ("%s %ld: %3u bytes sent\n", 
			pname, ++nin, n);
		fflush (stdout);
	}

	fflush (stdout);

#ifdef WATTCP
	n_close (fd);
#else
	close (fd);
#endif

/* Restore stdin for blocking.
*/
#ifndef MSDOS
	n = fcntl (STDIN_FILENO, F_GETFL);
	fcntl (STDIN_FILENO, F_SETFL, n&~FLY8_NONBLOCK);
#endif

	exit (0);
	return (0);
}

#else

int
main (int argc, char *argv[])
{
	printf ("upd not available\n");
	exit (0);
}

#endif
