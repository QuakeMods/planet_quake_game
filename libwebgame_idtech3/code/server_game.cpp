/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "server.h"

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
int	SV_NumForGentity( sharedEntity_t *ent ) {
	int num = ( (byte *)ent - (byte *)sv.gentities ) / sv.gentitySize;
	return num;
}

sharedEntity_t *SV_GentityNum( int num ) {
	sharedEntity_t *ent = (sharedEntity_t *)((byte *)sv.gentities + sv.gentitySize*(num));
	return ent;
}

playerState_t *SV_GameClientNum( int num ) {
	playerState_t *ps = (playerState_t *)((byte *)sv.gameClients + sv.gameClientSize*(num));
	return ps;
}

svEntity_t	*SV_SvEntityForGentity( sharedEntity_t *gEnt ) {
	if ( !gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES ) {
		Com_Error( ERR_DROP, "SV_SvEntityForGentity: bad gEnt" );
	}
	return &sv.svEntities[ gEnt->s.number ];
}

sharedEntity_t *SV_GEntityForSvEntity( svEntity_t *svEnt ) {
	int num = svEnt - sv.svEntities;
	return SV_GentityNum( num );
}

// Sends a command string to a client
void SV_GameSendServerCommand( int clientNum, const char *text ) {
	if ( clientNum == -1 ) {
		SV_SendServerCommand( NULL, "%s", text );
	} else {
		if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
			return;
		}
		SV_SendServerCommand( svs.clients + clientNum, "%s", text );	
	}
}

// Disconnects the client with a message
void SV_GameDropClient( int clientNum, const char *reason ) {
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		return;
	}
	SV_DropClient( svs.clients + clientNum, reason );	
}

// sets mins and maxs for inline bmodels
void SV_SetBrushModel( sharedEntity_t *ent, const char *name ) {
	clipHandle_t	h;
	vec3_t			mins, maxs;
	if (!name) {
		Com_Error( ERR_DROP, "SV_SetBrushModel: NULL" );
	}
	if (name[0] != '*') {
		Com_Error( ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name );
	}
	ent->s.modelindex = atoi( name + 1 );
	h = CM_InlineModel( ent->s.modelindex );
	CM_ModelBounds( h, mins, maxs );
	VectorCopy (mins, ent->r.mins);
	VectorCopy (maxs, ent->r.maxs);
	ent->r.bmodel = qtrue;
	ent->r.contents = -1;		// we don't know exactly what is in the brushes
	SV_LinkEntity( ent );		// FIXME: remove
}

// Also checks portalareas so that doors block sight
qboolean SV_inPVS (const vec3_t p1, const vec3_t p2) {
	int		leafnum;
	int		cluster;
	int		area1, area2;
	byte	*mask;
	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1 = CM_LeafArea (leafnum);
	mask = CM_ClusterPVS (cluster);
	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2 = CM_LeafArea (leafnum);
	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return qfalse;
	if (!CM_AreasConnected (area1, area2))
		return qfalse;		// a door blocks sight
	return qtrue;
}

// Does NOT check portalareas
qboolean SV_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2) {
	int leafnum = CM_PointLeafnum (p1);
	int cluster = CM_LeafCluster (leafnum);
	byte *mask = CM_ClusterPVS (cluster);
	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return qfalse;
	return qtrue;
}

void SV_AdjustAreaPortalState( sharedEntity_t *ent, qboolean open ) {
	svEntity_t *svEnt = SV_SvEntityForGentity( ent );
	if ( svEnt->areanum2 == -1 ) {
		return;
	}
	CM_AdjustAreaPortalState( svEnt->areanum, svEnt->areanum2, open );
}

// check for exact collision
qboolean	SV_EntityContact( vec3_t mins, vec3_t maxs, const sharedEntity_t *gEnt, int capsule ) {
	trace_t trace;
	clipHandle_t ch = SV_ClipHandleForEntity(gEnt);
	const float *origin = gEnt->r.currentOrigin;
	const float *angles = gEnt->r.currentAngles;
	CM_TransformedBoxTrace(&trace, vec3_origin, vec3_origin, mins, maxs, ch, -1, origin, angles, capsule);
	return trace.startsolid;
}

void SV_GetServerinfo( char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize );
	}
	Q_strncpyz( buffer, Cvar_InfoString( CVAR_SERVERINFO ), bufferSize );
}

void SV_LocateGameData( sharedEntity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGameClient ) {
	sv.gentities = gEnts;
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;
	sv.gameClients = clients;
	sv.gameClientSize = sizeofGameClient;
}

void SV_GetUsercmd( int clientNum, usercmd_t *cmd ) {
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum );
	}
	*cmd = svs.clients[clientNum].lastUsercmd;
}

//Called every time a map changes
void SV_ShutdownGameProgs() {
	G_ShutdownGame(qfalse);
	sv.gentities = NULL;
}

// Called for both a full init and a restart
void SV_InitGameVM( qboolean restart ) {
	// start the entity parsing at the beginning
	sv.entityParsePoint = CM_EntityString();
	// clear all gentity pointers that might still be set from
	// a previous level
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=522
	//   now done before GAME_INIT call
	for (int i=0; i<sv_maxclients->integer; i++) {
		svs.clients[i].gentity = NULL;
	}
	// use the current msec count for a random seed
	// init for this gamestate
	G_InitGame(sv.time, Com_Milliseconds(), restart);
#if EMSCRIPTEN
	EM_ASM_({
		callback_SV_InitGameVM($0);
	}, restart);
#endif
}

// Called on a map_restart, but not on a normal map change
void SV_RestartGameProgs( void ) {
	sv.gentities = NULL;
	G_ShutdownGame(qtrue);
	SV_InitGameVM( qtrue );
}

// Called on a normal map change, not on a map_restart
void SV_InitGameProgs( void ) {
	SV_InitGameVM( qfalse );
}

qboolean ConsoleCommand();

// See if the current console command is claimed by the game
qboolean SV_GameCommand( void ) {
	if ( sv.state != SS_GAME ) {
		return qfalse;
	}
	return ConsoleCommand();
}
