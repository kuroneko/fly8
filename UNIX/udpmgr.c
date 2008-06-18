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

#if HAVE_UDP

#include <time.h>
/*#include <sys/timeb.h>*/

#define PHEAD		msg
#include "fly8udp.h"

#if HAVE_SELECT
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#if SYS_GCC
#include <unistd.h>	/* STDIN_FILENO */
#elif SYS_MSWIN_32
#include <conio.h>
#endif



static char	admin[LADDRESS] = ADMIN_ADDR;
static char	*pname = 0;

#if SYS_WINSOCK
static WSADATA	wsaData;
#endif

static char	FAR msg[1500];		/* message buffer */
static int	UdpErrno = 0;


static char *
UdpError (void)
{
	static char	UdpMsg[256];

#if SYS_WINSOCK
	UdpErrno = WSAGetLastError ();
#else
	UdpErrno = errno;
#endif
	sprintf (UdpMsg, "errno %d: ", UdpErrno);
	strncat (UdpMsg, strerror (UdpErrno), sizeof (UdpMsg));
	return (UdpMsg);
}

static char *
print_addr (struct sockaddr_in *addr)
{
        unsigned char   *ip, *port;
        static char     str[32];

        ip  = (unsigned char *)&addr->sin_addr.s_addr;
        port = (unsigned char *)&addr->sin_port;
        sprintf (str, " %u.%u.%u.%u:0x%02x%02x", 
                ip[0], ip[1], ip[2], ip[3],
                port[0], port[1]);
        return (str);
}

#ifndef FLY8_NONBLOCK

static char	FAR inbuf[1500] = "", *pbuf = inbuf;

static int NEAR
getkbd (void)
{
#if SYS_MSDOS || (SYS_MSWIN_32 && !SYS_GNUWIN_B18)
	int	n;

	if (kbhit ()) {
		n = getche ();
		if  ('\r' == n)
			putch ('\n');
		return (n);
	}
#elif SYS_MSWIN
	MSG	msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage (&msg);
		if (WM_QUIT == msg.message)
			return (-1);
		if (WM_CHAR == msg.message) {
			putchar (msg.wParam);
			if  ('\r' == msg.wParam)
				putchar ('\n');
			return (msg.wParam);
		}
		DispatchMessage (&msg);
	}
#endif
	return (0);
}
#endif

/* returns:
 *  0 nothing to do yet.
 *  1 got a message
 * -1 error
*/
static int NEAR
getmsg (void)
{
	int	ret;

#ifdef FLY8_NONBLOCK
	if (NULL == fgets (PHDATA, sizeof(msg)-PHSIZE, stdin)) {
		if (EWOULDBLOCK == errno)
			ret = 0;
		else {
			printf ("%s: user input failed, exiting\n", 
				pname);
			ret = -1;
		}
	} else
		ret = 1;
#else
	int	n;

	while (0 != (n = getkbd ())) {
		if ('\r' == n || -1 == n)
			break;
		if (0x08 == n || 0x7f == n) {	/* delete */
			if (pbuf > inbuf)
				--pbuf;
		} else
			*pbuf++ = (char)n;
	}
	if (-1 == n)
		ret = -1;
	else if ('\r' == n) {
		*pbuf++ = '\n';
		*pbuf++ = '\0';
		strncpy (PHDATA, inbuf, sizeof(msg)-PHSIZE);
		pbuf = inbuf;
		ret = 1;
	} else
		ret = 0;
#endif
	return (ret);
}

static void
svr_msg (unsigned char *msg, int n)
{
	struct svr_stats	*p;

	switch (msg[PHSIZE]) {
	case SVM_STATS:
		p = (struct svr_stats *)msg;
		printf ("%s: stats:\n", pname);
		printf ("clients:      %ld\n", p->nclients);
		printf ("messages in:  %ld\n", p->nin);
		printf ("bytes    in:  %ld\n", p->ninb);
		printf ("messages out: %ld\n", p->nout);
		printf ("bytes    out: %ld\n", p->noutb);
		printf ("add no mem:   %ld\n", p->errs[0]);
		printf ("send failed:  %ld\n", p->errs[1]);
		printf ("recv failed:  %ld\n", p->errs[2]);
		fflush (stdout);
		break;
	case SVM_MSG:
		printf ("%s: message (%3u bytes):\n\t\"%s\"\n",
			pname, n, msg+PHSIZE+2);
		fflush (stdout);
		break;
	default:
		printf ("%s: unknown (%3u bytes): 0x%02x 0x%02x",
			pname, n, msg[PHSIZE+0], msg[PHSIZE+1]);
		fflush (stdout);
		break;
	}
}

int
main (int argc, char *argv[])
{
	SOCKET			fd;
	struct sockaddr_in	svr, cli;
	char			*protoname;
	struct protoent		*proto;
	long			nin;		/* incomming msgs counter */
	int			n;		/* incomming msg length */
	unsigned long		srv_addr;	/* server address */
	struct hostent		*hostptr;
	int			len;
	char			*servname;
	char			*p;
	int			portno = IPPORT_FLY8;
	int			serverr;
	char			*command;
	int			interactive;

	pname = argv[0];

#if SYS_MSWIN_16
	_wabout ("\
Fly8 UDP console\n\
\tby\n\
Eyal Lebedinsky\n\
\n\
eyal@eyal.emu.id.au\
");
#endif

#if SYS_WINSOCK
	if (WSAStartup (WINSOCK_VERSION, &wsaData)) {
		printf ("%s: WSAStartup failed: %s",
			pname, UdpError ());
		exit (1);
	}
	printf ("%s: winsock Description:  %s\n",
		pname, wsaData.szDescription);
	printf ("%s: winsock SystemStatus: %s\n",
		pname, wsaData.szSystemStatus);
#endif

#if SYS_WATTCP
	sock_init ();
#endif

	servname = strdup ("");
	command = NULL;
	interactive = 1;

	for (len = 0, n = 1; n < argc; ++n) {
		if ('-' != argv[n][0]) {
			switch (len) {
			case 0:
				free (servname);
				servname = strdup (argv[n]);
				break;
			default:
				fprintf (stderr, "too many arguments\n");
				exit (1);
			}
			++len;
			continue;
		}
		switch (argv[n][1]) {
		case 'e':
			interactive = 0;
		case 'c':
			if (++n >= argc) {
				fprintf (stderr, "missing %s operand\n",
					argv[n-1]);
				exit (1);
			}
			command = argv[n];
			break;
		default:
			fprintf (stderr, "bad option \"%s\"\n", argv[n]);
			exit (1);
		}
	}

#if SYS_WATTCP
	if (INVALID_SOCKET ==
		(fd = socket (AF_INET, SOCK_FLY8, IPPROTO_UDP))) {
#else
	protoname = "udp";
	if ((proto = getprotobyname (protoname)) == NULL) {
		printf ("%s: getprotobyname(%s) failed: %s\n", 
			pname, protoname, UdpError ());
		exit (1);
	}

	if (INVALID_SOCKET ==
		(fd = socket (AF_INET, SOCK_FLY8, proto->p_proto))) {
#endif
		printf ("%s: socket() failed: %s\n", pname, UdpError ());
		exit (1);
	}

#if SYS_WINSOCK
	{
		unsigned long	nonblocking = 1L;

		ioctlsocket (fd, FIONBIO, &nonblocking);
	}
#elif defined(FLY8_NONBLOCK)
	n = fcntl (fd, F_GETFL, 0);
	if (fcntl (fd, F_SETFL, n|FLY8_NONBLOCK) < 0) {
		printf ("%s: fcntl(socket) failed: %s\n",
			pname, strerror (errno));
		exit (1);
	}
#endif

/* Set up server (our output) address.
*/
	if (NULL != (p = strchr (servname, ':'))) {
		portno = atoi (p+1);
		*p = '\0';
	}
	if ('\0' == servname[0]) {
		free (servname);
		servname = strdup ("localhost");
	}

	if ((hostptr = gethostbyname (servname)) == NULL) {
		printf ("%s: gethostbyname(%s) failed: %s\n", 
			pname, servname, UdpError ());
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
	svr.sin_port        = htons (portno);

	printf ("server address is %s\n", print_addr (&svr));
	fflush (stdout);

/* Set up client (our input) address.
*/
	memset (&cli, 0, sizeof (cli));
	cli.sin_family      = AF_INET;
	cli.sin_addr.s_addr = htonl (INADDR_ANY);
	cli.sin_port        = htons (0);

	if (bind (fd, (struct sockaddr *) &cli, sizeof (cli)) < 0) {
		printf ("%s: bind() failed: %s\n", pname, UdpError ());
		exit (1);
	}

	printf ("my     address is %s\n",  print_addr (&cli));
	fflush (stdout);

	if (interactive) {
/* Set up stdin for non-blocking.
*/
#ifdef FLY8_NONBLOCK
		n = fcntl (STDIN_FILENO, F_GETFL, 0);
		if (fcntl (STDIN_FILENO, F_SETFL, n|FLY8_NONBLOCK) < 0) {
			printf ("%s: fcntl(stdin) failed: %s\n", pname,
				strerror (errno));
			exit (1);
		}
#endif
	}

	serverr = 0;
	for (nin = 0;;) {

		if (command) {
			strcpy (PHDATA, command);
			command = NULL;
			goto sendit;
		}

#if HAVE_SELECT
	{
		fd_set		readset, exceptset;
		int		maxfd;

		FD_ZERO (&readset);
		maxfd = 0;

		FD_SET (fileno (stdin), &readset);
		if (maxfd < fileno (stdin))
			maxfd = fileno (stdin);

		FD_SET (fd, &readset);
		if (maxfd < fd)
			maxfd = fd;

		exceptset = readset;

		if (select (maxfd+1, &readset, NULL, &exceptset, NULL) < 0) {
			printf ("%s: 'select' failed\n", pname);
			break;
		}
	}
#endif

/* Accept replies from server.
*/
		len = sizeof (cli);
		n = recvfrom (fd, msg, sizeof (msg), 0,
			(struct sockaddr *)&cli, (int *)&len);
#ifdef FLY8_SOCKWOULDBLOCK
		UdpError ();
		if (n < 0 && FLY8_SOCKWOULDBLOCK == UdpErrno)
			n = 0;
#endif
		if (n < 0) {
			if (!serverr) {
				printf ("%s: recvfrom error: %s\n", 
					pname, UdpError ());
				serverr = 1;
			}
		}

		if (n > 0) {
			if (serverr) {
				printf ("%s: server ok\n", 
					pname);
				serverr = 0;
			}
			svr_msg (msg, n);
		}

/* Get user input.
*/
		n = getmsg ();
		if (0 == n)
			continue;
		if (-1 == n)
			break;

/* Check for user requests. The FLY8_NONBLOCK seems to make it impossible to
 * get a good EOF on a ^D.
*/
		PHDATA[strlen(PHDATA)-1] = '\0';	/* remove NL */

		if (!strcmp (PHDATA, "help") || !strcmp (PHDATA, "h")
					|| !strcmp (PHDATA, "?")) {
			printf ("\t'shutdown' to kill Fly8 server\n");
			printf ("\t'quit' to quit this program\n");
			fflush (stdout);
			continue;
		}

		if (!strcmp (PHDATA, "end") ||
		    !strcmp (PHDATA, "exit") ||
		    !strcmp (PHDATA, "quit")) {
			printf ("%s: exit requested\n", 
				pname);
			break;
		}

/* Identify message as admin type.
*/
sendit:
		memset (msg, 0, PHSIZE);
		memcpy (PHFROM, admin, LADDRESS);
		n = strlen (PHDATA) + 1;
		PHLEN[0] = (unsigned char)(0x0ff & (len >> 8));
		PHLEN[1] = (unsigned char)(0x0ff & (len     ));

		n += PHSIZE;
		if (n != sendto (fd, msg, n, 0, (struct sockaddr *)&svr,
							sizeof (svr))) {
#ifdef FLY8_SOCKWOULDBLOCK
			UdpError ();
			if (FLY8_SOCKWOULDBLOCK == UdpErrno) {
				printf ("%s: WOULDBLOCK\n", 
					pname);
				fflush (stdout);
				continue;
			}
#endif
			printf ("%s: sendto() failed: %s\n", 
				pname, UdpError ());
			break;
		}

		printf ("%s %ld: %3u bytes sent\n", 
			pname, ++nin, n);
		fflush (stdout);

		if (!interactive)
			break;
	}

	fflush (stdout);

#if SYS_WATTCP
	n_close (fd);
#elif SYS_WINSOCK
	closesocket (fd);
#else
	close (fd);
#endif

#if SYS_WINSOCK
	WSACleanup ();
#endif

	if (interactive) {
/* Restore stdin for blocking.
*/
#ifdef FLY8_NONBLOCK
		n = fcntl (STDIN_FILENO, F_GETFL, 0);
		fcntl (STDIN_FILENO, F_SETFL, n&~FLY8_NONBLOCK);
#endif
		printf ("%s: exit completed\n", 
			pname);
	}

	fflush (stdout);

	return (0);
}

#else

#include <stdio.h>
#include <stdlib.h>


int
main (int argc, char *argv[])
{
	printf ("upd not available\n");
	return (0);
}

#endif /* if HAVE_UDP */
