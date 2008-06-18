/* --------------------------------- player.c ------------------------------- */

/* This is part of the flight simulator 'fly8'.
 * Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
*/

/* Manage participating players
*/

#include "fly.h"


static PLAYER	*players = 0;		/* active list */

extern PLAYER * FAR
player_add (PACKET *pack)
{
	PLAYER	*pl;

	if (F(NEW (pl)))
		return (0);

	pl->next = players;
	players = pl;
	pl->netport = pack->netport;
	pl->timeout = st.PlayerTimeout;
	memcpy (pl->address, pack->address, LADDRESS);
	PlName (pl);
	return (pl);
}

extern PLAYER * FAR
player_delete (PLAYER *player)
{
	PLAYER	*pl, *pp;

	if (!player)
		return (0);

	for (pp = 0, pl = players; pl; pp = pl, pl = pl->next) {
		if (pl == player) {
			pl = pl->next;
			if (pp)
				pp->next = pl;
			else
				players = pl;
			break;
		}
	}

	DEL0 (player);

	return (pl);
}

extern void FAR
players_delete (void)
{
	PLAYER	*pl;

	for (pl = players; pl;)
		pl = DEL (pl);
	players = 0;
}

/* Remove all objects of a remote player from our world.
*/
extern void FAR
player_remove (PLAYER *player)
{
	OBJECT	*p;

	for (p = CO; p; p = p->next) {
		if ((p->flags & F_IMPORTED) && p->rplayer == player)
			p->flags |= F_DEL|F_MOD;
	}
	if (player->flags & PL_PLAYING) {
		player->flags &= ~PL_PLAYING;
		player->flags |= PL_ACTIVE;
		netport_count (player, -1);
	}
}

/* Remove all (or just team) imported objects from our world.
*/
extern void FAR
players_remove (PLAYER *ptype)
{
	PLAYER	*pl;

	for (pl = players; pl; pl = pl->next) {
		if (ptype == st.all_team && stricmp (pl->team, st.teamname))
			continue;
		player_remove (pl);
	}
}

/* Delete all noise-messages from a player.
*/
LOCAL_FUNC void NEAR
player_flush (PLAYER *pl)
{
	PACKET	*pack;

	for (pack = pl->incoming; pack;) {
		pack = packet_del (pack);
		if (pl->flags & PL_PLAYING)
			++STATS_NETERRL;
		else
			++STATS_NETERRNOISE;
	}
	pl->incoming =  0;
	pl->tail = 0;
}

/* Delete all noise-messages.
*/
extern void FAR
players_flush (void)
{
	PLAYER	*pl;

	for (pl = players; pl; pl = pl->next)
		player_flush (pl);
}

/* purge silent players (life was never fair).
*/
extern void FAR
players_purge (void)
{
	PLAYER	*pl;

	for (pl = players; pl; pl = pl->next) {
		if (!(pl->flags & (PL_NOTIDLE & ~PL_ACTIVE)))
			continue;
		if (TIMEOUT (pl->timeout) > 0) {
			LogPrintf ("%s ", Tm->Ctime ());
			MsgWPrintf (-100, "Timed: %s:%s", pl->name, pl->team);
			if (pl->flags & PL_PLAYING)
				remote_noplay (pl);
			player_flush (pl);
			player_remove (pl);
			pl->flags &= ~PL_NOTIDLE;
		}
	}
}

/* Find a player by address.
*/
extern PLAYER * FAR
player_find (PACKET *pack)
{
	PLAYER	*pl;

	for (pl = players; pl; pl = pl->next)
		if (pl->netport == pack->netport &&
		    !memcmp (pl->address, pack->address, LADDRESS))
			break;
	return (pl);
}

/* Make sure a player is registered.
*/
extern PLAYER * FAR
player_active (PACKET *pack)
{
	PLAYER	*pl;

	if (F(pl = player_find (pack)))
		pl = player_add (pack);
	if (pl && !(pl->flags & PL_NOTIDLE))
		pl->flags |= PL_ACTIVE;
	return (pl);
}

/* Provide access to players list.
*/
extern PLAYER * FAR
player_next (PLAYER *pl)
{
	if (pl)
		return (pl->next);
	else
		return (players);
}

extern int FAR
players_init (void)
{
	players = 0;
	return (0);
}

extern void FAR
players_term (void)
{
	players_remove (st.all_known);
	players_delete ();
}
