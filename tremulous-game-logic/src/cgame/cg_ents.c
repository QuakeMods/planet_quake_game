/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2013 Darklegion Development
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

// cg_ents.c -- present snapshot entities, happens every single frame


#include "cg_local.h"

/*
======================
CG_DrawBoxFace

Draws a bounding box face
======================
*/
static void CG_DrawBoxFace( vec3_t a, vec3_t b, vec3_t c, vec3_t d )
{
  polyVert_t  verts[ 4 ];
  vec4_t      color = { 255.0f, 0.0f, 0.0f, 128.0f };

  VectorCopy( d, verts[ 0 ].xyz );
  verts[ 0 ].st[ 0 ] = 1;
  verts[ 0 ].st[ 1 ] = 1;
  Vector4Copy( color, verts[ 0 ].modulate );

  VectorCopy( c, verts[ 1 ].xyz );
  verts[ 1 ].st[ 0 ] = 1;
  verts[ 1 ].st[ 1 ] = 0;
  Vector4Copy( color, verts[ 1 ].modulate );

  VectorCopy( b, verts[ 2 ].xyz );
  verts[ 2 ].st[ 0 ] = 0;
  verts[ 2 ].st[ 1 ] = 0;
  Vector4Copy( color, verts[ 2 ].modulate );

  VectorCopy( a, verts[ 3 ].xyz );
  verts[ 3 ].st[ 0 ] = 0;
  verts[ 3 ].st[ 1 ] = 1;
  Vector4Copy( color, verts[ 3 ].modulate );

  trap_R_AddPolyToScene( cgs.media.outlineShader, 4, verts );
}

/*
======================
CG_DrawBoundingBox

Draws a bounding box
======================
*/
void CG_DrawBoundingBox( vec3_t origin, vec3_t mins, vec3_t maxs )
{
  vec3_t  ppp, mpp, mmp, pmp;
  vec3_t  mmm, pmm, ppm, mpm;

  ppp[ 0 ] = origin[ 0 ] + maxs[ 0 ];
  ppp[ 1 ] = origin[ 1 ] + maxs[ 1 ];
  ppp[ 2 ] = origin[ 2 ] + maxs[ 2 ];

  mpp[ 0 ] = origin[ 0 ] + mins[ 0 ];
  mpp[ 1 ] = origin[ 1 ] + maxs[ 1 ];
  mpp[ 2 ] = origin[ 2 ] + maxs[ 2 ];

  mmp[ 0 ] = origin[ 0 ] + mins[ 0 ];
  mmp[ 1 ] = origin[ 1 ] + mins[ 1 ];
  mmp[ 2 ] = origin[ 2 ] + maxs[ 2 ];

  pmp[ 0 ] = origin[ 0 ] + maxs[ 0 ];
  pmp[ 1 ] = origin[ 1 ] + mins[ 1 ];
  pmp[ 2 ] = origin[ 2 ] + maxs[ 2 ];

  ppm[ 0 ] = origin[ 0 ] + maxs[ 0 ];
  ppm[ 1 ] = origin[ 1 ] + maxs[ 1 ];
  ppm[ 2 ] = origin[ 2 ] + mins[ 2 ];

  mpm[ 0 ] = origin[ 0 ] + mins[ 0 ];
  mpm[ 1 ] = origin[ 1 ] + maxs[ 1 ];
  mpm[ 2 ] = origin[ 2 ] + mins[ 2 ];

  mmm[ 0 ] = origin[ 0 ] + mins[ 0 ];
  mmm[ 1 ] = origin[ 1 ] + mins[ 1 ];
  mmm[ 2 ] = origin[ 2 ] + mins[ 2 ];

  pmm[ 0 ] = origin[ 0 ] + maxs[ 0 ];
  pmm[ 1 ] = origin[ 1 ] + mins[ 1 ];
  pmm[ 2 ] = origin[ 2 ] + mins[ 2 ];

  //phew!

  CG_DrawBoxFace( ppp, mpp, mmp, pmp );
  CG_DrawBoxFace( ppp, pmp, pmm, ppm );
  CG_DrawBoxFace( mpp, ppp, ppm, mpm );
  CG_DrawBoxFace( mmp, mpp, mpm, mmm );
  CG_DrawBoxFace( pmp, mmp, mmm, pmm );
  CG_DrawBoxFace( mmm, mpm, ppm, pmm );
}


/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
                             qhandle_t parentModel, char *tagName )
{
  int           i;
  orientation_t lerped;

  // lerp the tag
  trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
                  1.0 - parent->backlerp, tagName );

  // FIXME: allow origin offsets along tag?
  VectorCopy( parent->origin, entity->origin );
  for( i = 0; i < 3; i++ )
    VectorMA( entity->origin, lerped.origin[ i ], parent->axis[ i ], entity->origin );

  // had to cast away the const to avoid compiler problems...
  MatrixMultiply( lerped.axis, ( (refEntity_t *)parent )->axis, entity->axis );
  entity->backlerp = parent->backlerp;
}


/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
                                    qhandle_t parentModel, char *tagName )
{
  int           i;
  orientation_t lerped;
  vec3_t        tempAxis[ 3 ];

//AxisClear( entity->axis );
  // lerp the tag
  trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
                  1.0 - parent->backlerp, tagName );

  // FIXME: allow origin offsets along tag?
  VectorCopy( parent->origin, entity->origin );
  for( i = 0; i < 3; i++ )
    VectorMA( entity->origin, lerped.origin[ i ], parent->axis[ i ], entity->origin );

  // had to cast away the const to avoid compiler problems...
  MatrixMultiply( entity->axis, lerped.axis, tempAxis );
  MatrixMultiply( tempAxis, ( (refEntity_t *)parent )->axis, entity->axis );
}



/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
void CG_SetEntitySoundPosition( centity_t *cent )
{
  if( cent->currentState.eFlags & EF_BMODEL )
  {
    vec3_t  origin;
    float   *v;

    v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
    VectorAdd( cent->lerpOrigin, v, origin );
    trap_S_UpdateEntityPosition( cent->currentState.number, origin );
  }
  else
    trap_S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects( centity_t *cent )
{
  // update sound origins
  CG_SetEntitySoundPosition( cent );

  // add loop sound
  if( cent->currentState.loopSound )
  {
    if( cent->currentState.eType != ET_SPEAKER )
    {
      trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
                              cgs.gameSounds[ cent->currentState.loopSound ] );
    }
    else
    {
      trap_S_AddRealLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
                                  cgs.gameSounds[ cent->currentState.loopSound ] );
    }
  }


  // constant light glow
  if ( cent->currentState.constantLight &&
       cent->currentState.eType != ET_PLAYER &&
       cent->currentState.eType != ET_LEV2_ZAP_CHAIN )
  {
    int   cl;
    int   i, r, g, b;

    cl = cent->currentState.constantLight;
    r = cl & 255;
    g = ( cl >> 8 ) & 255;
    b = ( cl >> 16 ) & 255;
    i = ( ( cl >> 24 ) & 255 ) * 4;
    trap_R_AddLightToScene( cent->lerpOrigin, i, r, g, b );
  }

  if( CG_IsTrailSystemValid( &cent->muzzleTS ) )
  {
    //FIXME hack to prevent tesla trails reaching too far
    if( cent->currentState.eType == ET_BUILDABLE )
    {
      vec3_t  front, back;

      CG_AttachmentPoint( &cent->muzzleTS->frontAttachment, front );
      CG_AttachmentPoint( &cent->muzzleTS->backAttachment, back );

      if( Distance( front, back ) > ( TESLAGEN_RANGE * M_ROOT3 ) )
        CG_DestroyTrailSystem( &cent->muzzleTS );
    }

    if( cg.time > cent->muzzleTSDeathTime && CG_IsTrailSystemValid( &cent->muzzleTS ) )
      CG_DestroyTrailSystem( &cent->muzzleTS );
  }
}

/*
==================
CG_Invisible
==================
*/
static void CG_Invisible( centity_t *cent )
{
  if( cent->currentState.number >= MAX_CLIENTS )
    return;

  //sanity check that particle systems are stopped when becoming a spectator without dying
  if( CG_IsParticleSystemValid( &cent->muzzlePS ) )
    CG_DestroyParticleSystem( &cent->muzzlePS );

  if( CG_IsTrailSystemValid( &cent->muzzleTS ) )
    CG_DestroyTrailSystem( &cent->muzzleTS );

  if( CG_IsParticleSystemValid( &cent->jetPackPS ) )
    CG_DestroyParticleSystem( &cent->jetPackPS );
}

/*
==================
CG_General
==================
*/
static void CG_General( centity_t *cent )
{
  refEntity_t     ent;
  entityState_t   *s1;

  s1 = &cent->currentState;

  // if set to invisible, skip
  if( !s1->modelindex )
    return;

  memset( &ent, 0, sizeof( ent ) );

  // set frame

  ent.frame = s1->frame;
  ent.oldframe = ent.frame;
  ent.backlerp = 0;

  VectorCopy( cent->lerpOrigin, ent.origin);
  VectorCopy( cent->lerpOrigin, ent.oldorigin);

  ent.hModel = cgs.gameModels[ s1->modelindex ];

  // player model
  if( s1->number == cg.snap->ps.clientNum )
    ent.renderfx |= RF_THIRD_PERSON;  // only draw from mirrors

  // convert angles to axis
  AnglesToAxis( cent->lerpAngles, ent.axis );

  // add to refresh list
  trap_R_AddRefEntityToScene( &ent );
}

/*
==================
CG_Teleportal
==================
*/
static void CG_Teleportal( centity_t *cent )
{
  refEntity_t     ent;
  entityState_t   *s1;

  s1 = &cent->currentState;

  // if set to invisible, skip
  if( !s1->modelindex )
    return;

  memset( &ent, 0, sizeof( ent ) );

  // set frame

  ent.frame = s1->frame;
  ent.oldframe = ent.frame;
  ent.backlerp = 0;

  VectorCopy( cent->lerpOrigin, ent.origin);
  VectorCopy( cent->lerpOrigin, ent.oldorigin);

  ent.hModel = cgs.media.portal;

  if( s1->modelindex2 == PORTAL_BLUE )
    ent.customSkin = cgs.media.portalBlueSkin;
  else if( s1->modelindex2 == PORTAL_RED )
    ent.customSkin = cgs.media.portalRedSkin;

  // get axis
  VectorCopy( s1->origin2, ent.axis[ 2 ] );
  PerpendicularVector( ent.axis[ 1 ], ent.axis[ 2 ] );
  VectorSubtract( vec3_origin, ent.axis[ 1 ], ent.axis[ 1 ] );
  CrossProduct( ent.axis[ 2 ], ent.axis[ 1 ], ent.axis[ 0 ] );

  // add to refresh list
  trap_R_AddRefEntityToScene( &ent );
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker( centity_t *cent )
{
  if( ! cent->currentState.clientNum )
  { // FIXME: use something other than clientNum...
    return;   // not auto triggering
  }

  if( cg.time < cent->miscTime )
    return;

  trap_S_StartSound( NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[ cent->currentState.eventParm ] );

  //  ent->s.frame = ent->wait * 10;
  //  ent->s.clientNum = ent->random * 10;
  cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom( );
}


//============================================================================

/*
===============
CG_LaunchMissile
===============
*/
static void CG_LaunchMissile( centity_t *cent )
{
  entityState_t       *es;
  const weaponInfo_t  *wi;
  particleSystem_t    *ps;
  trailSystem_t       *ts;
  weapon_t            weapon;
  weaponMode_t        weaponMode;

  es = &cent->currentState;

  weapon = es->weapon;
  if( weapon > WP_NUM_WEAPONS )
    weapon = WP_NONE;

  wi = &cg_weapons[ weapon ];
  weaponMode = es->generic1;

  if( wi->wim[ weaponMode ].missileParticleSystem )
  {
    ps = CG_SpawnNewParticleSystem( wi->wim[ weaponMode ].missileParticleSystem );

    if( CG_IsParticleSystemValid( &ps ) )
    {
      CG_SetAttachmentCent( &ps->attachment, cent );
      CG_AttachToCent( &ps->attachment );
      ps->charge = es->torsoAnim;
    }
  }

  if( wi->wim[ weaponMode ].missileTrailSystem )
  {
    ts = CG_SpawnNewTrailSystem( wi->wim[ weaponMode ].missileTrailSystem );

    if( CG_IsTrailSystemValid( &ts ) )
    {
      CG_SetAttachmentCent( &ts->frontAttachment, cent );
      CG_AttachToCent( &ts->frontAttachment );
    }
  }
}

/*
===============
CG_LaserMine
===============
*/

static void CG_LaserMine(centity_t *cent, refEntity_t *ent) {
  entityState_t           *es;
  const weaponInfo_t      *wi;
  weapon_t                weapon;
  weaponMode_t            weaponMode;
  const weaponInfoMode_t  *wim;

  es = &cent->currentState;

  if(es->eType != ET_MISSILE) {
    return;
  }

  weapon = es->weapon;

  if( weapon != WP_LASERMINE ) {
    return;
  }

  wi = &cg_weapons[ weapon ];
  weaponMode = es->generic1;

  wim = &wi->wim[ weaponMode ];

  AnglesToAxis(es->apos.trBase, ent->axis);

  //draw laser beam
  if(es->eFlags & EF_WARN_CHARGE) {
    if(!CG_IsTrailSystemValid( &cent->lasermineTS)) {
      //create a new trail
      cent->lasermineTS = CG_SpawnNewTrailSystem( cgs.media.lasermineTS);
    }

    if(CG_IsTrailSystemValid(&cent->lasermineTS)) {
      vec3_t  end;
      trace_t trace;
      bgentity_id             unlinked_humans[MAX_CLIENTS];
      int                     num_unnlinked_humans = 0;
      int                     i;

      //ignore friendly human players
      for(i = 0; i < MAX_CLIENTS; i++) {
        centity_t *cent = &cg_entities[i];

        if(!cent->valid) {
          continue;
        }

        if(!cent->is_in_solid_list) {
          continue;
        }

        if(!cent->linked) {
          continue;
        }

        if(cgs.clientinfo[i].team != TEAM_HUMANS) {
          continue;
        }

        CG_Unlink_Solid_Entity(i);
        BG_UEID_set(&unlinked_humans[num_unnlinked_humans], i);
        num_unnlinked_humans++;
      }

      VectorMA(es->pos.trBase, LASERMINE_TRIP_RANGE, es->origin2, end);
      CG_Unlink_Solid_Entity(es->number);
      CG_Trace(
        &trace, es->pos.trBase, NULL, NULL, end,
        (
          cgs.clientinfo[cg.clientNum].team != TEAM_HUMANS &&
          cgs.clientinfo[cg.clientNum].team != TEAM_NONE) ? MAGIC_TRACE_HACK : ENTITYNUM_NONE,
        *Temp_Clip_Mask(MASK_SHOT, 0));
      CG_Link_Solid_Entity(es->number);
      CG_SetAttachmentCent( &cent->lasermineTS->frontAttachment, cent );
      CG_AttachToCent( &cent->lasermineTS->frontAttachment );
      CG_SetAttachmentPoint( &cent->lasermineTS->backAttachment, trace.endpos );
      CG_AttachToPoint(&cent->lasermineTS->backAttachment);

      //relink unlinked human players
      for(i = 0; i < num_unnlinked_humans; i++) {
        CG_Link_Solid_Entity(BG_UEID_get_ent_num(&unlinked_humans[i]));
      }
    }

    trap_S_AddLoopingSound( es->number, cent->lerpOrigin, vec3_origin, cgs.media.lasermineIdleSound );
  } else if(CG_IsTrailSystemValid(&cent->lasermineTS)) {
    CG_DestroyTrailSystem(&cent->lasermineTS);
    cent->lasermineTS = NULL;
  }

}

/*
===============
CG_Missile
===============
*/
static void CG_Missile( centity_t *cent )
{
  refEntity_t             ent;
  entityState_t           *es;
  const weaponInfo_t      *wi;
  weapon_t                weapon;
  weaponMode_t            weaponMode;
  const weaponInfoMode_t  *wim;

  es = &cent->currentState;

  weapon = es->weapon;
  if( weapon > WP_NUM_WEAPONS )
    weapon = WP_NONE;

  wi = &cg_weapons[ weapon ];
  weaponMode = es->generic1;

  wim = &wi->wim[ weaponMode ];

  // add dynamic light
  if( wim->missileDlight )
  {
    trap_R_AddLightToScene( cent->lerpOrigin, wim->missileDlight,
      wim->missileDlightColor[ 0 ],
      wim->missileDlightColor[ 1 ],
      wim->missileDlightColor[ 2 ] );
  }

  // add missile sound
  if( wim->missileSound )
  {
    vec3_t  velocity;

    BG_EvaluateTrajectoryDelta( &cent->currentState.pos, cg.time, velocity );

    trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, velocity, wim->missileSound );
  }

  // create the render entity
  memset( &ent, 0, sizeof( ent ) );
  VectorCopy( cent->lerpOrigin, ent.origin );
  VectorCopy( cent->lerpOrigin, ent.oldorigin );

  if( wim->usesSpriteMissle )
  {
    ent.reType = RT_SPRITE;
    ent.radius = wim->missileSpriteSize +
                 wim->missileSpriteCharge * es->torsoAnim;
    ent.rotation = 0;
    ent.customShader = wim->missileSprite;
    ent.shaderRGBA[ 0 ] = 0xFF;
    ent.shaderRGBA[ 1 ] = 0xFF;
    ent.shaderRGBA[ 2 ] = 0xFF;
    ent.shaderRGBA[ 3 ] = 0xFF;
  }
  else
  {
    ent.hModel = wim->missileModel;
    ent.renderfx = wim->missileRenderfx | RF_NOSHADOW;

    // convert direction of travel into axis
    if( VectorNormalize2( es->pos.trDelta, ent.axis[ 0 ] ) == 0 )
      ent.axis[ 0 ][ 2 ] = 1;

    // spin as it moves
    if( es->pos.trType != TR_STATIONARY && wim->missileRotates )
      RotateAroundDirection( ent.axis, cg.time / 4 );
    else
      RotateAroundDirection( ent.axis, es->time );

    if( wim->missileAnimates )
    {
      int timeSinceStart = cg.time - es->time;

      if( wim->missileAnimLooping )
      {
        ent.frame = wim->missileAnimStartFrame +
          (int)( ( timeSinceStart / 1000.0f ) * wim->missileAnimFrameRate ) %
          wim->missileAnimNumFrames;
      }
      else
      {
        ent.frame = wim->missileAnimStartFrame +
          (int)( ( timeSinceStart / 1000.0f ) * wim->missileAnimFrameRate );

        if( ent.frame > ( wim->missileAnimStartFrame + wim->missileAnimNumFrames ) )
          ent.frame = wim->missileAnimStartFrame + wim->missileAnimNumFrames;
      }
    }
  }

  CG_LaserMine(cent, &ent);

  //only refresh if there is something to display
  if( wim->missileSprite || wim->missileModel )
    trap_R_AddRefEntityToScene( &ent );
}

/*
===============
CG_Mover
===============
*/
static void CG_Mover( centity_t *cent )
{
  refEntity_t     ent;
  entityState_t   *s1;

  s1 = &cent->currentState;

  if( !s1->modelindex )
    return;

  // create the render entity
  memset( &ent, 0, sizeof( ent ) );
  VectorCopy( cent->lerpOrigin, ent.origin );
  VectorCopy( cent->lerpOrigin, ent.oldorigin );
  AnglesToAxis( cent->lerpAngles, ent.axis );

  ent.renderfx = RF_NOSHADOW;

  // flicker between two skins (FIXME?)
  ent.skinNum = ( cg.time >> 6 ) & 1;

  // get the model, either as a bmodel or a modelindex
  if( s1->eFlags & EF_BMODEL )
    ent.hModel = cgs.inlineDrawModel[ s1->modelindex ];
  else
    ent.hModel = cgs.gameModels[ s1->modelindex ];

  // add to refresh list
  trap_R_AddRefEntityToScene( &ent );

  // add the secondary model
  if( s1->modelindex2 )
  {
    ent.skinNum = 0;
    ent.hModel = cgs.gameModels[ s1->modelindex2 ];
    trap_R_AddRefEntityToScene( &ent );
  }

}

/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam( centity_t *cent )
{
  refEntity_t     ent;
  entityState_t   *s1;

  s1 = &cent->currentState;

  // create the render entity
  memset( &ent, 0, sizeof( ent ) );
  VectorCopy( s1->pos.trBase, ent.origin );
  VectorCopy( s1->origin2, ent.oldorigin );
  AxisClear( ent.axis );
  ent.reType = RT_BEAM;

  ent.renderfx = RF_NOSHADOW;

  // add to refresh list
  trap_R_AddRefEntityToScene( &ent );
}


/*
===============
CG_Portal
===============
*/
static void CG_Portal( centity_t *cent )
{
  refEntity_t     ent;
  entityState_t   *s1;

  s1 = &cent->currentState;

  // create the render entity
  memset( &ent, 0, sizeof( ent ) );
  VectorCopy( cent->lerpOrigin, ent.origin );
  VectorCopy( s1->origin2, ent.oldorigin );
  ByteToDir( s1->eventParm, ent.axis[ 0 ] );
  PerpendicularVector( ent.axis[ 1 ], ent.axis[ 0 ] );

  // negating this tends to get the directions like they want
  // we really should have a camera roll value
  VectorSubtract( vec3_origin, ent.axis[ 1 ], ent.axis[ 1 ] );

  CrossProduct( ent.axis[ 0 ], ent.axis[ 1 ], ent.axis[ 2 ] );
  ent.reType = RT_PORTALSURFACE;
  ent.oldframe = s1->misc;
  ent.frame = s1->frame;    // rotation speed
  ent.skinNum = s1->clientNum / 256.0 * 360;  // roll offset

  // add to refresh list
  trap_R_AddRefEntityToScene( &ent );
}

//============================================================================

#define SETBOUNDS(v1,v2,r)  ((v1)[0]=(-r/2),(v1)[1]=(-r/2),(v1)[2]=(-r/2),\
                             (v2)[0]=(r/2),(v2)[1]=(r/2),(v2)[2]=(r/2))
#define RADIUSSTEP          0.5f

#define FLARE_OFF       0
#define FLARE_NOFADE    1
#define FLARE_TIMEFADE  2
#define FLARE_REALFADE  3

/*
=========================
CG_LightFlare
=========================
*/
static void CG_LightFlare( centity_t *cent )
{
  refEntity_t   flare;
  entityState_t *es;
  vec3_t        forward, delta;
  float         len;
  trace_t       tr;
  float         maxAngle;
  vec3_t        mins, maxs, start, end;
  float         srcRadius, srLocal, ratio = 1.0f;
  int           entityNum;

  es = &cent->currentState;

  if( cg.renderingThirdPerson )
    entityNum = MAGIC_TRACE_HACK;
  else
    entityNum = cg.predictedPlayerState.clientNum;

  //don't draw light flares
  if( cg_lightFlare.integer == FLARE_OFF )
    return;

  //flare is "off"
  if( es->eFlags & EF_NODRAW )
    return;

  CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, es->angles2,
            entityNum, *Temp_Clip_Mask(MASK_SHOT, 0) );

  //if there is no los between the view and the flare source
  //it definately cannot be seen
  if( tr.fraction < 1.0f || tr.allsolid )
    return;

  memset( &flare, 0, sizeof( flare ) );

  flare.reType = RT_SPRITE;
  flare.customShader = cgs.gameShaders[ es->modelindex ];
  flare.shaderRGBA[ 0 ] = 0xFF;
  flare.shaderRGBA[ 1 ] = 0xFF;
  flare.shaderRGBA[ 2 ] = 0xFF;
  flare.shaderRGBA[ 3 ] = 0xFF;

  //flares always drawn before the rest of the scene
  flare.renderfx |= RF_DEPTHHACK;

  //bunch of geometry
  AngleVectors( cent->lerpAngles, forward, NULL, NULL );
  VectorCopy( cent->lerpOrigin, flare.origin );
  VectorSubtract( flare.origin, cg.refdef.vieworg, delta );
  len = VectorLength( delta );
  VectorNormalize( delta );

  //flare is too close to camera to be drawn
  if( len < es->generic1 )
    return;

  //don't bother for flares behind the view plane
  if( DotProduct( delta, cg.refdef.viewaxis[ 0 ] ) < 0.0 )
    return;

  //only recalculate radius and ratio every three frames
  if( !( cg.clientFrame % 2 ) )
  {
    //can only see the flare when in front of it
    flare.radius = len / es->origin2[ 0 ];

    if( es->origin2[ 2 ] == 0 )
      srcRadius = srLocal = flare.radius / 2.0f;
    else
      srcRadius = srLocal = len / es->origin2[ 2 ];

    maxAngle = es->origin2[ 1 ];

    if( maxAngle > 0.0f )
    {
      float radiusMod = 1.0f - ( 180.0f - RAD2DEG(
            acos( DotProduct( delta, forward ) ) ) ) / maxAngle;

      if( radiusMod < 0.0f )
        radiusMod = 0.0f;

      flare.radius *= radiusMod;
    }

    if( flare.radius < 0.0f )
      flare.radius = 0.0f;

    VectorMA( flare.origin, -flare.radius, delta, end );
    VectorMA( cg.refdef.vieworg, flare.radius, delta, start );

    if( cg_lightFlare.integer == FLARE_REALFADE )
    {
      //"correct" flares
      CG_BiSphereTrace( &tr, cg.refdef.vieworg, end,
          1.0f, srcRadius, entityNum, *Temp_Clip_Mask(MASK_SHOT, 0));

      if( tr.fraction < 1.0f )
        ratio = tr.lateralFraction;
      else
        ratio = 1.0f;
    }
    else if( cg_lightFlare.integer == FLARE_TIMEFADE )
    {
      //draw timed flares
      SETBOUNDS( mins, maxs, srcRadius );
      CG_Trace( &tr, start, mins, maxs, end,
                entityNum, *Temp_Clip_Mask(MASK_SHOT, 0) );

      if( ( tr.fraction < 1.0f || tr.startsolid ) && cent->lfs.status )
      {
        cent->lfs.status = qfalse;
        cent->lfs.lastTime = cg.time;
      }
      else if( ( tr.fraction == 1.0f && !tr.startsolid ) && !cent->lfs.status )
      {
        cent->lfs.status = qtrue;
        cent->lfs.lastTime = cg.time;
      }

      //fade flare up
      if( cent->lfs.status )
      {
        if( cent->lfs.lastTime + es->time > cg.time )
          ratio = (float)( cg.time - cent->lfs.lastTime ) / es->time;
      }

      //fade flare down
      if( !cent->lfs.status )
      {
        if( cent->lfs.lastTime + es->time > cg.time )
        {
          ratio = (float)( cg.time - cent->lfs.lastTime ) / es->time;
          ratio = 1.0f - ratio;
        }
        else
          ratio = 0.0f;
      }
    }
    else if( cg_lightFlare.integer == FLARE_NOFADE )
    {
      //draw nofade flares
      SETBOUNDS( mins, maxs, srcRadius );
      CG_Trace( &tr, start, mins, maxs, end,
                entityNum, *Temp_Clip_Mask(MASK_SHOT, 0) );

      //flare source occluded
      if( ( tr.fraction < 1.0f || tr.startsolid ) )
        ratio = 0.0f;
    }
  }
  else
  {
    ratio        = cent->lfs.lastRatio;
    flare.radius = cent->lfs.lastRadius;
  }

  cent->lfs.lastRatio  = ratio;
  cent->lfs.lastRadius = flare.radius;

  if( ratio < 1.0f )
  {
    flare.radius *= ratio;
    flare.shaderRGBA[ 3 ] = (byte)( (float)flare.shaderRGBA[ 3 ] * ratio );
  }

  if( flare.radius <= 0.0f )
    return;

  trap_R_AddRefEntityToScene( &flare );
}

/*
=========================
CG_Lev2ZapChain
=========================
*/
static void CG_Lev2ZapChain( centity_t *cent )
{
  int           i;
  entityState_t *es;
  centity_t     *source = NULL, *target = NULL;
  int           entityNums[ LEVEL2_AREAZAP_MAX_TARGETS + 1 ];
  int           count;

  es = &cent->currentState;

  count = BG_UnpackEntityNumbers( es, entityNums, LEVEL2_AREAZAP_MAX_TARGETS + 1 );

  for( i = 1; i < count; i++ )
  {
    if( i == 1 )
    {
      // First entity is the attacker
      source = &cg_entities[ entityNums[ 0 ] ];
    }
    else
    {
      // Subsequent zaps come from the first target
      source = &cg_entities[ entityNums[ 1 ] ];
    }

    target = &cg_entities[ entityNums[ i ] ];

    if( !CG_IsTrailSystemValid( &cent->level2ZapTS[ i ] ) )
      cent->level2ZapTS[ i ] = CG_SpawnNewTrailSystem( cgs.media.level2ZapTS );

    if( CG_IsTrailSystemValid( &cent->level2ZapTS[ i ] ) )
    {
      CG_SetAttachmentCent( &cent->level2ZapTS[ i ]->frontAttachment, source );
      CG_SetAttachmentCent( &cent->level2ZapTS[ i ]->backAttachment, target );
      CG_AttachToCent( &cent->level2ZapTS[ i ]->frontAttachment );
      CG_AttachToCent( &cent->level2ZapTS[ i ]->backAttachment );
    }
  }
}

/*
================
CG_Get_Foundation_Ent_Num

Recursively determines if an entity is ultimately
supported by the world, a mover, or has no foundational support
================
*/
static int CG_Get_Foundation_Ent_Num(int ent_num) {
  static int depth = 0;
  int groundEntityNum;
  int foundation_for_ground;
  centity_t *cent;

  if(depth > 50) {
    return ENTITYNUM_NONE;
  }

  Com_Assert(ent_num >= 0 && ent_num < MAX_GENTITIES);

  cent = &cg_entities[ent_num];

  if(!cent->valid &&
    ent_num != cg.predictedPlayerState.clientNum) {
    return ENTITYNUM_NONE;
  }

  if(ent_num == cg.predictedPlayerState.clientNum) {
    groundEntityNum = cg.predictedPlayerState.groundEntityNum;
  } else {
    groundEntityNum = cent->currentState.groundEntityNum;
  }

  if(groundEntityNum == ENTITYNUM_NONE) {
    return ENTITYNUM_NONE;
  }

  //check to see if the ground entity is a foundation entitity
  if(
    groundEntityNum == ENTITYNUM_WORLD ||
    cg_entities[groundEntityNum].currentState.eType == ET_MOVER ) {
    return groundEntityNum;
  } else if(cg_entities[groundEntityNum].currentState.otherEntityNum != ENTITYNUM_NONE) {
    return cg_entities[groundEntityNum].currentState.otherEntityNum;
  }

  if(groundEntityNum < 0 || groundEntityNum >= MAX_GENTITIES) {
    if(cg_debugPVS.integer) {
      CG_Printf(
        "^1CG_Get_Foundation_Ent_Num: groundEntityNum %d for entity num %d is invalid^7\n",
        groundEntityNum, ent_num);
    }

    return ENTITYNUM_NONE;
  }

  //check the ground entity to see if it is on a foundation entity
  depth++;
  foundation_for_ground = CG_Get_Foundation_Ent_Num(groundEntityNum);
  depth--;
  return foundation_for_ground;
}

/*
============
CG_Get_Pusher_Num
============
*/
int CG_Get_Pusher_Num(int ent_num) {
  int          pusher_num;
  entityType_t eType;

  Com_Assert(ent_num >= 0 && ent_num < MAX_GENTITIES);

  if(ent_num == cg.predictedPlayerState.clientNum) {
    pusher_num = cg.predictedPlayerState.otherEntityNum;
    eType = ET_PLAYER;
  } else {
    pusher_num = cg_entities[ent_num].currentState.otherEntityNum;
    eType = cg_entities[ent_num].currentState.eType;
  }

  if(
    eType != ET_ITEM && eType != ET_BUILDABLE &&
    eType != ET_CORPSE && eType != ET_PLAYER) {
    return ENTITYNUM_NONE;
  }

  return (pusher_num == ENTITYNUM_NONE ? CG_Get_Foundation_Ent_Num(ent_num) : pusher_num);
}

/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
Returns any change for the YAW of delta_angles for the local playerState
=========================
*/
float CG_AdjustPositionForMover(
  const vec3_t pos_in,
  int          moverNum,
  int          fromTime,
  int          toTime,
  vec3_t       pos_out) {
  centity_t    *cent;
  vec3_t       oldOrigin, origin;
  vec3_t       oldAngles, angles;
  vec3_t       org, org2, move, move2, amove;
  vec3_t       matrix[3], transpose[3];

  if(moverNum < 0 || moverNum >= ENTITYNUM_MAX_NORMAL) {
    VectorCopy( pos_in, pos_out );
    return 0.0f;
  }

  cent = &cg_entities[moverNum];

  if(cent->currentState.eType != ET_MOVER) {
    VectorCopy(pos_in, pos_out);
    return 0.0f;
  }

  BG_EvaluateTrajectory(&cent->currentState.pos, fromTime, oldOrigin);
  BG_EvaluateTrajectory(&cent->currentState.apos, fromTime, oldAngles);

  BG_EvaluateTrajectory(&cent->currentState.pos, toTime, origin);
  BG_EvaluateTrajectory(&cent->currentState.apos, toTime, angles);

  VectorSubtract(origin, oldOrigin, move);
  VectorSubtract(angles, oldAngles, amove);

  // figure movement due to the pusher's amove
  BG_CreateRotationMatrix(amove, transpose);
  BG_TransposeMatrix(transpose, matrix);

  VectorSubtract(pos_in, oldOrigin, org);

  VectorCopy(org, org2);
  BG_RotatePoint(org2, matrix);
  VectorSubtract(org2, org, move2);
  // add movement
  VectorAdd(pos_in, move, pos_out);
  VectorAdd(pos_out, move2, pos_out);

  return amove[ YAW ];
}


/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition( centity_t *cent, int time )
{
  vec3_t    current, next;
  float     f;

  // it would be an internal error to find an entity that interpolates without
  // a snapshot ahead of the current one
  if( cg.nextSnap == NULL )
    CG_Error( "CG_InterpoateEntityPosition: cg.nextSnap == NULL" );

  if(time == cg.time) {
    f = cg.frameInterpolation;
  } else {
    int   delta;

    delta = ( cg.nextSnap->serverTime - cg.snap->serverTime );

    if( delta == 0 ) {
      f = 0;
    } else {
      f = (float)( time - cg.snap->serverTime ) / delta;
    }
  }

  // this will linearize a sine or parabolic curve, but it is important
  // to not extrapolate player positions if more recent data is available
  BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
  BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next );

  cent->lerpOrigin[ 0 ] = current[ 0 ] + f * ( next[ 0 ] - current[ 0 ] );
  cent->lerpOrigin[ 1 ] = current[ 1 ] + f * ( next[ 1 ] - current[ 1 ] );
  cent->lerpOrigin[ 2 ] = current[ 2 ] + f * ( next[ 2 ] - current[ 2 ] );

  BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
  BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next );

  cent->lerpAngles[ 0 ] = LerpAngle( current[ 0 ], next[ 0 ], f );
  cent->lerpAngles[ 1 ] = LerpAngle( current[ 1 ], next[ 1 ], f );
  cent->lerpAngles[ 2 ] = LerpAngle( current[ 2 ], next[ 2 ], f );
}

/*
===============
CG_CalcEntityLerpPositions

===============
*/
void CG_CalcEntityLerpPositions(centity_t *cent, int time)
{
  float  delta_yaw;
  // this will be set to how far forward projectiles will be extrapolated
  int timeshift = 0;

  // if this player does not want to see extrapolated players
  if( !cg_smoothClients.integer )
  {
    // make sure the clients use TR_INTERPOLATE
    if( cent->currentState.number < MAX_CLIENTS )
    {
      cent->currentState.pos.trType = TR_INTERPOLATE;
      cent->nextState.pos.trType = TR_INTERPOLATE;
    }
  }

  if( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE )
  {
    CG_InterpolateEntityPosition( cent, time );
    return;
  }

  // first see if we can interpolate between two snaps for
  // linear extrapolated clients
  if( cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
      cent->currentState.number < MAX_CLIENTS )
  {
    CG_InterpolateEntityPosition( cent, time );
    return;
  }

  if( cg_projectileNudge.integer &&
      !cg.demoPlayback &&
      cent->currentState.eType == ET_MISSILE &&
      !( cg.snap->ps.pm_flags & PMF_FOLLOW ) )
  {
    timeshift = cg.ping;
  }

  // just use the current frame and evaluate as best we can
  BG_EvaluateTrajectory( &cent->currentState.pos,
    ( time + timeshift ), cent->lerpOrigin );
  BG_EvaluateTrajectory( &cent->currentState.apos,
    ( time + timeshift ), cent->lerpAngles );

  if( timeshift )
  {
    trace_t tr;
    vec3_t lastOrigin;
	
    BG_EvaluateTrajectory( &cent->currentState.pos, time, lastOrigin );
	
    CG_Trace( &tr, lastOrigin, vec3_origin, vec3_origin, cent->lerpOrigin,
      cent->currentState.number, *Temp_Clip_Mask(MASK_SHOT, 0) );
	
    // don't let the projectile go through the floor
    if( tr.fraction < 1.0f )
      VectorLerp2( tr.fraction, lastOrigin, cent->lerpOrigin, cent->lerpOrigin );
  }

  // adjust for riding a mover if it wasn't rolled into the predicted
  // player state
  if(cent->currentState.number != cg.predictedPlayerState.clientNum) {
    delta_yaw = CG_AdjustPositionForMover(
                  cent->lerpOrigin, CG_Get_Pusher_Num(cent->currentState.number),
                  cg.snap->serverTime, time, cent->lerpOrigin);
    cent->lerpAngles[YAW] += delta_yaw;
  }
}


/*
================
CG_RangeMarker
================
*/
void CG_RangeMarker( centity_t *cent )
{
  qboolean drawS, drawI, drawF;
  float so, lo, th;
  rangeMarkerType_t rmType;
  float range;
  vec3_t rgb;

  //only display range markers for buildables on your team
  if( BG_Buildable( cent->currentState.modelindex )->team != cg.predictedPlayerState.stats[ STAT_TEAM ] )
    return;

  //only display range markers if you are a builder
  if( ( BG_GetPlayerWeapon( &cg.predictedPlayerState ) != WP_HBUILD ) &&
      cg.predictedPlayerState.weapon != WP_ABUILD && cg.predictedPlayerState.weapon != WP_ABUILD2 )
    return;

  if( !( cg_buildableRangeMarkerMask.integer &
         ( 1 << cent->currentState.modelindex ) ) )
    return;

  if( CG_GetRangeMarkerPreferences( &drawS, &drawI, &drawF, &so, &lo, &th ) &&
      CG_GetBuildableRangeMarkerProperties( cent->currentState.modelindex, &rmType, &range, rgb ) )
  {
    vec3_t origin, angles;

    if( BG_Buildable( cent->currentState.modelindex )->rangeMarkerOriginAtTop )
    {
      vec3_t mins, maxs;

      BG_BuildableBoundingBox( cent->currentState.modelindex, mins, maxs );
      VectorMA( cent->lerpOrigin, maxs[ 2 ],
                cent->currentState.origin2, origin );
    } else
      VectorCopy( cent->lerpOrigin, origin );

    if( BG_Buildable( cent->currentState.modelindex )->rangeMarkerUseNormal )
    {
      vectoangles( cent->currentState.origin2, angles );
    } else
      VectorCopy( cent->lerpAngles, angles );

    CG_DrawRangeMarker( rmType, origin, ( rmType > 0 ? angles : NULL ),
                        range, drawS, drawI, drawF, rgb, so, lo, th );
  }
}


/*
===============
CG_CEntityPVSEnter

===============
*/
static void CG_CEntityPVSEnter( centity_t *cent )
{
  entityState_t *es = &cent->currentState;

  if( cg_debugPVS.integer )
    CG_Printf( "Entity %d entered PVS\n", cent->currentState.number );

  switch( es->eType )
  {
    case ET_MISSILE:
      CG_LaunchMissile( cent );
      break;

    case ET_BUILDABLE:
      cent->lastBuildableHealth = es->misc;
      cent->turret_idle_scan_progress = 0;
      break;
  }

  //clear any particle systems from previous uses of this centity_t
  cent->muzzlePS = NULL;
  cent->muzzlePsTrigger = qfalse;
  cent->jetPackPS = NULL;
  cent->jetPackState = JPS_OFF;
  cent->buildablePS = NULL;
  cent->entityPS = NULL;
  cent->entityPSMissing = qfalse;

  //make sure that the buildable animations are in a consistent state
  //when a buildable enters the PVS
  cent->buildableAnim = cent->lerpFrame.animationNumber = BANIM_NONE;
  cent->oldBuildableAnim = es->legsAnim;
}


/*
===============
CG_CEntityPVSLeave

===============
*/
static void CG_CEntityPVSLeave( centity_t *cent )
{
  int           i;
  entityState_t *es = &cent->currentState;

  if( cg_debugPVS.integer )
    CG_Printf( "Entity %d left PVS\n", cent->currentState.number );
  switch( es->eType )
  {
    case ET_LEV2_ZAP_CHAIN:
      for( i = 0; i <= LEVEL2_AREAZAP_MAX_TARGETS; i++ )
      {
        if( CG_IsTrailSystemValid( &cent->level2ZapTS[ i ] ) )
          CG_DestroyTrailSystem( &cent->level2ZapTS[ i ] );
      }
      break;
  }

  if(CG_IsTrailSystemValid(&cent->lasermineTS)) {
    CG_DestroyTrailSystem(&cent->lasermineTS);
  }
}


/*
===============
CG_AddCEntity

===============
*/
static void CG_AddCEntity( centity_t *cent )
{
  // event-only entities will have been dealt with already
  if( cent->currentState.eType >= ET_EVENTS )
    return;

  // calculate the current origin
  CG_CalcEntityLerpPositions( cent, cg.time );

  // check to see if a body has been gibbed
  if( cg_blood.integer &&
      ( cent->currentState.eType == ET_PLAYER ||
        cent->currentState.eType == ET_CORPSE ) &&
      ( cent->currentState.otherEntityNum2 & SFL_GIBBED ) )
    cent->currentState.eType = ET_INVISIBLE;

  // add automatic effects
  CG_EntityEffects( cent );

  switch( cent->currentState.eType )
  {
    default:
      CG_Error( "Bad entity type: %i", cent->currentState.eType );
      break;

    case ET_INVISIBLE:
      CG_Invisible( cent );
      break;

    case ET_PUSH_TRIGGER:
    case ET_TELEPORT_TRIGGER:
    case ET_LOCATION:
      break;

    case ET_GENERAL:
      CG_General( cent );
      break;

    case ET_CORPSE:
      CG_Corpse( cent );
      break;

    case ET_PLAYER:
      CG_Player( cent );
      break;

    case ET_BUILDABLE:
      CG_Buildable( cent );
      CG_RangeMarker( cent );
      break;

    case ET_MISSILE:
      CG_Missile( cent );
      break;

    case ET_MOVER:
      CG_Mover( cent );
      break;

    case ET_BEAM:
      CG_Beam( cent );
      break;

    case ET_PORTAL:
      CG_Portal( cent );
      break;

    case ET_SPEAKER:
      CG_Speaker( cent );
      break;

    case ET_PARTICLE_SYSTEM:
      CG_ParticleSystemEntity( cent );
      break;

    case ET_ANIMMAPOBJ:
      CG_AnimMapObj( cent );
      break;

    case ET_MODELDOOR:
      CG_ModelDoor( cent );
      break;

    case ET_LIGHTFLARE:
      CG_LightFlare( cent );
      break;

    case ET_LEV2_ZAP_CHAIN:
      CG_Lev2ZapChain( cent );
      break;

    case ET_TELEPORTAL:
      CG_Teleportal( cent );
      break;
  }
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities( void )
{
  int             num;
  centity_t       *cent;
  playerState_t   *ps;

  // set cg.frameInterpolation
  if( cg.nextSnap )
  {
    int   delta;

    delta = ( cg.nextSnap->serverTime - cg.snap->serverTime );

    if( delta == 0 )
      cg.frameInterpolation = 0;
    else
      cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime ) / delta;
  }
  else
  {
    cg.frameInterpolation = 0;  // actually, it should never be used, because
                  // no entities should be marked as interpolating
  }

  // the auto-rotating items will all have the same axis
  cg.autoAngles[ 0 ] = 0;
  cg.autoAngles[ 1 ] = ( cg.time & 2047 ) * 360 / 2048.0;
  cg.autoAngles[ 2 ] = 0;

  cg.autoAnglesFast[ 0 ] = 0;
  cg.autoAnglesFast[ 1 ] = ( cg.time & 1023 ) * 360 / 1024.0f;
  cg.autoAnglesFast[ 2 ] = 0;

  AnglesToAxis( cg.autoAngles, cg.autoAxis );
  AnglesToAxis( cg.autoAnglesFast, cg.autoAxisFast );

  // generate and add the entity from the playerstate
  ps = &cg.predictedPlayerState;
  BG_PlayerStateToEntityState(
    ps, &cg.predictedPlayerEntity.currentState, &cg.pmext );
  cg.predictedPlayerEntity.valid = qtrue;
  CG_AddCEntity( &cg.predictedPlayerEntity );

  // lerp the non-predicted value for lightning gun origins
  CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ], cg.time );

  // scanner
  CG_UpdateEntityPositions( );

  for( num = 0; num < MAX_GENTITIES; num++ )
    cg_entities[ num ].valid = qfalse;

  // add each entity sent over by the server
  for( num = 0; num < cg.snap->numEntities; num++ )
  {
    cent = &cg_entities[ cg.snap->entities[ num ].number ];
    cent->valid = qtrue;
  }

  for( num = 0; num < MAX_GENTITIES; num++ )
  {
    cent = &cg_entities[ num ];

    if( cent->valid && !cent->oldValid )
      CG_CEntityPVSEnter( cent );
    else if( !cent->valid && cent->oldValid )
      CG_CEntityPVSLeave( cent );

    cent->oldValid = cent->valid;
  }

  // add each entity sent over by the server
  for( num = 0; num < cg.snap->numEntities; num++ )
  {
    cent = &cg_entities[ cg.snap->entities[ num ].number ];
    CG_AddCEntity( cent );
  }

  //make an attempt at drawing bounding boxes of selected entity types
  if( cg_drawBBOX.integer )
  {
    for( num = 0; num < cg.snap->numEntities; num++ )
    {
      float         x, zd, zu;
      vec3_t        mins, maxs;
      entityState_t *es;

      cent = &cg_entities[ cg.snap->entities[ num ].number ];
      es = &cent->currentState;

      switch( es->eType )
      {
        case ET_PLAYER:
        case ET_BUILDABLE:
        case ET_MISSILE:
        case ET_CORPSE:
          x = ( es->solid & 255 );
          zd = ( ( es->solid >> 8 ) & 255 );
          zu = ( ( es->solid >> 16 ) & 255 ) - 32;

          mins[ 0 ] = mins[ 1 ] = -x;
          maxs[ 0 ] = maxs[ 1 ] = x;
          mins[ 2 ] = -zd;
          maxs[ 2 ] = zu;

          CG_DrawBoundingBox( cent->lerpOrigin, mins, maxs );
          break;

        default:
          break;
      }
    }
  }
}
