/* --------------------------------- remote.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Handler for the remote comms (very high level).
 * This module is aware of the application data structures. It uses packet
 * services to communicate with remotely participating programs.
*/

/* The timestamp must be moved to the beginning of the messages. This way
 * out of order packets can be discarded. Right now it is too much bother
 * to parse to the bottom of the message and then find out it is stale; too
 * late!
*/

#include "fly.h"


#define	COMM_VERSION	0x0007

#define	PACKHEADLEN	64
#define	PAKGRAIN	32			/* must be power of 2 */
#define	ONEPACKLEN	64			/* min pack len */

/* Our comms protocol. All message numbers should be >1 and <0xff
*/

#define	RC_MAINT	0x01
#define	RC_ADD		0x02
#define	RC_STAT		0x03
#define	RC_URHIT	0x04
#define	RC_IMHIT	0x05
#define	RC_SHOOT	0x06
#define	RC_TEXT		0x07
#define	RC_TIMEREQ	0x08
#define	RC_TIMEACK	0x0a
#define	RC_ACTIVEQUERY	0x10		/* ping broadcast */
#define	RC_ACTIVEREPLY	0x11		/* ping reply */
#define	RC_NOTACTIVE	0x12		/* indicate going offline */
#define	RC_REQUEST	0x13		/* request to play */
#define	RC_REPLYPOS	0x14		/* accept player's request */
#define	RC_REPLYNEG	0x15		/* announce 'not playing' */

/* Enforce consistent byte order in message data
*/

#define PUTINIT \
	if (!(st.network & NET_INITED)) \
		return (1); \
	if (F(pack = packet_new (-1, -1))) \
		return (1); \
	d = pack->raw

#define PUTBYTE(b)	(*d++ = (Uchar)(b))
#define PUTWORD(w)	(t = (w), *d++ = (Uchar)(t>>8), *d++ = (Uchar)t)
#define PUTLONG(w)	(lt = (w), *d++ = (Uchar)(lt>>24), \
			 *d++ = (Uchar)(lt>>16), *d++ = (Uchar)(lt>>8), \
			 *d++ = (Uchar)lt)
#define	GETBYTE		(t = *(signed char *)d++)
#define	GETUBYTE	(t = *d++)
#define	GETWORD		(t = *(signed char *)d++<<8, t += *d++)
#define	GETUWORD	(t = *d++<<8, t += *d++)
#define	GETLONG		(lt = *d++, lt = (lt<<8)+*d++, lt = (lt<<8)+*d++, \
			 lt = (lt<<8)+*d++)

extern PACKET * FAR
packet_new (short size, short hlen)
{
	PACKET	*pack;

	if (F(NEW (pack)))
		return (0);

	if (size < 0)
		size = ONEPACKLEN;
	pack->length = size;

	if (hlen < 0)
		hlen = PACKHEADLEN;
	size = ((size + hlen) + PAKGRAIN-1) & ~(PAKGRAIN-1);
	pack->size = size;

	if (F(pack->data = memory_alloc (size))) {
		DEL (pack);
		return (0);
	}
	pack->raw = pack->data + hlen;

	return (pack);
}

extern PACKET * FAR
packet_del (PACKET *pack)
{
	memory_free (pack->data, pack->size);
	return (DEL (pack));
}

extern int FAR
crc (PACKET *pack)
{
	int	i, crc;
	Uchar	*p;

	for (p = pack->raw, crc = i = pack->length-2; i-- > 0;)
		crc = (crc ^ (crc << 5)) ^ (int)*p++;
	return (crc);
}

LOCAL_FUNC int NEAR
send_packet (PACKET *pack, Uchar *d, PLAYER *player)
/*
 * mode depends on 'player':
 *	player		send only to this player
 *	all_active	send to all PL_ACTIVE players
 *	all_team	send to all players on my team
 *	all_known	send to all known players
 *	all_pports	broadcast to all netports with players
 *	all_ports	broadcast to all netports
 *	0		(invalid)
*/
{
	PLAYER	*pl;
	int	t, ret;

	pack->length = (d - pack->raw) + 2;
	if ((Uint)pack->length > pack->size) {
		MsgEPrintf (-100, "Packet %02x too long (%d)",
				pack->raw[0], pack->length);
		packet_del (pack);
		return (1);
	}
	PUTWORD (crc (pack));

	if (player == st.all_ports && !(st.network & NET_NOBCAST))
		ret = packet_send (pack, 1);
	else if (player == st.all_pports && !(st.network & NET_NOBCAST))
		ret = packet_send (pack, 2);
	else if (player == st.all_known || player == st.all_active ||
							player == st.all_team){
		ret = 0;
		for (pl = 0; T(pl = player_next (pl));) {
			if (player == st.all_active && !(pl->flags & PL_SEND))
				continue;
			if (player == st.all_team &&
					stricmp (pl->team, st.teamname))
				continue;
			pack->netport = pl->netport;
			pack->address = pl->address;
			if (T(ret = packet_send (pack, 0)))
				break;
		}
	} else if (player) {
		pack->netport = player->netport;
		pack->address = player->address;
		ret = packet_send (pack, 0);
	} else
		ret = 1;		/* should not happen */
	packet_del (pack);
	return (ret);
}

LOCAL_FUNC int NEAR
send_command (int command, PLAYER *player)
{
	PACKET	*pack;
	Uchar	*d;

	PUTINIT;
	PUTBYTE (command);

	return (send_packet (pack, d, player));
}

LOCAL_FUNC int NEAR
send_stat (int command, OBJECT *p, PLAYER *player)
{
	int	t;
	long	lt;
	PACKET	*pack;
	Uchar	*d;

	p->maint_last = st.present;

	PUTINIT;
	PUTBYTE (command);
	PUTLONG (p->id);
	PUTWORD (p->name);
	PUTWORD (p->flags);
	PUTWORD (p->gpflags);
	PUTWORD (p->color);
	PUTWORD (p->damage);
	PUTWORD (p->a[X]);
	PUTWORD (p->a[Y]);
	PUTWORD (p->a[Z]);
	PUTWORD (p->da[X]);
	PUTWORD (p->da[Y]);
	PUTWORD (p->da[Z]);
	PUTLONG (p->R[X]);
	PUTLONG (p->R[Y]);
	PUTLONG (p->R[Z]);
	PUTWORD (p->V[X]);
	PUTWORD (p->V[Y]);
	PUTWORD (p->V[Z]);
	PUTWORD (p->speed);
	PUTLONG (st.present);

	return (send_packet (pack, d, player));
}

LOCAL_FUNC int NEAR
send_maint (int command, OBJECT *p, PLAYER *player)
{
	int	t;
	long	lt;
	PACKET	*pack;
	Uchar	*d;

	if (p->maint_rate && TIMEOUT (p->maint_last) < (long)p->maint_rate)
		return (0);

	p->maint_last = st.present;

	PUTINIT;
	PUTBYTE (command);
	PUTLONG (p->id);
	PUTWORD (p->a[X]);
	PUTWORD (p->a[Y]);
	PUTWORD (p->a[Z]);
	PUTLONG (p->R[X]);
	PUTLONG (p->R[Y]);
	PUTLONG (p->R[Z]);
	PUTWORD (p->V[X]);
	PUTWORD (p->V[Y]);
	PUTWORD (p->V[Z]);
	PUTLONG (st.present);

	return (send_packet (pack, d, player));
}

extern int FAR
remote_urhit (OBJECT *p, int speed, int extent, int damaging)
{
	int	t;
	long	lt;
	PACKET	*pack;
	Uchar	*d;

	PUTINIT;
	PUTBYTE (RC_URHIT);
	PUTLONG (p->rid);
	PUTWORD (speed);
	PUTWORD (extent);
	PUTWORD (damaging);

	return (send_packet (pack, d, p->rplayer));
}

extern int FAR
remote_imhit (OBJECT *p, int seed, int speed, int extent, int damaging)
{
	int	t;
	long	lt;
	PACKET	*pack;
	Uchar	*d;

	PUTINIT;
	PUTBYTE (RC_IMHIT);
	PUTLONG (p->id);
	PUTWORD (seed);
	PUTWORD (speed);
	PUTWORD (extent);
	PUTWORD (damaging);

	return (send_packet (pack, d, st.all_pports));
}

LOCAL_FUNC int NEAR
send_shoot (OBJECT *p, int weapon, int n, int seed, int interval)
{
	int	t;
	long	lt;
	PACKET	*pack;
	Uchar	*d;

	PUTINIT;
	PUTBYTE (RC_SHOOT);
	PUTLONG (p->id);
	PUTWORD (weapon);
	PUTWORD (n);
	PUTWORD (seed);
	PUTWORD (interval);

	return (send_packet (pack, d, st.all_pports));
}

LOCAL_FUNC int NEAR
send_playing (int command, PLAYER *player)
{
	int	t;
	long	lt;
	PACKET	*pack;
	Uchar	*d;

	PUTINIT;
	PUTBYTE (command);
	PUTWORD (st.ComVersion);
	PUTLONG (st.present);
	strcpy ((char *)d, st.nikname);
	d += strlen (st.nikname)+1;
	strcpy ((char *)d, st.teamname);
	d += strlen (st.teamname)+1;

	return (send_packet (pack, d, player));
}

LOCAL_FUNC int NEAR
send_text (char *text, PLAYER *player)
{
	int	t;
	PACKET	*pack;
	Uchar	*d;
	int	len;

	PUTINIT;
	PUTBYTE (RC_TEXT);
	t = pack->size - 1 - 1 - 2;	/* cmd, text, eol, crc */
	if ((len = strlen (text)) > t)
		len = t;
	memcpy (d, text, len);
	d += len;
	*d++ = '\0';

	return (send_packet (pack, d, player));
}

LOCAL_FUNC int NEAR
send_time (int command, PLAYER *player, Ulong time)
{
	long	lt;
	PACKET	*pack;
	Uchar	*d;

	PUTINIT;
	PUTBYTE (command);
	PUTLONG (time);

	return (send_packet (pack, d, player));
}

extern int FAR
send_obj (OBJECT *p, PLAYER *player)
{
	if (!(st.network & NET_INITED))
		return (1);
	if (!p) {
		packet_send (0, 1);	/* flush all ports */
		return (0);
	}

	if (p->flags & F_NEW)
		return (send_stat (RC_ADD, p, player));
	else if (p->flags & F_MOD)
		return (send_stat (RC_STAT, p, player));
	else if (p->flags & F_MAINT)
		return (send_maint (RC_MAINT, p, player));
	else
		return (0);
}

LOCAL_FUNC int NEAR
send_state (PLAYER *player)
{
	OBJECT	*p;

	for (p = CO; p; p = p->next) {
		if (p->flags & F_EXPORTED)
			send_stat (RC_ADD, p, player);
	}
	return (0);
}

#define MAX_TIME_ERR	4096	/* integrated error high watermark */

LOCAL_FUNC void NEAR
update_rtime (OBJECT *p, PLAYER *pl, PACKET *pack, Ulong rtime)
{
	if (EIM(p) && EIM(p)->lasttime > rtime)
		++STATS_NETERROOO;	/* out of order! */

	p->rtime = rtime - pl->rtime;			/* object currency */
	pl->rtimeErr += (int)(p->rtime - pack->arrived);/* time diff err */
	if (pl->rtimeErr > MAX_TIME_ERR) {
		++pl->rtime;
		pl->rtimeErr /= 2;
	} else if (pl->rtimeErr < -MAX_TIME_ERR) {
		--pl->rtime;
		pl->rtimeErr /= 2;
	}
}

/* process object-specific messages from a player.
*/

LOCAL_FUNC void NEAR
receive_object (OBJECT *p)
{
	int	t;
	long	lt;
	PACKET	*pack, *next, *prev;
	PLAYER	*pl;
	Uchar	*d;
	int	seed, speed, extent, n, weapon, damaging, count;

	if (F(pl = p->rplayer))
		return;

	count = 0;
	for (prev = 0, pack = pl->incoming; pack; prev = pack, pack = next) {
		next = pack->next;
		d = pack->raw;
		switch (*d++) {		/* command */
		case RC_STAT:
			GETLONG;
			if (p->rid != lt)
				continue;
			++count;
			GETWORD;	p->name = t;
			GETUWORD;
				p->flags = F_IMPORTED|F_ALIVE|
				    (t & ~(F_EXPORTED|F_MOD|F_DONE|F_FRIEND));
			GETUWORD;	n = p->gpflags;
					p->gpflags = t;
			GETUWORD;	p->color = t;
			GETWORD;	p->damage = t;
			GETWORD;	p->a[X] = t;
			GETWORD;	p->a[Y] = t;
			GETWORD;	p->a[Z] = t;
			GETWORD;	p->da[X] = t;
			GETWORD;	p->da[Y] = t;
			GETWORD;	p->da[Z] = t;
			GETLONG;	p->R[X] = lt;
			GETLONG;	p->R[Y] = lt;
			GETLONG;	p->R[Z] = lt;
			GETWORD;	p->V[X] = t;
			GETWORD;	p->V[Y] = t;
			GETWORD;	p->V[Z] = t;
			GETWORD;	p->speed = t;
			GETLONG;	update_rtime (p, pl, pack, (Ulong)lt);

			Mobj (p);
			p->time = FOREVER;
			if (EIM(p))
				EIM(p)->timeout = st.ObjectTimeout;

			if (p->flags & F_CC) {
				if (pl->flags & PL_FRIEND) {
					p->color = ST_FRIEND;
					p->flags |= F_FRIEND;
				} else
					p->color = ST_FOE;
			}

			break;

		case RC_MAINT:
			GETLONG;
			if (p->rid != lt)
				continue;
			++count;
			GETWORD;	p->a[X] = t;
			GETWORD;	p->a[Y] = t;
			GETWORD;	p->a[Z] = t;
			GETLONG;	p->R[X] = lt;
			GETLONG;	p->R[Y] = lt;
			GETLONG;	p->R[Z] = lt;
			GETWORD;	p->V[X] = t;
			GETWORD;	p->V[Y] = t;
			GETWORD;	p->V[Z] = t;
			GETLONG;	update_rtime (p, pl, pack, (Ulong)lt);

			Mobj (p);
			p->da[X] = p->da[Y] = p->da[Z] = 0;
			p->speed = ihypot3d (p->V);
			p->flags |= F_ALIVE;
			if (EIM(p))
				EIM(p)->timeout = st.ObjectTimeout;
			break;

		case RC_IMHIT:
			GETLONG;
			if (p->rid != lt)
				continue;
			++count;
			GETWORD;	seed = t;
			GETWORD;	speed = t;
			GETWORD;	extent = t;
			GETWORD;	damaging = t;
			object_hit (p, seed, speed, extent, damaging);
			break;

		case RC_SHOOT:
			GETLONG;
			if (p->rid != lt)
				continue;
			++count;
			GETWORD;	weapon = t;
			GETWORD;	n = t;
			GETWORD;	seed = t;
			GETWORD;	speed = t;	/* interval */
			shoot (p, weapon, n, seed, speed);
			break;

		default:
			continue;
		}
		if (prev)
			prev->next = next;
		else
			pl->incoming = next;
		packet_del (pack);
		pack = prev;		/* prepare for 'prev = pack' */
	}
	pl->tail = prev;
	if (count)
		pl->timeout = st.PlayerTimeout;
}

/* process general messages from a player.
*/

LOCAL_FUNC void NEAR
receive_player (PLAYER *pl, int mode)
{
	int	t;
	long	lt;
	PACKET	*pack, *next, *prev;
	OBJECT	*p;
	Uchar	*d;
	int	speed, extent, count, damaging;
	long	rid;
	char	msg[LADDRESS*2+1], *m;

	count = 0;
	for (prev = 0, pack = pl->incoming; pack; prev = pack, pack = next) {
		next = pack->next;
		d = pack->raw;
		switch (*d++) {		/* command */
		case RC_URHIT:
			++count;
			if (!(pl->flags & PL_RECEIVE))
				break;
			GETLONG;
			for (p = CO; p; p = p->next)
				if ((p->flags & F_EXPORTED) && p->id == lt)
					break;
			if (p) {
				GETWORD;	speed = t;
				GETWORD;	extent = t;
				GETWORD;	damaging = t;
				object_hit (p, Frand(), speed, extent,
					damaging);
			}
			break;

		case RC_STAT:
			if (!mode)
				continue;
			++count;
			if (!(pl->flags & PL_RECEIVE))
				break;
			++STATS_NETERRFOUND;	/* was lost, now found! */
			goto refresh;
		case RC_ADD:
			++count;
			if (!(pl->flags & PL_RECEIVE))
				break;
refresh:
			GETLONG;	rid = lt;
			GETWORD;

			if (F(p = create_object (t, 0)))
				break;

			p->rplayer = pl;
			p->rid = rid;
			GETWORD;
				p->flags = F_IMPORTED|F_ALIVE|
				    (t & ~(F_EXPORTED|F_MOD|F_DONE|F_FRIEND));
			GETWORD;	p->gpflags = t;
			GETWORD;	p->color = t;
			GETWORD;	p->damage = t;
			GETWORD;	p->a[X] = t;
			GETWORD;	p->a[Y] = t;
			GETWORD;	p->a[Z] = t;
			GETWORD;	p->da[X] = t;
			GETWORD;	p->da[Y] = t;
			GETWORD;	p->da[Z] = t;
			GETLONG;	p->R[X] = lt;
			GETLONG;	p->R[Y] = lt;
			GETLONG;	p->R[Z] = lt;
			GETWORD;	p->V[X] = t;
			GETWORD;	p->V[Y] = t;
			GETWORD;	p->V[Z] = t;
			GETWORD;	p->speed = t;
			GETLONG;	update_rtime (p, pl, pack, (Ulong)lt);

			Mobj (p);
			p->time = FOREVER;
			if (T(NEW (EIM(p)))) {
				p->e_type = ET_IMPORTED;
				EIM(p)->timeout = st.ObjectTimeout;
			}

			if (p->flags & F_CC) {
				if (pl->flags & PL_FRIEND) {
					p->color = ST_FRIEND;
					p->flags |= F_FRIEND;
				} else
					p->color = ST_FOE;
			}

			break;

		case RC_ACTIVEQUERY:			/* idle -> active */
			GETWORD;			/* version */
			pl->ComVersion = (Uint)t;
			if (pl->ComVersion < 7) {	/* obsolete */
				MsgWPrintf (100, "old ComVer %u: %s:%s",
					(Uint)t, pl->name, pl->team);
				break;
			}
			send_playing (RC_ACTIVEREPLY, pl);
			goto common_playing;

		case RC_ACTIVEREPLY:			/* idle -> active */
			GETWORD;			/* version */
			pl->ComVersion = (Uint)t;
			if (pl->ComVersion < 5) {	/* obsolete */
				MsgWPrintf (100, "ComVer %u: %s:%s",
					(Uint)t, pl->name, pl->team);
				break;
			}
common_playing:
			GETLONG;
			if (!(pl->flags & PL_PLAYING)) {
				pl->rtime = lt - pack->arrived;
				pl->rtimeErr = 0L;
			}
			++count;
			strncpy (pl->name, (char *)d, LNAME);
			if (pl->ComVersion >= 6) {
				d += strlen ((char *)d) + 1;
				strncpy (pl->team, (char *)d, LNAME);
				if (!stricmp (pl->team, st.teamname))
					pl->flags |= PL_FRIEND;
				else
					pl->flags &= ~PL_FRIEND;
			} else
				strncpy (pl->team, "[old]", LNAME);
			if (pl->flags & PL_PLAYING)
				break;
			if (pl->flags & (PL_NOTIDLE & ~PL_ACTIVE))
				;
			else {
				MsgWPrintf (-100, "Active: %s:%s",
					pl->name, pl->team);
				for (m = msg, t = 0; t < LADDRESS; ++t) {
					sprintf (m, "%02x", pl->address[t]);
					m += 2;
				}
				MsgPrintf (-100, "Addr:   %s", msg);
			}
			if (st.network & NET_AUTOCONNECT)
				remote_request (pl);
			break;

		case RC_NOTACTIVE:			/* active -> idle */
			++count;
			LogPrintf ("%s ", Tm->Ctime ());
			MsgWPrintf (-100, "Gone: %s:%s", pl->name, pl->team);
			player_remove (pl);
			pl->flags &= ~PL_NOTIDLE;
			break;

		case RC_REQUEST:			/* active -> pend */
			++count;
			MsgPrintf (100, "Asking: %s:%s", pl->name, pl->team);
			if (pl->flags & PL_NOTIDLE) {
				player_remove (pl);
				pl->flags &= ~PL_NOTIDLE;
			}
			pl->flags |= PL_PENDBOSS;
			if (st.network & NET_AUTOACCEPT)
				remote_reply (pl, 1);
			else if (st.network & NET_AUTODECLINE)
				remote_reply (pl, 0);
			else
				MsgWPrintf (100, "Pending: %s:%s",
					pl->name, pl->team);
			break;

		case RC_REPLYPOS:		/* pend -> play */
			++count;
			if (!(pl->flags & PL_PEND))
				break;
			if (pl->flags & PL_PENDREQUEST)
				send_command (RC_REPLYPOS, pl);
			pl->flags &= ~PL_PEND;
			pl->flags |= PL_PLAYING;
			netport_count (pl, 1);
			send_state (pl);
			LogPrintf ("%s ", Tm->Ctime ());
			MsgWPrintf (-100, "Joined: %s:%s", pl->name, pl->team);
			MsgWPrintf (-100, "  thru  %s",
				netport_name (pl->netport));
			MsgWPrintf (-100, "  addr  %s",
				netport_addr (pl->netport, pl->address));
			break;

		case RC_REPLYNEG:		/* pend/playing -> active */
			++count;
			if (pl->flags & PL_PEND)
				MsgWPrintf (100, "Declined: %s:%s",
					pl->name, pl->team);
			else if (pl->flags & PL_PLAYING) {
				LogPrintf ("%s ", Tm->Ctime ());
				MsgWPrintf (-100, "Not Playing: %s:%s",
					pl->name, pl->team);
			}
			player_remove (pl);
			pl->flags &= ~PL_NOTIDLE;
			pl->flags |= PL_ACTIVE;
			break;

		case RC_TEXT:
			++count;
			if (strlen ((char *)d) > 80)
				break;
			MsgWPrintf (100, "%s:%s says:", pl->name, pl->team);
			MsgWPrintf (100, "  %s", d);
			break;

		case RC_TIMEREQ:
			++count;
			GETLONG;			/* remote send time */
			send_time (RC_TIMEACK, pl, lt);
			break;

		case RC_TIMEACK:
			++count;
			GETLONG;			/* my send time */
			MsgPrintf (100, "%s:%s time: %lu",
				pl->name, pl->team, Tm->Milli () - lt);
			break;

		default:
			continue;
		}
		if (prev)
			prev->next = next;
		else
			pl->incoming = next;
		packet_del (pack);
		pack = prev;		/* prepare for 'prev = pack' */
	}
	pl->tail = prev;
	if (count)
		pl->timeout = st.PlayerTimeout;
}

extern void FAR
remote_receive (OBJECT *obj)
{
	PLAYER	*pl;

	if (!(st.network & NET_INITED))
		return;

	if (obj)
		receive_object (obj);
	else {
		netports_receive ();
		for (pl = 0; T(pl = player_next (pl));)
			receive_player (pl, 0);
	}
}

extern void FAR
remote_refresh (void)
{
	PLAYER	*pl;

	if (!(st.network & NET_INITED))
		return;

	for (pl = 0; T(pl = player_next (pl));)
		receive_player (pl, 1);
}

extern void FAR
remote_request (PLAYER *pl)
{
	if (pl->flags & PL_PLAYING)
		MsgPrintf (100, "already playing: %s:%s", pl->name, pl->team);
	else {
		MsgPrintf (50, "Requesting %s:%s", pl->name, pl->team);
		send_command (RC_REQUEST, pl);
		pl->flags &= ~PL_NOTIDLE;
		pl->flags |= PL_PENDREQUEST;
		pl->timeout = st.PlayerTimeout;
	}
}

extern void FAR
remote_reply (PLAYER *pl, int reply)
{
	if (!(pl->flags & PL_PENDBOSS)) {
		MsgPrintf (100, "not pending");
		return;
	}
	pl->flags &= ~PL_PEND;
	if (reply) {
		MsgPrintf (100, "Accepting: %s:%s", pl->name, pl->team);
		pl->flags |= PL_PENDCONFIRM;
		send_command (RC_REPLYPOS, pl);
	} else {
		MsgPrintf (100, "Declining: %s:%s", pl->name, pl->team);
		send_command (RC_REPLYNEG, pl);
	}
}

extern void FAR
remote_noplay (PLAYER *pl)
{
	if (st.network & NET_INITED)
		send_command (RC_REPLYNEG, pl);
}

extern void FAR
remote_ping (void)
{
	if (st.network & NET_INITED)
		send_playing (RC_ACTIVEQUERY, st.all_ports);
}

extern void FAR
remote_shoot (OBJECT *p, int weapon, int n, int seed, int interval)
{
	if (st.network & NET_INITED)
		send_shoot (p, weapon, n, seed, interval);
}

extern void FAR
remote_msg (char *text, PLAYER *pl)
{
	if (st.network & NET_INITED)
		send_text (text, pl);
}

extern void FAR
remote_time (PLAYER *pl)
{
	if (st.network & NET_INITED)
		send_time (RC_TIMEREQ, pl, Tm->Milli ());
}

extern int FAR
remote_init (void)
{
	if (!(st.network & NET_ON))
		return (0);

	st.ComVersion = COMM_VERSION;
	MsgPrintf (-100, "Net Ver  %u", (int)st.ComVersion);

	st.all_known   = ((PLAYER NEAR *)0)+1;
	st.all_active  = ((PLAYER NEAR *)0)+2;
	st.all_team    = ((PLAYER NEAR *)0)+3;
	st.all_ports   = ((PLAYER NEAR *)0)+4;
	st.all_pports  = ((PLAYER NEAR *)0)+5;
	st.all_players = ((PLAYER NEAR *)0)+6;
	st.no_players  = ((PLAYER NEAR *)0)+7;

	if (netports_init ())
		return (1);

	if (players_init ())
		return (1);

	st.network |= NET_INITED;
	return (0);
}

extern void FAR
remote_term (void)
{
	if (!(st.network & NET_INITED))
		return;

	send_command (RC_NOTACTIVE, st.all_ports);
	players_term ();
	netports_term ();

	if (STATS_NETERRLOW[1])
		LogPrintf ("in  av %lu max %lu\n",
			STATS_NETERRLOW[2]/STATS_NETERRLOW[0],
			STATS_NETERRLOW[1]);
	if (STATS_NETERRLOW[4])
		LogPrintf ("out av %lu max %lu\n",
			STATS_NETERRLOW[5]/STATS_NETERRLOW[3],
			STATS_NETERRLOW[4]);
	st.network &= ~NET_INITED;
}

#undef COMM_VERSION
#undef RC_MAINT
#undef RC_ADD
#undef RC_STAT
#undef RC_URHIT
#undef RC_IMHIT
#undef RC_SHOOT
#undef RC_TEXT
#undef RC_TIMEREQ
#undef RC_TIMEACK
#undef RC_ACTIVEQUERY
#undef RC_ACTIVEREPLY
#undef RC_NOTACTIVE
#undef RC_REQUEST
#undef RC_REPLYPOS
#undef RC_REPLYNEG
#undef PUTINIT
#undef PUTBYTE
#undef PUTWORD
#undef PUTLONG
#undef GETBYTE
#undef GETWORD
#undef GETLONG
#undef MAX_TIME_ERR
