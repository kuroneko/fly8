/* --------------------------------- stream.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for FIFO level comms. It will handle both FIFO (named pipes) as
 * well as tty type serial comms.
 *
 * Options:
 *  0 stream name
*/

#include "fly.h"

#if HAVE_FIFO

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define MAGIC1	((Uchar)0x5a)
#define MAGIC2	((Uchar)0xa5)
#define	COMHDSZ	4		/* header size */

typedef struct port PORT;

struct port {
	int	flags;
	char	*iname;
	char	*oname;
	int	ifd;
	int	ofd;
	long	last_in;
	int	panic;
	PACKET	*incoming;
	PACKET	*outgoing;
	PACKET	*tail;
	char	head[COMHDSZ];
	short	netport;
	Uchar	address[LADDRESS];
};

static PORT	ports[] = {
	{0, 0, 0, -1, -1},
	{0, 0, 0, -1, -1},
	{0, 0, 0, -1, -1},
	{0, 0, 0, -1, -1}
};
#define	NPORTS	rangeof(ports)

static int	nports = 0;		/* active com ports */

LOCAL_FUNC int NEAR
com_options (PORT *port, char *options)
{
	if (F(port->iname = get_sarg (options, "if=")))
		return (1);

	if (F(port->oname = get_sarg (options, "of="))) {
		port->iname = STRfree (port->iname);
		return (1);
	}

	return (0);
}

LOCAL_FUNC int FAR
com_oopen (PORT *port)
{
	if (port->ofd >= 0)
		return (0);
	if ((port->ofd = open (port->oname, O_WRONLY|O_NONBLOCK)) < 0)
		return (1);

	return (0);
}

LOCAL_FUNC int FAR
com_init (NETPORT *np, char *options)	/* init a physical com port */
{
	int	portno;
	PORT	*port;
	Uchar	unit[2];

	portno = np->unit-'1';
	if (portno < 0 || portno >= NPORTS) {
		MsgEPrintf (-100, "%s: bad port %c",
			np->NetDriver->name, np->unit);
		return (1);
	}
	port = &ports[portno];
	if (port->flags)
		return (0);		/* already inited */

	memset  (port->address, 0, LADDRESS);
	strncpy ((char *)port->address, np->NetDriver->name, LADDRESS);
	strncat ((char *)port->address, ".", LADDRESS);
	unit[0] = (Uchar)np->unit;
	unit[1] = '\0';
	strncat ((char *)port->address, (char *)unit, LADDRESS);

	if (com_options (port, options))
		return (1);

#ifdef SIGPIPE
	signal (SIGPIPE, SIG_IGN);
#endif

	port->ifd = open (port->iname, O_RDONLY|O_NONBLOCK);
	if (port->ifd < 0) {
		MsgEPrintf (-100, "%s: %s input open failed", 
			port->address, port->iname);
		port->iname = STRfree (port->iname);
		return (1);
	}

	port->flags = 1;
	port->netport = np->netport;
	++nports;

	return (0);
}

LOCAL_FUNC void FAR
com_term (NETPORT *np)			/* term a physical port */
{
	PACKET	*p;
	int	portno;
	PORT	*port;

	portno = np->unit-'1';
	if (portno < 0 || portno >= NPORTS)
		return;
	port = &ports[portno];
	if (!port->flags)
		return;

	if (port->ifd >= 0) {
		close (port->ifd);
		port->ifd = -1;
	}
	port->iname = STRfree (port->iname);

	if (port->ofd >= 0) {
		close (port->ofd);
		port->ofd = -1;
	}
	port->oname = STRfree (port->oname);

	port->flags = 0;

	for (p = port->outgoing; p;)
		p = packet_del (p);
	port->outgoing = 0;
	port->tail = 0;

	if (port->incoming) {
		packet_del (port->incoming);
		port->incoming = 0;
	}
	--nports;
}

LOCAL_FUNC int  FAR
com_out (PORT *port)
{
	PACKET	*pack;
	char	ch[1];
	int	to;

	if (com_oopen (port))
		return (1);

	to = 0;
	while (T(pack = port->outgoing)) {		/* try a few times */
		if (++to > 10) {
			++STATS_NETERRSENDSOFT;
			return (0);
		}
		if ((short)pack->flags < -COMHDSZ) {
			port->head[0] = MAGIC1;
			port->head[1] = MAGIC2;
			port->head[2] = (Uchar)0x00ff&(pack->length >>8);
			port->head[3] = (Uchar)0x00ff&(pack->length);
			++pack->flags;
		}
		if ((short)pack->flags < 0)
			ch[0] = port->head[COMHDSZ+(short)pack->flags];
		else
			ch[0] = pack->raw[pack->flags];
		if (1 == write (port->ofd, ch, 1)) {
			to = 0;
			if ((short)++pack->flags == pack->length) {
				if (F(port->outgoing = packet_del (pack)))
					port->tail = 0;
			}
		} else if (EAGAIN != errno)
			return (1);		/* real error */
	}
	return (0);
}

extern void FAR
com_poll (void)
{
	PORT	*port;

	if (nports <= 0)
		return;

	for (port = ports; port < &ports[rangeof(ports)]; ++port)
		if (port->flags && port->outgoing)
			com_out (port);
}

LOCAL_FUNC int FAR
com_send (NETPORT *np, PACKET *pack)
{
	int	portno;
	PORT	*port;
	PACKET	*pp;

	if (0 == pack)
		return (0);

	portno = np->unit-'1';
	if (portno < 0 || portno >= NPORTS)
		return (1);
	port = &ports[portno];
	if (!port->flags)
		return (1);

	if (!(pp = packet_new (pack->length, -1)))
		return (1);

	pp->netport = port->netport;
	pp->address = port->address;
	pp->length = pack->length;
	memcpy (pp->raw, pack->raw, pack->length);
	pp->flags = -COMHDSZ - 1;
	pp->next = 0;
	if (port->tail)
		port->tail->next = pp;
	else
		port->outgoing = pp;
	port->tail = pp;
	return (com_out (port));
}

LOCAL_FUNC int FAR
com_receive (NETPORT *np, int poll)
{
	int	portno, len;
	char	ch[1];
	PACKET	*pack;
	PORT	*port;

	com_poll ();

	portno = np->unit-'1';
	if (portno < 0 || portno >= NPORTS)
		return (1);
	port = &ports[portno];
	if (!port->flags)
		return (1);

	while (1 == read (port->ifd, ch, 1)) {
		port->last_in = st.present;
		pack = port->incoming;
		if (!pack) {
			if (port->panic) {	/* drop rest of packet */
				--port->panic;
				continue;
			}
			if (F(pack = packet_new (PAKPACKLEN, -1))) {
				port->panic = (0x00ff & ch[0]) - 1; /* length */
				continue;
			} 
			port->incoming = pack;
			pack->flags = (Ushort)-COMHDSZ;
			pack->next = 0;
		}
		if ((short)pack->flags < 0) {
			pack->raw[COMHDSZ+(short)pack->flags++] = (Uchar)ch[0];
			if (((short)pack->flags == 1-COMHDSZ &&
			     pack->raw[0] != MAGIC1) ||
			    ((short)pack->flags == 2-COMHDSZ &&
			     pack->raw[1] != MAGIC2) ||
			    ((short)pack->flags == 4-COMHDSZ &&
			     ((len = (pack->raw[2]<<8)+pack->raw[3]) < 2 ||
			      len > PAKPACKLEN))) {
				++STATS_NETERRD;
				pack->flags = (Ushort)-COMHDSZ;	/* reuse */
			} else if (pack->flags == 0)
				pack->length = (short)len;
			continue;
		}
		pack->raw[pack->flags++] = (Uchar)ch[0];
		if (pack->flags == (Ushort)pack->length) {
#if 0
			pack->netport = port->netport;
			pack->address = port->address;
			packet_deliver (pack);
			pack = 0;
			port->incoming = 0;
#else
			PACKET	*lpack;

			if (F(lpack = packet_new (pack->length, -1)))
				++STATS_NETERRD;
			else {
				lpack->netport = port->netport;
				lpack->length = pack->length;
				lpack->address = port->address;
				memcpy (lpack->raw, pack->raw, lpack->length);
				packet_deliver (lpack);
			}
			pack->flags = (Ushort)-COMHDSZ;	/* reuse */
#endif
		}
	}
	return (0);
}

struct NetDriver NEAR NetFifo = {
	"fifo",
	0,
	NULL,	/* extra */
	com_init,
	com_term,
	com_send,
	com_receive
};
#endif /* if HAVE_FIFO */
