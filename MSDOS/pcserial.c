/* --------------------------------- pcserial.c ----------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for packet level exchanges (low level). It uses the serial ports
 * as the communications medium. It moves packets across the wires with some
 * minimal checking (magic number in header).
 * Note: 'tail' is not used for now. If async send is expected then one should
 * copy the packet to a NEW(pack) and add it to the tail of the outgoing list.
 * At the moment the original packet MUST BE PRESERVED (it is finaly released
 * by the sender [packet_send() in remote.c].
 * 'incoming' holds the partialy received packet which is delivered when fully
 * acquired [packet_deliver() in netport.c].
 *
 * 'send' should be able to return a failure.
 *
 * Options:
 *  0 speed	[9600-115200]
 *  1 parity	['E', 'O', 'N'] for [even, odd, none]
 *  2 bits	[5, 6, 7, 8]					must be 8!
 *  3 stop	[0, 1, 2] for [1, 1.5, 2]
 *  4 xmode	['i', 'o', 'b'] for [input, output, both]	do not use!
 *  5 isize	[input buffer size]
 *  6 osize	[output buffer size]
 *  - irq	[hardware IRQ used for this serial com port]
 *  - base	[hardware base address used for this serial com port]
*/

#include "fly.h"
#include "com.h"


#define MAGIC1	((Uchar)0x5a)
#define MAGIC2	((Uchar)0xa5)
#define	COMHDSZ	4		/* header size */

typedef struct port PORT;

struct port {
	int	comport;
	long	speed;
	int	parity;
	int	bits;
	int	stop;
	int	xmode;
	int	isize;
	int	osize;
	int	irq;
	int	base;
	int	flags;
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
	{1, 115200L, 'N', 8, 1, 0, 4096, 0, -1, -1},
	{2, 115200L, 'N', 8, 1, 0, 4096, 0, -1, -1},
	{3, 115200L, 'N', 8, 1, 0, 4096, 0, -1, -1},
	{4, 115200L, 'N', 8, 1, 0, 4096, 0, -1, -1}
};
#define	NPORTS	(sizeof(ports)/sizeof(ports[0]))

static int	nports = 0;		/* active com ports */

LOCAL_FUNC int NEAR
SerOptions (PORT *port, char *options)
{
	char	*p;
	long	l;

	if (!get_niarg (options, 0, &l))
		port->speed = l;
	if (T(p = get_piarg (options, 1)))
		port->parity = *p;
	if (!get_niarg (options, 2, &l))
		port->bits = (int)l;
	if (!get_niarg (options, 3, &l))
		port->stop = (int)l;
	if (T(p = get_piarg (options, 4))) {
		if (*p == 'i')
			port->xmode = Xin;
		else if (*p == 'o')
			port->xmode = Xout;
		else if (*p == 'b')
			port->xmode = Xin|Xout;
		else if (*p == ':')
			;
		else {
			MsgEPrintf (-100, "%s: bad Xon option", port->address);
			return (1);
		}
	}
	if (!get_niarg (options, 5, &l))
		port->isize = (int)l;
	if (!get_niarg (options, 6, &l))
		port->osize = (int)l;
	port->irq  = get_narg (options, "irq",  &l) ? -1 : (int)l;
	port->base = get_narg (options, "base", &l) ? -1 : (int)l;

	return (0);
}

LOCAL_FUNC int FAR
SerInit (NETPORT *np, char *options)	/* init a physical com port */
{
	int	portno;
	PORT	*port;
	Uchar	unit[2];

	portno = np->unit-'1';
	if (portno < 0 || portno > 3) {
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

	if (SerOptions (port, options))
		return (1);

	if (!nports++)
		if (com_start (-1, 0L, 0, 0, 0, 0)) {
			MsgEPrintf (-100, "%s: driver failed", port->address);
			return (1);
		}

	if (port_set (port->comport, port->isize, port->osize, port->irq,
			port->base)) {
		if (!--nports)
			com_stop (-1);
		MsgEPrintf (-100, "%s: port set failed", port->address);
		return (1);
	}
	if (port_start (port->comport, port->speed, port->parity,
			port->bits, port->stop, port->xmode)) {
		if (!--nports)
			com_stop (-1);
		MsgEPrintf (-100, "%s: port start failed", port->address);
		return (1);
	}
	port->flags = 1;
	port->netport = np->netport;

	return (0);
}

LOCAL_FUNC void FAR
SerTerm (NETPORT *np)			/* term a physical port */
{
	PACKET	*p;
	int	portno;
	PORT	*port;

	portno = np->unit-'1';
	if (portno < 0 || portno > 3)
		return;
	port = &ports[portno];
	if (!port->flags)
		return;

	port_stop (port->comport);
	port->flags = 0;

	for (p = port->outgoing; p;)
		p = packet_del (p);
	port->outgoing = 0;
	port->tail = 0;

	if (port->incoming) {
		packet_del (port->incoming);
		port->incoming = 0;
	}

	if (!--nports)
		com_stop (-1);
}

#if 0

/* This function sends out a packet syncronously one byte at a time with
 * retries.
*/
LOCAL_FUNC int  FAR
SerOut (PORT *port)
{
	PACKET	*pack;
	int	ch;
	int	to;

	to = 0;
	while (pack = port->outgoing) {		/* block while sending */
		if (++to > 1000) {
			MsgEPrintf (10, "T/O com.%d", port->comport);
			flushccb (port->comport);
			port->outgoing = 0;
			return (1);
		}
		if (pack->flags < -COMHDSZ) {
			port->head[0] = MAGIC1;
			port->head[1] = MAGIC2;
			port->head[2] = (Uchar)0x00ff&(pack->length >>8)
			port->head[3] = (Uchar)0x00ff&(pack->length);
			++pack->flags;
		}
		if (pack->flags < 0)
			ch = port->head[COMHDSZ+pack->flags];
		else
			ch = pack->raw[pack->flags];
		if (!comout (port->comport, ch)) {
			to = 0;
			if (++pack->flags == pack->length) {
#if 0
				if (!(port->outgoing = packet_del (pack))
					port->tail = 0;
#else
				port->outgoing = 0;
#endif
			}
		}
	}
	return (0);
}

#endif

LOCAL_FUNC void  NEAR
SerStats (int comport, int which)
{
	int	c, i, info[3];

	get_ccbinfo (comport, info);
	if (which) {
		c = info[1];	/* output buffer size */
		i = 3;
	} else {
		c = info[0];	/* input  buffer size */
		i = 0;
	}

	++STATS_NETERRLOW[i];
	if (c > (int)STATS_NETERRLOW[i+1])
		STATS_NETERRLOW[i+1] += c;
	STATS_NETERRLOW[i+2] += c;
}

LOCAL_FUNC int FAR
SerSend (NETPORT *np, PACKET *pack)
{
	int	portno, ret;
	PORT	*port;
	Uchar	header[COMHDSZ];

	if (0 == pack)
		return (0);

	portno = np->unit-'1';
	if (portno < 0 || portno > 3)
		return (1);
	port = &ports[portno];
	if (!port->flags)
		return (1);

#if 0
	pack->flags = -COMHDSZ - 1;
	pack->next = 0;
	if (port->tail)
		port->tail->next = pack;
	else
		port->outgoing = pack;
	port->tail = pack;
	return (SerOut (port));
#else

	header[0] = MAGIC1;
	header[1] = MAGIC2;
	header[2] = (Uchar)(0x00ff&(pack->length >>8));
	header[3] = (Uchar)(0x00ff&(pack->length));
	ret = comoutb (port->comport, header, sizeof (header)) ||
	      comoutb (port->comport, pack->raw, pack->length);
	SerStats (port->comport, 1);	/* fullest output buffers about now */
	return (ret);
#endif
}

LOCAL_FUNC int FAR
SerReceive (NETPORT *np, int poll)
{
	int	ch, portno, len;
	PACKET	*pack;
	PORT	*port;

	portno = np->unit-'1';
	if (portno < 0 || portno > 3)
		return (1);
	port = &ports[portno];
	if (!port->flags)
		return (1);

	SerStats (port->comport, 0);
	while ((ch = comin (port->comport)) != -1) {
		port->last_in = st.present;
		pack = port->incoming;
		if (!pack) {
			if (port->panic) {	/* drop rest of packet */
				--port->panic;
				continue;
			}
			if (F(pack = packet_new (PAKPACKLEN, -1))) {
				port->panic = (0x00ff & ch) - 1; /* length */
				continue;
			} 
			port->incoming = pack;
			pack->flags = (Ushort)-COMHDSZ;
			pack->next = 0;
		}
		if ((short)pack->flags < 0) {
			pack->raw[COMHDSZ+pack->flags++] = (Uchar)ch;
			if (pack->flags == (Ushort)(1-COMHDSZ) &&
			     pack->raw[0] != MAGIC1 ||
			    pack->flags == (Ushort)(2-COMHDSZ) &&
			     pack->raw[1] != MAGIC2 ||
			    pack->flags == (Ushort)(4-COMHDSZ) &&
			     ((len = (pack->raw[2]<<8)+pack->raw[3]) < 2 ||
			      len > PAKPACKLEN)) {
				++STATS_NETERRD;
				pack->flags = (Ushort)-COMHDSZ;	/* restart */
			} else if (pack->flags == 4-COMHDSZ)
				pack->length = (short)len;
			continue;
		}
		pack->raw[pack->flags++] = (Uchar)ch;
		if (pack->flags == (Ushort)pack->length) {
#if 0
			pack->netport = port->netport;
			pack->address = port->address;
			packet_deliver (pack);
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
			pack->flags = (Ushort)-COMHDSZ;	/* reuse packet */
#endif
		}
	}
	return (0);
}

struct NetDriver NEAR NetCom = {
	"COM",
	0,
	NULL,	/* extra */
	SerInit,
	SerTerm,
	SerSend,
	SerReceive
};
