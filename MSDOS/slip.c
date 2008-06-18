/* --------------------------------- slip.c --------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for packet level exchanges (low level). It uses a packet driver
 * as the communications medium. It expects SLIP type packets.
 * Options:
 *  0 interrupt number (usualy 0x65)
*/

#include "fly.h"
#include "pktdrvr.h"

#include <dos.h>


#define MY_OFF(p)	(((Ushort FAR *)&(p))[0])
#define MY_SEG(p)	(((Ushort FAR *)&(p))[1])

#define INT_FIRST 	0x60
#define INT_LAST  	0x7f

#define	PKSSIZE		1024		/* packet-driver's stack size */

#define PHLEN		(2*LADDRESS+2+2)
#define PHEAD		(pack->raw-PHLEN)

typedef struct port PORT;
struct port {
	int	flags;
#define POF_ON		0x0001
	void (INTERRUPT FAR *pkint) (void);
	int	intno;
	int	netport;
	Uchar	address[LADDRESS];
	int	handle;
	PACKET	*pack;
	int	*stack;
	int	version;
	int	class;
	int	type;
	int	number;
	char	*name;
	int	basic;
};

static PORT	ports[] = {
	{0, pkint0},
	{0, pkint1},
	{0, pkint2},
	{0, pkint3}
};
#define	NDEV	(sizeof(ports)/sizeof(ports[0]))

static int	nports = 0;		/* number of active ports */

LOCAL_FUNC void FAR
SlpReceiver (Ushort FAR *p)
{
	PORT	*port;
	PACKET	*pack;
	char	*buff;

	st.flags1 |= SF_ASYNC;
	switch (p[RCV_AX]) {
	case 0:				/* Space allocate call */
		p[RCV_ES] = p[RCV_DI] = 0;
		if (p[RCV_ID] >= NDEV ||
		    p[RCV_CX] < 3 || p[RCV_CX] > (Ushort)PAKPACKLEN)
			goto badret;
		port = &ports[p[RCV_ID]];
		if (!(port->flags & POF_ON))
			goto badret;
		if (port->pack) {
			/* stats... */
			packet_del (port->pack);
			port->pack = 0;
		}
		if (F(pack = packet_new (p[RCV_CX], -1)))
			goto badret;
		port->pack = pack;
		pack->length = p[RCV_CX];
		buff = (char *)PHEAD;
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
		pack->netport = port->netport;
		pack->address = port->address;		/* from */

		packet_deliver (pack);
		port->pack = 0;
		break;
	default:
badret:
		++STATS_NETERRD;
		break;
	}
	st.flags1 &= ~SF_ASYNC;
}

LOCAL_FUNC int NEAR
SlpOptions (PORT *port, char *options)
{
	long	l;

	port->intno = get_niarg (options, 0, &l) ? -1 :  (int)l;

	return (0);
}

LOCAL_FUNC int FAR
SlpInit (NETPORT *np, char *options)
{
	int	portno, i;
	PORT	*port;
	Uchar	unit[2];

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV) {
		LogPrintf ("%s.%c: bad port\n",
			np->NetDriver->name, np->unit);
		return (1);
	}
	port = &ports[portno];
	if (port->flags & POF_ON) {
		LogPrintf ("%s: already on\n",
			port->address);
		return (1);
	}

	memset  (port->address, 0, LADDRESS);
	strncpy ((char *)port->address, np->NetDriver->name, LADDRESS);
	strncat ((char *)port->address, ".", LADDRESS);
	unit[0] = (Uchar)np->unit;
	unit[1] = '\0';
	strncat ((char *)port->address, (char *)unit, LADDRESS);

	if (SlpOptions (port, options))
		return (1);

	if (-1 == port->intno) {
		for (i = INT_FIRST; i <= INT_LAST; ++i) {
			if (test_for_pd (i)) {
				port->intno = i;
				break;
			}
		}
		if (-1 == port->intno) {
			LogPrintf ("%s: no driver found\n",
				port->address);
			return (1);
		}
	}
	if (!test_for_pd (port->intno)) {
		LogPrintf ("%s: no driver at 0x%x\n",
			port->address, port->intno);
		port->intno = -1;
		return (1);
	}
	LogPrintf ("%s: Intno 0x%x\n",
		port->address, port->intno);

	if (F(port->stack = (int *)memory_calloc (PKSSIZE,
						sizeof (*port->stack)))) {
		LogPrintf ("%s: no mem\n",
			port->address);
		return (1);
	}

	pkinit (portno, SlpReceiver, &port->stack[PKSSIZE]);

	port->handle = access_type (port->intno, CL_SERIAL_LINE, ANYTYPE, 0,
		(char FAR *)0, 0, port->pkint);
	if (-1 == port->handle) {
		LogPrintf ("%s: no handle\n",
			port->address);
		port->stack = memory_cfree (port->stack, PKSSIZE,
						sizeof (*port->stack));
		return (1);
	}
	if (driver_info (port->intno, port->handle, &port->version,
			&port->class, &port->type, &port->number, &port->name,
			&port->basic)) {
		port->basic = 1;	/* what else ? */
	}
	LogPrintf ("%s: Basic 0x%x\n",
		port->address, port->basic);

	port->flags |= POF_ON;
	port->netport = np->netport;
	port->pack = 0;
	++nports;

	LogPrintf ("%s: init ok\n",
		port->address);

	return (0);
}

LOCAL_FUNC void FAR
SlpTerm (NETPORT *np)
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
	port->flags = 0;
	port->stack = memory_cfree (port->stack, PKSSIZE,
						sizeof (*port->stack));

	LogPrintf ("%s.%c: term ok\n",
		np->NetDriver->name, np->unit);
}

LOCAL_FUNC int FAR
SlpSend (NETPORT *np, PACKET *pack)
{
	int	portno;
	PORT	*port;
	PACKET	*p;

	if (0 == pack)
		return (0);

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV)
		return (1);
	port = &ports[portno];
	if (!(port->flags & POF_ON))
		return (1);
	if (port->basic >= 5) {
		if (F(p = packet_new (-1, -1)))
			return (1);
		memcpy (p, pack, sizeof (*p));
		p->next = np->outgoing;
		np->outgoing = p;
		return (as_send_pkt (port->intno, (char *)PHEAD, p->length,
			pksends));
	} else
		return (send_pkt (port->intno, (char *)PHEAD, pack->length));
}

struct NetDriver NEAR NetSlip = {
	"SLIP",
	0,
	NULL,	/* extra */
	SlpInit,
	SlpTerm,
	SlpSend,
	0
};

