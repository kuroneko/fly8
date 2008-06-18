/* --------------------------------- netmgr.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Manage network ports (devices, cards, whatever you want to call these)
 * Only one port is available on most machines.
*/

#include "fly.h"


static NETPORT	*netports = 0;
static int	nnetports = 0;

LOCAL_FUNC int NEAR
packet_xmit (PACKET *pack, NETPORT *np)
{
	if ((np->NetDriver)->Send (np, pack)) {
		if (pack)
			++STATS_NETERRSEND;
		return (1);
	}
	if (pack) {
		STATS_NETSENTBYTES += pack->length;
		++STATS_NETSENTPACKETS;
	}
	return (0);
}

LOCAL_FUNC void NEAR
netport_term (int n)
{
	NETPORT		*np;
	PACKET		*pack;

	np = &netports[n];
	if (!(np->flags & NP_ON))
		return;
	if (np->outpak) {
		if (np->outpak->length > 1)
			packet_xmit (np->outpak, np);	/* flush packed */
		packet_del (np->outpak);
		np->outpak = 0;
	}
	(np->NetDriver)->Term (np);
	np->flags = 0;
	np->NetDriver = 0;

	for (pack = np->incoming[HEAD]; pack;)
		pack = packet_del (pack);
	np->incoming[HEAD] = np->incoming[TAIL] = 0;

	for (pack = np->outgoing; pack;)
		pack = packet_del (pack);
	np->outgoing = 0;
}

extern void FAR
netports_term (void)
{
	int	n;

	for (n = 0; n < nnetports; ++n)
		netport_term (n);

	netports = memory_cfree (netports, nnetports, sizeof (*netports));
	nnetports = 0;
}

LOCAL_FUNC int NEAR
netport_init (int n, char *ndname)
{
	NETPORT			*np;
	struct NetDriver	NEAR* FAR* nd;
	char			*options, *p;
	int			nlen;
	int			unit;
	int			i;

	if (n >= nnetports) {
		MsgEPrintf (-100, "netport: bad port %d", n);
		return (1);
	}

	if (F(nlen = strlen (ndname))) {
		MsgEPrintf (-100, "netport: no name");
		return (1);
	}

	unit = 0;
	options = strchr (ndname, '.');
	if (options) {
		nlen = options - ndname;
		unit = options[1];
		if (!nlen || !unit) {
			MsgEPrintf (-100, "netport: bad name %s", ndname);
			return (1);
		}
		options += 2;
	} else
		options = ndname;

	if (T(options = strchr (options, ':'))) {
		if (!nlen) {
			nlen = options - ndname;
			if (!nlen) {
				MsgEPrintf (-100, "netport: bad options %s",
					ndname);
				return (1);
			}
		}
	}

	for (nd = NetDrivers; *nd; ++nd) {
		if (!strnicmp ((*nd)->name, ndname, nlen) && !(*nd)->name[nlen])
			break;
	}
	if (!(*nd)) {
		MsgEPrintf (-100, "netport: unknown driver %s", ndname);
		return (1);
	}

	np = &netports[n];
	np->flags = 0;
	np->netport = n;
	np->unit = (char)unit;
	np->NetDriver = *nd;
	np->incoming[HEAD] = np->incoming[TAIL] = 0;

	MsgPrintf (-100, "Netport  %s.%c", (*nd)->name, (int)np->unit);

	if ((*nd)->Init (np, options))
		return (1);
	np->flags |= NP_ON;
	if (T(p = get_parg (options, "pack"))) {
		np->flags |= NP_PACKING;
		np->packlen = PAKPACKLEN;
		if ('=' == *p) {
			if (get_int (p+1, &i))
				MsgEPrintf (-100, "%s.%c bad 'pack' option",
						(*nd)->name, (int)np->unit);
			else if (i > PAKPACKLEN)
				MsgEPrintf (-100, "%s.%c 'pack' too large",
						(*nd)->name, (int)np->unit);
			else
				np->packlen = i;
		}
		MsgPrintf (-100, "%s.%c: packing %d", (*nd)->name,
				(int)np->unit, (int)np->packlen);
	}

	return (0);
}

extern int FAR
netports_init (void)
{
	int		n, i;
	struct netname	*nn;

	for (n = 0, nn = st.netnames; nn; ++n, nn = nn->next)
		;

	if (F(netports = (NETPORT *) memory_calloc (n, sizeof (*netports)))) {
		MsgEPrintf (-100, "netport: no mem");
		return (1);
	}
	nnetports = n;

	for (i = 0, n = 0, nn = st.netnames; nn; ++n, nn = nn->next)
		if (!netport_init (n, nn->name))
			++i;
	if (!i) {
		MsgEPrintf (-100, "netport: no active ports");
		netports_term ();
		return (1);
	}
	return (0);
}

LOCAL_FUNC void NEAR
packet_accept (PACKET *pack)
{
	PLAYER	*pl;
	int	lcrc;

	++STATS_NETRECEIVEDP;		/* count rcvd packet */
	lcrc = (pack->raw[pack->length-2] << 8)
		      + pack->raw[pack->length-1];
	if ((short)lcrc != (short)crc (pack)) {
		packet_del (pack);
		pack = 0;
		++STATS_NETERRCRC;
	} else if (T(pl = player_active (pack))) {
		pack->next = 0;
		if (pl->tail)
			pl->tail->next = pack;
		else
			pl->incoming = pack;
		pl->tail = pack;
	} else {
		packet_del (pack);	/* oops, drop the packet */
		pack = 0;
		++STATS_NETERRNOPLAYER;
	}
}

LOCAL_FUNC void NEAR
packet_unpack (PACKET *packed)
{
	Uchar	*p;
	PACKET	*pack;
	int	l;
	int	len;

	for (p = packed->raw + 1, l = packed->length - 1; l > 0; p += len) {
		len = ((int)p[0] << 8) + p[1];
		p += 2;
		l -= len + 2;
		if (l < 0) {
			++STATS_NETERRPACKED;		/* bad packing */
			break;
		}
		if (F(pack = packet_new (len, LADDRESS)))
			break;				/* loose it */
		pack->netport = packed->netport;	/* copy player's id */
		pack->address = pack->raw - LADDRESS;
		memcpy (pack->address, packed->address, LADDRESS);
		pack->arrived = packed->arrived;
		pack->length  = len;
		memcpy (pack->raw, p, len);
		packet_accept (pack);
	}
	packet_del (packed);
}

LOCAL_FUNC void NEAR
netport_receive (NETPORT *port)
{
	PACKET	*pack, *prev;
	Ulong	iflags;

	if (port->NetDriver->Poll)
		port->NetDriver->Poll (port, 0);

	for (;;) {
		iflags = Sys->Disable ();
		if (T(pack = port->incoming[HEAD]) &&
		    F(port->incoming[HEAD] = pack->next))
			port->incoming[TAIL] = 0;
		Sys->Enable (iflags);
		if (!pack)
			break;
		if (RC_PACKED == pack->raw[0])
			packet_unpack (pack);
		else
			packet_accept (pack);
	}

	for (prev = 0, pack = port->outgoing; pack;) {
		if (RC_SENDOK == pack->raw[0])
			;
		else if (RC_SENDFAIL == pack->raw[0])
			++STATS_NETERRSENDA;
		else {			/* send pending */
			prev = pack;
			pack = pack->next;
			continue;
		}
		if (prev)
			prev->next = pack->next;
		pack = packet_del (pack);
	}
	if (!prev)
		port->outgoing = 0;
}

extern void FAR
netports_receive (void)
{
	NETPORT		*port;
	int		i;

	for (i = 0, port = netports; i < nnetports; ++i, ++port) {
		if (port->flags & NP_ON)
			netport_receive (port);
	}
}

extern void FAR
netports_poll (void)
{
	NETPORT		*port;
	int		i;

	for (i = 0, port = netports; i < nnetports; ++i, ++port)
		if ((port->flags & NP_ON) && port->NetDriver->Poll)
			port->NetDriver->Poll (port, 1);
}

extern void FAR
PlName (PLAYER *pl)
{
	memset (pl->name, 0, LNAME);
	memset (pl->team, 0, LNAME);
#if 1
	sprintf (pl->name, "%02x%02x%02x",
		pl->address[3], pl->address[4], pl->address[5]);
	strncpy (pl->team, "[*]", LNAME);
#else
	NETPORT	*port;
	char	unit[2];

	port = &netports[pl->netport];
	if (!(port->flags & NP_ON))
		return;
	unit[0] = port->unit;
	unit[1] = '\0';
	strncpy (pl->name, port->NetDriver->name, LNAME);
	strncat (pl->name, ".", LNAME);
	strncat (pl->name, unit, LNAME);
#endif
}

extern void FAR
netport_count (PLAYER *pl, int delta)
{
	NETPORT	*port;

	port = &netports[pl->netport];
	if (!(port->flags & NP_ON))
		return;
	port->nplayers += delta;
}

extern int FAR
packet_deliver (PACKET *pack)
{
	NETPORT	*port;
	Ulong	iflags;

	port = &netports[pack->netport];
	if (!(port->flags & NP_ON)) {
		packet_del (pack);
		return (1);
	}
	pack->flags = 0;
	pack->next = 0;
	pack->arrived = st.lasttime;

	iflags = Sys->Disable ();
	if (port->incoming[TAIL])
		port->incoming[TAIL]->next = pack;
	else
		port->incoming[HEAD] = pack;
	port->incoming[TAIL] = pack;
	Sys->Enable (iflags);

	STATS_NETRECEIVEDBYTES += pack->length;
	++STATS_NETRECEIVEDPACKETS;
	return (0);
}

LOCAL_FUNC int NEAR
packet_dispatch (PACKET *pack, int port, int mode)
{
	NETPORT	*np;
	PACKET	*outpak;
	int	ret;

	np = &netports[port];
	if (!(np->flags & NP_ON))
		return (1);
	outpak = np->outpak;
	if (!pack) {
		ret = 0;
		if (outpak && outpak->length > 1) {
			ret |= packet_xmit (outpak, np);
			outpak->length = 1;	/* reuse outpak */
		}
		ret |= packet_xmit (pack, np);	/* flush port */
		return (ret);
	}
	++STATS_NETSENTP;
	if (!mode || !(np->flags & NP_PACKING))
		return (packet_xmit (pack, np));
	if (!outpak) {
		outpak = np->outpak = packet_new (np->packlen, -1);
		if (!outpak)			/* sorry, no can pak */
			return (packet_xmit (pack, np));
		outpak->length = 1;
		outpak->raw[0] = RC_PACKED;
	}
	if ((Uint)(pack->length + 2 + 1) > np->packlen) {
		ret  = packet_xmit (outpak, np);
		ret |= packet_xmit (pack, np);
		return (ret);
	}
	if ((Uint)(pack->length + 2) > (Uint)(np->packlen - outpak->length)) {
		packet_xmit (outpak, np);
		outpak->length = 1;		/* reuse outpak */
	}
	outpak->raw[outpak->length++] = (Uchar)(0x0ff&(pack->length >> 8));
	outpak->raw[outpak->length++] = (Uchar)(0x0ff&(pack->length));
	memcpy (&outpak->raw[outpak->length], pack->raw, pack->length);
	outpak->length += pack->length;
	return (0);
}

extern int FAR
packet_send (PACKET *pack, int mode)
/*
 * mode:
 * 0 send to addressed player
 * 1 broadcast to all netports
 * 2 broadcast to all active netports
*/
{
	int	port, ret;

	ret = 0;
	if (!pack) {
		for (port = 0; port < nnetports; ++port)
			ret |= packet_dispatch (pack, port, mode);
	} else if (mode) {
		if (st.network & NET_NOBCAST)
			return (1);
		pack->address = 0;	/* broadcast */
		for (port = 0; port < nnetports; ++port) {
			if (2 == mode && !netports[port].nplayers)
				continue;
			pack->netport = port;
			ret |= packet_dispatch (pack, port, mode);
		}
	} else
		ret |= packet_dispatch (pack, pack->netport, mode);
	return (ret);
}

static char	FAR name[32];
static char	FAR hex[4];

extern char * FAR
netport_name (int port)
{
	sprintf (name, "%s.%c",
		netports[port].NetDriver->name, (int)netports[port].unit);
	return (name);
}

extern char * FAR
netport_addr (int port, Uchar *addr)
{
	int	i;

	name[0] = '\0';
	for (i = 0; i < LADDRESS; ++i) {
		sprintf (hex, "%02x", (int)addr[i]);
		strcat (name, hex);
	}

	return (name);
}
