// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "bg_shared.h"

#define GRAPPLE_SPEED 900

pmove_t		*pm;
pml_t		pml;

// movement parameters
float	pm_stopspeed = 100.0f;
float	pm_duckScale = 0.25f;
float	pm_swimScale = 0.50f;
float	pm_wadeScale = 0.70f;

float	pm_accelerate = 10.0f;
float	pm_airaccelerate = 1.0f;
float	pm_wateraccelerate = 4.0f;
float	pm_flyaccelerate = 8.0f;
float	pm_slickaccelerate = 1.0f;
float   pm_ladderaccelerate = 3.0f;

float	pm_friction = 6.0f;
float	pm_waterfriction = 1.0f;
float	pm_flightfriction = 3.0f;
float	pm_spectatorfriction = 5.0f;

float	pm_airstopaccelerate = 1;
float	pm_strafeaccelerate = 1;
float	pm_wishspeed = 400;

int		pm_overbounces = 0;
int     pm_doublejump = 0;
int     pm_rampjump = 0;
int     pm_stairjump = 0;
int     pm_cpmstep = 0;
int		pm_stairfix = 0;
float   pm_aircontrol = 0;
int		pm_crouchslide = 0;
int     pm_walljump = 0;
int     pm_interference = 0;

int		weaponraisetime = 250.0f;
int		weapondroptime = 200.0f;

float	itemheight = 36.0f;

int     pm_rocketspeed = 900;
int     pm_grenadereload = 800;
float	pm_knockback_z = 24;

int     pm_jumppad = 2;
int     pm_noteles = 0;
int     pm_nodoors = 0;

int     pm_minrespawntime = 1700;

int		c_pmove = 0;
int     pm_respawntimer = 0;

int     pm_reversemap = 0;      // in 'defrag' if set to [1], swap start/stoptimer

#define WALLJUMP_VEL          380
#define WALLJUMP_PUSH         425
#define WALLJUMP_WAIT         400

/*
====================
BG_SharedCvarsUpdate
- int dfx_gametype
- int dfx_ruleset
- int dfx_mode
- int dfx_fastcapmode
- int dfx_obs
- int dfx_interference
- int dfx_reversemap
====================
*/
void BG_SharedCvarsUpdate ( int gt, int rs, int dfmode, int fcmode, int obs, int interference, int rev )
{
	// first clear the variables to default
	pm_accelerate = 10;
	pm_slickaccelerate = 1;
	pm_friction = 6;

	pm_airstopaccelerate = 1;
	pm_strafeaccelerate = 1;
	pm_wishspeed = 400;

	pm_knockback_z = 24;
	pm_rocketspeed = 900;
	pm_grenadereload = 800;

	weaponraisetime = 250;
	weapondroptime = 200;
	itemheight = 36;

	pm_doublejump = 0;
	pm_rampjump = 0;
	pm_cpmstep = 0;
	pm_stairjump = 0;
	pm_stairfix = 0;
	pm_aircontrol = 0;
	pm_crouchslide = 0;
	pm_walljump = 0;

	pm_interference = interference;
	pm_overbounces = obs;

	// fastcap vars
	pm_jumppad = 2; // jump pads are enabled
	pm_noteles = 0; // teleporters are enabled
	pm_nodoors = 0; // moving ents are enabled (doors...)

	// defrag vars
	pm_reversemap = 0;

	pm_minrespawntime = 200;


	// set certain vars depending on ruleset
	switch ( rs ) {
	case RS_VANILLA:

		break;

	case RS_CPM:
		pm_friction = 8;
		pm_accelerate = 15;
		pm_slickaccelerate = 15;

		pm_doublejump = 1;
		pm_rampjump = 1;
		pm_stairjump = 1;
		pm_cpmstep = 1;
		pm_aircontrol = 150;  // was 1
		pm_airstopaccelerate = 2.5;
		pm_strafeaccelerate = 70;
		pm_wishspeed = 30;

		pm_knockback_z = 40;
		pm_rocketspeed = 1000;

		weaponraisetime = 0;
		weapondroptime = 0;
		itemheight = 66;
		break;

	case RS_XVANILLA:
		pm_walljump = 1;
		pm_crouchslide = 1;
		break;

	case RS_XCPM:
		
		pm_friction = 8;
		pm_accelerate = 15;
		pm_slickaccelerate = 15;

		pm_doublejump = 1;
		pm_rampjump = 1;
		pm_stairjump = 1;
		pm_cpmstep = 1;
		pm_aircontrol = 150;  // was 1
		pm_airstopaccelerate = 2.5;
		pm_strafeaccelerate = 70;
		pm_wishspeed = 30;

		pm_knockback_z = 55;
		pm_rocketspeed = 1000;

		weaponraisetime = 0;
		weapondroptime = 0;
		itemheight = 66;
	
		pm_walljump = 1;
		pm_crouchslide = 1;
		break;
	}


	// when we are in fastcap, set things depending on fastcap_mode
	if ( gt == GT_FASTCAP ) {

		if ( fcmode > FC_DEFAULT ) {
			// doors will not spawn
			if ( fcmode == FC_NODOORS ) {
				pm_nodoors = 1;
			}

			// jump pads are killers
			if ( fcmode == FC_NOPADS ) {
				pm_jumppad = 1;
			}

			// teleporters won't work
			if ( fcmode == FC_NOTELES ) {
				pm_noteles = 1;
			}

			// jump pads are killers AND teleporters won't work
			if ( fcmode == FC_NOTELES_NOPADS ) {
				pm_jumppad = 1;
				pm_noteles = 1;
			}

			// disable all stuff
			if ( fcmode >= FC_DISABLEALL ) {
				pm_jumppad = 1;
				pm_noteles = 1;
				pm_nodoors = 1;
			}
		}

	}


	// when we are in defrag, set things depending on dfx_mode
	if ( gt == GT_DEFRAG ) {
		switch ( dfmode ){
		case DF_DEFAULT:
		case DF_STRAFE:
		case DF_ROCKET:
		case DF_PLASMA:
		case DF_COMBO:
			break;
		case DF_ICEMODE:
			pm_friction = 0;
			break;
		}

		if (rev)
		{
			pm_reversemap = 1;
			pm_nodoors = 1;
		}

	}

}

/*
===============
PM_AddEvent
===============
*/
void PM_AddEvent( int newEvent )
{
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps, -1 );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum )
{
	int	i;

	if ((entityNum == ENTITYNUM_WORLD) || (pm->numtouch >= MAXTOUCH))
    {
        return;
    }

	// see if it is already added
	for ( i = 0 ; i < pm->numtouch ; i++ )
    {
		if (pm->touchents[ i ] == entityNum)
        {
            return;
        }
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
===================
PM_StartTorsoAnim
===================
*/
static void PM_StartTorsoAnim( int anim )
{
	if ( pm->ps->pm_type >= PM_DEAD )
    {
        return;
    }
	pm->ps->torsoAnim = ( ( pm->ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )	| anim;
}


static void PM_StartLegsAnim( int anim )
{
	if ( (pm->ps->pm_type >= PM_DEAD) || (pm->ps->legsTimer > 0) )
    {
        return;
    }
	pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
}


static void PM_ContinueLegsAnim( int anim )
{
	if ( (( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == anim) || (pm->ps->legsTimer > 0) )
    {
        return;
    }
	PM_StartLegsAnim( anim );
}


static void PM_ContinueTorsoAnim( int anim )
{
	if ( (( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) == anim) || (pm->ps->torsoTimer > 0) )
    {
        return;
    }

	PM_StartTorsoAnim( anim );
}

static void PM_ForceLegsAnim( int anim )
{
	pm->ps->legsTimer = 0;
	PM_StartLegsAnim( anim );
}


/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce )
{
	float	backoff;
	float	change;
	int		i;

	backoff = DotProduct (in, normal);

	if ( backoff < 0 )
    {
        backoff *= overbounce;
    }
	else
    {
        backoff /= overbounce;
    }

	for ( i=0 ; i<3 ; i++ )
    {
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}

void PM_ClipVelocity2( vec3_t in, vec3_t normal, vec3_t out, float overbounce )
{
	float	backoff;
	float	change;
	int		i;

	backoff = DotProduct (in, normal);

	if ( backoff < 0 )
    {
        backoff *= overbounce;
    }
	else
    {
        backoff /= overbounce;
    }

	for ( i=0 ; i<2 ; i++ )
    {
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}

/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void )
{
	vec3_t	vec;
	float	*vel;
	float	speed, newspeed, control;
	float	drop;

	vel = pm->ps->velocity;

	VectorCopy(vel, vec);
	if (pml.walking)
    {
        vec[2] = 0;	// ignore slope movement
    }

	speed = VectorLength(vec);
	if (speed < 1)
    {
		vel[0] = 0;
		vel[1] = 0;	// allow sinking underwater

		// FIXME: still have z friction underwater?
		if (pm->ps->pm_type == PM_SPECTATOR || pm->ps->powerups[ PW_FLIGHT ])
        {
            vel[2] = 0.0f; // no slow-sinking/raising movements
        }
		return;
	}

	drop = 0;

	// apply ground friction
	if ( pm->waterlevel <= 1 )
    {
		if ( (pml.walking || pml.ladder) && !(pml.groundTrace.surfaceFlags & SURF_SLICK))
		{
			// if getting knocked back, no friction
			if ( ! (pm->ps->pm_flags & PMF_TIME_KNOCKBACK) )
			{
				control = speed < 100 ? 100 : speed;

				// check if crouch sliding is enabled
				if ( pm_crouchslide && (pm->ps->pm_flags & PMF_DUCKED) )
                {
					drop += control*1.2*pml.frametime;
				}
				else
                {
					drop += control*pm_friction*pml.frametime;
				}
			}
		}
	}

    // apply water friction even if just wading
	if ( pm->waterlevel ) {
		drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;
	}

	// apply flying friction
	if ( pm->ps->powerups[PW_FLIGHT])
        drop += speed*3*pml.frametime;

	if ( pm->ps->pm_type == PM_SPECTATOR)
        drop += speed*5*pml.frametime;

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
        newspeed = 0;

	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel )
{
    int		i;
    float	addspeed, accelspeed, currentspeed;
    vec3_t	vel;

    VectorCopy( pm->ps->velocity, vel );
    vel[2] = 0;

    currentspeed = DotProduct (pm->ps->velocity, wishdir);
    addspeed = wishspeed - currentspeed;

    if (addspeed <= 0)
        return;

    accelspeed = accel*pml.frametime*wishspeed;
    if (accelspeed > addspeed)
        accelspeed = addspeed;

    for (i=0 ; i<3 ; i++) {
        pm->ps->velocity[i] += accelspeed*wishdir[i];
    }

}


/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd )
{
	int		max;
	float	total;
	float	scale;

	max = abs(cmd->forwardmove);
	if (abs(cmd->rightmove) > max)
        max = abs(cmd->rightmove);

	if (abs(cmd->upmove) > max)
        max = abs(cmd->upmove);

	if ( !max )
        return 0;

	total = sqrt( cmd->forwardmove * cmd->forwardmove + cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove );
	scale = (float)pm->ps->speed * max / ( 127.0 * total );

	return scale;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs relative
to the facing dir
================
*/
static void PM_SetMovementDir( void )
{
	if ( pm->cmd.forwardmove || pm->cmd.rightmove )
    {

		if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 )
        {
			pm->ps->movementDir = 0;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 )
		{
			pm->ps->movementDir = 1;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 )
		{
			pm->ps->movementDir = 2;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 )
		{
			pm->ps->movementDir = 3;
		} else if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 )
		{
			pm->ps->movementDir = 4;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 )
		{
			pm->ps->movementDir = 5;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 )
		{
			pm->ps->movementDir = 6;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 )
		{
			pm->ps->movementDir = 7;
		}
	}

	else

    {
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if ( pm->ps->movementDir == 2 )
		{
			pm->ps->movementDir = 1;
		}
		else if ( pm->ps->movementDir == 6 )
		{
			pm->ps->movementDir = 7;
		}
	}
}


/*
=============
PM_CheckWallJump
=============
*/
static qbool PM_CheckWallJump( void )
{
	trace_t trace;
	vec3_t normal = {0, 0, 0};
	vec3_t down = {0, 0, -1};
	float closest = 1;
	int i;

	if ( !pm_walljump || pm->ps->stats[STAT_WJTIME] > 0)
		return qfalse;

	if ( pm->ps->stats[STAT_WJCOUNT] >= 3) // allow 3 walljumps
		return qfalse;

	if ( pm->ps->pm_flags & PMF_RESPAWNED )
		return qfalse;		// don't allow jump until all buttons are up

	if ( pm->cmd.upmove < 10 )
		return qfalse;      // not holding jump

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD )
		return qfalse;

	// don't walljump on steps
	VectorMA( pm->ps->origin, STEPSIZE, down, down);
	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask );
	if ( trace.fraction < 1.0 )
		return qfalse;

	for ( i = 0; i < 8; i++ ) {
		float dist;
		vec3_t end;
		end[0] = cos( 2 * M_PI * i / 8 );
		end[1] = sin( 2 * M_PI * i / 8 );
		end[2] = 0;
		VectorAdd( pm->ps->origin, end, end );

		pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask );

		if ( trace.fraction < closest ) {
				closest = trace.fraction;
				VectorCopy( trace.plane.normal, normal );
		}
	}

	if ( closest < 1 ) {
		pml.groundPlane = qfalse;		// jumping away
		pml.walking = qfalse;
		pm->ps->pm_flags |= PMF_JUMP_HELD;

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pm->ps->stats[STAT_WJTIME] = WALLJUMP_WAIT;
		pm->ps->stats[STAT_WJCOUNT]++;

		// clip against the normal so that adding the bounce speed pushes you
		// away (otherwise it might just slow you down if you jump right before
		// hitting the wall)
		PM_ClipVelocity( pm->ps->velocity, normal, pm->ps->velocity, OVERCLIP );

		// push off the wall
		VectorScale( normal, WALLJUMP_PUSH, normal );
		VectorAdd( pm->ps->velocity, normal, pm->ps->velocity );

		if ( pm->ps->velocity[2] < WALLJUMP_VEL )
			pm->ps->velocity[2] = WALLJUMP_VEL;

		PM_AddEvent( EV_JUMP );

		if ( pm->cmd.forwardmove >= 0 ) {
			PM_ForceLegsAnim( LEGS_JUMP );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}
        else
        {
			PM_ForceLegsAnim( LEGS_JUMPB );
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		return qtrue;
	}

	return qfalse;
}


/*
=============
PM_CheckLadder
=============
*/
static void PM_CheckLadder( void )
{
	trace_t trace;
	vec3_t flatforward;

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize( flatforward );

	VectorAdd( flatforward, pm->ps->origin, flatforward );

	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, flatforward, pm->ps->clientNum, pm->tracemask );

    if ( trace.fraction < 1.0 && (trace.surfaceFlags & SURF_LADDER) )
		pml.ladder = qtrue;
	else
		pml.ladder = qfalse;
}

/*
=============
PM_CheckJump
=============
*/
qbool PM_CheckJump( void )
{
    // don't allow jump until all buttons are up || not holding jump
	if ( ( pm->ps->pm_flags & PMF_RESPAWNED ) || ( pm->cmd.upmove < 10 ) )
        return qfalse;

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD ) {
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pml.groundPlane = qfalse;		// jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	// SLK: Rampjump
	if (pm_rampjump && pm->ps->velocity[2] > 0)
        pm->ps->velocity[2] += JUMP_VELOCITY;
    else
        pm->ps->velocity[2]  = JUMP_VELOCITY;

    // SLK: DoubleJump
    if(pm_doublejump) {
        if (pm->ps->stats[STAT_JUMPTIME] > 0) {
            pm->ps->velocity[2] += 100;
            pm->ps->stats[STAT_DFX_FLAG] |= DFXF_STAIRJUMP;
        }
        pm->ps->stats[STAT_JUMPTIME] = 400;
    }

	PM_AddEvent( EV_JUMP );

	if ( pm->cmd.forwardmove >= 0 )
    {
		PM_ForceLegsAnim( LEGS_JUMP );
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else
    {
		PM_ForceLegsAnim( LEGS_JUMPB );
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}

/*
=============
PM_CheckWaterJump
=============
*/
static qbool PM_CheckWaterJump(void)
{
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;
                          // check for water jump
	if( (pm->ps->pm_time) || (pm->waterlevel != 2) )
        return qfalse;

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	VectorMA (pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if (!(cont & CONTENTS_SOLID))
        return qfalse;

	spot[2] += 16;
	cont = pm->pointcontents (spot, pm->ps->clientNum );
	if (cont & (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY))
        return qfalse;

	// jump out of water
	VectorScale (pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================


/*
===================
PM_WaterJumpMove
Flying out of the water
===================
*/
static void PM_WaterJumpMove(void)
{
	// waterjump has no control, but falls
	PM_StepSlideMove( qtrue );

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if (pm->ps->velocity[2] < 0) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove( void )
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;
	float	vel;

	if (PM_CheckWaterJump() ) {
		PM_WaterJumpMove();
		return;
	}


	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );

	// user intentions
	//
	if ( !scale )
    {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60;		// sink towards bottom
	}
	else
    {
		for (i=0 ; i<3 ; i++)
		{
		    wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if ( wishspeed > pm->ps->speed * 0.5 )
		wishspeed = pm->ps->speed * 0.5;

	PM_Accelerate ( wishdir, wishspeed, pm_wateraccelerate );

	// make sure we can go up slopes easily under water
	if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 ) {
		vel = VectorLength(pm->ps->velocity);
		// slide along the ground plane
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP );

		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	PM_SlideMove( qfalse );
}


/*
===================
PM_LadderMove
===================
*/
static void PM_LadderMove( void )
{
	int			i;
	float		scale, wishspeed;
	vec3_t		wishdir;
	usercmd_t	cmd;

	PM_Friction();

	memcpy( &cmd, &pm->cmd, sizeof(usercmd_t) );
	scale = PM_CmdScale( &cmd );

	for ( i = 0; i < 3; i++ ) {
		wishdir[i] = scale * (pml.forward[i]*pm->cmd.forwardmove + pml.right[i]*pm->cmd.rightmove);
	}

	// moveup/movedown overrides looking up/down and walking
	if ( pm->cmd.upmove )
    {
		cmd.forwardmove = cmd.rightmove = 0;
		wishdir[2] = pm->cmd.upmove * PM_CmdScale( &cmd );
    }

    wishspeed = VectorNormalize( wishdir );// * PM_CmdScale( &cmd );

    if ( wishspeed > pm->ps->speed )
        wishspeed = pm->ps->speed;

    PM_Accelerate( wishdir, wishspeed, pm_ladderaccelerate );
    PM_SlideMove( qfalse );

}



/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove(void)
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;

	// normal slowdown
	PM_Friction ();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale )
    {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	}
	else
    {
		for (i=0 ; i<3 ; i++) {
			wishvel[i] = scale * pml.forward[i]*pm->cmd.forwardmove + scale * pml.right[i]*pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	PM_Accelerate (wishdir, wishspeed, 8);
	PM_StepSlideMove(qfalse);
}

/*
===================
PM_AirControl
===================
*/
static void PM_AirControl( pmove_t *pm, vec3_t wishdir, float wishspeed )
{
    float	zspeed, speed, dot, k;
    int		i;

    if ((pm->ps->movementDir && pm->ps->movementDir !=4) || wishspeed == 0.0)
        return; // can't control movement if not moving forward or backward

    zspeed = pm->ps->velocity[2];
    pm->ps->velocity[2] = 0;
    speed = VectorNormalize(pm->ps->velocity);

    dot = DotProduct(pm->ps->velocity,wishdir);
    k = 32;
    k *= pm_aircontrol*dot*dot*pml.frametime;

    if (dot > 0) {	// we can't change direction while slowing down
        for (i=0; i < 2; i++) {
            pm->ps->velocity[i] = pm->ps->velocity[i]*speed + wishdir[i]*k;
        }

        VectorNormalize(pm->ps->velocity);
    }

    for (i=0; i < 2; i++) {
        pm->ps->velocity[i] *=speed;
    }

    pm->ps->velocity[2] = zspeed;
}

/*
===================
PM_AirMove
===================
*/
static void PM_AirMove(void)
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float       accel, wishspeed2;
	float		scale;
	usercmd_t	cmd;

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 2 ; i++ ) {
        wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
    }

	wishvel[2] = 0;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// CPM Air Control
    if (pm_aircontrol > 0)
    {
        wishspeed2 = wishspeed;
        if ( DotProduct (pm->ps->velocity, wishdir ) < 0 )
            accel = pm_airstopaccelerate;
        else
            accel = pm_airaccelerate;

        if ( pm->ps->movementDir == 2 || pm->ps->movementDir == 6 )
        {
            if (wishspeed > pm_wishspeed)
                wishspeed = pm_wishspeed;

            accel = pm_strafeaccelerate;
        }
    }
    else
    {
        accel = 1.0;
    }

	// not on ground, so little effect on velocity
	PM_Accelerate ( wishdir, wishspeed, accel );
    if (pm_aircontrol > 0)
        PM_AirControl ( pm, wishdir, wishspeed2 );

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if (pml.groundPlane)
        PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP );

	PM_StepSlideMove (qtrue);
    PM_CheckWallJump ();
}

/*
===================
PM_GrappleMove
===================
*/
static void PM_GrappleMove( void )
{
	vec3_t vel, v;
	float vlen;

	VectorScale(pml.forward, -16, v);
	VectorAdd(pm->ps->grapplePoint, v, v);
	VectorSubtract(v, pm->ps->origin, vel);
	vlen = VectorLength(vel);
	VectorNormalize( vel );

	if (vlen <= 100)
        VectorScale(vel, 10 * vlen, vel);
	else
        VectorScale(vel, GRAPPLE_SPEED, vel);

	VectorCopy(vel, pm->ps->velocity);

	pml.groundPlane = qfalse;
}

/*
===================
PM_WalkMove
===================
*/
static void PM_WalkMove(void)
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;
	float		accelerate;
	float		vel;

	if ( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 )
    {
		// begin swimming
		PM_WaterMove();
		return;
	}

	if ( PM_CheckJump () )
    {
		// jumped away
		if ( pm->waterlevel > 1 )
            PM_WaterMove();
		else
            PM_AirMove();

		return;
	}

	PM_Friction ();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity (pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity (pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	//
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 3 ; i++ ) {
        wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
    }

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// clamp the speed lower if ducking
	if ( (pm->ps->pm_flags & PMF_DUCKED) && (wishspeed > pm->ps->speed * pm_duckScale) )
        wishspeed = pm->ps->speed * pm_duckScale;

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel )
    {
		float waterScale;

		waterScale = pm->waterlevel / 3.0;
		waterScale = 1.0 - 0.5 * waterScale;
		if ( wishspeed > pm->ps->speed * waterScale )
			wishspeed = pm->ps->speed * waterScale;
	}

	// when a player gets hit, he temporarily loses
	// full control, which allows him to be moved a bit
	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
    {
		accelerate = pm_slickaccelerate;
    } else if ( pm_crouchslide && (pm->ps->pm_flags & PMF_DUCKED)&& pm->ps->stats[STAT_CROUCHTIME] > 0 )
    {
		accelerate = 10.0; // pm_crouchaccelerate
	} else
	{
		accelerate = pm_accelerate;
	}

	PM_Accelerate (wishdir, wishspeed, accelerate);

	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || (pm->ps->pm_flags & PMF_TIME_KNOCKBACK) || pm_friction == 0.0f )
    {
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
    }

	if ( pm_respawntimer )
    {
        // no more overbounce at respawn
		// slide along the ground plane
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP );
	} else {
		vel = VectorLength(pm->ps->velocity);

		// slide along the ground plane
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP );

		// don't decrease velocity when going up or down a slope
		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	// don't do anything if standing still
	if (!pm->ps->velocity[0] && !pm->ps->velocity[1])
    {
        return;
    }

	PM_StepSlideMove( qfalse );
}

/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void )
{
	float forward;

	if ( !pml.walking )
        return;

	// extra friction
	forward = VectorLength (pm->ps->velocity);
	forward -= 10;
	if ( forward <= 0 ) {
		VectorClear (pm->ps->velocity);
	} else {
		VectorNormalize (pm->ps->velocity);
		VectorScale (pm->ps->velocity, forward, pm->ps->velocity);
	}
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove(void)
{
	float	    speed, drop, friction, control, newspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction
	speed = VectorLength (pm->ps->velocity);
	if (speed < 1)
    {
		VectorCopy (vec3_origin, pm->ps->velocity);
	} else
	{
		drop = 0;

		friction = pm_friction*1.5;	// extra friction
		control = speed < 100 ? 100 : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0) newspeed = 0;
		newspeed /= speed;
		VectorScale (pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for (i=0 ; i<3 ; i++) {
        wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
    }

	wishvel[2] += pm->cmd.upmove;
	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA (pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void )
{
	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
        return 0;
	} else if ( pml.groundTrace.surfaceFlags & SURF_METALSTEPS ) {
	   return EV_FOOTSTEP_METAL;
	} else if ( pml.groundTrace.surfaceFlags & SURF_FLESH ) {
        return EV_FOOTSTEP_FLESH;
	} else if ( pml.groundTrace.surfaceFlags & SURF_STONE ) {
        return EV_FOOTSTEP_STONE;
	} else if ( pml.groundTrace.surfaceFlags & SURF_GRASS ) {
        return EV_FOOTSTEP_GRASS;
	} else if ( pml.groundTrace.surfaceFlags & SURF_TALLGRASS ) {
        return EV_FOOTSTEP_TALLGRASS;
	} else if ( pml.groundTrace.surfaceFlags & SURF_WOOD ) {
        return EV_FOOTSTEP_WOOD;
	} else if ( pml.groundTrace.surfaceFlags & SURF_SAND ) {
        return EV_FOOTSTEP_SAND;
	} else if ( pml.groundTrace.surfaceFlags & SURF_HOT ) {
        return EV_FOOTSTEP_LAVA;
	} else if ( (pml.groundTrace.surfaceFlags & SURF_SLICK) || (pml.groundTrace.surfaceFlags & SURF_ICE) ) {
        return EV_FOOTSTEP_ICE;
	}

	return EV_FOOTSTEP;
}

/*
================
PM_LandsoundForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_LandsoundForSurface( void )
{
	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
        return 0;
	} else if ( pml.groundTrace.surfaceFlags & SURF_METALSTEPS ) {
	   return EV_LANDSOUND_METAL;
	} else if ( pml.groundTrace.surfaceFlags & SURF_FLESH ) {
        return EV_LANDSOUND_FLESH;
	} else if ( pml.groundTrace.surfaceFlags & SURF_STONE ) {
        return EV_LANDSOUND_STONE;
	} else if ( pml.groundTrace.surfaceFlags & SURF_GRASS ) {
        return EV_LANDSOUND_GRASS;
	} else if ( pml.groundTrace.surfaceFlags & SURF_TALLGRASS ) {
        return EV_LANDSOUND_TALLGRASS;
	} else if ( pml.groundTrace.surfaceFlags & SURF_WOOD ) {
        return EV_LANDSOUND_WOOD;
	} else if ( pml.groundTrace.surfaceFlags & SURF_SAND ) {
        return EV_LANDSOUND_SAND;
	} else if ( pml.groundTrace.surfaceFlags & SURF_HOT ) {
        return EV_LANDSOUND_LAVA;
	} else if ( (pml.groundTrace.surfaceFlags & SURF_SLICK) || (pml.groundTrace.surfaceFlags & SURF_ICE) ) {
        return EV_LANDSOUND_ICE;
	}

	return EV_LANDSOUND;
}

/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand(void)
{
	float		delta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;

	// decide which landing animation to use
	if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP )
		PM_ForceLegsAnim( LEGS_LANDB );
	else
		PM_ForceLegsAnim( LEGS_LAND );

	pm->ps->legsTimer = TIMER_LAND;

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den =  b * b - 4 * a * c;
	if ( den < 0 )
        return;

	t = (-b - sqrt( den ) ) / ( 2 * a );

	delta = vel + t * acc;
	delta = delta*delta * 0.0001;

	// ducking while falling doubles damage
	if ( pm->ps->pm_flags & PMF_DUCKED )
        delta *= 2;

	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 )
        return;

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 )
        delta *= 0.25;

	if ( pm->waterlevel == 1 )
        delta *= 0.5;

	if ( delta < 1 )
        return;

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if ( !(pml.groundTrace.surfaceFlags & SURF_NODAMAGE) ) {
		if ( delta > 60 )
		{
			PM_AddEvent( EV_FALL_FAR );
		} else if ( delta > 40 )
		{
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[STAT_HEALTH] > 0 )   PM_AddEvent( EV_FALL_MEDIUM );

		} else if ( delta > 7 )
		{
			PM_AddEvent( EV_FALL_SHORT );
			PM_AddEvent( PM_LandsoundForSurface() );
		} else
		{
			PM_AddEvent( PM_FootstepForSurface() );
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}

/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid( trace_t *trace )
{
	int			i, j, k;
	vec3_t		point;

	if ( pm->debugLevel )
        Com_Printf("%i:allsolid\n", c_pmove);

	// jitter around
	for (i = -1; i <= 1; i++)
    {
		for (j = -1; j <= 1; j++)
		{
			for (k = -1; k <= 1; k++)
			{
				VectorCopy(pm->ps->origin, point);
				point[0] += (float) i;
				point[1] += (float) j;
				point[2] += (float) k;
				pm->trace (trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				if ( !trace->allsolid )
                {
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace (trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}


/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void )
{
	trace_t		trace;
	vec3_t		point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
    {
		// we just transitioned into freefall
		if ( pm->debugLevel )
            Com_Printf("%i:lift\n", c_pmove);

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[2] -= 64;

		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if ( trace.fraction == 1.0 )
        {
			if ( pm->cmd.forwardmove >= 0 )
			{
				PM_ForceLegsAnim( LEGS_JUMP );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else
			{
				PM_ForceLegsAnim( LEGS_JUMPB );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void )
 {
	vec3_t		point;
	trace_t		trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( (trace.allsolid) && (!PM_CorrectAllSolid(&trace)) )
        return;

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 )
    {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[2] > 0 && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10 )
    {
		if ( pm->debugLevel )
            Com_Printf("%i:kickoff\n", c_pmove);

		// go into jump animation
		if ( pm->cmd.forwardmove >= 0 )
        {
			PM_ForceLegsAnim( LEGS_JUMP );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		} else
		{
			PM_ForceLegsAnim( LEGS_JUMPB );
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

    // slopes that are too steep will not be considered onground
    if ( trace.plane.normal[2] < 0.7f ) {
        if ( pm->debugLevel )
            Com_Printf("%i:steep\n", c_pmove);

        // FIXME: if they can't slide down the slope, let them
        // walk (sharp crevices)
        pm->ps->groundEntityNum = ENTITYNUM_NONE;
        pml.groundPlane = qtrue;
        pml.walking = qfalse;
        return;
    }

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
    {
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

    if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
    {
        // just hit the ground
        if ( pm->debugLevel )
            Com_Printf("%i:Land\n", c_pmove);

        PM_CrashLand();

        // kill obs
        if ( (!pm_overbounces) || pml.groundTrace.surfaceFlags & SURF_NOOB )
            PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP );


        // don't do landing time if we were just going down a slope
        if ( pml.previous_velocity[2] < -200 )
        {
            // don't allow another jump for a little while
            pm->ps->pm_flags |= PMF_TIME_LAND;
            pm->ps->pm_time = 250;
        }
    }

	pm->ps->groundEntityNum = trace.entityNum;

	// kill obs
	if ( ((!pm_overbounces) || pml.groundTrace.surfaceFlags & SURF_NOOB) && ( trace.plane.normal[2] == 1 ) )
        pm->ps->velocity[2] = 0;

	PM_AddTouchEnt( trace.entityNum );
}


/*
=================
PM_SetWaterLevel
=================
*/
static void PM_SetWaterLevel(void)
{
	vec3_t		point;
	int			cont;
	int			sample1;
	int			sample2;

	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;
	cont = pm->pointcontents( point, pm->ps->clientNum );

	if ( cont & MASK_WATER )
    {
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents (point, pm->ps->clientNum );
		if (cont & MASK_WATER)
        {
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents (point, pm->ps->clientNum );
			if (cont & MASK_WATER)  pm->waterlevel = 3;

		}
	}
}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck (void)
{
	trace_t	trace;

	pm->mins[0] = -15;
	pm->mins[1] = -15;
	pm->maxs[0] = 15;
	pm->maxs[1] = 15;
	pm->mins[2] = MINS_Z;

	if (pm->ps->pm_type == PM_DEAD)
    {
		pm->maxs[2] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	if (pm->cmd.upmove < 0)
    {	// duck
		pm->ps->pm_flags |= PMF_DUCKED;
	} else
	{	// stand up if possible
		if (pm->ps->pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pm->maxs[2] = 32;
			pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
			if (!trace.allsolid) pm->ps->pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
    {
		pm->maxs[2] = 16;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;

		if ( !(pm->ps->pm_flags & PMF_DUCK_HELD) )
		{
			pm->ps->pm_flags |= PMF_DUCK_HELD;
			pm->ps->stats[STAT_CROUCHTIME] = 750;
		}

	} else
	{
		pm->maxs[2] = 32;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}
}



//===================================================================


/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps(void)
{
	float		bobmove;
	float		xyspeedQ;
	int			old;
	qbool	    footstep;


	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	// xyspeedQ = pm->ps->velocity[0] * pm->ps->velocity[0]	+ pm->ps->velocity[1] * pm->ps->velocity[1];

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE && (!pml.ladder) )
    {
		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 )
            PM_ContinueLegsAnim( LEGS_SWIM );

		return;
	} else if ( pml.ladder ) {
        PM_ContinueLegsAnim( LEGS_WALK );

	}

	// if not trying to move
	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove )
    {
		xyspeedQ = pm->ps->velocity[0] * pm->ps->velocity[0] + pm->ps->velocity[1] * pm->ps->velocity[1];
		if ( xyspeedQ < 5.0*5.0 ) { // not using sqrt() there
			pm->ps->bobCycle = 0;	// start at beginning of cycle again
			if ( pm->ps->pm_flags & PMF_DUCKED )
				PM_ContinueLegsAnim( LEGS_IDLECR );
            else
				PM_ContinueLegsAnim( LEGS_IDLE );

		}
		return;
	}


	footstep = qfalse;

	if ( pm->ps->pm_flags & PMF_DUCKED )
    {
		bobmove = 0.5;	// ducked characters bob much faster

		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN )
        {
			PM_ContinueLegsAnim( LEGS_BACKCR );
		} else
		{
			PM_ContinueLegsAnim( LEGS_WALKCR );
		}

	}
	else
    {
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) )
		{

			bobmove = 0.4f;	// faster speeds bob faster
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				PM_ContinueLegsAnim( LEGS_BACK );
			} else {
				PM_ContinueLegsAnim( LEGS_RUN );
			}
			footstep = qtrue;

		}
		else
        {
			bobmove = 0.3f;	// walking bobs slow
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN )
			{
				PM_ContinueLegsAnim( LEGS_BACKWALK );
			}
            else
            {
				PM_ContinueLegsAnim( LEGS_WALK );
			}
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)( old + bobmove * pml.msec ) & 255;

	// if we just crossed a cycle boundary, play an apropriate footstep event
	if ( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 )
    {
        if ( pml.ladder )
                PM_AddEvent ( EV_FOOTSTEP_METAL ); // SLK could check for surfaceparm too here

		if ( pm->waterlevel == 0 )
		{
			// on ground will only play sounds if running
			if ( footstep )
                PM_AddEvent( PM_FootstepForSurface() );

		} else if ( pm->waterlevel == 1 )
		{
			// splashing
			PM_AddEvent( EV_FOOTSPLASH );
		} else if ( pm->waterlevel == 2 )
		{
			// wading / swimming at surface
			PM_AddEvent( EV_SWIM );
		} else if ( pm->waterlevel == 3 )
		{
			// no sound when completely underwater
		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents(void)
{	// FIXME?

	// if just entered a water volume, play a sound
	if (!pml.previous_waterlevel && pm->waterlevel)
        PM_AddEvent( EV_WATER_TOUCH );

	// if just completely exited a water volume, play a sound
	if (pml.previous_waterlevel && !pm->waterlevel)
        PM_AddEvent( EV_WATER_LEAVE );

	// check for head just going under water
    if (pml.previous_waterlevel != 3 && pm->waterlevel == 3)
        PM_AddEvent( EV_WATER_UNDER );

	// check for head just coming out of water
	if (pml.previous_waterlevel == 3 && pm->waterlevel != 3)
        PM_AddEvent( EV_WATER_CLEAR );
}


/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange(int weapon)
{
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS )
        return;

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) )
        return;

	if ( pm->ps->weaponstate == WEAPON_DROPPING )
    {
		pm->ps->eFlags &= ~EF_FIRING;
		return;
	}

	PM_AddEvent( EV_CHANGE_WEAPON );
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += weapondroptime;
	PM_StartTorsoAnim( TORSO_DROP );
}


/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange(void)
{
	int	weapon;

	weapon = pm->cmd.weapon;
	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS )
        weapon = WP_NONE;

	if ( !( pm->ps->stats[STAT_WEAPONS] & ( 1 << weapon ) ) )
        weapon = WP_NONE;

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->eFlags &= ~EF_FIRING;
	pm->ps->weaponTime += weaponraisetime;
	PM_StartTorsoAnim( TORSO_RAISE );
}


/*
==============
PM_TorsoAnimation

==============
*/
static void PM_TorsoAnimation( void )
{
	if ( pm->ps->weaponstate == WEAPON_READY )
    {
		if ( pm->ps->weapon == WP_GAUNTLET )
			PM_ContinueTorsoAnim( TORSO_STAND2 );
		else
			PM_ContinueTorsoAnim( TORSO_STAND );

		return;
	}
}


/*
==============
PM_Weapon

Generates weapon events and modifies the weapon counter
==============
*/
static void PM_Weapon(void)
{
	int	addTime;

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED )
        return;

	// ignore if spectator
	if ( pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR )
        return;

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 )
    {
		pm->ps->weapon = WP_NONE;
		return;
	}

	// check for item using
	if ( pm->cmd.buttons & BUTTON_USE_HOLDABLE )
    {
		if ( ! ( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) )
		{
			if ( bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag == HI_MEDKIT && !(pm->ps->stats[STAT_HEALTH] >= (pm->ps->stats[STAT_MAX_HEALTH] + 25)) ){
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				PM_AddEvent( EV_USE_ITEM0 + bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag );
				pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
			}

			return;
		}
	}
	else
    {
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}


	// make weapon function
	if ( pm->ps->weaponTime > 0 )	pm->ps->weaponTime -= pml.msec;

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if ( pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING )
    {
		if ( pm->ps->weapon != pm->cmd.weapon )
            PM_BeginWeaponChange( pm->cmd.weapon );
	}

	if ( pm->ps->weaponTime > 0 )
        return;

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING )
    {
		PM_FinishWeaponChange();
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_RAISING )
    {
		pm->ps->weaponstate = WEAPON_READY;
		if ( pm->ps->weapon == WP_GAUNTLET )
			PM_StartTorsoAnim( TORSO_STAND2 );
		else
			PM_StartTorsoAnim( TORSO_STAND );

		return;
	}

	// check for fire
	if ( ! (pm->cmd.buttons & BUTTON_ATTACK) )
    {
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	// don't use a weapon we don't have
	if ( ! (pm->ps->stats[STAT_WEAPONS] & (1 << pm->ps->weapon)) ) {
		return;
	}

	// start the animation even if out of ammo
	if ( pm->ps->weapon == WP_GAUNTLET )
    {
		// the guantlet only "fires" when it actually hits something
		if ( !pm->gauntletHit )
		{
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			return;
		}
		PM_StartTorsoAnim( TORSO_ATTACK2 );
	}
	else if ( pm->ps->weapon != WP_GRAPPLING_HOOK )
    {
		PM_StartTorsoAnim( TORSO_ATTACK );
	}

    // no need to keep sending weapon fire event for the hook
	if ( pm->ps->weapon == WP_GRAPPLING_HOOK && pm->ps->weaponstate == WEAPON_FIRING )
		return;

	pm->ps->weaponstate = WEAPON_FIRING;



	// check for out of ammo
	if ( ! pm->ps->ammo[ pm->ps->weapon ] )
    {
		PM_AddEvent( EV_NOAMMO );
		pm->ps->weaponTime += 500;
		return;
	}

	// take an ammo away if not infinite
	if ( pm->ps->ammo[ pm->ps->weapon ] != -1 )
		pm->ps->ammo[ pm->ps->weapon ]--;

	// fire weapon
	PM_AddEvent( EV_FIRE_WEAPON );

	switch( pm->ps->weapon )
	{
        default:
        case WP_GAUNTLET:
            addTime = 400;
            break;
        case WP_LIGHTNING:
            addTime = 50;
            break;
        case WP_SHOTGUN:
            addTime = 1000;
            break;
        case WP_MACHINEGUN:
            addTime = 100;
            break;
        case WP_GRENADE_LAUNCHER:
            addTime = pm_grenadereload;
            break;
        case WP_ROCKET_LAUNCHER:
            addTime = 800;
            break;
        case WP_PLASMAGUN:
            addTime = 100;
            break;
        case WP_RAILGUN:
            addTime = 1500;
            break;
        case WP_BFG:
            addTime = 200;
            break;
        case WP_GRAPPLING_HOOK:
            addTime = 400;
            break;
	}

	if ( pm->ps->powerups[PW_HASTE] )
        addTime /= 1.3;

	pm->ps->weaponTime += addTime;
}

/*
================
PM_Animate
================
*/

static void PM_Animate(void)
{
	if ( pm->cmd.buttons & BUTTON_GESTURE )
    {
		if ( pm->ps->torsoTimer == 0 )
		{
			PM_StartTorsoAnim( TORSO_GESTURE );
			pm->ps->torsoTimer = TIMER_GESTURE;
			PM_AddEvent( EV_TAUNT );
		}
	}
}


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers(void)
{
	// drop misc timing counter
	if ( pm->ps->pm_time )
    {
		if ( pml.msec >= pm->ps->pm_time )
		{
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		}
        else
        {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if ( pm->ps->legsTimer > 0 )
    {
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 )
            pm->ps->legsTimer = 0;
	}

	if ( pm->ps->torsoTimer > 0 )
    {
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 )
            pm->ps->torsoTimer = 0;
	}
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd )
{
	short	temp;
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || (ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0) )
		return;		// no view changes at all

	// circularly clamp the angles with deltas
	for (i = 0 ; i < 3 ; i++) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if ( i == PITCH )
		{
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 )
			{
				ps->delta_angles[i] = (16000 - cmd->angles[i]) & 0xFFFF;
				temp = 16000;
			} else if ( temp < -16000 )
			{
				ps->delta_angles[i] = (-16000 - cmd->angles[i]) & 0xFFFF;
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}
}


/*
================
PmoveSingle
================
*/
void trap_SnapVector( float *v );

void PmoveSingle (pmove_t *pmove)
{
 	vec3_t oldOrigin;

	pm = pmove;

	// SLK copy current origin to oldOrigin for comparing it later
	VectorCopy(pm->ps->origin, oldOrigin);

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;


	if ( !pm_interference || pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 )
        pm->cmd.buttons &= ~BUTTON_WALKING;

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK )
        pm->ps->eFlags |= EF_TALK;
	else
        pm->ps->eFlags &= ~EF_TALK;


	// set the firing flag for continuous beam weapons
	if ( !(pm->ps->pm_flags & PMF_RESPAWNED)
    && pm->ps->pm_type != PM_INTERMISSION
    && pm->ps->pm_type != PM_NOCLIP
    && ( pm->cmd.buttons & BUTTON_ATTACK )
    && pm->ps->ammo[ pm->ps->weapon ] )
    {
        pm->ps->eFlags |= EF_FIRING;
	}
	else
    {
		pm->ps->eFlags &= ~EF_FIRING;
	}

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[STAT_HEALTH] > 0 && !( pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE) ) )
        pm->ps->pm_flags &= ~PMF_RESPAWNED;


	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK )
    {
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset (&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if ( pml.msec < 1 )
    {
        pml.msec = 1;
	}
	else if ( pml.msec > 200 )
    {
        pml.msec = 200;
	}

	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy (pm->ps->origin, pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy (pm->ps->velocity, pml.previous_velocity);

	pml.frametime = pml.msec * 0.001;

	// update the viewangles
	PM_UpdateViewAngles( pm->ps, &pm->cmd );

	AngleVectors (pm->ps->viewangles, pml.forward, pml.right, pml.up);

	if ( pm->cmd.upmove < 10 )
    {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
    }

    if ( pm->cmd.upmove >= 0 )
    {
		pm->ps->pm_flags &= ~PMF_DUCK_HELD;
	}

	// decide if backpedaling animations should be used
	if ( pm->cmd.forwardmove < 0 )
    {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	}
	else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) )
    {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( pm->ps->pm_type >= PM_DEAD )
    {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( pm_respawntimer )
    {
		pm_respawntimer -= pml.msec;
		if ( pm_respawntimer < 0 )
            pm_respawntimer = 0;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR )
    {
		PM_CheckDuck ();
		PM_FlyMove ();
		PM_DropTimers ();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP )
    {
		PM_NoclipMove ();
		PM_DropTimers ();
		return;
	}

	if ( (pm->ps->pm_type == PM_FREEZE) || (pm->ps->pm_type == PM_INTERMISSION ))
        return;		// no movement at all

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck ();

	// set groundentity
	PM_GroundTrace();

	if (pm->ps->pm_type == PM_DEAD)
        PM_DeadMove ();

    PM_CheckLadder();
	PM_DropTimers();

    // SLK DoubleJump/stair jump
    if (pm->ps->stats[STAT_JUMPTIME] > 0)
    {
        pm->ps->stats[STAT_JUMPTIME] -= pml.msec;
    }
    else if (pm->ps->stats[STAT_DFX_FLAG] & DFXF_STAIRJUMP)
    {
        pm->ps->stats[STAT_DFX_FLAG] &= ~DFXF_STAIRJUMP;
    }

    // SLK handle crouchslide timing
    if (pm->ps->stats[STAT_CROUCHTIME] > 0)
    {
		pm->ps->stats[STAT_CROUCHTIME] -= pml.msec;
	}

	// SLK Handle Walljumping
	if( pm->ps->groundEntityNum != ENTITYNUM_NONE )
    {
		pm->ps->stats[STAT_WJTIME] = WALLJUMP_WAIT;
		pm->ps->stats[STAT_WJCOUNT] = 0;
	}
	else  if (pm->ps->stats[STAT_WJTIME] > 0)
    {
		pm->ps->stats[STAT_WJTIME] -= pml.msec;
	}


    //SLK add keystate to playerstate
    pm->ps->stats[STAT_DFX_FLAG] &= ~DFXF_KEYBACK;
    pm->ps->stats[STAT_DFX_FLAG] &= ~DFXF_KEYFORWARD;
    if (pm->cmd.forwardmove > 0)
    {
        pm->ps->stats[STAT_DFX_FLAG] |= DFXF_KEYFORWARD;
    }
    else if (pm->cmd.forwardmove < 0)
    {
        pm->ps->stats[STAT_DFX_FLAG] |= DFXF_KEYBACK;
    }

    pm->ps->stats[STAT_DFX_FLAG] &= ~DFXF_KEYLEFT;
    pm->ps->stats[STAT_DFX_FLAG] &= ~DFXF_KEYRIGHT;
    if (pm->cmd.rightmove > 0)
    {
        pm->ps->stats[STAT_DFX_FLAG] |= DFXF_KEYRIGHT;
    }
    else if (pm->cmd.rightmove < 0)
    {
        pm->ps->stats[STAT_DFX_FLAG] |= DFXF_KEYLEFT;
    }

    pm->ps->stats[STAT_DFX_FLAG] &= ~DFXF_KEYCROUCH;
    pm->ps->stats[STAT_DFX_FLAG] &= ~DFXF_KEYJUMP;
    if (pm->cmd.upmove > 0)
    {
        pm->ps->stats[STAT_DFX_FLAG] |= DFXF_KEYJUMP;
    }
    else if (pm->cmd.upmove < 0)
    {
        pm->ps->stats[STAT_DFX_FLAG] |= DFXF_KEYCROUCH;
    }

    // Allow moving around on the ground when using hook
    if ( pm->ps->pm_flags & PMF_GRAPPLE_PULL ) {
		PM_GrappleMove();
	}

	if ( pm->ps->powerups[PW_FLIGHT] ) {
		// flight powerup doesn't allow jump and has different friction
		PM_FlyMove();
	}
	else if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)	{
		PM_WaterJumpMove();
	}
	else if ( pm->waterlevel > 1 ) {
		// swimming
		PM_WaterMove();
    }
    else if ( pml.ladder ) {
		// on a ladder
		PM_LadderMove();
    }
	else if ( pml.walking ) {
		// walking on ground
		PM_WalkMove();
	}

	else {
		// airborne
		PM_AirMove();
	}

	PM_Animate();

    // set groundentity, watertype, and waterlevel
	if (DistanceSquared(pm->ps->origin, oldOrigin) > 0) {
		PM_GroundTrace();
		PM_SetWaterLevel();
	}

	// weapons
	PM_Weapon();

	// torso animation
	PM_TorsoAnimation();

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();

	// don't snap velocity in free-fly or we will be not able to stop via flight friction
	if ( pm->ps->powerups[PW_FLIGHT] && !pml.groundPlane )
        return;

	// snap some parts of playerstate to save network bandwidth
	trap_SnapVector( pm->ps->velocity );
}


//
// Per-client physics modifications
//

#define MAX_MODDED_VARS 1024

enum {
	V_INT,
	V_FLOAT
};

typedef struct {
	void *var;
	int type;
	qbool norestore;
	union {
		int i;
		float f;
	} value, oldvalue;
} modVar_t;

struct {
	modVar_t moddedVars[MAX_MODDED_VARS];
	int numModdedVars;
} clientVars[MAX_CLIENTS];

// current client's modded vars
static modVar_t *moddedVars;
static int *numModdedVars;

static qbool setModdedVarContext( unsigned int clientNum ) {
	if ( clientNum >= MAX_CLIENTS ) {
		return qfalse;
	}

	moddedVars = clientVars[clientNum].moddedVars;
	numModdedVars = &clientVars[clientNum].numModdedVars;
	return qtrue;
}

#define modVarFloat( clientNum, var, newvalue, norestore ) modVar_( clientNum, var, V_FLOAT, 0, newvalue, norestore )
#define modVarInt( clientNum, var, newvalue, norestore ) modVar_( clientNum, var, V_INT, newvalue, 0, norestore )
static void modVar_( unsigned int clientNum, void *var, int type, int i, float f, qbool norestore ) {
	if ( !setModdedVarContext( clientNum ) ) {
		return;
	}

	if ( *numModdedVars >= MAX_MODDED_VARS ) {
		return;
	}

	moddedVars[*numModdedVars].var = var;
	moddedVars[*numModdedVars].type = type;
	moddedVars[*numModdedVars].norestore = norestore;
	switch ( type ) {
		case V_INT:
			moddedVars[*numModdedVars].oldvalue.i = *(int *)var;
			moddedVars[*numModdedVars].value.i = i;
			break;
		case V_FLOAT:
			moddedVars[*numModdedVars].oldvalue.f = *(float *)var;
			moddedVars[*numModdedVars].value.f = f;
			break;
		default:
			break;
	}
	(*numModdedVars)++;
}

void modVars( int clientNum ) {
	int i;

	if ( !setModdedVarContext( clientNum ) ) {
		return;
	}

	for ( i = 0; i < *numModdedVars; i++ ) {
		switch ( moddedVars[i].type ) {
			case V_INT:
				*(int *)moddedVars[i].var = moddedVars[i].value.i;
				break;
			case V_FLOAT:
				*(float *)moddedVars[i].var = moddedVars[i].value.f;
				break;
			default:
				break;
		}
	}
}

void restoreVars( int clientNum ) {
	int i;

	if ( !setModdedVarContext( clientNum ) ) {
		return;
	}

	for ( i = 0; i < *numModdedVars; i++ ) {
		if ( moddedVars[i].norestore )
			continue;

		switch ( moddedVars[i].type ) {
			case V_INT:
				*(int *)moddedVars[i].var = moddedVars[i].oldvalue.i;
				break;
			case V_FLOAT:
				*(float *)moddedVars[i].var = moddedVars[i].oldvalue.f;
				break;
			default:
				break;
		}
	}

	*numModdedVars = 0;
}

void BG_TouchPhysicsTrigger( playerState_t *ps, entityState_t *s ) {
	switch ( s->weapon ) {
		case PHYSICS_GRAVITY:
			modVarInt( ps->clientNum, &ps->gravity, s->angles2[0], qfalse );
			break;
		case PHYSICS_SPEED:
			// norestore because some hud elements need the modified value.
			// it doesn't need to be restored anyway because ps->speed is
			// reset every frame before pmove.
			modVarInt( ps->clientNum, &ps->speed, s->angles2[0], qtrue );
			break;
		case PHYSICS_FRICTION:
			modVarFloat( ps->clientNum, &pm_friction, s->angles2[0], qfalse );
			break;
		default:
			break;
	}
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove (pmove_t *pmove)
{
	int	finalTime;

	modVars( pmove->ps->clientNum );

	finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime )
        return;	// should not happen

	if ( finalTime > pmove->ps->commandTime + 1000 )
        pmove->ps->commandTime = finalTime - 1000;

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount+1) & ((1<<PS_PMOVEFRAMECOUNTBITS)-1);

	if ( pmove->ps->pm_flags & PMF_RESPAWNED && pm_respawntimer == 0 )
        pm_respawntimer = 250;

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while ( pmove->ps->commandTime != finalTime )
    {
		int	msec;

		msec = finalTime - pmove->ps->commandTime;

		if ( pmove->pmove_fixed )
        {
			if (msec > pmove->pmove_msec)
                msec = pmove->pmove_msec;
		}
		else
        {
			if (msec > 66)
                msec = 66;
		}

		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle( pmove );

		if (pmove->ps->pm_flags & PMF_JUMP_HELD)
            pmove->cmd.upmove = 20;
	}

	restoreVars( pmove->ps->clientNum );
}
