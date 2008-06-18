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
 *  int=	interrupt number (default auto detect)
 *  type=	packet type (default is FLY8_ETYPE=0xf8f8)
 *  nbufs=	number if recv buffs (default NRBUFS=20)
 *  rsize=	max size of recv packets (default RBUFSIZE=1500)
 *  ssize=	max size of send packets (default SBUFSIZE=1500)
 *  recvrate=	millis between recv polls (default RECVRATE=5)
*/

#include "fly.h"
#include "djgpp.h"


#define RECVRATE	5		/* default recv rate, millis */
#define NRBUFS		20		/* default no of recv buffers */
#define RBUFSIZE	1500		/* default size of recv buffer */
#define SBUFSIZE	1500		/* default size of send buffer */
#define	FLY8_ETYPE	0xf8f8		/* default ether packet type */

#define SIZEPAR(x)	(((x)+15) >> 4)

#define INT_FIRST	0x60		/* auto detect interrupt range */
#define INT_LAST	0x7f

#define PD_ETHER	1
#define PD_SLIP		6

#define PD_DRIVER_INFO	0x1ff
#define PD_ACCESS	0x200
#define PD_RELEASE	0x300
#define PD_SEND		0x400
#define PD_GET_ADDRESS	0x600
#define CARRY		1		/* carry bit in flags register */


#define MACADDRESS	6

#define ETHDEST		0		/* ethernet header */
#define ETHSRCE		(ETHDEST + MACADDRESS)
#define ETHTYPE		(ETHSRCE + MACADDRESS)
#define ETHNNN		(ETHTYPE + 2)

#define APADDRESS	MACADDRESS

#define APHDEST		0		/* Fly8 header */
#define APHSRCE		(APHDEST + APADDRESS)
#define APHLEN		(APHSRCE + APADDRESS)
#define APHNNN		(APHLEN  + 2)

typedef struct port PORT;
struct port {
	int		flags;
#define POF_ON		0x0001
#define POF_SEGINFO	0x0002
#define POF_PKT		0x0004
#define POF_ALL		(POF_SEGINFO|POF_PKT)
	int		iport;
	struct NetDriver *driver;
	short		netport;		/* back-pointer */
	int		intno;			/* packet-driver interrupt */
	int		version;
	Ushort		class;
	int		type;
	int		number;
	char		*name;
	int		basic;
	Ushort		recv_rate;	/* millisecs between rcv polls */
	Ulong		next_recv;	/* next scheduled recv */
	int		nrbufs;
	int		rbufsize;
	int		sbufsize;
	char		*rbufs_inuse;
	int		rbuf;
	int		sbuf;
	int		rbuflin;
	int		sbuflin;
	Ushort		pktipofs;	/* offset from header to start of pkt */
	_go32_dpmi_seginfo	seg_info;
	__dpmi_regs	regs;
	Uchar		mac[MACADDRESS];	/* my MAC address */
	Uchar		addr[APADDRESS];	/* my MAC address */

	Uchar		etype[2];		/* eth_type for fly8 */
	Ushort		pkt_handle;
};

static PORT	ports[] = {
	{0, 1},
	{0, 2},
	{0, 3},
	{0, 4}
};
#define	NDEV	(rangeof(ports))

static int	nports = 0;		/* number of active ports */

/* This is the real mode packet driver interrupt handler.
*/
static Uchar pktasm[] = 
#include "pktasm.inc"
;

static char pkt_line[] = "PKT DRVR";


LOCAL_FUNC void FAR	PktTerm (NETPORT *np);
LOCAL_FUNC int FAR	PktInit (NETPORT *np, char *options);
LOCAL_FUNC int FAR	PktSendPD (PORT *port, char *buffer, int length);
LOCAL_FUNC void FAR	PktReceivePD (PORT *port);

LOCAL_FUNC int FAR	PktPoll (NETPORT *np, int poll);
LOCAL_FUNC int NEAR	PktOptions (PORT *port, char *options);

LOCAL_FUNC int NEAR	PktReceiveETH (PORT *port, PACKET *pack);
LOCAL_FUNC int NEAR	PktSendETH (PORT *port, PACKET *p, Uchar *h, int len,
	Uchar *ether_type);
LOCAL_FUNC int NEAR	PktSendAP (PORT *port, PACKET *p, Uchar *h, int len);
LOCAL_FUNC int FAR	PktSend (NETPORT *np, PACKET *p);



LOCAL_FUNC void FAR
PktTerm (NETPORT *np)
{
	int	portno;
	PORT	*port;

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV)
		return;
	port = &ports[portno];

	if (port->flags & POF_PKT) {
		clear_dpmi_regs (&port->regs);
		port->regs.x.ax = PD_RELEASE;
		port->regs.x.bx = port->pkt_handle;
		__dpmi_int (port->intno, &port->regs);
		if (port->regs.x.flags & CARRY)
			LogPrintf ("%s.%u %s\n",
				port->driver->name, port->iport,
				"ERROR releasing packet driver for IP");
		port->flags &= ~POF_PKT;
	}

	if (port->flags & POF_SEGINFO) {
		_go32_dpmi_free_dos_memory (&port->seg_info);
		port->flags &= ~POF_SEGINFO;
	}

	if (port->rbufs_inuse)
		port->rbufs_inuse = memory_free (port->rbufs_inuse,
			port->nrbufs);

	if (port->flags & POF_ON) {
		port->flags &= ~POF_ON;
		LogPrintf ("%s.%c: term ok\n",
			np->NetDriver->name, np->unit);
	}
}

LOCAL_FUNC int FAR
PktInit (NETPORT *np, char *options)
{
	int	portno;
	PORT	*port;
	Ushort	tf, tt;
	int	tbuf;
	int	adrlin;
	int	class;
	int	i;
	Ushort	wbuf;
	Ulong	l;
	char	temp[sizeof (pkt_line)-1];

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

	port->flags &= ~POF_ALL;

	port->driver = np->NetDriver;
	port->netport = np->netport;

	if (PktOptions (port, options))
		goto abort;

/* Find a packet driver
*/
	if (-1 == port->intno) {
		tf = INT_FIRST;
		tt = INT_LAST;
	} else
		tf = tt = port->intno;
	port->intno = -1;

	for (; tf <= tt; ++tf) {
		_go32_dpmi_get_real_mode_interrupt_vector (tf,
			&port->seg_info);
		adrlin = (port->seg_info.rm_segment << 4)
						+ port->seg_info.rm_offset;
		if (adrlin) {
			dosmemget (adrlin+3, strlen (pkt_line), temp);
			if (!memcmp (temp, pkt_line, strlen (pkt_line))) {
				port->intno = tf;
				break;
			}
		}
	}

	if (-1 == port->intno) {
		LogPrintf ("%s.%c: no driver\n",
			np->NetDriver->name, np->unit);
		return (1);
	}
	LogPrintf ("%s.%c: Intno 0x%x\n",
		np->NetDriver->name, np->unit,
		port->intno);

/* allocate memory
*/
	if (F(port->rbufs_inuse = (char *)memory_alloc (port->nrbufs))) {
		LogPrintf ("%s.%c: no 'inuse' mem\n",
			np->NetDriver->name, np->unit);
		goto abort;
	}

	l =	  SIZEPAR (sizeof(pktasm))		/* code  */
		+ SIZEPAR (port->nrbufs * (port->rbufsize+3))	/* recv bufor */
		+ SIZEPAR (SBUFSIZE)			/* send bufor */
		+ 1;					/* type info */
	if (l >= 0x10000) {
		LogPrintf ("%s.%c: %s\n",
			np->NetDriver->name, np->unit,
			"callot allocate more than 64k for bufs");
		goto abort;
	}

	port->seg_info.size = l;
	if (_go32_dpmi_allocate_dos_memory (&port->seg_info)) {
		LogPrintf ("%s.%c: %s\n",
			np->NetDriver->name, np->unit,
			"Not enought Dos memory for packet driver bufor");
		goto abort;
	}
	port->flags |= POF_SEGINFO;

/* Set up relative offset of the different parts.
*/
	port->rbuf = SIZEPAR(sizeof(pktasm)) << 4;
	port->sbuf = port->rbuf + (SIZEPAR(port->nrbufs*(port->rbufsize+3)) << 4);
	tbuf = port->sbuf + (SIZEPAR(SBUFSIZE) << 4);

/* Linear address of the same parts.
*/
	adrlin = port->seg_info.rm_segment << 4;
	port->rbuflin = adrlin + port->rbuf;
	port->sbuflin = adrlin + port->sbuf;

/* Set up interrupt handler info in handler header.
*/
	*((Ushort *)(pktasm+0)) = port->nrbufs;
	*((Ushort *)(pktasm+2)) = port->rbufsize;
	*((Ushort *)(pktasm+4)) = port->rbuf;
	*((Ushort *)(pktasm+6)) = port->rbuf+port->nrbufs;
	dosmemput (pktasm, sizeof (pktasm), adrlin);

/* Zero out in-use flags (1 byte for each recv buff).
*/
	memset (port->rbufs_inuse, 0, port->nrbufs);
	dosmemput (port->rbufs_inuse, port->nrbufs, port->rbuflin);

/* Fill in recv buffs structures.
*/
	adrlin = port->rbuflin + port->nrbufs;
	for (i = 0; i < port->nrbufs; i++) {
		wbuf = port->rbuf + i;
		dosmemput (&wbuf, 2, adrlin);
		adrlin += port->rbufsize+2;
	}

/* Fill in ether-type info.
*/
	adrlin = (port->seg_info.rm_segment << 4) + tbuf;
	dosmemput (&port->etype, 2, adrlin);


/* Find out about the driver
*/
	clear_dpmi_regs (&port->regs);
	port->regs.x.ax = PD_DRIVER_INFO;
	__dpmi_int (port->intno, &port->regs);

/* Handle old versions, assume a class and just keep trying
*/
	if (port->regs.x.flags & CARRY) {
		for (class = 0; class < 2; ++class) {
			port->class = (class) ? PD_SLIP : PD_ETHER;
			for (port->type = 1; port->type < 128; ++port->type) {
				clear_dpmi_regs (&port->regs);
				port->regs.x.ax = PD_ACCESS | port->class; /* ETH, SLIP */
				port->regs.x.bx = port->type;	/* type */
				port->regs.x.dx = 0;	/* if number */
				port->regs.x.cx = (port->class == PD_SLIP)
						? 0 : sizeof (port->etype);
				port->regs.x.ds = port->seg_info.rm_segment;
				port->regs.x.si = tbuf;
				port->regs.x.es = port->seg_info.rm_segment;
				port->regs.x.di = 8;	/* begin of code */
				__dpmi_int (port->intno,
					&port->regs);
				if (!(port->regs.x.flags & CARRY)) {
					i = port->regs.x.ax;	/* handle */
					break;
				}
			}
			if (port->type == 128) {
				LogPrintf ("%s.%u: %s\n",
					port->driver->name, port->iport,
					"ERROR initializing packet driver");
				goto abort;
			}

/* Found a working type, release it
*/
			clear_dpmi_regs (&port->regs);
			port->regs.x.ax = PD_RELEASE;
			port->regs.x.bx = i;		/* handle */
			__dpmi_int (port->intno, &port->regs);
		}
		port->basic = 1;	/* what else ? */
	} else {
		port->version = port->regs.x.bx;
		port->class   = port->regs.h.ch;
		port->type    = port->regs.x.dx;
		port->number  = port->regs.h.cl;
		port->basic   = port->regs.h.al;

		switch (port->class) {
		case PD_ETHER :
			port->pktipofs = 14;
			break;
		case PD_SLIP :
			break;
		default :
			LogPrintf ("%s.%u: %s%s\n",
				port->driver->name, port->iport,
				"ERROR: only Ethernet or SLIP packet drivers",
				" allowed");
			goto abort;
		}
	}

	LogPrintf ("%s.%c: Basic 0x%x\n",
		np->NetDriver->name, np->unit,
		port->basic);

/* Register the receiver for our packet type.
*/
	clear_dpmi_regs (&port->regs);
	port->regs.x.ax = PD_ACCESS | port->class;
	port->regs.x.bx = 0xffff;			/* any type */
	port->regs.x.dx = 0;				/* if number */
	port->regs.x.cx = (port->class == PD_SLIP) ? 0 : sizeof (port->etype);
	port->regs.x.ds = port->seg_info.rm_segment;	/* type */
	port->regs.x.si = tbuf;
	port->regs.x.es = port->seg_info.rm_segment;	/* receiver */
	port->regs.x.di = 8;				/* entry point */

	__dpmi_int (port->intno, &port->regs);
	if (port->regs.x.flags & CARRY) {
		LogPrintf ("%s.%u: Error 0x%x %s\n",
			port->driver->name, port->iport,
			port->regs.x.dx >> 8,
			"accessing packet driver");
		goto abort;
	}
	port->pkt_handle = port->regs.x.ax;
	port->flags |= POF_PKT;

	port->flags |= POF_ON;
	++nports;

/* Get ethernet address (MAC)
*/
	clear_dpmi_regs (&port->regs);
	port->regs.x.ax = PD_GET_ADDRESS;
	port->regs.x.bx = port->pkt_handle;
	port->regs.x.es = port->seg_info.rm_segment;
	port->regs.x.di = tbuf+4;
	port->regs.x.cx = sizeof (port->mac);
	__dpmi_int (port->intno, &port->regs);

	if (port->regs.x.flags & CARRY) {
		LogPrintf ("%s.%c: my MAC  failed %0x\n",
			np->NetDriver->name, np->unit,
			(int)port->regs.h.dh);
		memset (port->mac, 0, sizeof (port->mac));
	} else {
		adrlin = (port->seg_info.rm_segment << 4) + tbuf+4;
		dosmemget (adrlin, sizeof (port->mac), port->mac);

		LogPrintf ("%s.%c: my  MAC  %02x%02x%02x-%02x%02x%02x\n",
			np->NetDriver->name, np->unit,
			port->mac[0], port->mac[1], port->mac[2],
			port->mac[3], port->mac[4], port->mac[5]);
	}
	memcpy (port->addr, port->mac, MACADDRESS);

	port->next_recv = st.present + port->recv_rate;

	LogPrintf ("%s.%c: init ok\n",
		np->NetDriver->name, np->unit);

	return (0);

abort:
	PktTerm (np);
	return (1);
}

LOCAL_FUNC int FAR
PktSendPD (PORT *port, char *buffer, int length)
{
	if (length > SBUFSIZE)
		return (1);
	dosmemput (buffer, length, port->sbuflin);

	clear_dpmi_regs (&port->regs);
	port->regs.x.ax = PD_SEND;
	port->regs.x.ds = port->seg_info.rm_segment;
	port->regs.x.si = port->sbuf;
	port->regs.x.cx = length;
	__dpmi_int (port->intno, &port->regs);
	if (port->regs.x.flags & CARRY)
		return (1);

	return (0);
}

LOCAL_FUNC void NEAR
PktReceivePD (PORT *port)
{
	int	i;
	int	n;
	PACKET	*pack;
	int	adrlin;

	dosmemget (port->rbuflin, port->nrbufs, port->rbufs_inuse);

	for (n = i = 0, adrlin = port->rbuflin + port->nrbufs + 2;
			i < port->nrbufs;
			i++, adrlin += port->rbufsize+2) {
		if (!port->rbufs_inuse[i])
			continue;

		if (T(pack = packet_new (port->rbufsize, 0))) {
			pack->netport = port->netport;
			dosmemget (adrlin, port->rbufsize, pack->data);

			switch (PktReceiveETH (port, pack)) {
			case 0:			/* packet accepted */
				break;
			case 1:			/* packet rejected */
				packet_del (pack);
				break;
			default:		/* packet error */
				packet_del (pack);
				++STATS_NETERRD;
				break;
			}
		} else
			++STATS_NETERRD;
		port->rbufs_inuse[i] = 0;	/* release buffer */
		++n;
	}

	if (n)
		dosmemput (port->rbufs_inuse, port->nrbufs, port->rbuflin);
}


/* Do some housekeeping. This is necessary since we try to absolutely
 * minimize the work done in the packet receiver routine.
*/
LOCAL_FUNC int FAR
PktPoll (NETPORT *np, int poll)
{
	int	portno;
	PORT	*port;

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV)
		return (1);
	port = &ports[portno];
	if (!(port->flags & POF_ON))
		return (1);

/* empty receiver buffer.
*/
	if (!poll || port->next_recv <= st.present) {
		PktReceivePD (port);
		port->next_recv = st.present + port->recv_rate;
	}

	return (0);
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

	if (get_narg (options, "nbufs=", &l))
		l = NRBUFS;
	port->nrbufs = (int)l;

	if (get_narg (options, "rsize=", &l))
		l = RBUFSIZE;
	port->rbufsize = (int)l;

	if (get_narg (options, "ssize=", &l))
		l = SBUFSIZE;
	port->sbufsize = (int)l;

	if (get_narg (options, "recvrate=", &l))
		l = RECVRATE;
	port->recv_rate = (Ushort)l;

	if (get_narg (options, "type=", &l))
		l = FLY8_ETYPE;
	ComPBw (port->etype, (int)l);

	return (0);
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

	return (PktSendPD (port, (char *)h, len));
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

	PktPoll (np, 1);
	return (ret);
}

struct NetDriver NEAR NetPKT = {
	"PKT",
	0,
	NULL,	/* extra */
	PktInit,
	PktTerm,
	PktSend,
	PktPoll
};
