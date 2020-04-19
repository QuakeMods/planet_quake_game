#include "gsc_player.hpp"

#if COMPILE_PLAYER == 1

void gsc_player_velocity_set(scr_entref_t id)
{
	vec3_t velocity;

	if ( ! stackGetParams("v", &velocity))
	{
		stackError("gsc_player_velocity_set() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	gentity_t *entity = &g_entities[id];

	if (entity->client == NULL)
	{
		stackError("gsc_player_velocity_set() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	VectorCopy(velocity, entity->client->ps.velocity);
	stackPushBool(qtrue);
}

void gsc_player_velocity_add(scr_entref_t id)
{
	vec3_t velocity;

	if ( ! stackGetParams("v", &velocity))
	{
		stackError("gsc_player_velocity_add() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	gentity_t *entity = &g_entities[id];

	if (entity->client == NULL)
	{
		stackError("gsc_player_velocity_add() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	VectorAdd(entity->client->ps.velocity, velocity, entity->client->ps.velocity);
	stackPushBool(qtrue);
}

void gsc_player_velocity_get(scr_entref_t id)
{
	gentity_t *entity = &g_entities[id];

	if (entity->client == NULL)
	{
		stackError("gsc_player_velocity_get() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	stackPushVector(entity->client->ps.velocity);
}

void gsc_player_clientuserinfochanged(scr_entref_t id)
{
	ClientUserinfoChanged(id);
	stackPushBool(qtrue);
}

void gsc_player_get_userinfo(scr_entref_t id)
{
	char *key;

	if ( ! stackGetParams("s", &key))
	{
		stackError("gsc_player_get_userinfo() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_get_userinfo() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	char *val = Info_ValueForKey(client->userinfo, key);

	if (strlen(val))
		stackPushString(val);
	else
		stackPushUndefined();
}

void gsc_player_set_userinfo(scr_entref_t id)
{
	char *key, *value;

	if ( ! stackGetParams("ss", &key, &value))
	{
		stackError("gsc_player_set_userinfo() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_set_userinfo() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	Info_SetValueForKey(client->userinfo, key, value);
	stackPushBool(qtrue);
}

void gsc_player_button_ads(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_button_ads() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushBool(client->lastUsercmd.buttons & KEY_MASK_ADS_MODE ? qtrue : qfalse);
}

void gsc_player_button_left(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_button_left() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushBool(client->lastUsercmd.rightmove == KEY_MASK_MOVELEFT ? qtrue : qfalse);
}

void gsc_player_button_right(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_button_right() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushBool(client->lastUsercmd.rightmove == KEY_MASK_MOVERIGHT ? qtrue : qfalse);
}

void gsc_player_button_forward(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_button_forward() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushBool(client->lastUsercmd.forwardmove == KEY_MASK_FORWARD ? qtrue : qfalse);
}

void gsc_player_button_back(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_button_back() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushBool(client->lastUsercmd.forwardmove == KEY_MASK_BACK ? qtrue : qfalse);
}

void gsc_player_button_leanleft(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_button_leanleft() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushBool(client->lastUsercmd.buttons & KEY_MASK_LEANLEFT ? qtrue : qfalse);
}

void gsc_player_button_leanright(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_button_leanright() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushBool(client->lastUsercmd.buttons & KEY_MASK_LEANRIGHT ? qtrue : qfalse);
}

void gsc_player_button_reload(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_button_reload() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushBool(client->lastUsercmd.buttons & KEY_MASK_RELOAD ? qtrue : qfalse);
}

void gsc_player_button_jump(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_button_jump() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushBool(client->lastUsercmd.buttons & KEY_MASK_JUMP ? qtrue : qfalse);
}

void gsc_player_button_frag(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_button_frag() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushBool(client->lastUsercmd.buttons & KEY_MASK_FRAG ? qtrue : qfalse);
}

void gsc_player_button_smoke(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_button_smoke() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushBool(client->lastUsercmd.buttons & KEY_MASK_SMOKE ? qtrue : qfalse);
}

void gsc_player_stance_get(scr_entref_t id)
{
	gentity_t *entity = &g_entities[id];

	if (entity->client == NULL)
	{
		stackError("gsc_player_stance_get() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	if (entity->s.eFlags & EF_CROUCHING)
		stackPushString("duck");
	else if (entity->s.eFlags & EF_PRONE)
		stackPushString("lie");
	else
		stackPushString("stand");
}

void gsc_player_stance_set(scr_entref_t id)
{
	char *stance;

	if ( ! stackGetParams("s", &stance))
	{
		stackError("gsc_player_stance_set() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	gentity_t *entity = &g_entities[id];

	if (entity->client == NULL)
	{
		stackError("gsc_player_stance_set() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	int event;

	if (strcmp(stance, "stand") == 0)
		event = EV_STANCE_FORCE_STAND;
	else if (strcmp(stance, "crouch") == 0)
		event = EV_STANCE_FORCE_CROUCH;
	else if (strcmp(stance, "prone") == 0)
		event = EV_STANCE_FORCE_PRONE;
	else
	{
		stackError("gsc_player_stance_set() invalid argument '%s'. Valid arguments are: 'stand' 'crouch' 'prone'", stance);
		stackPushUndefined();
		return;
	}

	G_AddPredictableEvent(entity, event, 0);
	stackPushBool(qtrue);
}

void gsc_player_spectatorclient_get(scr_entref_t id)
{
	gentity_t *entity = &g_entities[id];

	if (entity->client == NULL)
	{
		stackError("gsc_player_spectatorclient_get() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	if (entity->client->spectatorClient == -1)
		stackPushUndefined();
	else
		stackPushEntity(&g_entities[entity->client->spectatorClient]);
}

void gsc_player_getip(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_getip() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	char tmp[16];
	snprintf(tmp, sizeof(tmp), "%d.%d.%d.%d", client->netchan.remoteAddress.ip[0], client->netchan.remoteAddress.ip[1], client->netchan.remoteAddress.ip[2], client->netchan.remoteAddress.ip[3]);

	stackPushString(tmp);
}

void gsc_player_getping(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_getping() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushInt(client->ping);
}

void gsc_player_clientcommand(scr_entref_t id)
{
	ClientCommand(id);
	stackPushBool(qtrue);
}

void gsc_player_getlastconnecttime(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_getlastconnecttime() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushInt(client->lastConnectTime);
}

void gsc_player_getlastmsg(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_getlastmsg() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushInt(svs.time - client->lastPacketTime);
}

void gsc_player_getclientstate(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_getclientstate() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushInt(client->state);
}

void gsc_player_addresstype(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_addresstype() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushInt(client->netchan.remoteAddress.type);
}

void gsc_player_renameclient(scr_entref_t id)
{
	char *name;

	if ( ! stackGetParams("s", &name))
	{
		stackError("gsc_player_renameclient() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (strlen(name) > 32)
	{
		stackError("gsc_player_renameclient() player name is longer than 32 characters");
		stackPushUndefined();
		return;
	}

	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_renameclient() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	Info_SetValueForKey(client->userinfo, "name", name);
	strcpy(client->name, name);

	stackPushBool(qtrue);
}

void gsc_player_outofbandprint(scr_entref_t id)
{
	char *cmd;

	if ( ! stackGetParams("s", &cmd))
	{
		stackError("gsc_player_outofbandprint() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_outofbandprint() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	NET_OutOfBandPrint(NS_SERVER, client->netchan.remoteAddress, cmd);
	stackPushBool(qtrue);
}

void gsc_player_connectionlesspacket(scr_entref_t id)
{
	char *cmd;

	if ( ! stackGetParams("s", &cmd))
	{
		stackError("gsc_player_connectionlesspacket() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_connectionlesspacket() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	byte bufData[131072];
	msg_t msg;

	MSG_Init(&msg, bufData, sizeof(bufData));

	MSG_WriteByte(&msg, svc_nop);
	MSG_WriteShort(&msg, 0);
	MSG_WriteLong(&msg, -1);
	MSG_WriteString(&msg, cmd);

	SV_ConnectionlessPacket(client->netchan.remoteAddress, &msg);
	stackPushBool(qtrue);
}

void gsc_player_resetnextreliabletime(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_resetnextreliabletime() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	client->floodprotect = 0;
	stackPushBool(qtrue);
}

void gsc_player_ismantling(scr_entref_t id)
{
	gentity_t *entity = &g_entities[id];

	if (entity->client == NULL)
	{
		stackError("gsc_player_ismantling() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	if (entity->client->ps.pm_flags & PMF_MANTLE)
		stackPushBool(qtrue);
	else
		stackPushBool(qfalse);
}

void gsc_player_isonladder(scr_entref_t id)
{
	gentity_t *entity = &g_entities[id];

	if (entity->client == NULL)
	{
		stackError("gsc_player_isonladder() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	if (entity->client->ps.pm_flags & PMF_LADDER)
		stackPushBool(qtrue);
	else
		stackPushBool(qfalse);
}

void gsc_player_getjumpslowdowntimer(scr_entref_t id)
{
	gentity_t *entity = &g_entities[id];

	if (entity->client == NULL)
	{
		stackError("gsc_player_getjumpslowdowntimer() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	stackPushInt(entity->client->ps.pm_time);
}

void gsc_player_setg_speed(scr_entref_t id)
{
	int speed;

	if ( ! stackGetParams("i", &speed))
	{
		stackError("gsc_player_setg_speed() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_setg_speed() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	extern int player_g_speed[MAX_CLIENTS];

	if (speed < 0)
	{
		stackError("gsc_player_setg_speed() param must be equal or above zero");
		stackPushUndefined();
		return;
	}

	player_g_speed[id] = speed;
	stackPushBool(qtrue);
}

void gsc_player_setg_gravity(scr_entref_t id)
{
	int gravity;

	if ( ! stackGetParams("i", &gravity))
	{
		stackError("gsc_player_setg_gravity() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_setg_gravity() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	extern int player_g_gravity[MAX_CLIENTS];

	if (gravity < 0)
	{
		stackError("gsc_player_setg_gravity() param must be equal or above zero");
		stackPushUndefined();
		return;
	}

	player_g_gravity[id] = gravity;
	stackPushBool(qtrue);
}

void gsc_player_setweaponfiremeleedelay(scr_entref_t id)
{
	int delay;

	if ( ! stackGetParams("i", &delay))
	{
		stackError("gsc_player_setweaponfiremeleedelay() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (delay < 0)
	{
		stackError("gsc_player_setweaponfiremeleedelay() param must be equal or above zero");
		stackPushUndefined();
		return;
	}

	gentity_t *entity = &g_entities[id];

	if (entity->client == NULL)
	{
		stackError("gsc_player_setweaponfiremeleedelay() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	entity->client->ps.weaponDelay = delay;
	stackPushBool(qtrue);
}

void gsc_player_set_anim(scr_entref_t id)
{
	char *animation;

	if ( ! stackGetParams("s", &animation))
	{
		stackError("gsc_player_set_anim() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	extern int custom_animation[MAX_CLIENTS];
	int animationIndex = 0;

	if (strcmp(animation, "none") != 0)
		animationIndex = BG_AnimationIndexForString(animation);

	custom_animation[id] = animationIndex;
	stackPushBool(qtrue);
}

void gsc_player_getcooktime(scr_entref_t id)
{
	gentity_t *entity = &g_entities[id];

	if (entity->client == NULL)
	{
		stackError("gsc_player_getcooktime() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	stackPushInt(entity->client->ps.grenadeTimeLeft);
}

void gsc_kick_slot()
{
	int id;
	char* msg;

	if ( ! stackGetParams("is", &id, &msg))
	{
		stackError("gsc_kick_slot() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_kick_slot() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	if (client == NULL)
	{
		stackPushUndefined();
		return;
	}

	if (client->netchan.remoteAddress.type == NA_LOOPBACK)
	{
		stackPushUndefined();
		return;
	}

	SV_DropClient(client, msg);
	stackPushBool(qtrue);
}

void gsc_player_setguid(scr_entref_t id)
{
	int guid;

	if ( ! stackGetParams("i", &guid))
	{
		stackError("gsc_player_setguid() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_setguid() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	client->guid = guid;
	stackPushBool(qtrue);
}

void gsc_player_clienthasclientmuted(scr_entref_t id)
{
	int id2;

	if ( ! stackGetParams("i", &id2))
	{
		stackError("gsc_player_clienthasclientmuted() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_clienthasclientmuted() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	stackPushInt(SV_ClientHasClientMuted(id, id2));
}

void gsc_player_getlastgamestatesize(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_getlastgamestatesize() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	extern int gamestate_size[MAX_CLIENTS];
	stackPushInt(gamestate_size[id]);
}

void gsc_player_getfps(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_getfps() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	extern int clientfps[MAX_CLIENTS];
	stackPushInt(clientfps[id]);
}

void gsc_player_isbot(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_isbot() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	client_t *client = &svs.clients[id];

	stackPushBool(client->bot);
}

void gsc_player_disableitempickup(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_disableitempickup() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	extern int player_disableitempickup[MAX_CLIENTS];

	player_disableitempickup[id] = 1;
	stackPushBool(qtrue);
}

void gsc_player_enableitempickup(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_enableitempickup() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	extern int player_disableitempickup[MAX_CLIENTS];

	player_disableitempickup[id] = 0;
	stackPushBool(qtrue);
}

void gsc_player_getcurrentoffhandslotammo(scr_entref_t id)
{
	if (id >= MAX_CLIENTS)
	{
		stackError("gsc_player_getcurrentoffhandslotammo() entity %i is not a player", id);
		stackPushUndefined();
		return;
	}

	playerState_t *ps = SV_GameClientNum(id);

	stackPushInt(ps->ammoclip[ps->offHandIndex - 1]);
}

#endif
