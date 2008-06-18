/* --------------------------------- udp.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for server based UDP level networking (low level).
 * An address here is IP address (4 bytes) + UDP port num (2 bytes).
 *
 * It needs the server fly8udp for operation, it will NOT talk directly
 * to another fly8.
*/

#include "fly.h"

#ifdef HAVE_UDP

#define PHDATA		pack->raw
#include "fly8udp.h"


typedef struct port PORT;
struct port {
	int		flags;
#define POF_ON		0x0001
	int		fd;
	struct sockaddr_in	svr, cli;
	int		netport;		/* back-pointer */
	char		*server;		/* server name */
	Uchar		address[LADDRESS];	/* my address */
};

static PORT	FAR ports[] = {
	{0, -1},
	{0, -1},
	{0, -1},
	{0, -1},
};
#define	NDEV	rangeof(ports)

static int	nports = 0;		/* number of active ports */

LOCAL_FUNC int NEAR
UdpOptions (PORT *port, char *options)
{
	char	*p;
	long	l;

	if (T(p = get_siarg (options, 0)))
		port->server = p;
	else
		return (1);

	if (get_narg (options, "port=", &l))
		l = IPPORT_FLY8;
	port->svr.sin_port = htons ((Ushort)l);

	return (0);
}

LOCAL_FUNC int FAR
UdpInit (NETPORT *np, char *options)
{
	int		portno;
	PORT		*port;
	char		*protoname;
	struct protoent	*proto;
	struct hostent	*hostptr;
	Ulong		srv_addr;		/* server address */
	int		n;

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV) {
		MsgEPrintf (-100, "%s.%c: bad port",
			np->NetDriver->name, np->unit);
		return (1);
	}
	port = &ports[portno];
	if (port->flags & POF_ON) {
		MsgEPrintf (-100, "%s.%c: already on",
			np->NetDriver->name, np->unit);
		return (1);
	}

	memset (&port->svr, 0, sizeof (port->svr));
	memset (&port->cli, 0, sizeof (port->cli));

	if (UdpOptions (port, options))
		return (1);
#ifdef MSDOS
	sock_init ();

	if ((port->fd = socket (AF_INET, SOCK_FLY8, IPPROTO_UDP)) < 0) {
#else
	protoname = "udp";
	if ((proto = getprotobyname (protoname)) == NULL) {
		MsgEPrintf (-100, "%s.%c: getprotobyname(%s) failed: %s",
			np->NetDriver->name, np->unit,
			protoname, strerror (errno));
		return (1);
	}

	if ((port->fd = socket (AF_INET, SOCK_FLY8, proto->p_proto)) < 0) {
#endif
		MsgEPrintf (-100, "%s.%c: socket() failed: %s",
			np->NetDriver->name, np->unit, strerror (errno));
		return (1);
	}

#ifndef MSDOS
	n = fcntl (port->fd, F_GETFL);
	if (fcntl (port->fd, F_SETFL, n|FLY8_NONBLOCK) < 0) {
		MsgEPrintf (-100, "%s.%c: fcntl() failed: %s",
			np->NetDriver->name, np->unit, strerror (errno));
		return (1);
	}
#endif

/* Set up server (our output) address.
*/
	if ((hostptr = gethostbyname (port->server)) == NULL) {
		MsgEPrintf (-100, "%s.%c: gethostbyname(%s) failed: %s",
			np->NetDriver->name, np->unit,
			port->server, strerror (errno));
		return (1);
	}

	if (hostptr->h_addrtype != AF_INET) {
		MsgEPrintf (-100, "%s.%c: not AF_INET address",
			np->NetDriver->name, np->unit);
		return (1);
	}

	srv_addr = ((struct in_addr *)hostptr->h_addr_list[0])->s_addr;
	MsgPrintf (-100, "%s.%c: server is %08lx",
			np->NetDriver->name, np->unit, ntohl (srv_addr));

	port->svr.sin_family      = AF_INET;
	port->svr.sin_addr.s_addr = srv_addr;

/* Set up client (our input) address.
*/
	port->cli.sin_family      = AF_INET;
	port->cli.sin_addr.s_addr = htonl (INADDR_ANY);
	port->cli.sin_port        = htons (0);

	if (bind (port->fd, (struct sockaddr *) &port->cli,
						sizeof (port->cli)) < 0) {
		MsgEPrintf (-100, "%s.%c: bind() failed: %s",
			np->NetDriver->name, np->unit, 	strerror (errno));
		return (1);
	}
	memcpy (port->address,   (char *)&port->cli.sin_addr.s_addr, 4);
	memcpy (port->address+4, (char *)&port->cli.sin_port,        2);

	port->flags |= POF_ON;
	port->netport = np->netport;
	++nports;

	return (0);
}

LOCAL_FUNC void FAR
UdpTerm (NETPORT *np)
{
	int	portno;
	PORT	*port;

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV)
		return;
	port = &ports[portno];
	if (!(port->flags & POF_ON))
		return;

	port->server = STRfree (port->server);
	if (port->fd >= 0) {
#ifdef WATTCP
		n_close (port->fd);
#else
		close (port->fd);
#endif
		port->fd = -1;
	}
	port->flags = 0;
	--nports;
}

LOCAL_FUNC int FAR
UdpSend (NETPORT *np, PACKET *pack)
{
	int	portno;
	PORT	*port;
	int	n;

	if (0 == pack)
		return (0);

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV)
		return (1);
	port = &ports[portno];
	if (!(port->flags & POF_ON))
		return (1);

	if (pack->address) {
		memcpy (PHTO, pack->address, LADDRESS);	/* to */
		pack->address = 0;			/* private */
	} else
		memset (PHTO, 0xff, LADDRESS);		/* broadcast */
	memcpy (PHFROM, port->address, LADDRESS);	/* from */
	PHLEN[0] = (Uchar)(0x0ff&(pack->length >> 8));	/* length */
	PHLEN[1] = (Uchar)(0x0ff&(pack->length));

	n = PHSIZE + pack->length;
	if (n != sendto (port->fd, PHEAD, n, 0, (struct sockaddr *)&port->svr,
							sizeof (port->svr))) {
#ifndef MSDOS
		if (EWOULDBLOCK == errno)
			return (1);	/* busy */
		else
#endif
			return (1);	/* error */
	}

	return (0);
}

static Uchar	FAR msg[1500];		/* message buffer */

LOCAL_FUNC int FAR
UdpReceive (NETPORT *np)
{
	int	portno;
	PORT	*port;
	PACKET	*pack = NULL;
	int	n, len, limit;

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV)
		return (1);
	port = &ports[portno];
	if (!(port->flags & POF_ON))
		return (1);

	for (limit = 256; limit-- > 0;) {
		len = sizeof (struct sockaddr);
		n = recvfrom (port->fd, (char *)msg, sizeof (msg), 0,
				(struct sockaddr *)&port->cli, &len);
		if (n < 0) {
#ifndef MSDOS
			if (EWOULDBLOCK != errno)
#endif
			{
				MsgEPrintf (5, "%s.%c: recvfrom() failed: %s",
					np->NetDriver->name, np->unit,
					strerror (errno));
				return (1);
			}
		} else
#ifdef MSDOS
		if (n > 0)
#endif
		{
			len = PHLEN-PHEAD;
			len = (msg[len] << 8) + msg[len+1];
			if (len < 3 || len+PHSIZE != n) {
				++STATS_NETERRD;
				continue;
			}

			if (F(pack = packet_new (len, -1))) {
				++STATS_NETERRD;
				continue;
			}
			memcpy (PHEAD, (char *)msg, n);
			pack->netport = port->netport;
			pack->length = len;
			pack->address = PHFROM;		/* from */
			packet_deliver (pack);
			pack = 0;
			continue;
		}
		break;
	}

	return (0);
}

struct NetDriver NEAR NetUdp = {
	"UDP",
	0,
	NULL,	/* extra */
	UdpInit,
	UdpTerm,
	UdpSend,
	UdpReceive
};
#endif /* ifdef HAVE_UDP */
