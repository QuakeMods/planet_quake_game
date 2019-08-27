// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

// g_client.c -- client functions that don't happen every frame

const vec3_t	playerMins = {-15, -15, -24};
const vec3_t	playerMaxs = { 15,  15,  32};

static char	    ban_reason[MAX_CVAR_VALUE_STRING];

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32) REVERSE
This is the default player start
*/
void SP_info_player_start(gentity_t *ent)
{
	extern int pm_reversemap;
	
	ent->s.eType = ET_SPAWNPOINT;
	ent->s.legsAnim = LEGS_IDLE;
	ent->s.torsoAnim = TORSO_STAND;
	
	if ( (!pm_reversemap) && ent->spawnflags & 1)
		ent->spawnflags &= ~1;

	trap_LinkEntity( ent );
}

void SP_info_player_red(gentity_t *ent)
{
	ent->s.eType = ET_SPAWNPOINT;
	ent->s.legsAnim = LEGS_IDLE;
	ent->s.torsoAnim = TORSO_STAND;

	trap_LinkEntity( ent );
}

void SP_info_player_blue(gentity_t *ent)
{
	ent->s.eType = ET_SPAWNPOINT;
	ent->s.legsAnim = LEGS_IDLE;
	ent->s.torsoAnim = TORSO_STAND;

	trap_LinkEntity( ent );
}

void SP_info_player_green(gentity_t *ent)
{
	ent->s.eType = ET_SPAWNPOINT;
	ent->s.legsAnim = LEGS_IDLE;
	ent->s.torsoAnim = TORSO_STAND;

	trap_LinkEntity( ent );
}

void SP_info_player_yellow(gentity_t *ent)
{
	ent->s.eType = ET_SPAWNPOINT;
	ent->s.legsAnim = LEGS_IDLE;
	ent->s.torsoAnim = TORSO_STAND;

	trap_LinkEntity( ent );
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent )
{
}


/*
=======================================================================
  SelectSpawnPoint
=======================================================================
*/

/*
================
SelectNearestDeathmatchSpawnPoint
Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from )
{
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_start")) != NULL)
    {
		VectorSubtract( spot->s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist )
        {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}

/*
===========
SelectRandomFurthestSpawnPoint
Chooses a player start, deathmatch start, etc
============
*/
static gentity_t *SelectRandomFurthestSpawnPoint( const gentity_t *ent, vec3_t avoidPoint, vec3_t origin, vec3_t angles )
{
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[MAX_SPAWN_POINTS];
	gentity_t	*list_spot[MAX_SPAWN_POINTS];
	int			numSpots, i, j, n;
	int			selection;
	int			checkTelefrag;
	int			checkMask;

    checkTelefrag = qtrue;
	checkMask = 3;

__search:

	checkTelefrag = checkMask & 1;

	numSpots = 0;
	for ( n = 0 ; n < level.numSpawnSpots ; n++ )
    {
		spot = level.spawnSpots[n];

		if ( spot->fteam != TEAM_FREE && level.numSpawnSpotsFFA > 0 )
			continue;

		if ( checkTelefrag )
			continue;

		VectorSubtract( spot->s.origin, avoidPoint, delta );
		dist = VectorLength( delta );

		for ( i = 0; i < numSpots; i++ )
		{
			if( dist > list_dist[i] )
			{
				if (numSpots >= MAX_SPAWN_POINTS)
					numSpots = MAX_SPAWN_POINTS - 1;

				for( j = numSpots; j > i; j-- )
				{
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}

				list_dist[i] = dist;
				list_spot[i] = spot;

				numSpots++;
				break;
			}
		}

		if(i >= numSpots && numSpots < MAX_SPAWN_POINTS)
		{
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}

	if ( !numSpots )
    {
		if ( checkMask <= 0 )
		{
			G_Error( "Couldn't find a spawn point" );
			return NULL;
		}
		checkMask--;
		goto __search; // next attempt with different flags
	}

	// select a random spot from the spawn points furthest away
	selection = random() * (numSpots / 2);
	spot = list_spot[ selection ];

	VectorCopy( spot->s.angles, angles );
	VectorCopy( spot->s.origin, origin );
	origin[2] += 9.0f;

	return spot;
}


/*
===========
SelectSpawnPoint

Chooses a player start
============
*/
gentity_t *SelectSpawnPoint( gentity_t *ent, vec3_t avoidPoint, vec3_t origin, vec3_t angles )
{
	return SelectRandomFurthestSpawnPoint( ent, avoidPoint, origin, angles );
}


/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
gentity_t *SelectInitialSpawnPoint( gentity_t *ent, vec3_t origin, vec3_t angles )
{
	gentity_t	*spot;
	int n;

	spot = NULL;

	for ( n = 0; n < level.numSpawnSpotsFFA; n++ )
    {
		spot = level.spawnSpots[ n ];
		if ( spot->fteam != TEAM_FREE )
			continue;
		if ( spot->spawnflags & 1 )
			break;
		else
			spot = NULL;
	}

	if ( !spot )
		return SelectSpawnPoint( ent, vec3_origin, origin, angles );

	VectorCopy( spot->s.angles, angles );
	VectorCopy( spot->s.origin, origin );
	origin[2] += 9.0f;

	return spot;
}


/*
===========
SelectTeamSpawnPoint
============
*/
gentity_t *SelectTeamSpawnPoint( gentity_t *ent, vec3_t origin, vec3_t angles )
{
	gentity_t	*spot;
	int n;
	
	spot = NULL;
	
	for ( n = 0; n < level.numTeamSpawnSpots; n++ )
    {
		spot = level.TeamSpawnSpots[ n ];
		if ( ent->client->sess.sessionTeam == TEAM_RED && spot->fteam == TEAM_RED && level.redSpawnSpot ) {
			break;
		} else if ( ent->client->sess.sessionTeam == TEAM_BLUE && spot->fteam == TEAM_BLUE && level.blueSpawnSpot ) {
			break;
		} else if ( ent->client->sess.sessionTeam == TEAM_GREEN && spot->fteam == TEAM_GREEN && level.greenSpawnSpot ) {
			break;
		} else if ( ent->client->sess.sessionTeam == TEAM_YELLOW && spot->fteam == TEAM_YELLOW && level.yellowSpawnSpot ) {
			break;
		} else {
			continue;
		}
	}
	
	VectorCopy( spot->s.angles, angles );
	VectorCopy( spot->s.origin, origin );
	origin[2] += 9.0f;

	return spot;
}


/*
===========
SelectSpectatorSpawnPoint
============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles )
{
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );
	if ( dfx_gametype.integer != GT_TEAMRUN )
		return level.spawnSpots[ SPAWN_SPOT_INTERMISSION ];
	else
		return level.TeamSpawnSpots[ SPAWN_SPOT_TEAM_INTERMISSION ];
}


/*
=======================================================================
BODYQUE
=======================================================================
*/

/*
===============
InitBodyQue
===============
*/
void InitBodyQue (void)
{
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++)
    {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodySink
After sitting around for five seconds, fall into the ground and disappear
=============
*/
void BodySink( gentity_t *ent )
{
	if ( level.time - ent->timestamp > 6500 )
    {
		// the body ques are never actually freed, they are just unlinked
		trap_UnlinkEntity( ent );
		ent->physicsObject = qfalse;
		return;
	}
	ent->nextthink = level.time + FRAMETIME;
	ent->s.pos.trBase[2] -= 1;
}


/*
=============
CopyToBodyQue

A player is re spawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue( gentity_t *ent )
{
	gentity_t		*body;
	int			contents;

	trap_UnlinkEntity (ent);

	// if client is in a no-drop area, don't leave the body
	contents = trap_PointContents( ent->s.origin, -1 );
	if ( contents & CONTENTS_NODROP )
    {
		return;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

	trap_UnlinkEntity (body);

	body->s = ent->s;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc
	body->s.powerups = 0;	        // clear power-ups
	body->s.loopSound = 0;	        // clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE )
    {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	} else
	{
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	// change the animation to the last-frame only, so the sequence
	// doesn't repeat anew for the body
	switch ( body->s.legsAnim & ~ANIM_TOGGLEBIT )
	{
	case BOTH_DEATH1:
	case BOTH_DEAD1:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
		break;
	case BOTH_DEATH2:
	case BOTH_DEAD2:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
		break;
	case BOTH_DEATH3:
	case BOTH_DEAD3:
	default:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
		break;
	}

	body->r.svFlags = ent->r.svFlags;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + 5000;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH )
    {
		body->takedamage = qfalse;
	} else
	{
		body->takedamage = qtrue;
	}

	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );
	trap_LinkEntity( body );
}


//======================================================================

/*
==================
SetClientViewAngle
==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle )
{
	int	i, cmdAngle;
	gclient_t	*client;

	client = ent->client;

	// set the delta angle
	for (i = 0 ; i < 3 ; i++)
    {
		cmdAngle = ANGLE2SHORT(angle[i]);
		client->ps.delta_angles[i] = cmdAngle - client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy( ent->s.angles, client->ps.viewangles );
}


/*
================
respawn
================
*/
void respawn( gentity_t *ent )
{
	gentity_t	*tent;

	if ( ent->health <= 0 )
        CopyToBodyQue( ent );

	ClientSpawn( ent );

	// add a teleportation effect
	tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
	tent->s.clientNum = ent->s.clientNum;

	// optimize bandwidth
	if ( level.intermissiontime ) {
		tent->r.svFlags = SVF_SINGLECLIENT;
		tent->r.singleClient = ent->s.clientNum;
	}
}


/*
================
TeamCount

Returns number of players on a team
================
*/
int TeamCount( int ignoreClientNum, team_t team )
{
	int	i, count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ )
    {
		if ( i == ignoreClientNum )
            continue;

		if ( level.clients[i].pers.connected == CON_DISCONNECTED )
            continue;

		if ( level.clients[i].sess.sessionTeam == team )
            count++;
	}

	return count;
}


/*
================
TeamConnectedCount

Returns number of active players on a team
================
*/
int TeamConnectedCount( int ignoreClientNum, team_t team )
{
	int	i, count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ )
    {
		if ( i == ignoreClientNum )
            continue;

		if ( level.clients[i].pers.connected != CON_CONNECTED )
            continue;

		if ( level.clients[i].sess.sessionTeam == team )
            count++;
	}

	return count;
}


/*
================
PickTeam
================
*/
team_t PickTeam( int ignoreClientNum ) {
	int		counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount( ignoreClientNum, TEAM_BLUE );
	counts[TEAM_RED] = TeamCount( ignoreClientNum, TEAM_RED );
	counts[TEAM_GREEN] = TeamCount ( ignoreClientNum, TEAM_GREEN );
	counts[TEAM_YELLOW] = TeamCount ( ignoreClientNum, TEAM_YELLOW );

	if ( counts[TEAM_BLUE] = 0 ) 
		return TEAM_BLUE;
	
	if ( counts[TEAM_RED] = 0 ) 
		return TEAM_RED;
		
	if ( counts[TEAM_GREEN] = 0 ) 
		return TEAM_GREEN;
		
	if ( counts[TEAM_YELLOW] = 0 ) 
		return TEAM_YELLOW;
		
	// all slots in use, go spec
	return TEAM_SPECTATOR;
}



/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a user info variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.

returns qfalse in case of invalid user info
============
*/
qbool ClientUserinfoChanged( int clientNum )
{
	gentity_t *ent;
	int		    teamTask, teamLeader, team, health;
	char	    *s;
	char	    model[MAX_QPATH];
	char	    headModel[MAX_QPATH];
	char	    oldname[MAX_NETNAME];
	gclient_t	*client;
	char	    c1[8];
	char	    c2[8];
	char	    userinfo[MAX_INFO_STRING];

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate( userinfo ) )
    {
		Q_strcpy( ban_reason, "bad userinfo" );
		if ( client && client->pers.connected != CON_DISCONNECTED )
			trap_DropClient( clientNum, ban_reason );

		return qfalse;
	}

	if ( client->pers.connected == CON_DISCONNECTED )
    {
		// we just checked if connecting player can join server
		// so quit now as some important data like player team is still not set
		return qtrue;
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	if ( !strcmp( s, "localhost" ) )
    {
		client->pers.localClient = qtrue;
	} else
    {
		client->pers.localClient = qfalse;
	}

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	if ( !atoi( s ) )
    {
		client->pers.predictItemPickup = qfalse;
	} else
	{
		client->pers.predictItemPickup = qtrue;
	}

	// set name
	Q_strncpyz( oldname, client->pers.netname, sizeof( oldname ) );
	s = Info_ValueForKey( userinfo, "name" );
	BG_CleanName( s, client->pers.netname, sizeof( client->pers.netname ), "UnnamedPlayer" );

	if ( client->sess.sessionTeam == TEAM_SPECTATOR )
    {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD )
		{
			Q_strncpyz( client->pers.netname, "scoreboard", sizeof(client->pers.netname) );
		}
	}

	if ( client->pers.connected == CON_CONNECTED )
    {
		if ( strcmp( oldname, client->pers.netname ) )
		{
			G_BroadcastServerCommand( -1, va("print \"%s" S_COLOR_WHITE " renamed to %s\n\"", oldname, client->pers.netname) );
		}
	}

	// set max health
	health = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	client->pers.maxHealth = health;
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > HEALTH_SOFT_LIMIT ) {
		client->pers.maxHealth = HEALTH_SOFT_LIMIT;
	}

	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	// set model
	Q_strncpyz( model, Info_ValueForKey( userinfo, "model" ), sizeof( model ) );
	Q_strncpyz( headModel, Info_ValueForKey( userinfo, "headmodel" ), sizeof( headModel ) );
	
	team = client->sess.sessionTeam;
	/*
	if ( dfx_gametype.integer == GT_TEAMRUN ) {
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "g" ) ) {
			team = TEAM_BLUE;
		} else if ( !Q_stricmp( s, "green" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_GREEN;
		} else if ( !Q_stricmp( s, "yellow" ) || !Q_stricmp( s, "y" ) ) {
			team = TEAM_YELLOW;
		}
	} 
*/
	// team task (0 = none, 1 = offence, 2 = defence)
	teamTask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	// team Leader (1 = leader, 0 is normal player)
	teamLeader = client->sess.teamLeader;

	// colors
	Q_strncpyz( c1, Info_ValueForKey( userinfo, "color1" ), sizeof( c1 ) );
	Q_strncpyz( c2, Info_ValueForKey( userinfo, "color2" ), sizeof( c2 ) );

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds

    s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\c1\\%s\\c2\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d",
        client->pers.netname, client->sess.sessionTeam, model, headModel, c1, c2,
        client->pers.maxHealth, client->sess.wins, client->sess.losses, teamTask, teamLeader );


	trap_SetConfigstring( CS_PLAYERS + clientNum, s );

	// this is not the userinfo, more like the configstring actually
	G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, s );

	return qtrue;
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
const char *ClientConnect( int clientNum, qbool firstTime, qbool isBot )
{
	char		*value;
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];
	gentity_t	*ent;
	qbool	isAdmin;

	if ( clientNum >= level.maxclients )
		return "Bad connection slot.";

	ent = &g_entities[ clientNum ];
	ent->client = level.clients + clientNum;

	if ( firstTime )
    {
		// cleanup previous data manually
		// because client may silently (re)connect without ClientDisconnect in case of crash for example
		if ( level.clients[ clientNum ].pers.connected != CON_DISCONNECTED )
			ClientDisconnect( clientNum );

		// remove old entity from the world
		trap_UnlinkEntity( ent );
		ent->r.contents = 0;
		ent->s.eType = ET_INVISIBLE;
		ent->s.eFlags = 0;
		ent->s.modelindex = 0;
		ent->s.clientNum = clientNum;
		ent->s.number = clientNum;
		ent->takedamage = qfalse;
	}

	ent->inuse = qfalse;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

 	// IP filtering
 	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=500
 	// recommanding PB based IP / GUID banning, the builtin system is pretty limited
 	// check to see if they are on the banned IP list
	value = Info_ValueForKey( userinfo, "ip" );

	if ( !strcmp( value, "localhost" ) )
		isAdmin = qtrue;
	else
		isAdmin = qfalse;

	if ( !isAdmin && G_FilterPacket( value ) )
    {
		return "You are banned from this server.";
	}

	// we don't check password for bots and local client
	// NOTE: local client <-> "ip" "localhost"
	// this means this client is not running in our current process
	if (!isAdmin)
    {
		// check for a password
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) )
		{
			value = Info_ValueForKey( userinfo, "password" );
			if ( strcmp( g_password.string, value ) )
				return "Invalid password";
		}
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

	memset( client, 0, sizeof( *client ) );

	client->ps.clientNum = clientNum;

	if ( !ClientUserinfoChanged( clientNum ) )
		return ban_reason;

	// read or initialize the session data
	if ( firstTime || level.newSession )
    {
		value = Info_ValueForKey( userinfo, "team" );
		G_InitSessionData( client, value );
		G_WriteClientSessionData( client );
	}

	G_ReadClientSessionData( client );

	ent->inuse = qtrue;

	// get and distribute relevant paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );

	client->pers.connected = CON_CONNECTING;

	ClientUserinfoChanged( clientNum );

	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime )
    {
		G_BroadcastServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname ) );

		// mute all prints until completely in game
		client->pers.inGame = qfalse;
	} else
	{
		client->pers.inGame = qtrue; // FIXME: read from session data?
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	return NULL;
}


/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin( int clientNum )
{
	gentity_t	*ent;
	gclient_t	*client;
	gentity_t	*tent;
	int			flags;
	int			spawns;

	ent = g_entities + clientNum;
	client = level.clients + clientNum;

	if ( ent->r.linked )
		trap_UnlinkEntity( ent );

	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	if ( client->pers.connected == CON_DISCONNECTED )
        return;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;
	spawns = client->ps.persistant[PERS_SPAWN_COUNT];

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;
	client->ps.persistant[PERS_SPAWN_COUNT] = spawns;

	// locate ent at a spawn point
	ClientSpawn( ent );

	if ( !client->pers.inGame )
		BroadcastTeamChange( client, -1 );

	if ( client->sess.sessionTeam != TEAM_SPECTATOR )
    {
		// send event
		tent = G_TempEntity( client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = ent->s.clientNum;

		client->sess.spectatorTime = 0;

		if ( dfx_gametype.integer != GT_ADVENTURE && !client->pers.inGame )
        {
			G_BroadcastServerCommand( -1, va("print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname) );
		}
	}

	client->pers.inGame = qtrue;

	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	// count current clients and rank for scoreboard
	CalculateRanks();
}


/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn(gentity_t *ent)
{
	int		            index;
	vec3_t	            spawn_origin, spawn_angles;
	gclient_t	        *client;
	int		            i;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	int		            persistant[MAX_PERSISTANT];
	gentity_t	        *spawnPoint;
	int		            flags;
	int		            savedPing;
	int		            accuracy_hits, accuracy_shots;
	int		            eventSequence;
	char	            userinfo[MAX_INFO_STRING];
	qbool               isSpectator;

	index = ent - g_entities;
	client = ent->client;

	trap_UnlinkEntity( ent );

	isSpectator = client->sess.sessionTeam == TEAM_SPECTATOR;
	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( isSpectator ) {
		spawnPoint = SelectSpectatorSpawnPoint ( spawn_origin, spawn_angles );
	} else if ( dfx_gametype.integer == GT_TEAMRUN ) {
		spawnPoint = SelectTeamSpawnPoint ( ent, spawn_origin, spawn_angles );
	} else if ( client->pers.useRespawnPoint ) {
        VectorCopy( client->pers.respawnOrigin, spawn_origin );
		VectorCopy( client->pers.respawnAngles, spawn_angles );
		// still want any items that the nearest spawn point gives
		spawnPoint = SelectNearestDeathmatchSpawnPoint( spawn_origin );
	} else {
		do {
			// the first spawn should be at a good looking spot
			if ( !client->pers.initialSpawn && client->pers.localClient ) {
				client->pers.initialSpawn = qtrue;
				spawnPoint = SelectInitialSpawnPoint( ent, spawn_origin, spawn_angles );
			} else {
				// don't spawn near existing origin if possible
				spawnPoint = SelectSpawnPoint( ent, client->ps.origin, spawn_origin, spawn_angles );
			}

			break;

		} while ( 1 );
	}
	client->pers.teamState.state = TEAM_ACTIVE;	
		
	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = client->ps.eFlags & (EF_TELEPORT_BIT | EF_VOTED );
	flags ^= EF_TELEPORT_BIT;

	// unlagged
	G_ResetHistory( ent );
	client->saved.leveltime = 0;

	// clear everything but the persistant data
	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;
	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		persistant[i] = client->ps.persistant[i];
	}
	eventSequence = client->ps.eventSequence;

	Com_Memset (client, 0, sizeof(*client));

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;
	client->lastkilled_client = -1;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}
	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;
	client->ps.persistant[PERS_SCORE] = 0;

	client->airOutTime = level.time + 12000;

	trap_GetUserinfo( index, userinfo, sizeof(userinfo) );
	// set max health
	client->pers.maxHealth = atoi( Info_ValueForKey( userinfo, "handicap" ) );
	if ( client->pers.maxHealth < 1 || client->pers.maxHealth > HEALTH_SOFT_LIMIT ) {
		client->pers.maxHealth = HEALTH_SOFT_LIMIT;
	}
	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->inuse = qtrue;
	ent->classname = "player";
	if ( isSpectator ) {
		ent->takedamage = qfalse;
		ent->r.contents = 0;
		ent->clipmask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
		client->ps.pm_type = PM_SPECTATOR;
	} else {
		ent->takedamage = qtrue;
		ent->r.contents = CONTENTS_BODY;
		ent->clipmask = MASK_PLAYERSOLID;
	}

	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	VectorCopy (playerMins, ent->r.mins);
	VectorCopy (playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;
	
	switch ( dfx_gametype.integer ){
    case GT_FASTCAP:
        if ( dfx_fastcapmode.integer == FC_ROCKET ) {
            client->ps.stats[STAT_WEAPONS] = (1<<WP_ROCKET_LAUNCHER);   // rocket launcher mode, only RL, infinite ammo
            client->ps.ammo[WP_ROCKET_LAUNCHER] = -1;
        }
        else if ( dfx_fastcapmode.integer == FC_PLASMA ) {
            client->ps.stats[STAT_WEAPONS] = (1<<WP_PLASMAGUN);         // plasma gun mode, only PG, infinite ammo
            client->ps.ammo[WP_PLASMAGUN] = -1;
        }
        else if ( dfx_fastcapmode.integer == FC_COMBO ) {
            client->ps.stats[STAT_WEAPONS] = (1<<WP_ROCKET_LAUNCHER) + (1<<WP_PLASMAGUN) + (1<<WP_GRENADE_LAUNCHER);  // combo mode, PG+RL+GL, infinite ammo
            client->ps.ammo[WP_PLASMAGUN] = -1;
            client->ps.ammo[WP_ROCKET_LAUNCHER] = -1;
            client->ps.ammo[WP_GRENADE_LAUNCHER] = -1;
        }
        else {
            client->ps.stats[STAT_WEAPONS] = (1<<WP_GAUNTLET) + (1<<WP_MACHINEGUN);
            client->ps.ammo[WP_MACHINEGUN] = 100;
        }
        break;
    case GT_ADVENTURE:
        client->ps.stats[STAT_WEAPONS] = (1<<WP_GAUNTLET) - 1;
        break;
    case GT_DEFRAG:
        if (dfx_mode.integer <= DF_STRAFE || dfx_mode.integer >= DF_ICEMODE) {         // SLK: default, strafe and ice mode spawn with gauntlet and machine gun
            client->ps.stats[STAT_WEAPONS] = (1<<WP_GAUNTLET) + (1<<WP_MACHINEGUN);
            client->ps.ammo[WP_MACHINEGUN] = 100;
        }
        else if (dfx_mode.integer == DF_ROCKET) {
            client->ps.stats[STAT_WEAPONS] = (1<<WP_ROCKET_LAUNCHER);   // rocket launcher mode, only RL, infinite ammo
            client->ps.ammo[WP_ROCKET_LAUNCHER] = -1;
        }
        else if (dfx_mode.integer == DF_PLASMA) {
            client->ps.stats[STAT_WEAPONS] = (1<<WP_PLASMAGUN);         // plasma gun mode, only PG, infinite ammo
            client->ps.ammo[WP_PLASMAGUN] = -1;
        }
        else if (dfx_mode.integer == DF_COMBO) {
            client->ps.stats[STAT_WEAPONS] = (1<<WP_ROCKET_LAUNCHER) + (1<<WP_PLASMAGUN) + (1<<WP_GRENADE_LAUNCHER);    // combo mode, PG+RL+GL, infinite ammo
            client->ps.ammo[WP_PLASMAGUN] = -1;
            client->ps.ammo[WP_ROCKET_LAUNCHER] = -1;
            client->ps.ammo[WP_GRENADE_LAUNCHER] = -1;
        }

        break;

    case GT_TEAMRUN:
        client->ps.stats[STAT_WEAPONS] = (1<<WP_GAUNTLET) + (1<<WP_MACHINEGUN);
        client->ps.ammo[WP_MACHINEGUN] = 50;
        break;

    case GT_FREESTYLE:
        client->ps.stats[STAT_WEAPONS] = (1<<WP_NUM_WEAPONS-1) - 1;
        client->ps.ammo[WP_MACHINEGUN] = -1;
        client->ps.ammo[WP_GRENADE_LAUNCHER] = -1;
        client->ps.ammo[WP_LIGHTNING] = -1;
        client->ps.ammo[WP_PLASMAGUN] = -1;
        client->ps.ammo[WP_RAILGUN] = -1;
        client->ps.ammo[WP_ROCKET_LAUNCHER] = -1;
        client->ps.ammo[WP_SHOTGUN] = -1;
        client->ps.ammo[WP_BFG] = -1;
        break;
	}

	client->ps.ammo[WP_GAUNTLET] = -1;
    client->ps.ammo[WP_GRAPPLING_HOOK] = -1;

	// health will count down towards max_health
	ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] + 25;

	//set secrets
    if ( client->sess.secrets )
		client->ps.persistant[PERS_SECRETS] = client->sess.secrets;

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, spawn_angles );

	// entity should be unlinked before calling G_KillBox()
	if ( !isSpectator )
		G_KillBox( ent );

	// force the base weapon up
	if (dfx_gametype.integer != GT_ADVENTURE )
        client->ps.weapon = WP_MACHINEGUN;
    else
        client->ps.weapon = WP_GAUNTLET;

	client->ps.weaponstate = WEAPON_READY;

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	// set default animations
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if ( level.intermissiontime )
    {
		MoveClientToIntermission( ent );
	} 
	else
	{
		if ( !isSpectator )
			trap_LinkEntity( ent );
		
		// fire the targets of the spawn point
		G_UseTargets( spawnPoint, ent );

		// select the highest weapon number available, after any
		// spawn given items have fired
		client->ps.weapon = 1;
		for ( i = WP_NUM_WEAPONS - 1 ; i > 0 ; i-- )
        {
			if (client->ps.stats[STAT_WEAPONS] & (1 << i) && client->ps.ammo[i])
			{
				client->ps.weapon = i;
				break;
			}
		}
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	client->pers.cmd.serverTime = level.time;
	ClientThink( ent-g_entities );

	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
	VectorCopy( client->ps.origin, ent->r.currentOrigin );

	// run the presend to set anything else
	ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

	// reset trigger_multiple wait table
	for ( i = 0; i < MAX_GENTITIES; i++ )
    {
		if ( g_entities[i].targetname && g_entities[i].item  ) {
            // targeted items don't spawn until triggered
			g_entities[i].waittable[client->ps.clientNum] = -1;
		} else {
			g_entities[i].waittable[client->ps.clientNum] = 0;
		}
	}
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum )
{
	gentity_t	*ent;
	gentity_t	*tent;
	int			i;

	ent = g_entities + clientNum;
	if (!ent->client) // || ent->client->pers.connected == CON_DISCONNECTED)
		return;

	// stop any following clients
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR
          && level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
          && level.clients[i].sess.spectatorClient == clientNum )
        {
                StopFollowing( &g_entities[i], qtrue );
        }
	}

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED && ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = ent->s.clientNum;
	}

	G_RevertVote( ent->client );

	G_LogPrintf( "ClientDisconnect: %i\n", clientNum );

	trap_UnlinkEntity( ent );
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;

	trap_SetConfigstring( CS_PLAYERS + clientNum, "" );

	G_ClearClientSessionData( ent->client );

	CalculateRanks();
}
