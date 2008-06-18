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
 *   int=	interrupt number (default auto detect)
 *   nbufs=	number if recv buffs (default NRBUFS=20)
 *   rsize=	max size of recv packets (default RBUFSIZE=1500)
 *   ssize=	max size of send packets (default SBUFSIZE=1500)
 *   recvrate=	millis between recv polls (default RECVRATE=5)
 *  +sip=	server IP address
 *   sport=	server UDP port (default FLY8_SPORT=0xf8f8)
 *  +ip=	my IP address
 *   port=	my UDP port (default FLY8_PORT=0xf8f9)
 *
 * When we receive a packet, it is put into ->data, then the logic determines
 * where ->raw is.
 *
 * When we send a packet, the data is in ->raw and the protocol determines
 * where the header starts.
*/

#include "fly.h"
#include "djgpp.h"
#include "pcudp.h"

/* chksum.x */
extern int	FAR chksum (Uchar *buf, int len);


#define RECVRATE	5		/* default recv rate, millis */
#define NRBUFS		20		/* default no of recv buffers */
#define RBUFSIZE	1500		/* default size of recv buffer */
#define SBUFSIZE	1500		/* default size of send buffer */

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


typedef struct port	PORT;
struct port {
	int		flags;
#define POF_ON		0x0001
#define POF_SEGINFO	0x0002
#define POF_IP		0x0004
#define POF_ARP		0x0008
#define POF_ALL		(POF_SEGINFO|POF_IP|POF_ARP)
	int		iport;
	struct NetDriver *driver;
	short		netport;
	int		intno;			/* packet-driver interrupt */
	int		version;
	Ushort		class;
	int		type;
	int		number;
	char		*name;		/* not filled-in yet! */
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
	Uchar		addr[APADDRESS];	/* my Fly8 address */

	Uchar		ip[IPADDRESS];		/* my IP address */
	Uchar		udport[UDPADDRESS];	/* my UDP port */
	PACKET		*head;			/* outgoing packets */
	PACKET		*tail;
	Ushort		ip_handle;
	Ushort		arp_handle;
};

static PORT	ports[] = {
	{0, 1},
	{0, 2},
	{0, 3},
	{0, 4}
};
#define	NDEV	(rangeof(ports))	/* number of available ports */

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

/* This is the real mode packet driver interrupt handler.
*/
static Uchar pktasm[] = 
#include "pktasm.inc"
;

static char pkt_line[] = "PKT DRVR";


LOCAL_FUNC void FAR	PuTerm (NETPORT *np);
LOCAL_FUNC int FAR	PuInit (NETPORT *np, char *options);
LOCAL_FUNC int FAR	PuSendPD (PORT *port, char *buffer, int length);
LOCAL_FUNC void FAR	PuReceivePD (PORT *port);

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


#if 0

LOCAL_FUNC void FAR
hex_buf (PORT *port, char *title, Uchar *p, int len)
{
	int	i;

	LogPrintf ("%s.%u: %s",
		port->driver->name, port->iport, title);
	for (i = 0; i < len; ++i) {
		if (!(i%32))
			LogPrintf ("\n  ");
		else if (!(i%4))
			LogPrintf (" ");
		LogPrintf ("%02x", *p++);
	}
	LogPrintf ("\n");
}
#endif


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

	if (port->flags & POF_ARP) {
		clear_dpmi_regs (&port->regs);
		port->regs.x.ax = PD_RELEASE;
		port->regs.x.bx = port->arp_handle;
		__dpmi_int (port->intno, &port->regs);
		if (port->regs.x.flags & CARRY)
			LogPrintf ("%s.%u: %s\n",
				port->driver->name, port->iport,
				"ERROR releasing packet driver for ARP");
		port->flags &= ~POF_ARP;
	}

	if (port->flags & POF_IP) {
		clear_dpmi_regs (&port->regs);
		port->regs.x.ax = PD_RELEASE;
		port->regs.x.bx = port->ip_handle;
		__dpmi_int (port->intno, &port->regs);
		if (port->regs.x.flags & CARRY)
			LogPrintf ("%s.%u: ERROR releasing packet driver for IP\n",
				port->driver->name, port->iport);
		port->flags &= ~POF_IP;
	}

	if (port->flags & POF_SEGINFO) {
		_go32_dpmi_free_dos_memory (&port->seg_info);
		port->flags &= ~POF_SEGINFO;
	}

	if (port->rbufs_inuse)
		port->rbufs_inuse = memory_free (port->rbufs_inuse,
			port->nrbufs);

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

	if (port->flags & POF_ON) {
		port->flags &= ~POF_ON;
		LogPrintf ("%s.%c: term ok\n",
			np->NetDriver->name, np->unit);
	}
}

/* Called to initialize this driver.
*/
LOCAL_FUNC int FAR
PuInit (NETPORT *np, char *options)
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
	__dpmi_regs	reg2;

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

	if (PuOptions (port, options))
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

	l =	  SIZEPAR (sizeof(pktasm))		/* code */
		+ SIZEPAR (port->nrbufs * (port->rbufsize+3))	/* recv bufor */
		+ SIZEPAR (port->sbufsize)			/* send bufor */
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
	tbuf = port->sbuf + (SIZEPAR(port->sbufsize) << 4);

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
	dosmemput (&ether_ip,  2, adrlin);
	dosmemput (&ether_arp, 2, adrlin+2);

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
						? 0 : sizeof (ether_ip);
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

/* Register the receiver for IP.
*/
	clear_dpmi_regs (&port->regs);
	port->regs.x.ax = PD_ACCESS | port->class;
	port->regs.x.bx = 0xffff;			/* any type */
	port->regs.x.dx = 0;				/* if number */
	port->regs.x.cx = (port->class == PD_SLIP) ? 0 : sizeof (ether_ip);
	port->regs.x.ds = port->seg_info.rm_segment;	/* type */
	port->regs.x.si = tbuf;
	port->regs.x.es = port->seg_info.rm_segment;	/* receiver */
	port->regs.x.di = 8;				/* begin of code */

	memcpy (&reg2, &port->regs, sizeof (port->regs));
	reg2.x.si = tbuf+2;

	__dpmi_int (port->intno, &port->regs);
	if (port->regs.x.flags & CARRY) {
		LogPrintf ("%s.%u: Error 0x%x %s\n",
			port->driver->name, port->iport,
			port->regs.x.dx >> 8,
			"accessing packet driver");
		goto abort;
	}
	port->ip_handle = port->regs.x.ax;
	port->flags |= POF_IP;

/* Register the receiver for ARP.
*/
	if (port->class != PD_SLIP) {
		__dpmi_int (port->intno, &reg2);
		if (reg2.x.flags & CARRY) {
			LogPrintf ("%s.%u: Error 0x%x %s\n",
				port->driver->name, port->iport,
				reg2.x.dx >> 8,
				"accessing packet driver");
			goto abort;
		}
		port->arp_handle = reg2.x.ax;
		port->flags |= POF_ARP;
	}

	port->flags |= POF_ON;
	++nports;

/* Get ethernet address (MAC)
*/
	clear_dpmi_regs (&port->regs);
	port->regs.x.ax = PD_GET_ADDRESS;
	port->regs.x.bx = port->ip_handle;
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
		MsgWPrintf (-100, "%s.%c: my  IP   %u.%u.%u.%u",
			np->NetDriver->name, np->unit,
			port->ip[0], port->ip[1], port->ip[2], port->ip[3]);
		LogPrintf ("%s.%c: my  port %02x%02x\n",
			np->NetDriver->name, np->unit,
			port->udport[0], port->udport[1]);
	}

	memcpy (svr_addr,             svr_ip,       IPADDRESS);
	memcpy (svr_addr+IPADDRESS,   svr_udport,   UDPADDRESS);

	memcpy (port->addr,           port->ip,     IPADDRESS);
	memcpy (port->addr+IPADDRESS, port->udport, UDPADDRESS);

	memset (svr_mac,              0xff,         MACADDRESS);

	have_svr = 0;
	next_query = st.present;	/* query immediately */

	port->next_recv = st.present + port->recv_rate;

	LogPrintf ("%s.%c: init ok\n",
		np->NetDriver->name, np->unit);

	return (0);

abort:
	PuTerm (np);
	return (1);
}

LOCAL_FUNC int FAR
PuSendPD (PORT *port, char *buffer, int length)
{
	if (length > port->sbufsize)
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
PuReceivePD (PORT *port)
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

			switch (PuReceiveETH (port, pack)) {
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
PuPoll (NETPORT *np, int poll)
{
	int	portno;
	PORT	*port;
	PACKET	*pack;
	PACKET	*p;
	Ulong	flags;
	Uchar	*h;
	int	ret;

	portno = np->unit-'1';
	if (portno < 0 || portno >= NDEV)
		return (1);
	port = &ports[portno];
	if (!(port->flags & POF_ON))
		return (1);

/* empty receiver buffer.
*/
	if (!poll || port->next_recv <= st.present) {
		PuReceivePD (port);
		port->next_recv = st.present + port->recv_rate;
	}

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

/* Build a pseudo header sor the checksum.
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
