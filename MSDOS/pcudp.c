/* --------------------------------- pcudp.c -------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for packet level exchanges (low level). It uses a packet driver
 * as the communications medium at the UDP level. It uses ARP to locate
 * the server and also responds to ARP requests.
 *
 * One MUST make sure that 'ip' is unique and 'sip' is correct.
 * '+' options are mandatory!
 *
 *   int=	interrupt number (default is auto detect 0x60-0x80)
 *  +sip=	server IP address
 *   sport=	server UDP port (default is FLY8_SPORT which is usualy 0xf8f8)
 *  +ip=	my IP address
 *   port=	my UDP port (default is FLY8_PORT which is usualy 0xf8f9)
 *
 * When we receive a packet, it is put into ->data, then the logic determins
 * where ->raw is.
 *
 * When we send a packet, the data is in ->raw and the protocol determins
 * where the header starts.
*/

#include "fly.h"
#include "pktdrvr.h"
#include "pcudp.h"

#include <dos.h>


/* msubs.asm */
extern int	FAR chksum (Uchar FAR *buf, int len);


#define MY_OFF(p)	(((Ushort far *)&(p))[0])
#define MY_SEG(p)	(((Ushort far *)&(p))[1])

#define INT_FIRST 	0x60
#define INT_LAST  	0x7f

typedef struct port	PORT;
struct port {
	int	flags;
#define POF_ON		0x0001
	void (interrupt far *pkint) (void);
	int	iport;
	struct NetDriver *driver;
	short	netport;
	int	intno;			/* packet-driver interrupt */
	Uchar	mac[MACADDRESS];	/* my MAC address */
	Uchar	ip[IPADDRESS];		/* my IP address */
	Uchar	udport[UDPADDRESS];	/* my UDP port */
	Uchar	addr[APADDRESS];	/* my Fly8 address */
	int	handle;			/* packet-driver handle */
	PACKET	*pack;			/* packet being received */
	PACKET	*head;			/* outgoing packets */
	PACKET	*tail;
	int	*stack;			/* packet-driver stack */
#define	PKSSIZE		1024		/* packet-driver stack size (words) */
	int	version;
	int	class;
	int	type;
	int	number;
	char	*name;
	int	basic;
};

static PORT	ports[] = {
	{0, pkint0, 1},
	{0, pkint1, 2},
	{0, pkint2, 3},
	{0, pkint3, 4}
};
#define	NDEV	(rangeof(ports))

static int	nports = 0;		/* number of active ports */
static Uchar	ether_arp[2] =
			{0x0ff & (FLY8_ARP>> 8), 0x0ff & FLY8_ARP};
static Uchar	ether_ip [2] =
			{0x0ff & (FLY8_IP >> 8), 0x0ff & FLY8_IP};
static Uchar	arp_query[2] =
			{0x0ff & (FLY8_ARPQ >> 8), 0x0ff & FLY8_ARPQ};
static Uchar	arp_reply[2] =
			{0x0ff & (FLY8_ARPR >> 8), 0x0ff & FLY8_ARPR};
static Uint	identification = 0;

static int	have_svr = 0;
static Ulong	next_query = 0;
static Uchar	svr_mac[MACADDRESS];
static Uchar	svr_ip[IPADDRESS];
static Uchar	svr_udport[UDPADDRESS];
static Uchar	svr_addr[APADDRESS];


LOCAL_FUNC int FAR	PuInit (NETPORT *np, char *options);
LOCAL_FUNC void FAR	PuTerm (NETPORT *np);
LOCAL_FUNC int FAR	PuSendPD (PORT *port, char *buffer, int length);
LOCAL_FUNC void FAR	PuReceivePD (Ushort FAR *p);
LOCAL_FUNC int FAR	PuPoll (NETPORT *np, int poll);

LOCAL_FUNC int NEAR	PuOptions (PORT *port, char *options);
LOCAL_FUNC int NEAR	PuReceiveARP (PORT *port, PACKET *pack);
LOCAL_FUNC int NEAR	PuReceiveIP  (PORT *port, PACKET *pack);
LOCAL_FUNC int NEAR	PuReceiveETH (PORT *port, PACKET *pack);
LOCAL_FUNC int NEAR	PuSendETH (PORT *port, PACKET *p, Uchar *h, int len,
	Uchar *ether_type);
LOCAL_FUNC int NEAR	PuSendIP (PORT *port, PACKET *p, Uchar *h, int len,
	int ip_type);
LOCAL_FUNC int NEAR	PuSendUDP (PORT *port, PACKET *p, Uchar *h, int len);
LOCAL_FUNC int NEAR	PuSendAP (PORT *port, PACKET *p, Uchar *h, int len);
LOCAL_FUNC int NEAR	PuSendARPquery (PORT *port, PACKET *p, Uchar *h);
LOCAL_FUNC int NEAR	PuSendARPreply (PORT *port, PACKET *p, Uchar *h,
	Uchar *q);
LOCAL_FUNC int FAR	PuSend (NETPORT *np, PACKET *p);


/* Called to initialize this driver.
*/
LOCAL_FUNC int FAR
PuInit (NETPORT *np, char *options)
{
	int	portno, rc, i;
	PORT	*port;

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV) {
		LogPrintf ("%s.%c: bad port\n",
			np->NetDriver->name, np->unit);
		return (1);
	}
	port = &ports[portno];
	if (port->flags & POF_ON) {
		LogPrintf ("%s.%c: already on\n",
			np->NetDriver->name, np->unit);
		return (1);
	}

	port->driver = np->NetDriver;
	port->netport = np->netport;
	if (PuOptions (port, options))
		return (1);

	if (-1 == port->intno) {
		for (i = INT_FIRST; i <= INT_LAST; ++i) {
			if (test_for_pd (i)) {
				port->intno = i;
				break;
			}
		}
		if (-1 == port->intno) {
			LogPrintf ("%s.%c: no driver found\n",
				np->NetDriver->name, np->unit);
			return (1);
		}
	}
	if (!test_for_pd (port->intno)) {
		LogPrintf ("%s.%c: no driver at 0x%x\n",
			np->NetDriver->name, np->unit,
			port->intno);
		port->intno = -1;
		return (1);
	}
	LogPrintf ("%s.%c: Intno 0x%x\n",
		np->NetDriver->name, np->unit,
		port->intno);

	if (F(port->stack = (int *)memory_calloc (PKSSIZE,
						sizeof (*port->stack)))) {
		LogPrintf ("%s.%c: no mem\n",
			np->NetDriver->name, np->unit);
		return (1);
	}

	pkinit (portno, PuReceivePD, &port->stack[PKSSIZE]);

	port->handle = access_type (port->intno, CL_ETHERNET, ANYTYPE, 0,
		(char *)ether_arp, 0, port->pkint);
	if (-1 == port->handle) {
		LogPrintf ("%s.%c: no handle\n",
			np->NetDriver->name, np->unit);
		port->stack = memory_cfree (port->stack, PKSSIZE,
						sizeof (*port->stack));
		return (1);
	}
	if (driver_info (port->intno, port->handle, &port->version,
			&port->class, &port->type, &port->number, &port->name,
			&port->basic)) {
		port->basic = 1;	/* what else ? */
	}
	LogPrintf ("%s.%c: Basic 0x%x\n",
		np->NetDriver->name, np->unit,
		port->basic);

	port->pack = 0;
	port->flags |= POF_ON;
	++nports;

	rc = get_address (port->intno, port->handle, (char *)port->mac,
						sizeof (port->mac));
	if (rc) {
		LogPrintf ("%s.%c: my MAC  failed %0x\n",
			np->NetDriver->name, np->unit,
			Derr);
		memset (port->mac, 0, sizeof (port->mac));
	} else {
		LogPrintf ("%s.%c: my  MAC  %02x%02x%02x-%02x%02x%02x\n",
			np->NetDriver->name, np->unit,
			port->mac[0], port->mac[1], port->mac[2],
			port->mac[3], port->mac[4], port->mac[5]);
		MsgWPrintf (-100, "%s.%c: my  IP   %u.%u.%u.%u",
			np->NetDriver->name, np->unit,
			port->ip[0], port->ip[1], port->ip[2], port->ip[3]);
		LogPrintf ("%s.%c: my  port %02x%02x\n",
			np->NetDriver->name, np->unit,
			port->udport[0], port->udport[1]);
	}

	memcpy (svr_addr,           svr_ip,     IPADDRESS);
	memcpy (svr_addr+IPADDRESS, svr_udport, UDPADDRESS);

	memcpy (port->addr,           port->ip,     IPADDRESS);
	memcpy (port->addr+IPADDRESS, port->udport, UDPADDRESS);

	memset (svr_mac, 0xff, MACADDRESS);

	have_svr = 0;
	next_query = st.present;	/* query immediately */

	LogPrintf ("%s.%c: init ok\n",
		np->NetDriver->name, np->unit);

	return (0);
}

/* Called to terminate this driver.
*/
LOCAL_FUNC void FAR
PuTerm (NETPORT *np)
{
	int	portno;
	PORT	*port;
	PACKET	*pack;

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV)
		return;
	port = &ports[portno];
	if (!(port->flags & POF_ON))
		return;
	release_type (port->intno, port->handle);
	if (port->pack) {
		packet_del (port->pack);
		port->pack = 0;
	}

/* delete queued ARP packets.
*/
	for (;;) {
		if (T(pack = port->head)) {
			if (F(port->head = pack->next))
				port->tail = NULL;
		} else
			break;
		packet_del (pack);
	}

	port->flags = 0;
	port->stack = memory_cfree (port->stack, PKSSIZE,
						sizeof (*port->stack));

	LogPrintf ("%s.%c: term ok\n",
		np->NetDriver->name, np->unit);
}

LOCAL_FUNC int FAR
PuSendPD (PORT *port, char *buffer, int length)
{
	return (send_pkt (port->intno, buffer, length));
}

/* This function is called by the packet driver when a packet arrives. Each
 * packet attracts two calls: in the first one (ax = 0) we get the packet size
 * and should provide a buffer area in return. The second call (ax = 1) informs
 * us that the packet was copied into the buffer.
 *
 * It is executed with interrupts off - don't do more than the absolute
 * minimum in here.
*/
LOCAL_FUNC void FAR
PuReceivePD (Ushort FAR *p)
{
	PORT	*port;
	PACKET	*pack;
	Uchar	*buff;

	st.flags1 |= SF_ASYNC;
	switch (p[RCV_AX]) {
	case 0:				/* Space allocate call */
		p[RCV_ES] = p[RCV_DI] = 0;
		if (p[RCV_ID] >= NDEV ||
		    p[RCV_CX] < ETHNNN || p[RCV_CX] > (Ushort)PAKPACKLEN)
			goto badret;
		port = &ports[p[RCV_ID]];
		if (!(port->flags & POF_ON))
			goto badret;
		if (port->pack) {	/* stray packet? */
			/* stats... */
			packet_del (port->pack);
			port->pack = 0;
		}
		if (F(pack = packet_new (p[RCV_CX], 0)))
			goto badret;
		port->pack = pack;
		buff = pack->data;
		p[RCV_ES] = MY_SEG (buff);
		p[RCV_DI] = MY_OFF (buff);
		break;
	case 1:				/* Packet complete call */
		if (p[RCV_ID] >= NDEV)
			goto badret;
		port = &ports[p[RCV_ID]];
		if (!(port->flags & POF_ON))
			goto badret;
		if (F(pack = port->pack))
			goto badret;
		port->pack = 0;
		pack->netport = port->netport;

		switch (PuReceiveETH (port, pack)) {
		case 0:		/* packet accepted */
			break;
		case 1:		/* packet rejected */
			packet_del (pack);
			break;
		default:	/* packet error */
			packet_del (pack);
			goto badret;
		}
		break;
	default:
badret:
		++STATS_NETERRD;
		break;
	}
	st.flags1 &= ~SF_ASYNC;
}



/* Do some housekeeping. This is necessary since we try to absolutely
 * minimize the work done in the packet receiver routine.
*/
LOCAL_FUNC int FAR
PuPoll (NETPORT *np, int poll)
{
	PORT	*port;
	PACKET	*p;
	PACKET	*pack;
	int	portno;
	Ulong	flags;
	int	ret;
	Uchar	*h;

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV)
		return (1);
	port = &ports[portno];
	if (!(port->flags & POF_ON))
		return (1);

/* handle queued ARP packets.
*/
	for (;;) {
		flags = Sys->Disable ();
		if (T(pack = port->head)) {
			if (F(port->head = pack->next))
				port->tail = NULL;
		}
		Sys->Enable (flags);
		if (!pack)
			break;

		h = pack->raw;
		if (!memcmp (h+ARPHOP, arp_reply, sizeof (arp_reply))) {
			memcpy (svr_mac, h+ARPHESRCE, MACADDRESS);
			have_svr = 1;
			LogPrintf (
				"%s.%u: svr MAC  %02x%02x%02x-%02x%02x%02x\n",
				port->driver->name, port->iport,
				svr_mac[0], svr_mac[1], svr_mac[2],
				svr_mac[3], svr_mac[4], svr_mac[5]);
			MsgWPrintf (-100, "%s.%u: svr IP   %u.%u.%u.%u",
				port->driver->name, port->iport,
				svr_ip[0], svr_ip[1], svr_ip[2], svr_ip[3]);
			LogPrintf ("%s.%u: svr port %02x%02x\n",
				port->driver->name, port->iport,
				svr_udport[0], svr_udport[1]);
		} else if (!memcmp (h+ARPHOP, arp_query, sizeof (arp_query))) {
			if (0 == (p = packet_new (0, -1))) {
				LogPrintf ("%s.%u: no packet\n",
					port->driver->name, port->iport);
			} else {
				PuSendARPreply (port, p, p->raw, pack->raw);
				packet_del (p);
			}
		}
		packet_del (pack);
	}

/* send ARP query if needed.
*/
	ret = 0;
	if (!have_svr && st.present >= next_query) {
		if (0 == (p = packet_new (0, -1))) {
			LogPrintf ("%s.%u: no packet\n",
				port->driver->name, port->iport);
			ret = 1;
		} else {
			ret = PuSendARPquery (port, p, p->raw);
			packet_del (p);
			next_query = st.present + 100;	/* 100ms wait */
		}
	}

	return (ret);
}

/* Parse the options that this driver expects.
*/
LOCAL_FUNC int NEAR
PuOptions (PORT *port, char *options)
{
	long	l;
	char	*p;
	Uint	i1, i2, i3, i4;

	if (get_narg (options, "int=", &l))
		l = -1;
	port->intno = (int)l;

	if (F(p = get_sarg (options, "sip="))) {
		LogPrintf ("%s.%u: missing 'sip' option\n",
			port->driver->name, port->iport);
		return (1);
	}
	if (4 != sscanf (p, "%3u.%3u.%3u.%3u", &i1, &i2, &i3, &i4) ||
			i1 > 255 || i2 > 255 || i3 > 255 || i4 > 255) {
		LogPrintf ("%s.%u: bad 'sip' option\n",
			port->driver->name, port->iport);
		return (1);
	}
	STRfree (p);
	svr_ip[0] = (Uchar)i1;
	svr_ip[1] = (Uchar)i2;
	svr_ip[2] = (Uchar)i3;
	svr_ip[3] = (Uchar)i4;

	if (get_narg (options, "sport=", &l))
		l = FLY8_SPORT;
	ComPBw (svr_udport, (int)l);

	if (F(p = get_sarg (options, "ip="))) {
		LogPrintf ("%s.%u: missing 'ip' option\n",
			port->driver->name, port->iport);
		return (1);
	}
	if (4 != sscanf (p, "%3u.%3u.%3u.%3u", &i1, &i2, &i3, &i4) ||
			i1 > 255 || i2 > 255 || i3 > 255 || i4 > 255) {
		LogPrintf ("%s.%u: bad 'ip' option\n",
			port->driver->name, port->iport);
		return (1);
	}
	STRfree (p);
	port->ip[0] = (Uchar)i1;
	port->ip[1] = (Uchar)i2;
	port->ip[2] = (Uchar)i3;
	port->ip[3] = (Uchar)i4;

	if (get_narg (options, "port=", &l))
		l = FLY8_PORT;
	ComPBw (port->udport, (int)l);

	return (0);
}

/* Accept an ARP packet if it is for us and looks valid. Do not process it
 * just yet - for speed we simply put in on a queue for later processing.
 * We keep RARP too but they would be discarded later.
*/
LOCAL_FUNC int NEAR
PuReceiveARP (PORT *port, PACKET *pack)
{
	int	ret;
	Uchar	*h;
	int	len;

	ret = -1;
	do {
		len = pack->length;
		if (len < ARPHNNN)
			break;		/* bad ARP packet */
		h = pack->raw;

/* Note: we ignore broadcasts here.
*/
		if (memcmp (h+ARPHIDEST, port->ip,  sizeof (port->ip)) &&
		    memcmp (h+ARPHEDEST, port->mac, sizeof (port->mac))) {
			ret = 1;
			break;		/* not for us */
		}
		pack->next = NULL;
		if (port->tail)
			port->tail->next = pack;
		else
			port->head = pack;
		port->tail = pack;
		ret = 0;
	} while (0);

	return (ret);
}

#if 0

/* The following three functions are much clearer than the integrated one
 * that replaces them. However, micro&soft vc1.5 dies horribly when inlining
 * them.
*/

LOCAL_FUNC int NEAR	PuReceiveAP  (PORT *port, PACKET *pack);
LOCAL_FUNC int NEAR	PuReceiveUDP (PORT *port, PACKET *pack);

/* Accept a Fly8 APplication packet.
*/
LOCAL_FUNC int NEAR
PuReceiveAP (PORT *port, PACKET *pack)
{
	int	ret;
	Uchar	*h;
	int	len;
	int	n;

	ret = 1;
	do {
		len = pack->length;
		if (len < APHNNN)
			break;
		h = pack->raw;
		n = ComGBw (h+APHLEN);

/* Some systems round the size up so we cannot check for exact match.
*/
		if (n < 3 || n > len)
			break;
		pack->address = h+APHSRCE;
		pack->raw += APHNNN;
		pack->length = (short)n;
		packet_deliver (pack);
		ret = 0;	/* packet always deleted */
	} while (0);

	return (ret);
}

/* Accept a UDP packet.
*/
LOCAL_FUNC int NEAR
PuReceiveUDP (PORT *port, PACKET *pack)
{
	int	ret;
	Uchar	*h;
	int	len;

	ret = 1;
	do {
		len = pack->length;
		if (len < UDPHNNN)
			break;
		h = pack->raw;
		if (memcmp (h+UDPHSRCE, svr_udport,   sizeof (svr_udport)) ||
		    memcmp (h+UDPHDEST, port->udport, sizeof (port->udport)))
			break;
		pack->raw    += UDPHNNN;
		pack->length -= UDPHNNN;
		ret = PuReceiveAP (port, pack);
	} while (0);

	return (ret);
}

/* Accept an IP packet.
*/
LOCAL_FUNC int NEAR
PuReceiveIP (PORT *port, PACKET *pack)
{
	int	ret;
	Uchar	*h;
	int	len;

	ret = 1;
	do {
		if (!have_svr)
			break;

		len = pack->length;
		if (len < IPHNNN)
			break;
		h = pack->raw;
		if (FLY8_UDP != h[IPHPROTO])
			break;
		if (memcmp (h+IPHSRCE,  svr_ip,       sizeof (svr_ip)) ||
		    memcmp (h+IPHDEST,  port->ip,     sizeof (port->ip)))
			break;
		pack->raw    += IPHNNN;
		pack->length -= IPHNNN;
		ret = PuReceiveUDP (port, pack);
	} while (0);

	return (ret);
}

#else

/* Accept an IP packet.
*/
LOCAL_FUNC int NEAR
PuReceiveIP (PORT *port, PACKET *pack)
{
	int	ret;
	int	n;
	Uchar	*h;
	int	len;

	ret = -1;
	do {
		if (!have_svr) {
			ret = 1;
			break;		/* no server yet */
		}

/* Accept an IP packet.
*/
		len = pack->length;
		if (len < IPHNNN)
			break;		/* bad IP len */
		h   = pack->raw;
		if (FLY8_UDP != h[IPHPROTO]) {
			ret = 1;
			break;		/* wrong protocol */
		}
		if (memcmp (h+IPHSRCE, svr_ip,   sizeof (svr_ip)) ||
		    memcmp (h+IPHDEST, port->ip, sizeof (port->ip))) {
			ret = 1;
			break;		/* wrong IP */
		}
		h   += IPHNNN;
		len -= IPHNNN;

/* Accept a UDP packet.
*/
		if (len < UDPHNNN)
			break;		/* bad UDP len */
		if (memcmp (h+UDPHSRCE, svr_udport,   sizeof (svr_udport)) ||
		    memcmp (h+UDPHDEST, port->udport, sizeof (port->udport))) {
			ret = 1;
			break;		/* wrong UDP port */
		}
		h   += UDPHNNN;
		len -= UDPHNNN;

/* Accept a Fly8 APplication packet.
*/
		if (len < APHNNN)
			break;		/* bad AP len */
		n = ComGBw (h+APHLEN);

/* Some systems round the size up so we cannot check for exact match.
*/
		if (n < 3 || n > len)
			break;		/* bad internal len */
		pack->raw     = h+APHNNN;
		pack->length  = (short)n;
		pack->address = h+APHSRCE;
		packet_deliver (pack);
		ret = 0;		/* packet always deleted */
	} while (0);

	return (ret);
}

#endif

/* Accept an incoming ethernet packet. It simply dispatches it to the
 * proper handler based on the packet type.
*/
LOCAL_FUNC int NEAR
PuReceiveETH (PORT *port, PACKET *pack)
{
	Uchar	*h;

	h = pack->raw;
	pack->raw    += ETHNNN;
	pack->length -= ETHNNN;

	if (!memcmp (h+ETHTYPE, ether_arp, sizeof (ether_arp)))
		return (PuReceiveARP (port, pack));
	else if (!memcmp (h+ETHTYPE, ether_ip, sizeof (ether_ip)))
		return (PuReceiveIP (port, pack));
	else
		return (1);		/* not our packet type */
}

/* Package an ethernet packet.
*/
LOCAL_FUNC int NEAR
PuSendETH (PORT *port, PACKET *p, Uchar *h, int len, Uchar *ether_type)
{
	len += ETHNNN;
	h   -= ETHNNN;
	memcpy (h+ETHDEST, svr_mac,    MACADDRESS);
	memcpy (h+ETHSRCE, port->mac,  MACADDRESS);
	memcpy (h+ETHTYPE, ether_type, 2);

	return (PuSendPD (port, (char *)h, len));
}

/* Package an IP packet.
*/
LOCAL_FUNC int NEAR
PuSendIP (PORT *port, PACKET *p, Uchar *h, int len, int ip_type)
{
	len += IPHNNN;
	h   -= IPHNNN;
		h[IPHVER]   = (char)IP_VER;
		h[IPHTOS]   = (char)IP_TOS_LOWDELAY;
	ComPBw (h+IPHLEN,   (Uint)len);
	ComPBw (h+IPHID,    identification++);
	ComPBw (h+IPHFLAGS, IP_DF);		/* don't fragment */
		h[IPHTTL]   = (char)IP_TTL;
		h[IPHPROTO] = (char)ip_type;
	memset (h+IPHCHECK, 0,            2);
	memcpy (h+IPHSRCE,  port->ip,     IPADDRESS);
	memcpy (h+IPHDEST,  svr_ip,       IPADDRESS);
	ComPLw (h+IPHCHECK, (Uint)~chksum (h, IPHNNN/2));

	return (PuSendETH (port, p, h, len, ether_ip));
}

/* Package a UDP packet.
*/
LOCAL_FUNC int NEAR
PuSendUDP (PORT *port, PACKET *p, Uchar *h, int len)
{
	int	chk;

	len += UDPHNNN;
	h   -= UDPHNNN;
	memcpy (h+UDPHSRCE,  port->udport, UDPADDRESS);
	memcpy (h+UDPHDEST,  svr_udport,   UDPADDRESS);
	ComPBw (h+UDPHLEN,   (Uint)len);
	memset (h+UDPHCHECK, 0,            2);

/* We add a slack byte for even size, this is needed for the checksum.
 * A packet is always a multiple of a large (power of 2) granularity
 * factor so we always have room.
*/
	if (len & 1)
		h[len] = '\0';

/* Build a pseudo-header for the checksum.
*/
	h -= UDPXNNN;
	memcpy (h+UDPXSRCE,  port->ip,     IPADDRESS);
	memcpy (h+UDPXDEST,  svr_ip,       IPADDRESS);
		h[UDPXZERO]  = (char)0;
		h[UDPXPROTO] = (char)FLY8_UDP;
	ComPBw (h+UDPXLEN,   (Uint)len);
	if (0 == (chk = ~chksum (h, (UDPXNNN+len+1)/2)))
		chk = 0xffff;

/* Now store the checksum.
*/
	h += UDPXNNN;
	ComPLw (h+UDPHCHECK, (Uint)chk);

	return (PuSendIP (port, p, h, len, FLY8_UDP));
}

/* Package a Fly8 application packet.
*/
LOCAL_FUNC int NEAR
PuSendAP (PORT *port, PACKET *p, Uchar *h, int len)
{
	h -= APHNNN;
	if (p->address)
		memcpy (h+APHDEST, p->address, APADDRESS);
	else
		memset (h+APHDEST, 0xff,       APADDRESS);
	memcpy (h+APHSRCE, port->addr, APADDRESS);
	ComPBw (h+APHLEN,  (Uint)len);
	len += APHNNN;

	return (PuSendUDP (port, p, h, len));
}

/* Send an ARP query. This is done repeatedly until the server responds.
*/
LOCAL_FUNC int NEAR
PuSendARPquery (PORT *port, PACKET *p, Uchar *h)
{
	h -= ARPHNNN;
	ComPBw (h+ARPHHTYPE,  1U);		/* hardware: ethernet */
	ComPBw (h+ARPHPTYPE,  FLY8_IP);		/* protocol: IP */
		h[ARPHHSIZE]= MACADDRESS;
		h[ARPHPSIZE]= IPADDRESS;
	memcpy (h+ARPHOP,     arp_query, sizeof (arp_query));
	memcpy (h+ARPHESRCE,  port->mac, MACADDRESS);
	memcpy (h+ARPHISRCE,  port->ip,  IPADDRESS);
	memset (h+ARPHEDEST,  0,         MACADDRESS);
	memcpy (h+ARPHIDEST,  svr_ip,    IPADDRESS);

	return (PuSendETH (port, p, h, ARPHNNN, ether_arp));
}

/* Send an ARP reply. This is in respnse to a received query.
*/
LOCAL_FUNC int NEAR
PuSendARPreply (PORT *port, PACKET *p, Uchar *h, Uchar *q)
{
	h -= ARPHNNN;
	memcpy (h+ARPHHTYPE, q+ARPHHTYPE, 2);
	memcpy (h+ARPHPTYPE, q+ARPHPTYPE, 2);
	memcpy (h+ARPHHSIZE, q+ARPHHSIZE, 1);
	memcpy (h+ARPHPSIZE, q+ARPHPSIZE, 1);
	memcpy (h+ARPHOP,    arp_reply,   sizeof (arp_reply));
	memcpy (h+ARPHESRCE, port->mac,   MACADDRESS);
	memcpy (h+ARPHISRCE, q+ARPHIDEST, IPADDRESS);
	memcpy (h+ARPHEDEST, q+ARPHESRCE, MACADDRESS);
	memcpy (h+ARPHIDEST, q+ARPHISRCE, IPADDRESS);

	return (PuSendETH (port, p, h, ARPHNNN, ether_arp));
}

/* Send a packet. Directly called from the main program.
*/
LOCAL_FUNC int FAR
PuSend (NETPORT *np, PACKET *p)
{
	PORT	*port;
	int	portno;
	int	ret;

	ret = 1;
	do {
		if (!p) {
			ret = 0;
			break;
		}
		if (!have_svr)
			break;
		portno = np->unit-'1';
		if (portno < 0 || portno >= NDEV)
			break;
		port = &ports[portno];
		if (!(port->flags & POF_ON))
			break;
		ret = PuSendAP (port, p, p->raw, p->length);
	} while (0);

	PuPoll (np, 1);
	return (ret);
}

struct NetDriver NEAR NetPcUDP = {
	"PcUDP",
	0,
	NULL,	/* extra */
	PuInit,
	PuTerm,
	PuSend,
	PuPoll
};
