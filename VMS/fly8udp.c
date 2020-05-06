/* --------------------------------- fly8udp.c ------------------------------ */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* This server is needed when using the udp driver.
*/

#include "config.h"

#ifdef HAVE_UDP

#include <stdio.h>
#include <time.h>
#include <sys/timeb.h>
#define PHEAD		msg
#include "fly8udp.h"


static char	admin[LADDRESS] = ADMIN_ADDR;

#define FLY8_TTL	10		/* 10 seconds idle time allowed */

struct client {
	struct client		*next;
	struct sockaddr_in	addr;
	time_t			timeout;
};

static struct client	*clients = 0;	/* active clients list */
static int		nclients = 0;	/* active clients count */
static char		*pname = 0;	/* program name */

static long		nin = 0, ninb = 0, nout = 0, noutb = 0;
static long		errs[10] = {0};

extern char *
my_ctime (void)
{
	time_t	tm;
	char	*t;

	tm = time (0);
	t = ctime (&tm);
	t[strlen (t) - 1] = '\0';	/* kill NewLine */
	return (t);
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
		printf ("%s: %s added %08lx:%04x\n", 
			pname, my_ctime (), ntohl (p->addr.sin_addr.s_addr),
			ntohs (p->addr.sin_port));
		fflush (stdout);
		++nclients;
	}
	p->timeout = time (0) + FLY8_TTL;
}

/* Echo an incomming packet to all other registered clients.
*/
static void
echo_clients (int fd, struct sockaddr_in *sin, char *msg, int len)
{
	struct client	*p, *pp, *del;
	time_t		now;
	int		private;

	
	private = len && memcmp (PHTO, "\377\377\377\377\377\377", LADDRESS);
	now = time (0);

	for (pp = 0, p = clients; p;) {
		if (p->timeout < now) {
			printf ("%s: %s timed %08lx:%04x\n", 
				pname, my_ctime (),
				ntohl (p->addr.sin_addr.s_addr),
				ntohs (p->addr.sin_port));
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
#ifdef FLY8_DEBUG
			printf ("%s sendto() failed to %08lx:%04x\n", 
				pname, ntohl (p->addr.sin_addr.s_addr),
				ntohs (p->addr.sin_port));
			fflush (stdout);
#endif
			++errs[1];
		} else {
#ifdef FLY8_DEBUG
			printf ("%s %3u bytes echo %08lx:%04x\n", 
				pname, len, ntohl (p->addr.sin_addr.s_addr),
				ntohs (p->addr.sin_port));
			fflush (stdout);
#endif
			++nout;
			noutb += len;
		}
		pp = p;
		p = pp->next;
	}
}

struct svr_stats {
	int	msgid;
#define	SVM_STATS	0x0001
	int	nclients;
	long	nin;
	long	ninb;
	long	nout;
	long	noutb;
	long	ernomem;
	long	ersend;
};

static char msg1[] = "SSSSSSDDDDDDLLstats received";

static void
show_stats (int fd, struct sockaddr_in *p)
{
	if (p) {
#if 0
		struct svr_stats	stats;

		if (sendto (fd, &svr_stats, sizeof(svr_stats), 0, 
			(struct sockaddr *)&p->addr, sizeof (p->addr)) != len){
#else
		if (sendto (fd, msg1, sizeof(msg1), 0, 
			(struct sockaddr *)p, sizeof (*p)) != sizeof(msg1)){
#endif
		}
	}

	printf ("%s: %s stats:\n", pname, my_ctime ());
	printf ("clients:      %d\n",  nclients);
	printf ("messages in:  %ld\n", nin);
	printf ("bytes    in:  %ld\n", ninb);
	printf ("messages out: %ld\n", nout);
	printf ("bytes    out: %ld\n", noutb);
	printf ("add no mem:   %ld\n", errs[0]);
	printf ("send failed:  %ld\n", errs[1]);
	printf ("recv failed:  %ld\n", errs[2]);
	fflush (stdout);
}

static char msg2[] = "bad admin request received";

static void
show_badmin (int fd, struct sockaddr_in *p)
{
	if (p) {
		if (sendto (fd, msg2, sizeof(msg2), 0, 
			(struct sockaddr *)p, sizeof (*p)) != sizeof(msg1)){
		}
	}
}

static char	FAR msg[1500];			/* message buffer */

int
main (int argc, char *argv[])
{
	int			fd;
	struct sockaddr_in	sin;
	char			*protoname;
	struct protoent		*proto;
	int			sinlen;		/* address length */
	int			n;		/* incomming message length */

	pname = argv[0];

#ifdef WATTCP
	sock_init ();

	if ((fd = socket (AF_INET, SOCK_FLY8, IPPROTO_UDP)) < 0) {
#else
	protoname = "udp";
	if ((proto = getprotobyname (protoname)) == NULL) {
		printf ("%s: %s getprotobyname(%s) failed: %s\n", 
			pname, my_ctime (), protoname, strerror (errno));
		fflush (stdout);
		exit (1);
	}
	if ((fd = socket (AF_INET, SOCK_FLY8, proto->p_proto)) < 0) {
#endif
		printf ("%s: %s socket() failed: %s\n",
			pname, my_ctime (), strerror (errno));
		fflush (stdout);
		exit (1);
	}

	memset (&sin, 0, sizeof (sin));
	sin.sin_family      = AF_INET;
	sin.sin_addr.s_addr = htonl (INADDR_ANY);
	sin.sin_port        = htons (IPPORT_FLY8);

	if (bind (fd, (struct sockaddr *) &sin, sizeof (struct sockaddr)) < 0) {
		printf ("%s: %s bind() failed: %s\n",
			pname, my_ctime (), strerror (errno));
		fflush (stdout);
		exit (1);
	}

	for (;;) {
		sinlen = sizeof (struct sockaddr);
		if ((n = recvfrom (fd, msg, sizeof(msg), 0,
				(struct sockaddr *)&sin, &sinlen)) < 0) {
			printf ("%s: %s recvfrom() failed: %s\n", 
				pname, my_ctime (), strerror (errno));
			fflush (stdout);
			++errs[2];
			continue;
		}
#ifdef WATTCP
		if (0 == n) {
			echo_clients (fd, NULL, msg, 0);
			continue;
		}
#endif
#ifdef FLY8_DEBUG
		printf ("%s %ld: %3u bytes from %08lx:%04x\n", 
			pname, nin, n, ntohl (sin.sin_addr.s_addr),
			ntohs (sin.sin_port));
		fflush (stdout);
#endif

/* Handle administrative requests first.
*/
		if (!memcmp (PHFROM, admin, LADDRESS)) {
			echo_clients (fd, NULL, msg, 0);
			if (!strcmp ("shutdown\n", PHDATA)) {
				printf ("%s: %s shutdown requested\n",
					pname, my_ctime ());
				break;
			}

			if (!strcmp ("stats\n", PHDATA)) {
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

#ifdef WATTCP
	n_close (fd);
#else
	close (fd);
#endif

	show_stats (0, NULL);

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
