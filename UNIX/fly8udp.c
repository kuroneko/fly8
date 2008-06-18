/* --------------------------------- fly8udp.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* This server is needed when using the udp driver.
*/

#include "config.h"

#if HAVE_UDP

#include <time.h>
/*#include <sys/timeb.h>*/

#define PHEAD		msg
#include "fly8udp.h"


static char	admin[LADDRESS] = ADMIN_ADDR;

#define FLY8_TTL	10		/* 10 seconds idle time allowed */

struct client {
	struct client		*next;
	struct sockaddr_in	addr;
	time_t			timeout;
};

#if SYS_WINSOCK
static WSADATA		wsaData;
#endif

static struct client	*clients = 0;	/* active clients list */
static char		*pname = 0;	/* program name */

static int		UdpErrno = 0;

/* stats
*/
static long		nclients = 0;	/* active clients count */
static long		nin = 0;
static long		ninb = 0;
static long		nout = 0;
static long		noutb = 0;
static long		errs[10] = {0};


static char *
UdpError (void)
{
#if SYS_WINSOCK
	UdpErrno = WSAGetLastError ();
#else
	UdpErrno = errno;
#endif
	return (strerror (UdpErrno));
}

static char *
my_ctime (void)
{
	time_t	tm;
	char	*t;

	tm = time (0);
	t = ctime (&tm);
	t[strlen (t) - 1] = '\0';	/* kill NewLine */
	return (t);
}

static char *
print_addr (struct sockaddr_in *addr)
{
	unsigned char	*ip, *port;
	static char	str[30];

	ip  = (unsigned char *)&addr->sin_addr.s_addr;
	port = (unsigned char *)&addr->sin_port;
	sprintf (str, " %u.%u.%u.%u:0x%02x%02x", 
		ip[0], ip[1], ip[2], ip[3],
		port[0], port[1]);
	return (str);
}

/* If this packet is from a new client then register it.
*/
static void
add_client (struct sockaddr_in *sin, int sinlen)
{
	struct client	*p;

	for (p = clients; p; p = p->next) {
		if (p->addr.sin_addr.s_addr == sin->sin_addr.s_addr &&
		    p->addr.sin_port        == sin->sin_port)
			break;
	}
	if (!p) {
		p = (struct client *)malloc (sizeof (*p));
		if (!p) {
			++errs[0];
			return;			/* no mem */
		}
		memcpy (&p->addr, sin, sinlen);
		p->next = clients;
		clients = p;
		printf ("%s: %s added %s\n",
			pname, my_ctime (), print_addr (&p->addr));
		fflush (stdout);
		++nclients;
	}
	p->timeout = time (0) + FLY8_TTL;
}

/* Echo an incomming packet to all other registered clients.
*/
static void
echo_clients (SOCKET fd, struct sockaddr_in *sin, char *msg, int len)
{
	struct client	*p, *pp, *del;
	time_t		now;
	int		private;

	
	private = len && memcmp (PHTO, "\377\377\377\377\377\377", LADDRESS);
	now = time (0);

	for (pp = 0, p = clients; p;) {
		if (p->timeout < now) {
			printf ("%s: %s timed %s\n", 
				pname, my_ctime (), print_addr (&p->addr));
			fflush (stdout);
			--nclients;
			del = p;
			p = p->next;
			if (pp)
				pp->next = p;
			else
				clients = p;
			free (del);
			continue;
		}

/* do not echo if no input, or to the packet source, or to wronf dest for
 * private packets.
*/
		if (!len ||
		    (p->addr.sin_addr.s_addr == sin->sin_addr.s_addr &&
		     p->addr.sin_port        == sin->sin_port) ||
		    (private &&
		     (memcmp (PHTO,   (char *)&p->addr.sin_addr.s_addr, 4) ||
		      memcmp (PHTO+4, (char *)&p->addr.sin_port, 2))))
			;
		else if (sendto (fd, msg, len, 0, (struct sockaddr *)&p->addr, 
						sizeof (p->addr)) != len) {
#ifdef DEBUG_FLY8UDP
			printf ("%s sendto() failed to %s\n", 
				pname, print_addr (&p->addr));
			fflush (stdout);
#endif
			++errs[1];
		} else {
#ifdef DEBUG_FLY8UDP
			printf ("%s %3u bytes echo %s\n", 
				pname, len, print_addr (&p->addr));
			fflush (stdout);
#endif
			++nout;
			noutb += len;
		}
		pp = p;
		p = pp->next;
	}
}

static int
send_msg (SOCKET fd, struct sockaddr_in *p, char *msg, int length)
{
	if (p) {
		memset (msg, ' ', PHSIZE);
		msg[PHSIZE+0] = SVM_MSG;
		msg[PHSIZE+1] = 0;
		if (sendto (fd, msg, length, 0, (struct sockaddr *)p,
						sizeof (*p)) != length)
			return (-1);
		return (0);
	}
	return (-1);
}

static void
show_stats (SOCKET fd, struct sockaddr_in *p)
{
	if (p) {
		struct svr_stats	stats;

		memset (stats.hdr, ' ', sizeof (stats.hdr));
		stats.msgid[0] = SVM_STATS;
		stats.msgid[1] = 0;
		stats.nclients = nclients;
		stats.nin      = nin;
		stats.ninb     = ninb;
		stats.nout     = nout;
		stats.noutb    = noutb;
		memcpy (stats.errs, errs, sizeof (stats.errs));

		if (sendto (fd, (char FAR *)&stats, sizeof (stats), 0,
			(struct sockaddr *)p, sizeof (*p))
						 != sizeof (stats)) {
		}
	}
#if 0
	printf ("%s: %s stats:\n", pname, my_ctime ());
	printf ("clients:      %ld\n", nclients);
	printf ("messages in:  %ld\n", nin);
	printf ("bytes    in:  %ld\n", ninb);
	printf ("messages out: %ld\n", nout);
	printf ("bytes    out: %ld\n", noutb);
	printf ("add no mem:   %ld\n", errs[0]);
	printf ("send failed:  %ld\n", errs[1]);
	printf ("recv failed:  %ld\n", errs[2]);
	fflush (stdout);
#endif
}

static char	FAR msg_badmin[] = "SSSSSSDDDDDDLLttbad admin request received";

static void
show_badmin (SOCKET fd, struct sockaddr_in *p)
{
	send_msg (fd, p, msg_badmin, sizeof (msg_badmin));
}

static char	msg_shutdown[] = "SSSSSSDDDDDDLLttshutdown accepted";

static void
show_shutdown (SOCKET fd, struct sockaddr_in *p)
{
	send_msg (fd, p, msg_shutdown, sizeof (msg_shutdown));
}

static char	FAR msg[1500];			/* message buffer */

int
main (int argc, char *argv[])
{
	SOCKET			fd;
	struct sockaddr_in	sin;
	char			*protoname;
	struct protoent		*proto;
	int			sinlen;		/* address length */
	int			n;		/* incomming message length */
	int			portno = IPPORT_FLY8;

	pname = argv[0];

	if (argc > 1)
		portno = atoi (argv[1]);

#if SYS_MSWIN_16
	_wabout ("\
Fly8 UDP server\n\
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

#if SYS_WATTCP
	if (INVALID_SOCKET ==
			(fd = socket (AF_INET, SOCK_FLY8, IPPROTO_UDP))) {
#else
	protoname = "udp";
	if ((proto = getprotobyname (protoname)) == NULL) {
		printf ("%s: %s getprotobyname(%s) failed: %s\n", 
			pname, my_ctime (), protoname, UdpError ());
		fflush (stdout);
		exit (1);
	}
	if (INVALID_SOCKET ==
			(fd = socket (AF_INET, SOCK_FLY8, proto->p_proto))) {
#endif
		printf ("%s: %s socket() failed: %s\n",
			pname, my_ctime (), UdpError ());
		fflush (stdout);
		exit (1);
	}

	memset (&sin, 0, sizeof (sin));
	sin.sin_family      = AF_INET;
	sin.sin_addr.s_addr = htonl (INADDR_ANY);
	sin.sin_port        = htons (portno);

	if (bind (fd, (struct sockaddr *) &sin,
					sizeof (struct sockaddr)) < 0) {
		printf ("%s: %s bind() failed: %s\n",
			pname, my_ctime (), UdpError ());
		fflush (stdout);
		exit (1);
	}

	printf ("%s: listenning on port 0x%x (%d)\n",
		pname, portno, portno);

	for (;;) {
		sinlen = sizeof (struct sockaddr);
		n = recvfrom (fd, msg, sizeof(msg), 0,
				(struct sockaddr *)&sin, &sinlen);
#ifdef FLY8_SOCKWOULDBLOCK
		UdpError ();
		if (n < 0 && FLY8_SOCKWOULDBLOCK == UdpErrno)
			n = 0;
		else
#endif
		if (n < 0) {
			printf ("%s: %s recvfrom() failed: %s\n", 
				pname, my_ctime (), UdpError ());
			fflush (stdout);
			++errs[2];
			continue;
		}
#if SYS_WATTCP
		if (0 == n) {
			echo_clients (fd, NULL, msg, 0);
			continue;
		}
#endif
#ifdef DEBUG_FLY8UDP
		printf ("%s %ld: %3u bytes from %s\n", 
			pname, nin, n, print_addr (&sin));
		fflush (stdout);
#endif

/* Handle administrative requests first.
*/
		if (!memcmp (PHFROM, admin, LADDRESS)) {
			echo_clients (fd, NULL, msg, 0);
			if (!strcmp ("shutdown", PHDATA)) {
				printf ("%s: %s shutdown requested\n",
					pname, my_ctime ());
				show_shutdown (fd, &sin);
				break;
			}

			if (!strcmp ("stats", PHDATA)) {
				show_stats (fd, &sin);
				continue;
			}
			show_badmin (fd, &sin);
			continue;
		}

/* This is a client packet, pass it on.
*/
		++nin;
		ninb += n;

/* Fill in the internal 'from address' in the packet since Fly8 has some
 * trouble getting it right. If it is already filled in then this msg came
 * from another server.
*/
		if (!memcmp (PHFROM, "\0\0\0\0\0\0", LADDRESS)) {
			memcpy (PHFROM, (char *)&sin.sin_addr.s_addr, 4);
			memcpy (PHFROM+4, (char *)&sin.sin_port, 2);
		}

		add_client (&sin, sinlen);
		echo_clients (fd, &sin, msg, n);
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
	show_stats (0, NULL);

	printf ("%s: %s shutdown completed\n",
		pname, my_ctime ());
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
