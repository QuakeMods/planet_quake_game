/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2013 Darklegion Development
Copyright (C) 2012-2018 ET:Legacy team <mail@etlegacy.com>
Copyright (C) 2015-2019 GrangerHub

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, see <https://www.gnu.org/licenses/>

===========================================================================
*/

#include "server.h"

// Attack log file is started when server is init (!= sv_running 1!)
// we even log attacks when the server is waiting for rcon and doesn't run a map
int attHandle = 0; // server attack log file handle


char alternateInfos[2][2][BIG_INFO_STRING];

static playMap_t playMap[MAX_PLAYMAP_QUEUE_ENTRIES];
/*
===============
SV_PlayMap_Save_Queue_Entry

Used by the PlayMap system to save a queue entry
===============
*/
void SV_PlayMap_Save_Queue_Entry( playMap_t pm, int index ) {
	assert(
		index >= 0 &&
		index < MAX_PLAYMAP_QUEUE_ENTRIES &&
		"SV_PlayMap_Save_Queue_For_Index: invalid index");

	playMap[index] = pm;
}

/*
===============
SV_PlayMap_Clear_Saved_Queue

Clears the saved PlayMap queue, when the server inits, and before the queue is saved
===============
*/
void SV_PlayMap_Clear_Saved_Queue( int default_flags ) {
	int index;

	for(index = 0; index < MAX_PLAYMAP_QUEUE_ENTRIES; index++) {
		playMap[index].console = qfalse;
		playMap[index].guid[0] = '\0';
		playMap[index].layout[0] = '\0';
		playMap[index].mapName[0] = '\0';
		playMap[index].flags = default_flags;
	}
}

/*
===============
SV_PlayMap_Save_Queue_Entry

Used by the PlayMap system to get a saved queue entry
===============
*/
playMap_t SV_PlayMap_Get_Queue_Entry( int index ) {
	assert(
		index >= 0 &&
		index < MAX_PLAYMAP_QUEUE_ENTRIES &&
		"SV_PlayMap_Save_Queue_For_Index: invalid index");

	return playMap[index];
}

static size_t roster_id = 0;
static pers_scrim_t scrim;
static const pers_scrim_t scrim_defaults =
{
	0,           //int                    mode;
	0,           //int                    win_condition;
	qfalse,      //qboolean               timed_income;
	2,           //int                    sudden_death_mode;
	25,          //int                    sudden_death_time;
	35,          //int                    time_limit;
	{            //pers_scrim_team_info_t team[];
		{            //pers_scrim_team_info_t team[0];
			"",          //char   name[MAX_COLORFUL_NAME_LENGTH];
			0,           //int    current_team;
			qfalse,      //qboolean has_captain;
			"",          //char     captain_guid[ 33 ];
			0,           //int    wins;
			0,           //int    losses;
			0,           //int    draws;
			{              //scrim_team_roster_t roster;
				{              //scrim_team_member_t members[];
					{              //scrim_team_member_t members[0];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[1];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[2];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[3];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[4];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[5];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[6];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[7];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[8];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[9];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[10];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[11];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[12];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[13];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[14];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[15];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[16];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[17];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[18];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[19];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[20];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[21];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[22];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[23];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[24];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[25];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[26];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[27];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[28];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[29];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[30];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[31];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[32];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[33];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[34];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[35];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[36];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[37];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[38];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[39];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[40];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[41];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[42];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[43];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[44];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[45];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[46];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[47];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[48];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[49];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[50];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[51];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[52];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[53];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[54];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[55];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[56];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[57];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[58];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[59];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[60];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[61];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[62];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[63];
						.inuse = qfalse //qboolean inuse
					}
				}
			}
		},
		{            //pers_scrim_team_info_t team[1];
			"Red",       //char   name[MAX_COLORFUL_NAME_LENGTH];
			1,           //int    current_team;
			qfalse,      //qboolean has_captain;
			"",          //char     captain_guid[ 33 ];
			0,           //int    wins;
			0,           //int    losses;
			0,           //int    draws;
			{              //scrim_team_roster_t roster;
				{              //scrim_team_member_t members[];
					{              //scrim_team_member_t members[0];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[1];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[2];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[3];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[4];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[5];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[6];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[7];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[8];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[9];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[10];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[11];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[12];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[13];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[14];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[15];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[16];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[17];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[18];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[19];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[20];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[21];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[22];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[23];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[24];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[25];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[26];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[27];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[28];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[29];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[30];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[31];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[32];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[33];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[34];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[35];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[36];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[37];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[38];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[39];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[40];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[41];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[42];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[43];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[44];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[45];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[46];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[47];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[48];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[49];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[50];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[51];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[52];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[53];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[54];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[55];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[56];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[57];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[58];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[59];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[60];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[61];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[62];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[63];
						.inuse = qfalse //qboolean inuse
					}
				}
			}
		},
		{            //pers_scrim_team_info_t team[2];
			"Blue",      //char   name[MAX_COLORFUL_NAME_LENGTH];
			2,           //int    current_team;
			qfalse,      //qboolean has_captain;
			"",          //char     captain_guid[ 33 ];
			0,           //int    wins;
			0,           //int    losses;
			0,           //int    draws;
			{              //scrim_team_roster_t roster;
				{              //scrim_team_member_t members[];
					{              //scrim_team_member_t members[0];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[1];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[2];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[3];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[4];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[5];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[6];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[7];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[8];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[9];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[10];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[11];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[12];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[13];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[14];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[15];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[16];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[17];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[18];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[19];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[20];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[21];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[22];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[23];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[24];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[25];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[26];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[27];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[28];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[29];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[30];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[31];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[32];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[33];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[34];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[35];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[36];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[37];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[38];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[39];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[40];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[41];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[42];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[43];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[44];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[45];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[46];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[47];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[48];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[49];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[50];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[51];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[52];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[53];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[54];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[55];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[56];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[57];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[58];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[59];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[60];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[61];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[62];
						.inuse = qfalse //qboolean inuse
					},
					{              //scrim_team_member_t members[63];
						.inuse = qfalse //qboolean inuse
					}
				}
			}
		},
	},
	0,           //int                    previous_round_win;
	0,           //int                    rounds_completed;
	5,           //int                    max_rounds;
};

/*
===============
SV_Scrim_Init
===============
*/
void SV_Scrim_Init(void) {
	scrim = scrim_defaults;
	roster_id = 0;
}

/*
===============
SV_Scrim_Save
===============
*/
void SV_Scrim_Save(pers_scrim_t *scrim_input) {
	scrim = *scrim_input;
}

/*
===============
SV_Scrim_Load
===============
*/
void SV_Scrim_Load(pers_scrim_t *scrim_input) {
	*scrim_input = scrim;
}

/*
===============
SV_Scrim_Get_New_Roster_ID
===============
*/
size_t SV_Scrim_Get_New_Roster_ID(void) {
	return (roster_id++);
}

/*
===============
SV_Scrim_Get_Last_Roster_ID
===============
*/
size_t SV_Scrim_Get_Last_Roster_ID(void) {
	return roster_id;
}

/*
===============
SV_SendConfigstring

Creates and sends the server command necessary to update the CS index for the
given client
===============
*/
static void SV_SendConfigstring(client_t *client, int index)
{
	const char *configstring;
	int maxChunkSize = MAX_STRING_CHARS - 24;
	int len;

	if( sv.configstrings[index].restricted && Com_ClientListContains(
		&sv.configstrings[index].clientList, client - svs.clients ) ) {
		// Send a blank config string for this client if it's listed
		SV_SendServerCommand( client, "cs %i \"\"\n", index );
		return;
	}

	if ( index <= CS_SYSTEMINFO && client->netchan.alternateProtocol != 0 ) {
		configstring = alternateInfos[index][ client->netchan.alternateProtocol - 1 ];
	} else {
		configstring = sv.configstrings[index].s;
	}

	len = strlen(configstring);

	if( len >= maxChunkSize ) {
		int		sent = 0;
		int		remaining = len;
		char	*cmd;
		char	buf[MAX_STRING_CHARS];

		while (remaining > 0 ) {
			if ( sent == 0 ) {
				cmd = "bcs0";
			}
			else if( remaining < maxChunkSize ) {
				cmd = "bcs2";
			}
			else {
				cmd = "bcs1";
			}
			Q_strncpyz( buf, &configstring[sent],
				maxChunkSize );

			SV_SendServerCommand( client, "%s %i \"%s\"\n", cmd,
				index, buf );

			sent += (maxChunkSize - 1);
			remaining -= (maxChunkSize - 1);
		}
	} else {
		// standard cs, just send it
		SV_SendServerCommand( client, "cs %i \"%s\"\n", index,
			configstring );
	}
}

/*
===============
SV_UpdateConfigstrings

Called when a client goes from CS_PRIMED to CS_ACTIVE.  Updates all
Configstring indexes that have changed while the client was in CS_PRIMED
===============
*/
void SV_UpdateConfigstrings(client_t *client)
{
	int index;

	for( index = 0; index < MAX_CONFIGSTRINGS; index++ ) {
		// if the CS hasn't changed since we went to CS_PRIMED, ignore
		if(!client->csUpdated[index])
			continue;

		// do not always send server info to all clients
		if ( index == CS_SERVERINFO && client->gentity &&
			(client->gentity->r.svFlags & SVF_NOSERVERINFO) ) {
			continue;
		}
		SV_SendConfigstring(client, index);
		client->csUpdated[index] = qfalse;
	}
}

/*
===============
SV_SetConfigstring

===============
*/
void SV_SetConfigstring (int index, const char *val) {
	qboolean modified[3] = { qfalse, qfalse, qfalse };
	int		i;
	client_t	*client;

	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error (ERR_DROP, "SV_SetConfigstring: bad index %i", index);
	}

	if ( !val ) {
		val = "";
	}

	if ( index <= CS_SYSTEMINFO ) {
		for ( i = 1; i < 3; ++i ) {
			char info[BIG_INFO_STRING];

			Q_strncpyz( info, val, sizeof( info ) );
			if ( index == CS_SERVERINFO ) {
				Info_SetValueForKey_Big( info, "protocol", ( i == 1 ? "70" : "69" ) );
			} else if ( i == 2 ) {
				Info_SetValueForKey_Big( info, "sv_paks", Cvar_VariableString( "sv_alternatePaks" ) );
				Info_SetValueForKey_Big( info, "sv_pakNames", Cvar_VariableString( "sv_alternatePakNames" ) );
				Info_SetValueForKey_Big( info, "sv_referencedPaks", Cvar_VariableString( "sv_referencedAlternatePaks" ) );
				Info_SetValueForKey_Big( info, "sv_referencedPakNames", Cvar_VariableString( "sv_referencedAlternatePakNames" ) );
				Info_SetValueForKey_Big( info, "cl_allowDownload", "1, you should set it yourself" );
				if ( !( sv_allowDownload->integer & DLF_NO_REDIRECT ) ) {
					Info_SetValueForKey_Big( info, "sv_wwwBaseURL", Cvar_VariableString( "sv_dlUrl" ) );
					Info_SetValueForKey_Big( info, "sv_wwwDownload", Cvar_VariableString( "1, you should set it yourself" ) );
				}
			}

			if ( strcmp( info, alternateInfos[index][i - 1] ) ) {
				modified[i] = qtrue;
				strcpy( alternateInfos[index][i - 1], info );
			}
		}

		if ( strcmp( val, sv.configstrings[index].s ) ) {
			modified[0] = qtrue;
			Z_Free( sv.configstrings[index].s );
			sv.configstrings[index].s = CopyString( val );
		}

		if ( !modified[0] && !modified[1] && !modified[2] ) {
			return;
		}
	} else {
		// don't bother broadcasting an update if no change
		if ( !strcmp( val, sv.configstrings[ index ].s ) ) {
			return;
		}

		// change the string in sv
		Z_Free( sv.configstrings[index].s );
		sv.configstrings[index].s = CopyString( val );
	}

	// send it to all the clients if we aren't
	// spawning a new server
	if ( sv.state == SS_GAME || sv.restarting ) {

		// send the data to all relevent clients
		for (i = 0, client = svs.clients; i < sv_maxclients->integer ; i++, client++) {
			if ( index <= CS_SYSTEMINFO && !modified[ client->netchan.alternateProtocol ] ) {
				continue;
			}

			if ( client->state < CS_ACTIVE ) {
				if ( client->state == CS_PRIMED )
					client->csUpdated[ index ] = qtrue;
				continue;
			}
			// do not always send server info to all clients
			if ( index == CS_SERVERINFO && client->gentity && (client->gentity->r.svFlags & SVF_NOSERVERINFO) ) {
				continue;
			}

			SV_SendConfigstring(client, index);
		}
	}
}

/*
===============
SV_GetConfigstring

===============
*/
void SV_GetConfigstring( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetConfigstring: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error (ERR_DROP, "SV_GetConfigstring: bad index %i", index);
	}
	if ( !sv.configstrings[index].s ) {
		buffer[0] = 0;
		return;
	}

	Q_strncpyz( buffer, sv.configstrings[index].s, bufferSize );
}

/*
===============
SV_SetConfigstringRestrictions
===============
*/
void SV_SetConfigstringRestrictions (int index, const clientList_t* clientList) {
	int i;
	clientList_t oldClientList = sv.configstrings[index].clientList;

	sv.configstrings[index].clientList = *clientList;
	sv.configstrings[index].restricted = qtrue;

	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			if ( Com_ClientListContains( &oldClientList, i ) !=
				Com_ClientListContains( clientList, i ) ) {
				// A client has left or joined the restricted list, so update
				SV_SendConfigstring(&svs.clients[i], index);
			}
		}
	}
}


/*
===============
SV_SetUserinfo

===============
*/
void SV_SetUserinfo( int index, const char *val ) {
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error (ERR_DROP, "SV_SetUserinfo: bad index %i", index);
	}

	if ( !val ) {
		val = "";
	}

	Q_strncpyz( svs.clients[index].userinfo, val, sizeof( svs.clients[ index ].userinfo ) );
	Q_strncpyz( svs.clients[index].name, Info_ValueForKey( val, "name" ), sizeof(svs.clients[index].name) );
	//for backwards compatibility approx hex color codes to hardcoded ansi
	//color codes
	Q_ApproxStrHexColors(
		svs.clients[index].name, svs.clients[index].name_ansi,
		sizeof(svs.clients[index].name), sizeof(svs.clients[index].name_ansi));
}



/*
===============
SV_GetUserinfo

===============
*/
void SV_GetUserinfo( int index, char *buffer, int bufferSize ) {
	if ( bufferSize < 1 ) {
		Com_Error( ERR_DROP, "SV_GetUserinfo: bufferSize == %i", bufferSize );
	}
	if ( index < 0 || index >= sv_maxclients->integer ) {
		Com_Error (ERR_DROP, "SV_GetUserinfo: bad index %i", index);
	}
	Q_strncpyz( buffer, svs.clients[ index ].userinfo, bufferSize );
}


/*
================
SV_SetNumbers

Remnant from SV_CreateBaseline().
================
*/

static void SV_SetNumbers( void ) {
	sharedEntity_t *svent;
	int				entnum;

	for ( entnum = 1; entnum < sv.num_entities ; entnum++ ) {
		svent = SV_GentityNum(entnum);
		if (!svent->r.linked) {
			continue;
		}
		svent->s.number = entnum;
	}
}


/*
===============
SV_BoundMaxClients

===============
*/
static void SV_BoundMaxClients( int minimum ) {
	// get the current maxclients value
	Cvar_Get( "sv_maxclients", "8", 0 );

	sv_maxclients->modified = qfalse;

	if ( sv_maxclients->integer < minimum ) {
		Cvar_Set( "sv_maxclients", va("%i", minimum) );
	} else if ( sv_maxclients->integer > MAX_CLIENTS ) {
		Cvar_Set( "sv_maxclients", va("%i", MAX_CLIENTS) );
	}
}


/*
===============
SV_Startup

Called when a host starts a map when it wasn't running
one before.  Successive map or map_restart commands will
NOT cause this to be called, unless the game is exited to
the menu system first.
===============
*/
static void SV_Startup( void ) {
	if ( svs.initialized ) {
		Com_Error( ERR_FATAL, "SV_Startup: svs.initialized" );
	}
	SV_BoundMaxClients( 1 );

	svs.clients = Z_Malloc (sizeof(client_t) * sv_maxclients->integer );
	if ( com_dedicated->integer ) {
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES;
	} else {
		// we don't need nearly as many when playing locally
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * MAX_SNAPSHOT_ENTITIES;
	}
	svs.initialized = qtrue;

	// Don't respect sv_killserver unless a server is actually running
	if ( sv_killserver->integer ) {
		Cvar_Set( "sv_killserver", "0" );
	}

	Cvar_Set( "sv_running", "1" );

	// Join the ipv6 multicast group now that a map is running so clients can scan for us on the local network.
	NET_JoinMulticast6();
}


/*
==================
SV_ChangeMaxClients
==================
*/
void SV_ChangeMaxClients( void ) {
	int		oldMaxClients;
	int		i;
	client_t	*oldClients;
	int		count;

	// get the highest client number in use
	count = 0;
	for ( i = 0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			if (i > count)
				count = i;
		}
	}
	count++;

	oldMaxClients = sv_maxclients->integer;
	// never go below the highest client number in use
	SV_BoundMaxClients( count );
	// if still the same
	if ( sv_maxclients->integer == oldMaxClients ) {
		return;
	}

	oldClients = Hunk_AllocateTempMemory( count * sizeof(client_t) );
	// copy the clients to hunk memory
	for ( i = 0 ; i < count ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			oldClients[i] = svs.clients[i];
		}
		else {
			Com_Memset(&oldClients[i], 0, sizeof(client_t));
		}
	}

	// free old clients arrays
	Z_Free( svs.clients );

	// allocate new clients
	svs.clients = Z_Malloc ( sv_maxclients->integer * sizeof(client_t) );
	Com_Memset( svs.clients, 0, sv_maxclients->integer * sizeof(client_t) );

	// copy the clients over
	for ( i = 0 ; i < count ; i++ ) {
		if ( oldClients[i].state >= CS_CONNECTED ) {
			svs.clients[i] = oldClients[i];
		}
	}

	// free the old clients on the hunk
	Hunk_FreeTempMemory( oldClients );

	// allocate new snapshot entities
	if ( com_dedicated->integer ) {
		svs.numSnapshotEntities = sv_maxclients->integer * PACKET_BACKUP * MAX_SNAPSHOT_ENTITIES;
	} else {
		// we don't need nearly as many when playing locally
		svs.numSnapshotEntities = sv_maxclients->integer * 4 * MAX_SNAPSHOT_ENTITIES;
	}
}

/*
================
SV_ClearServer
================
*/
static void SV_ClearServer(void) {
	int i;

	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( i <= CS_SYSTEMINFO ) {
			alternateInfos[i][0][0] = alternateInfos[i][1][0] = '\0';
		}
		if ( sv.configstrings[i].s ) {
			Z_Free( sv.configstrings[i].s );
		}
	}
	Com_Memset (&sv, 0, sizeof(sv));
}

/*
================
SV_TouchCGame

  touch the cgame.vm so that a pure client can load it if it's in a seperate pk3
================
*/
static void SV_TouchCGame(void) {
	fileHandle_t	f;
	char filename[MAX_QPATH];

	Com_sprintf( filename, sizeof(filename), "vm/%s.qvm", "cgame" );
	FS_FOpenFileRead( filename, &f, qfalse );
	if ( f ) {
		FS_FCloseFile( f );
	}
}

/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.
This is NOT called for map_restart
================
*/
void SV_SpawnServer( char *server, qboolean killBots ) {
	int			i;
	int			checksum;
	char		systemInfo[16384];
	const char	*p;

	// shut down the existing game if it is running
	SV_ShutdownGameProgs();

	Com_Printf ("------ Server Initialization ------\n");
	Com_Printf ("Server: %s\n",server);

	// if not running a dedicated server CL_MapLoading will connect the client to the server
	// also print some status stuff
	CL_MapLoading();

	// make sure all the client stuff is unloaded
	CL_ShutdownAll(qfalse);

	// clear the whole hunk because we're (re)loading the server
	Hunk_Clear();

	// clear collision map data
	CM_ClearMap();

	// init client structures and svs.numSnapshotEntities
	if ( !Cvar_VariableValue("sv_running") ) {
		SV_Startup();
	} else {
		// check for maxclients change
		if ( sv_maxclients->modified ) {
			SV_ChangeMaxClients();
		}
	}

	// clear pak references
	FS_ClearPakReferences(0);

	// allocate the snapshot entities on the hunk
	svs.snapshotEntities = Hunk_Alloc( sizeof(entityState_t)*svs.numSnapshotEntities, h_high );
	svs.nextSnapshotEntities = 0;

	// toggle the server bit so clients can detect that a
	// server has changed
	svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

	for (i=0 ; i<sv_maxclients->integer ; i++) {
		// save when the server started for each client already connected
		if (svs.clients[i].state >= CS_CONNECTED) {
			svs.clients[i].oldServerTime = sv.time;
		}
	}

	// wipe the entire per-level structure
	SV_ClearServer();
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( i <= CS_SYSTEMINFO ) {
			alternateInfos[i][0][0] = alternateInfos[i][1][0] = '\0';
		}
		sv.configstrings[i].s = CopyString("");
		sv.configstrings[i].restricted = qfalse;
		Com_Memset(&sv.configstrings[i].clientList, 0, sizeof(clientList_t));
	}

	// make sure we are not paused
	Cvar_Set("cl_paused", "0");

	// get a new checksum feed and restart the file system
	sv.checksumFeed = ( ((int) rand() << 16) ^ rand() ) ^ Com_Milliseconds();
	FS_Restart( sv.checksumFeed );

	// advertise GPP-compatible extensions
	Cvar_Set( "sv_gppExtension", "1" );

	CM_LoadMap( va("maps/%s.bsp", server), qfalse, &checksum );

	// set serverinfo visible name
	Cvar_Set( "mapname", server );

	Cvar_Set( "sv_mapChecksum", va("%i",checksum) );

	// serverid should be different each time
	sv.serverId = com_frameTime;
	sv.restartedServerId = sv.serverId; // I suppose the init here is just to be safe
	sv.checksumFeedServerId = sv.serverId;
	Cvar_Set( "sv_serverid", va("%i", sv.serverId ) );

	// clear physics interaction links
	SV_ClearWorld ();

	// media configstring setting should be done during
	// the loading stage, so connected clients don't have
	// to load during actual gameplay
	sv.state = SS_LOADING;

	// load and spawn all other entities
	SV_InitGameProgs();

	// run a few frames to allow everything to settle
	for (i = 0;i < 3; i++)
	{
		dll_G_RunFrame( sv.time );
		sv.time += 100;
		svs.time += 100;
	}

	// Set Numbers. Not sure if necessary.
	SV_SetNumbers();

	for (i=0 ; i<sv_maxclients->integer ; i++) {
		// send the new gamestate to all connected clients
		if (svs.clients[i].state >= CS_CONNECTED) {
			char	*denied;

			// connect the client again
			denied = VM_ExplicitArgPtr( gvm, (intptr_t)dll_ClientConnect( i, qfalse ) );	// firstTime = qfalse
			if ( denied ) {
				// this generally shouldn't happen, because the client
				// was connected before the level change
				SV_DropClient( &svs.clients[i], denied );
			} else {
				// when we get the next packet from a connected client,
				// the new gamestate will be sent
				svs.clients[i].state = CS_CONNECTED;
			}
		}
	}

	// run another frame to allow things to look at all the players
	dll_G_RunFrame( sv.time );
	sv.time += 100;
	svs.time += 100;

	if ( sv_pure->integer ) {
		// the server sends these to the clients so they will only
		// load pk3s also loaded at the server
		p = FS_LoadedPakChecksums( qfalse );
		Cvar_Set( "sv_paks", p );
		p = FS_LoadedPakChecksums( qtrue );
		Cvar_Set( "sv_alternatePaks", p );
		if (strlen(p) == 0) {
			Com_Printf( "WARNING: sv_pure set but no PK3 files loaded\n" );
		}
		p = FS_LoadedPakNames( qfalse );
		Cvar_Set( "sv_pakNames", p );
		p = FS_LoadedPakNames( qtrue );
		Cvar_Set( "sv_alternatePakNames", p );

		// if a dedicated pure server we need to touch the cgame because it could be in a
		// seperate pk3 file and the client will need to load the latest cgame.qvm
		if ( com_dedicated->integer ) {
			SV_TouchCGame();
		}
	}
	else {
		Cvar_Set( "sv_paks", "" );
		Cvar_Set( "sv_pakNames", "" );
		Cvar_Set( "sv_alternatePaks", "" );
		Cvar_Set( "sv_alternatePakNames", "" );
	}
	// the server sends these to the clients so they can figure
	// out which pk3s should be auto-downloaded
	p = FS_ReferencedPakChecksums( qfalse );
	Cvar_Set( "sv_referencedPaks", p );
	p = FS_ReferencedPakChecksums( qtrue );
	Cvar_Set( "sv_referencedAlternatePaks", p );
	p = FS_ReferencedPakNames( qfalse );
	Cvar_Set( "sv_referencedPakNames", p );
	p = FS_ReferencedPakNames( qtrue );
	Cvar_Set( "sv_referencedAlternatePakNames", p );

	// save systeminfo and serverinfo strings
	Q_strncpyz( systemInfo, Cvar_InfoString_Big( CVAR_SYSTEMINFO ), sizeof( systemInfo ) );
	cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	SV_SetConfigstring( CS_SYSTEMINFO, systemInfo );

	SV_SetConfigstring( CS_SERVERINFO, Cvar_InfoString( CVAR_SERVERINFO ) );
	cvar_modifiedFlags &= ~CVAR_SERVERINFO;

	// any media configstring setting now should issue a warning
	// and any configstring changes should be reliably transmitted
	// to all clients
	sv.state = SS_GAME;

	// send a heartbeat now so the master will get up to date info
	SV_Heartbeat_f();

	Hunk_SetMark();

#ifndef DEDICATED
	if ( com_dedicated->integer ) {
		// restart renderer in order to show console for dedicated servers
		// launched through the regular binary
		CL_StartHunkUsers( qtrue );
	}
#endif

	Com_Printf ("-----------------------------------\n");
}

/**
 * @brief SV_WriteAttackLog
 * @param[in] log
 */
void SV_WriteAttackLog(const char *log)
{
	if (attHandle > 0)
	{
		char    string[512]; // 512 chars seem enough here
		qtime_t time;

		Com_RealTime(&time);
		Com_sprintf(string, sizeof(string), "%i/%i/%i %i:%i:%i %s", 1900 + time.tm_year, time.tm_mday, time.tm_mon + 1, time.tm_hour, time.tm_min, time.tm_sec, log);
		(void) FS_Write(string, strlen(string), attHandle);
	}

	if (sv_protect->integer & SVP_CONSOLE)
	{
		Com_Printf("%s", log);
	}
}

/**
 * @brief SV_InitAttackLog
 */
void SV_InitAttackLog(void)
{
	if (sv_protectLog->string[0] == '\0')
	{
		Com_Printf("Not logging server attacks to disk.\n");
	}
	else
	{
		// in sync so admins can check this at runtime
		FS_FOpenFileByMode(sv_protectLog->string, &attHandle, FS_APPEND_SYNC);

		if (attHandle <= 0)
		{
			Com_Printf("WARNING: Couldn't open server attack logfile %s\n", sv_protectLog->string);
		}
		else
		{
			Com_Printf("Logging server attacks to %s\n", sv_protectLog->string);
			SV_WriteAttackLog("-------------------------------------------------------------------------------\n");
			SV_WriteAttackLog("Start server attack log\n");
			SV_WriteAttackLog("-------------------------------------------------------------------------------\n");
		}
	}
}

/**
 * @brief SV_CloseAttackLog
 */
void SV_CloseAttackLog(void)
{
	if (attHandle > 0)
	{
		SV_WriteAttackLog("-------------------------------------------------------------------------------\n");
		SV_WriteAttackLog("End server attack log\n");
		SV_WriteAttackLog("-------------------------------------------------------------------------------\n");
		Com_Printf("Server attack log closed.\n");
	}

	FS_FCloseFile(attHandle);

	attHandle = 0;  // local handle
}

/*
===============
SV_Init

Only called at main exe startup, not for each game
===============
*/
void SV_Init (void)
{
	int a;
	int index;

	SV_AddOperatorCommands ();

	SV_PlayMap_Clear_Saved_Queue(0);

	SV_Scrim_Init( );

	// serverinfo vars
	Cvar_Get ("timelimit", "0", CVAR_SERVERINFO);
	Cvar_Get ("sv_keywords", "", CVAR_SERVERINFO);
	sv_mapname = Cvar_Get ("mapname", "nomap", CVAR_SERVERINFO | CVAR_ROM);
	sv_privateClients = Cvar_Get ("sv_privateClients", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get ("sv_hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE );
	sv_maxclients = Cvar_Get ("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH);

	sv_minRate = Cvar_Get ("sv_minRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_maxRate = Cvar_Get ("sv_maxRate", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_dlRate = Cvar_Get("sv_dlRate", "100", CVAR_ARCHIVE | CVAR_SERVERINFO);
	sv_minPing = Cvar_Get ("sv_minPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );
	sv_maxPing = Cvar_Get ("sv_maxPing", "0", CVAR_ARCHIVE | CVAR_SERVERINFO );

	// systeminfo
	Cvar_Get ("sv_cheats", "1", CVAR_SYSTEMINFO | CVAR_ROM );
	sv_serverid = Cvar_Get ("sv_serverid", "0", CVAR_SYSTEMINFO | CVAR_ROM );
	sv_pure = Cvar_Get ("sv_pure", "1", CVAR_SYSTEMINFO );
#ifdef USE_VOIP
	sv_voip = Cvar_Get("sv_voip", "1", CVAR_SYSTEMINFO | CVAR_LATCH);
	Cvar_CheckRange(sv_voip, 0, 1, qtrue);
#endif
	Cvar_Get ("sv_paks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_pakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_referencedPaks", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_referencedPakNames", "", CVAR_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_alternatePaks", "", CVAR_ALTERNATE_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_alternatePakNames", "", CVAR_ALTERNATE_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_referencedAlternatePaks", "", CVAR_ALTERNATE_SYSTEMINFO | CVAR_ROM );
	Cvar_Get ("sv_referencedAlternatePakNames", "", CVAR_ALTERNATE_SYSTEMINFO | CVAR_ROM );

	// server vars
	sv_rconPassword = Cvar_Get ("rconPassword", "", CVAR_TEMP );
	sv_privatePassword = Cvar_Get ("sv_privatePassword", "", CVAR_TEMP );
	sv_fps = Cvar_Get ("sv_fps", "40", CVAR_TEMP );
	sv_timeout = Cvar_Get ("sv_timeout", "200", CVAR_TEMP );
	sv_zombietime = Cvar_Get ("sv_zombietime", "2", CVAR_TEMP );

	sv_allowDownload = Cvar_Get ("sv_allowDownload", "0", CVAR_SERVERINFO);
	Cvar_Get ("sv_dlURL", "http://downloads.tremulous.net", CVAR_SERVERINFO | CVAR_ARCHIVE);

	sv_protect    = Cvar_Get("sv_protect", "7", CVAR_ARCHIVE);
	sv_protectLog = Cvar_Get("sv_protectLog", "sv_protect.log", CVAR_ARCHIVE);
	SV_InitAttackLog();

	for (a = 0; a < 3; ++a)
	{
		sv_masters[a][0] = Cvar_Get(va("sv_%smaster1", (a == 2 ? "alt2" : a == 1 ? "alt1" : "")), MASTER_SERVER_NAME, 0);
		for(index = 1; index < MAX_MASTER_SERVERS; index++)
			sv_masters[a][index] = Cvar_Get(va("sv_%smaster%d", (a == 2 ? "alt2" : a == 1 ? "alt1" : ""), index + 1), "", CVAR_ARCHIVE);
	}

	sv_reconnectlimit = Cvar_Get ("sv_reconnectlimit", "3", 0);
	sv_showloss = Cvar_Get ("sv_showloss", "0", 0);
	sv_padPackets = Cvar_Get ("sv_padPackets", "0", 0);
	sv_killserver = Cvar_Get ("sv_killserver", "0", 0);
	sv_mapChecksum = Cvar_Get ("sv_mapChecksum", "", CVAR_ROM);
	sv_lanForceRate = Cvar_Get ("sv_lanForceRate", "1", CVAR_ARCHIVE );
}


/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage( char *message ) {
	int			i, j;
	client_t	*cl;

	// send it twice, ignoring rate
	for ( j = 0 ; j < 2 ; j++ ) {
		for (i=0, cl = svs.clients ; i < sv_maxclients->integer ; i++, cl++) {
			if (cl->state >= CS_CONNECTED) {
				// don't send a disconnect to a local client
				if ( cl->netchan.remoteAddress.type != NA_LOOPBACK ) {
					SV_SendServerCommand( cl, "print \"%s\n\"\n", message );
					SV_SendServerCommand( cl, "disconnect \"%s\"", message );
				}
				// force a snapshot to be sent
				cl->lastSnapshotTime = 0;
				SV_SendClientSnapshot( cl );
			}
		}
	}
}


/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown( char *finalmsg ) {
	// close attack log
	SV_CloseAttackLog();

	if ( !com_sv_running || !com_sv_running->integer ) {
		return;
	}

	Com_Printf( "----- Server Shutdown (%s) -----\n", finalmsg );

	NET_LeaveMulticast6();

	if ( svs.clients && !com_errorEntered ) {
		SV_FinalMessage( finalmsg );
	}

	SV_RemoveOperatorCommands();
	SV_MasterShutdown();
	SV_ShutdownGameProgs();

	// free current level
	SV_ClearServer();

	// free server static data
	if(svs.clients)
	{
		int index;

		for(index = 0; index < sv_maxclients->integer; index++)
			SV_FreeClient(&svs.clients[index]);

		Z_Free(svs.clients);
	}
	Com_Memset( &svs, 0, sizeof( svs ) );

	Cvar_Set( "sv_running", "0" );
	Cvar_Set("ui_singlePlayerActive", "0");

	Com_Printf( "---------------------------\n" );

	// disconnect any local clients
	if( sv_killserver->integer != 2 )
		CL_Disconnect( qfalse );
}
