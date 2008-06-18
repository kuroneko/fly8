/* --------------------------------- packet.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for packet level exchanges (low level). It uses a packet driver
 * as the communications medium. At the moment it expects etherslip but it
 * is rather general.
 * Here we assume only one handle is used and ANYTYPE was selected. Up to 4
 * drivers can be handled, all call the same fuction with the first argument
 * 'dev' indicating which one it is.
 * Options:
 *  0 interrupt number (usualy 0x65)
 *  1 packet type (default is FLY8_ETYPE which is usualy 0xf8f8)
*/

#include "fly.h"
#include "pktdrvr.h"

#include <dos.h>


#define MY_OFF(p)	(((Ushort FAR *)&(p))[0])
#define MY_SEG(p)	(((Ushort FAR *)&(p))[1])

#define INT_FIRST 	0x60
#define INT_LAST  	0x7f

#define	FLY8_ETYPE	0xf8f8		/* ether packet type */

#define MACADDRESS	6

#define ETHDEST		0		/* ethernet header */
#define ETHSRCE		(ETHDEST   + MACADDRESS)
#define ETHTYPE		(ETHSRCE   + MACADDRESS)
#define ETHNNN		(ETHTYPE   + 2)

#define APADDRESS	MACADDRESS

#define APHDEST		0		/* Fly8 header */
#define APHSRCE		(APHDEST   + APADDRESS)
#define APHLEN		(APHSRCE   + APADDRESS)
#define APHNNN		(APHLEN    + 2)

#define	PKSSIZE		1024		/* packet-driver's stack size */

typedef struct port PORT;
struct port {
	int	flags;
#define POF_ON		0x0001
	void (INTERRUPT FAR *pkint) (void);
	int	iport;
	struct NetDriver *driver;
	short	netport;		/* back-pointer */
	int	intno;			/* packet-driver interrupt */
	Uchar	mac[MACADDRESS];	/* my MAC address */
	Uchar	addr[APADDRESS];	/* my MAC address */
	Uchar	etype[2];		/* eth_type for fly8 */
	int	handle;			/* packet-driver handle */
	PACKET	*pack;			/* packet being received */
	int	*stack;			/* packet-driver stack */
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


LOCAL_FUNC void FAR	PktReceivePD (Ushort FAR *p);
LOCAL_FUNC int NEAR	PktReceiveETH (PORT *port, PACKET *pack);
LOCAL_FUNC int NEAR	PktSendETH (PORT *port, PACKET *p, Uchar *h, int len,
	Uchar *ether_type);
LOCAL_FUNC int NEAR	PktSendAP (PORT *port, PACKET *p, Uchar *h, int len);
LOCAL_FUNC int FAR	PktSend (NETPORT *np, PACKET *p);


LOCAL_FUNC void FAR
PktReceivePD (Ushort FAR *p)
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

		if (PktReceiveETH (port, pack)) {
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

/* Accept a Fly8 APplication packet.
*/
LOCAL_FUNC int NEAR
PktReceiveAP (PORT *port, PACKET *pack)
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

/* Accept an incoming ethernet packet. We know it is our special packet type
 * since we requested a selection by the packet driver.
*/
LOCAL_FUNC int NEAR
PktReceiveETH (PORT *port, PACKET *pack)
{
	pack->raw    += ETHNNN;
	pack->length -= ETHNNN;

	return (PktReceiveAP (port, pack));
}

LOCAL_FUNC int NEAR
PktOptions (PORT *port, char *options)
{
	long	l;

	if (get_narg (options, "int=", &l))
		l = -1;
	port->intno = (int)l;

	if (get_narg (options, "type=", &l))
		l = FLY8_ETYPE;
	ComPBw (port->etype, (int)l);

	return (0);
}

LOCAL_FUNC int FAR
PktInit (NETPORT *np, char *options)
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
	if (PktOptions (port, options))
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

	pkinit (portno, PktReceivePD, &port->stack[PKSSIZE]);

	port->handle = access_type (port->intno, CL_ETHERNET, ANYTYPE, 0,
		(char FAR *)port->etype, 2, port->pkint);
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
	}
	memcpy (port->addr, port->mac, MACADDRESS);

	LogPrintf ("%s.%c: init ok\n",
		np->NetDriver->name, np->unit);

	return (0);
}

LOCAL_FUNC void FAR
PktTerm (NETPORT *np)
{
	int	portno;
	PORT	*port;

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
	port->flags = 0;
	port->stack = memory_cfree (port->stack, PKSSIZE,
						sizeof (*port->stack));

	LogPrintf ("%s.%c: term ok\n",
		np->NetDriver->name, np->unit);
}

/* Package an ethernet packet.
*/
LOCAL_FUNC int NEAR
PktSendETH (PORT *port, PACKET *p, Uchar *h, int len, Uchar *ether_type)
{
	Uchar	*aph;

	aph = h;
	len += ETHNNN;
	h   -= ETHNNN;
	memcpy (h+ETHDEST, aph+APHDEST, MACADDRESS);
	memcpy (h+ETHSRCE, port->mac,   MACADDRESS);
	memcpy (h+ETHTYPE, ether_type,  2);

	return (send_pkt (port->intno, (char *)h, len));
}

/* Package a Fly8 application packet.
*/
LOCAL_FUNC int NEAR
PktSendAP (PORT *port, PACKET *p, Uchar *h, int len)
{
	h -= APHNNN;
	if (p->address)
		memcpy (h+APHDEST, p->address, APADDRESS);
	else
		memset (h+APHDEST, 0xff,       APADDRESS);
	memcpy (h+APHSRCE, port->addr, APADDRESS);
	ComPBw (h+APHLEN,  (Uint)len);
	len += APHNNN;

	return (PktSendETH (port, p, h, len, port->etype));
}

/* Send a packet. Directly called from the main program.
*/
LOCAL_FUNC int FAR
PktSend (NETPORT *np, PACKET *p)
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
		portno = np->unit-'1';
		if (portno < 0 || portno >= NDEV)
			break;
		port = &ports[portno];
		if (!(port->flags & POF_ON))
			break;
		ret = PktSendAP (port, p, p->raw, p->length);
	} while (0);

	return (ret);
}

struct NetDriver NEAR NetPack = {
	"PKT",
	0,
	NULL,	/* extra */
	PktInit,
	PktTerm,
	PktSend,
	0	/* poll */
};
