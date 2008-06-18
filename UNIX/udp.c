/* --------------------------------- udp.c ---------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for server based UDP level networking (low level).
 * An address here is IP address (4 bytes) + UDP port num (2 bytes).
 *
 * It needs the server 'fly8udp' for operation, it will NOT talk directly
 * to another fly8.
*/

#include "config.h"

#if HAVE_UDP

#define PHDATA		pack->raw
#include "fly8udp.h"
#include "fly.h"


typedef struct port PORT;
struct port {
	int		flags;
#define POF_ON			0x0001
#define POF_SVRERR		0x0002
#if SYS_WINSOCK
#define POF_WINSOCK_SETUP	0x0100
#endif
	SOCKET		fd;
	struct sockaddr_in	svr, cli;
	int		netport;		/* back-pointer */
	char		*server;		/* server name */
	Uchar		address[LADDRESS];	/* my address */
#if SYS_WINSOCK
	WSADATA		wsaData;
#endif
};

static PORT	FAR ports[] = {
	{0, INVALID_SOCKET},
	{0, INVALID_SOCKET},
	{0, INVALID_SOCKET},
	{0, INVALID_SOCKET},
};
#define	NDEV	rangeof (ports)

static int	UdpErrno = 0;
static int	nports = 0;		/* number of active ports */

LOCAL_FUNC int NEAR
UdpOptions (PORT *port, char *options)
{
	char	*p;
	long	l;

	if (T(p = get_sarg (options, "server=")))
		port->server = p;
	else
		port->server = STRdup ("localhost");

	if (get_narg (options, "port=", &l))
		l = IPPORT_FLY8;
	port->svr.sin_port = htons ((Ushort)l);

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

	if (INVALID_SOCKET != port->fd) {
#if SYS_WATTCP
		n_close (port->fd);
#elif SYS_WINSOCK
		closesocket (port->fd);
#else
		close (port->fd);
#endif
		port->fd = INVALID_SOCKET;
	}

#if SYS_WINSOCK
	if (port->flags & POF_WINSOCK_SETUP) {
		WSACleanup ();
		port->flags &= ~POF_WINSOCK_SETUP;
	}
#endif
	if (port->server)
		port->server = STRfree (port->server);

	if (!(port->flags & POF_ON))
		return;

	port->flags &= ~POF_ON;
	--nports;

	LogPrintf ("%s.%c: term ok\n",
		np->NetDriver->name, np->unit);
}

LOCAL_FUNC char * NEAR
UdpError (void)
{
#if SYS_WINSOCK
	UdpErrno = WSAGetLastError ();
#else
	UdpErrno = errno;
#endif
	return (strerror (UdpErrno));
}

LOCAL_FUNC int FAR
UdpInit (NETPORT *np, char *options)
{
	int		portno;
	PORT		*port;
	char		*protoname;
	struct protoent	*proto;
	struct hostent	*hostptr;
	Uchar		*srv;		/* server address */

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

	port->fd = INVALID_SOCKET;
	port->server = NULL;
	memset (&port->svr, 0, sizeof (port->svr));
	memset (&port->cli, 0, sizeof (port->cli));

	if (UdpOptions (port, options))
		goto badret;

#if SYS_WINSOCK
	if (WSAStartup (WINSOCK_VERSION, &port->wsaData)) {
		MsgEPrintf (-100, "%s.%c: WSAStartup failed: %s",
			np->NetDriver->name, np->unit,
			UdpError ());
		goto badret;
	}
	LogPrintf ("%s.%c: winsock Description:  %s\n",
		np->NetDriver->name, np->unit,
		port->wsaData.szDescription);
	LogPrintf ("%s.%c: winsock SystemStatus: %s\n",
		np->NetDriver->name, np->unit,
		port->wsaData.szSystemStatus);

	port->flags |= POF_WINSOCK_SETUP;
#endif

#if SYS_WATTCP
	sock_init ();
#endif

#if SYS_WATTCP
	if (INVALID_SOCKET ==
		(port->fd = socket (AF_INET, SOCK_FLY8, IPPROTO_UDP))) {
#else
	protoname = "udp";
	if (F(proto = getprotobyname (protoname))) {
		MsgEPrintf (-100, "%s.%c: getprotobyname(\"%s\") failed: %s",
			np->NetDriver->name, np->unit,
			protoname, UdpError ());
		goto badret;
	}

	if (INVALID_SOCKET ==
		(port->fd = socket (AF_INET, SOCK_FLY8, proto->p_proto))) {
#endif
		MsgEPrintf (-100, "%s.%c: socket() failed: %s",
			np->NetDriver->name, np->unit, UdpError ());
		goto badret;
	}

#if SYS_WINSOCK
	{
		Ulong	nonblocking = 1L;

		ioctlsocket (port->fd, FIONBIO, &nonblocking);
	}
#elif defined(FLY8_NONBLOCK)
	{
		int	n;

		n = fcntl (port->fd, F_GETFL, 0);
		if (fcntl (port->fd, F_SETFL, n|FLY8_NONBLOCK) < 0) {
			MsgEPrintf (-100, "%s.%c: fcntl() failed: %s",
				np->NetDriver->name, np->unit,
				strerror (errno));
			goto badret;
		}
	}
#endif

/* Set up server (our output) address.
*/
	if (F(hostptr = gethostbyname (port->server))) {
		MsgEPrintf (-100, "%s.%c: gethostbyname(%s) failed: %s",
			np->NetDriver->name, np->unit,
			port->server, UdpError ());
		goto badret;
	}

	if (hostptr->h_addrtype != AF_INET) {
		MsgEPrintf (-100, "%s.%c: not AF_INET address",
			np->NetDriver->name, np->unit);
		goto badret;
	}

	srv = (Uchar *)&((struct in_addr *)hostptr->h_addr_list[0])->s_addr;
	MsgPrintf (-100, "%s.%c: server is %u.%u.%u.%u",
			np->NetDriver->name, np->unit,
			srv[0], srv[1], srv[2], srv[3]);

	port->svr.sin_family      = AF_INET;
	port->svr.sin_addr.s_addr =
			((struct in_addr *)hostptr->h_addr_list[0])->s_addr;

/* Set up client (our input) address.
*/
	port->cli.sin_family      = AF_INET;
	port->cli.sin_addr.s_addr = htonl (INADDR_ANY);
	port->cli.sin_port        = htons (0);

	if (bind (port->fd, (struct sockaddr *) &port->cli,
						sizeof (port->cli)) < 0) {
		MsgEPrintf (-100, "%s.%c: bind() failed: %s",
			np->NetDriver->name, np->unit, 	UdpError ());
		goto badret;
	}
	memcpy (port->address,   (char *)&port->cli.sin_addr.s_addr, 4);
	memcpy (port->address+4, (char *)&port->cli.sin_port,        2);

	port->flags |= POF_ON;
	port->netport = np->netport;
	++nports;

	LogPrintf ("%s.%c: init ok\n",
		np->NetDriver->name, np->unit);

	return (0);

badret:
	UdpTerm (np);
	return (1);
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
	if (n != sendto (port->fd, (char *)PHEAD, n, 0,
			(struct sockaddr *)&port->svr, sizeof (port->svr))) {
#ifdef FLY8_SOCKWOULDBLOCK
		UdpError ();
		if (FLY8_SOCKWOULDBLOCK == UdpErrno)
			return (1);	/* busy */
		else
#endif
			return (1);	/* error */
	}

	return (0);
}

static Uchar	FAR msg[1500];		/* message buffer */

LOCAL_FUNC int FAR
UdpReceive (NETPORT *np, int poll)
{
	int	portno;
	PORT	*port;
	PACKET	*pack = NULL;
	int	n, len, limit, ret;

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV)
		return (1);
	port = &ports[portno];
	if (!(port->flags & POF_ON))
		return (1);

	for (ret = 0, limit = 256; limit-- > 0;) {
		len = sizeof (struct sockaddr);
		n = recvfrom (port->fd, (char *)msg, sizeof (msg), 0,
				(struct sockaddr *)&port->cli, &len);
		if (n < 0) {
#ifdef FLY8_SOCKWOULDBLOCK
			UdpError ();
			if (FLY8_SOCKWOULDBLOCK == UdpErrno)
				break;
#endif
#if 0 && !SYS_WINSOCK	/* testing */
			if (!(port->flags & POF_SVRERR)) {
				MsgEPrintf (-50, 
					"%s.%c: server not responding: %s",
					np->NetDriver->name, np->unit,
					UdpError ());
				port->flags |=  POF_SVRERR;
				++STATS_NETERRD;
			}
#endif
			ret = 1;
			break;		/* recv error */
		}

		if (0 == n)
			break;		/* no messages */

		if (port->flags & POF_SVRERR) {
			MsgEPrintf (-50, 
				"%s.%c: server ok",
				np->NetDriver->name, np->unit);
			port->flags &=  ~POF_SVRERR;
		}

		len = PHLEN-PHEAD;
		len = (msg[len] << 8) + msg[len+1];
		if (len < 3 || len+PHSIZE != n) {
			++STATS_NETERRD;
			continue;
		}

		if (F(pack = packet_new ((short)len, (short)PHSIZE))) {
			++STATS_NETERRD;
			continue;
		}
		memcpy (PHEAD, (char *)msg, n);
		pack->netport = (short)port->netport;
		pack->address = PHFROM;		/* from */
		packet_deliver (pack);
		pack = 0;
	}

	return (ret);
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
#endif /* if HAVE_UDP */
