#include "WeaponSelector.h"
#include "../bot.h"
#include "../../../gameshared/q_collision.h"



inline void BotWeaponSelector::Debug( const char *format, ... ) const {
	va_list va;
	va_start( va, format );
	AI_Debugv( game.edicts[bot->EntNum()].r.client->netname, format, va );
	va_end( va );
}

inline bool BotWeaponSelector::BotHasQuad() const { return ::HasQuad( game.edicts + bot->EntNum() ); }
inline bool BotWeaponSelector::BotHasShell() const { return ::HasShell( game.edicts + bot->EntNum() ); }
inline bool BotWeaponSelector::BotHasPowerups() const { return ::HasPowerups( game.edicts + bot->EntNum() ); }
inline bool BotWeaponSelector::BotIsCarrier() const { return ::IsCarrier( game.edicts + bot->EntNum() ); }

inline const int *BotWeaponSelector::Inventory() const { return bot->PlayerState()->inventory; }

template <int Weapon>
inline int BotWeaponSelector::AmmoReadyToFireCount() const {
	if( !Inventory()[Weapon] ) {
		return 0;
	}
	return Inventory()[WeaponAmmo < Weapon > ::strongAmmoTag] + Inventory()[WeaponAmmo < Weapon > ::weakAmmoTag];
}

inline int BotWeaponSelector::BlastsReadyToFireCount() const {
	// Check only strong ammo, the weak ammo enables blade attack
	const auto *inventory = Inventory();
	return inventory[WEAP_GUNBLADE] && inventory[AMMO_GUNBLADE];
}

inline int BotWeaponSelector::ShellsReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_RIOTGUN>(); }
inline int BotWeaponSelector::GrenadesReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_GRENADELAUNCHER>(); }
inline int BotWeaponSelector::RocketsReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_ROCKETLAUNCHER>(); }
inline int BotWeaponSelector::PlasmasReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_PLASMAGUN>(); }
inline int BotWeaponSelector::BulletsReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_MACHINEGUN>(); }
inline int BotWeaponSelector::LasersReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_LASERGUN>(); }
inline int BotWeaponSelector::WavesReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_SHOCKWAVE>(); }
inline int BotWeaponSelector::BoltsReadyToFireCount() const { return AmmoReadyToFireCount<WEAP_ELECTROBOLT>(); }

void BotWeaponSelector::Frame( const WorldState &cachedWorldState ) {
	if( nextFastWeaponSwitchActionCheckAt > level.time ) {
		return;
	}

	if( !bot->selectedEnemies.AreValid() ) {
		return;
	}

	// cachedWorldState is cached for Think() period and might be out of sync with selectedEnemies
	if( cachedWorldState.EnemyOriginVar().Ignore() ) {
		return;
	}

	// Disallow "fast weapon switch actions" while a bot has quad.
	// The weapon balance and usage is completely different for a quad bearer.
	if( cachedWorldState.HasQuadVar() ) {
		return;
	}

	if( CheckFastWeaponSwitchAction( cachedWorldState ) ) {
		nextFastWeaponSwitchActionCheckAt = level.time + 750;
	}
}

void BotWeaponSelector::Think( const WorldState &cachedWorldState ) {
	if( bot->weaponsUsageModule.GetSelectedWeapons().AreValid() ) {
		return;
	}

	if( !bot->selectedEnemies.AreValid() ) {
		return;
	}

	// cachedWorldState is cached for Think() period and might be out of sync with selectedEnemies
	if( cachedWorldState.EnemyOriginVar().Ignore() ) {
		return;
	}

	if( weaponChoiceRandomTimeoutAt <= level.time ) {
		weaponChoiceRandom = random();
		weaponChoiceRandomTimeoutAt = level.time + 2000;
	}

	SuggestAimWeapon( cachedWorldState );
}

bool BotWeaponSelector::CheckFastWeaponSwitchAction( const WorldState &worldState ) {
	if( game.edicts[bot->EntNum()].r.client->ps.stats[STAT_WEAPON_TIME] >= 64 ) {
		return false;
	}

	// Easy bots do not perform fast weapon switch actions
	if( bot->Skill() < 0.33f ) {
		return false;
	}

	if( bot->Skill() < 0.66f ) {
		// Mid-skill bots do these actions in non-think frames occasionally
		if( bot->ShouldSkipThinkFrame() && random() > bot->Skill() ) {
			return false;
		}
	}

	int chosenWeapon = WEAP_NONE;
	if( worldState.DamageToKill() < 50 ) {
		chosenWeapon = SuggestFinishWeapon( worldState );
	} else {
		// Try to hit escaping enemies hard in a single shot
		bool botMovesFast, enemyMovesFast;
		if( IsEnemyEscaping( worldState, &botMovesFast, &enemyMovesFast ) ) {
			chosenWeapon = SuggestHitEscapingEnemyWeapon( worldState, botMovesFast, enemyMovesFast );
		}
	}

	if( chosenWeapon != WEAP_NONE ) {
		unsigned timeoutPeriod = 64 + 1;
		const auto &selectedWeapons = bot->weaponsUsageModule.GetSelectedWeapons();
		if( selectedWeapons.TimeoutAt() > level.time + 64 + 1 ) {
			timeoutPeriod += level.time - selectedWeapons.timeoutAt - 64 - 1;
		}
		SetSelectedWeapons( chosenWeapon, selectedWeapons.ScriptWeaponNum(), true, timeoutPeriod );
		return true;
	}

	return false;
}

static constexpr float CLOSE_RANGE = 150.0f;

inline float GetLaserRange() {
	const auto lgDef = GS_GetWeaponDef( WEAP_LASERGUN );
	return ( lgDef->firedef.timeout + lgDef->firedef.timeout ) / 2.0f;
}

void BotWeaponSelector::SuggestAimWeapon( const WorldState &worldState ) {
	Vec3 botOrigin( bot->Origin() );
	TestTargetEnvironment( botOrigin, bot->selectedEnemies.LastSeenOrigin(), bot->selectedEnemies.Ent() );

	if( GS_Instagib() ) {
		// TODO: Select script weapon too
		SetSelectedWeapons( SuggestInstagibWeapon( worldState ), weaponChoicePeriod );
		return;
	}

	if( BotHasQuad() ) {
		// TODO: Select script weapon too
		SetSelectedWeapons( SuggestQuadBearerWeapon( worldState ), weaponChoicePeriod );
		return;
	}

	if( worldState.EnemyIsOnSniperRange() ) {
		SuggestSniperRangeWeapon( worldState );
	} else if( worldState.EnemyIsOnFarRange() ) {
		SuggestFarRangeWeapon( worldState );
	} else if( worldState.EnemyIsOnMiddleRange() ) {
		SuggestMiddleRangeWeapon( worldState );
	} else {
		SuggestCloseRangeWeapon( worldState );
	}
}

void BotWeaponSelector::SuggestSniperRangeWeapon( const WorldState &worldState ) {
	int chosenWeapon = WEAP_NONE;

	if( bot->selectedEnemies.PendingWeapon() == WEAP_ELECTROBOLT ) {
		if( weaponChoiceRandom > 0.5f ) {
			if( PlasmasReadyToFireCount() && weaponChoiceRandom > 0.75f ) {
				chosenWeapon = WEAP_PLASMAGUN;
			} else if( BulletsReadyToFireCount() ) {
				chosenWeapon = WEAP_MACHINEGUN;
			}
		}
	} else if( !bot->ShouldAimPrecisely() ) {
		// In this case try preferring weapons that does not require precise aiming.
		// Otherwise the bot is unlikely to start firing at all due to view angles mismatch.
		if( PlasmasReadyToFireCount() && weaponChoiceRandom > 0.7f ) {
			chosenWeapon = WEAP_PLASMAGUN;
		} else if( ShellsReadyToFireCount() ) {
			chosenWeapon = WEAP_RIOTGUN;
		}
	}

	if( chosenWeapon == WEAP_NONE ) {
		// Add a hack for draining health with MG if there is no health to pick on the level
		if( worldState.DamageToKill() > 50.0f && ( level.gametype.spawnableItemsMask & IT_HEALTH ) ) {
			if( BoltsReadyToFireCount() ) {
				chosenWeapon = WEAP_ELECTROBOLT;
			} else if( BulletsReadyToFireCount() ) {
				chosenWeapon = WEAP_MACHINEGUN;
			}
		} else {
			if( BulletsReadyToFireCount() ) {
				chosenWeapon = WEAP_MACHINEGUN;
			} else if( BoltsReadyToFireCount() ) {
				chosenWeapon = WEAP_ELECTROBOLT;
			}
		}
	}

	// Still not chosen
	if( chosenWeapon == WEAP_NONE ) {
		// These weapons must have been selected otherwise
		assert( !BulletsReadyToFireCount() );
		assert( !BoltsReadyToFireCount() );

		if( ShellsReadyToFireCount() ) {
			chosenWeapon = WEAP_RIOTGUN;
		} else if( BlastsReadyToFireCount() ) {
			chosenWeapon = WEAP_GUNBLADE;
		} else if( RocketsReadyToFireCount() ) {
			chosenWeapon = WEAP_ROCKETLAUNCHER;
		} else if( WavesReadyToFireCount() ) {
			chosenWeapon = WEAP_SHOCKWAVE;
		} else {
			chosenWeapon = WEAP_GUNBLADE;
		}
	}

	Debug( "(sniper range)... : chose %s \n", GS_GetWeaponDef( chosenWeapon )->name );

	int scriptWeaponTier;
	if( const auto *scriptWeaponDef = SuggestScriptWeapon( worldState, &scriptWeaponTier ) ) {
		bool preferBuiltinWeapon = true;
		if( scriptWeaponTier > 3 ) {
			preferBuiltinWeapon = false;
		} else if( scriptWeaponTier > 1 ) {
			preferBuiltinWeapon = ( chosenWeapon == WEAP_ELECTROBOLT );
		}
		SetSelectedWeapons( chosenWeapon, scriptWeaponDef->weaponNum, preferBuiltinWeapon, weaponChoicePeriod );
	} else {
		SetSelectedWeapons( chosenWeapon, weaponChoicePeriod );
	}
}
struct WeaponAndScore {
	int weapon;
	float score;
	WeaponAndScore( int weapon = WEAP_NONE, float score = 0.0f ) {
		this->weapon = weapon;
		this->score = score;
	}
};

int BotWeaponSelector::ChooseWeaponByScores( struct WeaponAndScore *begin, struct WeaponAndScore *end ) {
	int weapon = WEAP_NONE;
	const int pendingWeapon = bot->PlayerState()->stats[STAT_PENDING_WEAPON];
	float pendingWeaponScore = std::numeric_limits<float>::infinity();
	for( WeaponAndScore *it = begin; it != end; ++it ) {
		if( pendingWeapon == it->weapon ) {
			pendingWeaponScore = it->score;
			break;
		}
	}
	float maxScore = 0.0f;
	if( pendingWeaponScore != std::numeric_limits<float>::infinity() ) {
		float weightDiffThreshold = 0.3f;
		// Do not switch too often continuous fire weapons
		if( pendingWeapon == WEAP_PLASMAGUN || pendingWeapon == WEAP_MACHINEGUN ) {
			weightDiffThreshold += 0.15f;
		} else if( pendingWeapon == WEAP_LASERGUN ) {
			weightDiffThreshold += 0.15f;
		}
		for( WeaponAndScore *it = begin; it != end; ++it ) {
			float currScore = it->score;
			if( maxScore < currScore ) {
				// Do not change weapon if its score is almost equal to current one to avoid weapon choice "jitter"
				// when a bot tries to change weapon infinitely when weapon scores are close to each other
				if( pendingWeapon == it->weapon || fabsf( currScore - pendingWeaponScore ) > weightDiffThreshold ) {
					maxScore = currScore;
					weapon = it->weapon;
				}
			}
		}
	} else {
		for( WeaponAndScore *it = begin; it != end; ++it ) {
			float currScore = it->score;
			if( maxScore < currScore ) {
				maxScore = currScore;
				weapon = it->weapon;
			}
		}
	}

	return weapon;
}

void BotWeaponSelector::SuggestFarRangeWeapon( const WorldState &worldState ) {
	// First, try to choose a long range weapon of EB, MG, PG and RG
	enum { EB, MG, PG, RG, WEIGHTS_COUNT };
	WeaponAndScore weaponScores[4] =
	{
		WeaponAndScore( WEAP_ELECTROBOLT, 1.0f * BoundedFraction( BoltsReadyToFireCount(), 2.0f ) ),
		WeaponAndScore( WEAP_MACHINEGUN, 1.0f * BoundedFraction( BulletsReadyToFireCount(), 10.0f ) ),
		WeaponAndScore( WEAP_PLASMAGUN, 0.8f * BoundedFraction( PlasmasReadyToFireCount(), 15.0f ) ),
		WeaponAndScore( WEAP_RIOTGUN, 0.6f * BoundedFraction( ShellsReadyToFireCount(), 2 ) )
	};

	weaponScores[EB].score += bot->Skill() / 3;

	const auto &selectedEnemies = bot->selectedEnemies;

	// Counteract EB with PG or MG
	if( selectedEnemies.PendingWeapon() == WEAP_ELECTROBOLT ) {
		weaponScores[PG].score *= 1.3f;
		weaponScores[MG].score *= 1.2f;
	}
	// Counteract PG with MG or EB
	if( selectedEnemies.PendingWeapon() == WEAP_PLASMAGUN ) {
		weaponScores[EB].score *= 1.4f;
		weaponScores[MG].score *= 1.2f;
	}
	// Counteract MG with PG or EB
	if( selectedEnemies.PendingWeapon() == WEAP_MACHINEGUN ) {
		weaponScores[PG].score *= 1.2f;
		weaponScores[EB].score *= 1.3f;
	}

	// Do not use plasma on fast-moving side-to-side enemies
	Vec3 targetMoveDir( selectedEnemies.LastSeenVelocity() );
	float enemySpeed = targetMoveDir.SquaredLength();
	if( enemySpeed > 0.1f ) {
		enemySpeed = SQRTFAST( enemySpeed );
	}
	if( enemySpeed > DEFAULT_DASHSPEED ) {
		targetMoveDir *= 1.0f / enemySpeed;
		Vec3 botToTargetDir = selectedEnemies.LastSeenOrigin() - bot->Origin();
		botToTargetDir.NormalizeFast();

		float speedFactor = BoundedFraction( enemySpeed - DEFAULT_DASHSPEED, 1000.0f - DEFAULT_DASHSPEED );
		float dirFactor = fabsf( botToTargetDir.Dot( targetMoveDir ) );
		// If enemy moves fast but on botToTargetDir line, pg score is unaffected
		weaponScores[PG].score *= 1.0f - ( 1.0f - dirFactor ) * speedFactor;
	}

	if( bot->IsInSquad() ) {
		// In squad prefer MG to gun down an enemy together
		weaponScores[MG].score *= 1.75f;
	}

	// Add extra scores for weapons that do not require precise aiming in this case
	if( !bot->ShouldAimPrecisely() ) {
		weaponScores[PG].score *= 3.0f;
		weaponScores[RG].score *= 2.0f;
	}

	// Add a hack for draining health with MG if there is no health to pick on the level
	if( !( level.gametype.spawnableItemsMask & IT_HEALTH ) ) {
		weaponScores[MG].score *= 2.25f;
		weaponScores[RG].score *= 1.25f;
	}

	int chosenWeapon = ChooseWeaponByScores( weaponScores, weaponScores + WEIGHTS_COUNT );

	if( chosenWeapon == WEAP_NONE ) {
		if( targetEnvironment.factor > 0.5f ) {
			if( WavesReadyToFireCount() && RocketsReadyToFireCount() ) {
				if( bot->WillRetreat() ) {
					chosenWeapon = WEAP_SHOCKWAVE;
				} else {
					chosenWeapon = WEAP_ROCKETLAUNCHER;
				}
			} else {
				if( WavesReadyToFireCount() ) {
					chosenWeapon = WEAP_SHOCKWAVE;
				} else if( RocketsReadyToFireCount() ) {
					chosenWeapon = WEAP_ROCKETLAUNCHER;
				} else if( BlastsReadyToFireCount() ) {
					chosenWeapon = WEAP_GUNBLADE;
				}
			}
		} else {
			if( bot->WillRetreat() && WavesReadyToFireCount() ) {
				chosenWeapon = WEAP_SHOCKWAVE;
			}
		}
	}

	// Still not chosen
	if( chosenWeapon == WEAP_NONE ) {
		// These weapons must have been selected otherwise
		assert( !BoltsReadyToFireCount() );
		assert( !BulletsReadyToFireCount() );
		assert( !PlasmasReadyToFireCount() );
		assert( !ShellsReadyToFireCount() );

		if( RocketsReadyToFireCount() ) {
			chosenWeapon = WEAP_ROCKETLAUNCHER;
		} else if( BlastsReadyToFireCount() ) {
			chosenWeapon = WEAP_GUNBLADE;
		} else if( WavesReadyToFireCount() ) {
			chosenWeapon = WEAP_SHOCKWAVE;
		} else if( GrenadesReadyToFireCount() ) {
			chosenWeapon = WEAP_GRENADELAUNCHER;
		} else {
			chosenWeapon = WEAP_GUNBLADE;
		}
	}

	Debug( "(far range)... chose %s\n", GS_GetWeaponDef( chosenWeapon )->name );

	int scriptWeaponTier;
	if( const auto *scriptWeaponDef = SuggestScriptWeapon( worldState, &scriptWeaponTier ) ) {
		bool preferBuiltinWeapon = true;
		if( scriptWeaponTier > 3 ) {
			preferBuiltinWeapon = false;
		} else if( scriptWeaponTier > 1 ) {
			preferBuiltinWeapon = ( chosenWeapon == WEAP_ELECTROBOLT );
		}
		SetSelectedWeapons( chosenWeapon, scriptWeaponDef->weaponNum, preferBuiltinWeapon, weaponChoicePeriod );
	} else {
		SetSelectedWeapons( chosenWeapon, weaponChoicePeriod );
	}
}

void BotWeaponSelector::SuggestMiddleRangeWeapon( const WorldState &worldState ) {
	const float lgRange = GetLaserRange();
	// Should be equal to max mid range distance - min mid range distance
	const float midRangeLen = lgRange - CLOSE_RANGE;
	// Relative distance from min mid range distance
	const float midRangeDistance = worldState.DistanceToEnemy() - CLOSE_RANGE;

	const auto &selectedEnemies = bot->selectedEnemies;

	int chosenWeapon = WEAP_NONE;

	enum { RL, LG, PG, SW, MG, RG, GL, GB, WEIGHTS_COUNT };
	WeaponAndScore weaponScores[8];
	weaponScores[RL].weapon = WEAP_ROCKETLAUNCHER;
	weaponScores[LG].weapon = WEAP_LASERGUN;
	weaponScores[PG].weapon = WEAP_PLASMAGUN;
	weaponScores[SW].weapon = WEAP_SHOCKWAVE;
	weaponScores[MG].weapon = WEAP_MACHINEGUN;
	weaponScores[RG].weapon = WEAP_RIOTGUN;
	weaponScores[GL].weapon = WEAP_GRENADELAUNCHER;
	weaponScores[GB].weapon = WEAP_GUNBLADE;

	weaponScores[RL].score = 1.5f * BoundedFraction( RocketsReadyToFireCount(), 3.0f );
	weaponScores[LG].score = 1.5f * BoundedFraction( LasersReadyToFireCount(), 15.0f );
	weaponScores[PG].score = 0.7f * BoundedFraction( PlasmasReadyToFireCount(), 15.0f );
	weaponScores[SW].score = 2.0f * BoundedFraction( WavesReadyToFireCount(), 2.0f );
	weaponScores[MG].score = 1.0f * BoundedFraction( BulletsReadyToFireCount(), 15.0f );
	weaponScores[RG].score = 0.7f * BoundedFraction( ShellsReadyToFireCount(), 3.0f );
	weaponScores[GL].score = 0.5f * BoundedFraction( GrenadesReadyToFireCount(), 5.0f );
	weaponScores[GB].score = 0.3f * BlastsReadyToFireCount();

	if( bot->IsInSquad() ) {
		// In squad prefer continuous fire weapons to burn an enemy quick together
		float boost = 1.5f;
		weaponScores[LG].score *= boost;
		weaponScores[PG].score *= boost;
		weaponScores[MG].score *= boost;
	}

	if( bot->WillAdvance() ) {
		weaponScores[RL].score *= 1.3f;
		weaponScores[GL].score *= 0.7f;
		weaponScores[RG].score *= 1.1f;
	}
	if( bot->WillRetreat() ) {
		// Retreating is the only situation where bots use SW properly
		weaponScores[SW].score *= 2.0f;
		// Plasma is a great defensive weapon
		weaponScores[PG].score *= 1.5f;
		weaponScores[LG].score *= 1.1f;
		weaponScores[MG].score *= 1.1f;
		weaponScores[GL].score *= 1.1f;
	}

	// 1 on mid range bound, 0 on close range bound
	float distanceFactor = ( worldState.DistanceToEnemy() - CLOSE_RANGE ) / ( lgRange - CLOSE_RANGE );

	weaponScores[RL].score *= 1.0f - distanceFactor;
	weaponScores[LG].score *= 0.7f + 0.3f * distanceFactor;
	weaponScores[PG].score *= 1.0f - 0.4f * distanceFactor;
	weaponScores[SW].score *= 1.0f - distanceFactor;
	weaponScores[MG].score *= 0.3f + 0.7f * distanceFactor;
	weaponScores[RG].score *= 1.0f - 0.7f * distanceFactor;
	// GL score is maximal in the middle on mid-range zone and is zero on the zone bounds
	weaponScores[GL].score *= 1.0f - fabsf( midRangeDistance - midRangeLen / 2.0f ) / midRangeDistance;
	// Bots are pretty good in prediction of GB shots
	weaponScores[GB].score *= 1.0f - 0.3f * distanceFactor;

	weaponScores[RL].score *= targetEnvironment.factor;
	weaponScores[LG].score *= 1.0f - 0.4f * targetEnvironment.factor;
	weaponScores[PG].score *= 0.5f + 0.5f * targetEnvironment.factor;
	// Do not modify SW score by environment factor.
	// Environment is two-sided, it could be even more useful in rather open one.
	weaponScores[MG].score *= 1.0f - 0.4f * targetEnvironment.factor;
	weaponScores[RG].score *= 1.0f - 0.5f * targetEnvironment.factor;
	weaponScores[GL].score *= targetEnvironment.factor;
	weaponScores[GB].score *= 0.5f + 0.5f * targetEnvironment.factor;

	// Add extra scores for weapons that do not require precise aiming in this case
	if( !bot->ShouldAimPrecisely() ) {
		weaponScores[PG].score *= 1.5f;
		weaponScores[RG].score *= 2.0f;
		weaponScores[GL].score *= 1.5f;
		weaponScores[SW].score *= 1.5f;
		if( bot->WillRetreat() ) {
			weaponScores[PG].score *= 1.5f;
			weaponScores[GL].score *= 1.5f;
		}
	}

	// Ignore the nested checks if the bot is retreating / an enemy is in a tight environment
	if( !bot->WillRetreat() && targetEnvironment.factor < 0.5f ) {
		// Do not use SW on escaping enemies, and prefer it on chasing enemies
		Vec3 targetMoveDir( selectedEnemies.LastSeenVelocity() );
		float enemySpeed = targetMoveDir.SquaredLength();
		if( enemySpeed > ( DEFAULT_DASHSPEED ) * ( DEFAULT_DASHSPEED ) ) {
			enemySpeed = SQRTFAST( enemySpeed );
			targetMoveDir *= 1.0f / enemySpeed;
			Vec3 botToTargetDir = selectedEnemies.LastSeenOrigin() - bot->Origin();
			botToTargetDir.NormalizeFast();

			float speedFactor = BoundedFraction( enemySpeed - DEFAULT_DASHSPEED, 1000.0f - DEFAULT_DASHSPEED );
			// -1.0 if the enemy is escaping straight away from the bot
			//  0.5 if the enemy moves side-to-side
			// +1.0 if the enemy is chasing the bot in a straight line
			float dirFactor = botToTargetDir.Dot( targetMoveDir );
			// 0.1 for worst case (the enemy is escaping on high speed)
			// 1.9 for best case (the enemy is chasing on high speed)
			float scoreScale = 1.0f + 0.9f * speedFactor * dirFactor;
			weaponScores[SW].score *= scoreScale;
		}
	}

	// Prefer high-dps weapons for unaware enemies
	if( !worldState.HasThreateningEnemyVar() ) {
		float speedThreshold = DEFAULT_DASHSPEED + 50.0f;
		if( selectedEnemies.LastSeenVelocity().SquaredLength() < speedThreshold * speedThreshold ) {
			weaponScores[PG].score *= 1.5f;
			weaponScores[SW].score *= 1.5f;
		}
	}

	chosenWeapon = ChooseWeaponByScores( weaponScores, weaponScores + WEIGHTS_COUNT );
	if( chosenWeapon == WEAP_NONE ) {
		if( BoltsReadyToFireCount() ) {
			chosenWeapon = WEAP_ELECTROBOLT;
		} else {
			chosenWeapon = WEAP_GUNBLADE;
		}
	}

	int scriptWeaponTier = 0;
	if( const auto *scriptWeaponDef = SuggestScriptWeapon( worldState, &scriptWeaponTier ) ) {
		bool preferBuiltinWeapon = scriptWeaponTier < BuiltinWeaponTier( chosenWeapon );
		SetSelectedWeapons( chosenWeapon, scriptWeaponDef->weaponNum, preferBuiltinWeapon, weaponChoicePeriod );
	} else {
		SetSelectedWeapons( chosenWeapon, weaponChoicePeriod );
	}
}

void BotWeaponSelector::SuggestCloseRangeWeapon( const WorldState &worldState ) {
	int chosenWeapon = WEAP_NONE;

	int lasersCount = LasersReadyToFireCount();
	int rocketsCount = RocketsReadyToFireCount();
	int plasmasCount = PlasmasReadyToFireCount();
	int wavesCount = WavesReadyToFireCount();

	float distanceFactor = BoundedFraction( worldState.DistanceToEnemy(), CLOSE_RANGE );

	if( g_allow_selfdamage->integer ) {
		if( ShellsReadyToFireCount() && ( worldState.DamageToKill() < 90 || weaponChoiceRandom < 0.4 ) ) {
			chosenWeapon = WEAP_RIOTGUN;
		} else if( rocketsCount > 0 && worldState.DamageToBeKilled() > 100.0f - 75.0f * distanceFactor ) {
			chosenWeapon = WEAP_ROCKETLAUNCHER;
		} else if( wavesCount > 0 && worldState.DamageToBeKilled() > 80.0f - 40.0f * distanceFactor ) {
			chosenWeapon = WEAP_SHOCKWAVE;
		} else if( plasmasCount > 10 && worldState.DamageToBeKilled() > 75.0f - 50.0f * distanceFactor ) {
			chosenWeapon = WEAP_PLASMAGUN;
		} else if( lasersCount > 10 ) {
			chosenWeapon = WEAP_LASERGUN;
		} else if( BlastsReadyToFireCount() ) {
			chosenWeapon = WEAP_GUNBLADE;
		}
	} else {
		if( rocketsCount ) {
			chosenWeapon = WEAP_ROCKETLAUNCHER;
		} else if( wavesCount ) {
			chosenWeapon = WEAP_SHOCKWAVE;
		} else if( lasersCount > 10 ) {
			chosenWeapon = WEAP_LASERGUN;
		} else if( plasmasCount ) {
			chosenWeapon = WEAP_PLASMAGUN;
		} else if( BlastsReadyToFireCount() ) {
			chosenWeapon = WEAP_GUNBLADE;
		}
	}
	// Still not chosen
	if( chosenWeapon == WEAP_NONE ) {
		int shellsCount = ShellsReadyToFireCount();
		if( shellsCount > 0 ) {
			chosenWeapon = WEAP_RIOTGUN;
		} else if( plasmasCount > 0 ) {
			chosenWeapon = WEAP_PLASMAGUN;
		} else if( lasersCount > 0 ) {
			chosenWeapon = WEAP_LASERGUN;
		} else if( BulletsReadyToFireCount() ) {
			chosenWeapon = WEAP_MACHINEGUN;
		} else if( BoltsReadyToFireCount() ) {
			chosenWeapon = WEAP_ELECTROBOLT;
		} else {
			chosenWeapon = WEAP_GUNBLADE;
		}
	}

	Debug( "(close range) : chose %s \n", GS_GetWeaponDef( chosenWeapon )->name );

	int scriptWeaponTier;
	if( const auto *scriptWeaponDef = SuggestScriptWeapon( worldState, &scriptWeaponTier ) ) {
		bool preferBuiltinWeapon = scriptWeaponTier < BuiltinWeaponTier( chosenWeapon );
		SetSelectedWeapons( chosenWeapon, scriptWeaponDef->weaponNum, preferBuiltinWeapon, weaponChoicePeriod );
	} else {
		SetSelectedWeapons( chosenWeapon, weaponChoicePeriod );
	}
}

const AiScriptWeaponDef *BotWeaponSelector::SuggestScriptWeapon( const WorldState &worldState, int *effectiveTier ) {
	const auto &scriptWeaponDefs = bot->weaponsUsageModule.scriptWeaponDefs;
	const auto &scriptWeaponCooldown = bot->weaponsUsageModule.scriptWeaponCooldown;

	const AiScriptWeaponDef *bestWeapon = nullptr;
	float bestScore = 0.000001f;

	const float distanceToEnemy = worldState.DistanceToEnemy();

	for( unsigned i = 0; i < scriptWeaponDefs.size(); ++i ) {
		const auto &weaponDef = scriptWeaponDefs[i];
		int cooldown = scriptWeaponCooldown[i];
		if( cooldown >= 1000 ) {
			continue;
		}

		if( distanceToEnemy > weaponDef.maxRange ) {
			continue;
		}
		if( distanceToEnemy < weaponDef.minRange ) {
			continue;
		}

		float score = 1.0f;

		score *= 1.0f - BoundedFraction( cooldown, 1000.0f );

		if( GS_SelfDamage() ) {
			float estimatedSelfDamage = 0.0f;
			estimatedSelfDamage = weaponDef.maxSelfDamage;
			estimatedSelfDamage *= ( 1.0f - BoundedFraction( worldState.DistanceToEnemy(), weaponDef.splashRadius ) );
			if( estimatedSelfDamage > 100.0f ) {
				continue;
			}
			if( worldState.DistanceToEnemy() < estimatedSelfDamage ) {
				continue;
			}
			score *= 1.0f - BoundedFraction( estimatedSelfDamage, 100.0f );
		}

		// We assume that maximum ordinary tier is 3
		score *= weaponDef.tier / 3.0f;

		// Treat points in +/- 192 units of best range as in best range too
		float bestRangeLowerBounds = weaponDef.bestRange - std::min( 192.0f, weaponDef.bestRange - weaponDef.minRange );
		float bestRangeUpperBounds = weaponDef.bestRange + std::min( 192.0f, weaponDef.maxRange - weaponDef.bestRange );

		if( distanceToEnemy < bestRangeLowerBounds ) {
			score *= distanceToEnemy / bestRangeLowerBounds;
		} else if( distanceToEnemy > bestRangeUpperBounds ) {
			score *= ( distanceToEnemy - bestRangeUpperBounds ) / ( weaponDef.maxRange - bestRangeLowerBounds );
		}

		if( score > bestScore ) {
			bestScore = score;
			bestWeapon = &scriptWeaponDefs[i];
			*effectiveTier = (int)( score * 3.0f + 0.99f );
		}
	}

	return bestWeapon;
}

int BotWeaponSelector::SuggestFinishWeapon( const WorldState &worldState ) {
	const float distance = worldState.DistanceToEnemy();
	const float damageToBeKilled = worldState.DamageToBeKilled();
	const float damageToKill = worldState.DamageToKill();

	if( worldState.DistanceToEnemy() < CLOSE_RANGE ) {
		if( g_allow_selfdamage->integer && damageToBeKilled < 75 ) {
			if( LasersReadyToFireCount() > damageToKill * 0.3f * 14 ) {
				return WEAP_LASERGUN;
			}
			if( ShellsReadyToFireCount() > 0 ) {
				return WEAP_RIOTGUN;
			}
			if( BulletsReadyToFireCount() > damageToKill * 0.3f * 10 ) {
				return WEAP_MACHINEGUN;
			}
			if( PlasmasReadyToFireCount() > damageToKill * 0.3f * 14 ) {
				return WEAP_PLASMAGUN;
			}
			if( BlastsReadyToFireCount() ) {
				return WEAP_GUNBLADE;
			}
			// Hard bots do not do this high risk action
			if( bot->Skill() < 0.66f ) {
				if( RocketsReadyToFireCount() ) {
					return WEAP_ROCKETLAUNCHER;
				}
			}
		} else {
			if( targetEnvironment.factor > 0 ) {
				if( RocketsReadyToFireCount() ) {
					return WEAP_ROCKETLAUNCHER;
				}
			}
			if( ShellsReadyToFireCount() ) {
				return WEAP_RIOTGUN;
			}
			if( PlasmasReadyToFireCount() > worldState.DamageToKill() * 0.3f * 14 ) {
				return WEAP_PLASMAGUN;
			}
			if( LasersReadyToFireCount() > worldState.DamageToKill() * 0.3f * 14 ) {
				return WEAP_LASERGUN;
			}
			if( BulletsReadyToFireCount() > worldState.DamageToKill() * 0.3f * 10 ) {
				return WEAP_MACHINEGUN;
			}
		}
		return WEAP_NONE;
	}

	const float lgRange = GetLaserRange();
	if( distance < lgRange ) {
		if( distance < 0.5f * lgRange ) {
			if( targetEnvironment.factor > 0.6f && RocketsReadyToFireCount() ) {
				return WEAP_ROCKETLAUNCHER;
			}
			if( distance < 0.25f * lgRange && BlastsReadyToFireCount() ) {
				return WEAP_GUNBLADE;
			}
		}
		if( LasersReadyToFireCount() > damageToKill * 0.3f * 14 ) {
			return WEAP_LASERGUN;
		}
		if( ShellsReadyToFireCount() ) {
			return WEAP_RIOTGUN;
		}
		if( BulletsReadyToFireCount() > damageToKill * 0.3f * 10 ) {
			return WEAP_MACHINEGUN;
		}
		if( BlastsReadyToFireCount() ) {
			return WEAP_GUNBLADE;
		}
		if( PlasmasReadyToFireCount() > damageToKill * 0.3f * 14 ) {
			return WEAP_PLASMAGUN;
		}

		return WEAP_NONE;
	}

	if( damageToKill > 40 && BoltsReadyToFireCount() ) {
		return WEAP_ELECTROBOLT;
	}

	if( BulletsReadyToFireCount() > damageToKill * 0.3f * 10 ) {
		return WEAP_MACHINEGUN;
	}

	if( BlastsReadyToFireCount() ) {
		return WEAP_GUNBLADE;
	}

	return WEAP_NONE;
}

static bool IsEscapingFromStandingEntity( const edict_t *escaping, const edict_t *standing, float escapingVelocitySqLen ) {
	// Too low relative speed with almost standing enemy
	float speedThreshold = DEFAULT_PLAYERSPEED * 1.35f;
	if( escapingVelocitySqLen < speedThreshold * speedThreshold ) {
		return false;
	}

	Vec3 escapingVelocityDir( escaping->velocity );
	escapingVelocityDir *= Q_RSqrt( escapingVelocitySqLen );

	Vec3 escapingToStandingDir( standing->s.origin );
	escapingToStandingDir -= escaping->s.origin;

	float len = escapingToStandingDir.SquaredLength();
	if( len < 1 ) {
		return false;
	}

	escapingToStandingDir *= Q_RSqrt( len );
	return escapingToStandingDir.Dot( escapingVelocityDir ) < -0.5f;
}

bool BotWeaponSelector::IsEnemyEscaping( const WorldState &worldState, bool *botMovesFast, bool *enemyMovesFast ) {
	// Very basic. Todo: Check env. behind an enemy or the bot, is it really tries to escape or just pushed on a wall

	const edict_t *self = game.edicts + bot->EntNum();
	const auto &selectedEnemies = bot->selectedEnemies;

	float botVelocitySqLen = VectorLengthSquared( self->velocity );
	float enemyVelocitySqLen = selectedEnemies.LastSeenVelocity().SquaredLength();

	// Enemy is moving fast
	if( enemyVelocitySqLen >= DEFAULT_DASHSPEED * DEFAULT_DASHSPEED ) {
		// Both entities are moving fast
		if( botVelocitySqLen >= DEFAULT_DASHSPEED * DEFAULT_DASHSPEED ) {
			Vec3 botVelocityDir( self->velocity );
			Vec3 enemyVelocityDir( selectedEnemies.LastSeenVelocity() );
			enemyVelocityDir *= Q_RSqrt( enemyVelocitySqLen );
			botVelocityDir *= Q_RSqrt( botVelocitySqLen );
			if( botVelocityDir.Dot( enemyVelocityDir ) < -0.5f ) {
				*botMovesFast = true;
				*enemyMovesFast = true;
				return true;
			}
			return false;
		}
		// Bot is standing or walking, direction of its speed does not matter
		if( IsEscapingFromStandingEntity( selectedEnemies.Ent(), self, enemyVelocitySqLen ) ) {
			*botMovesFast = false;
			*enemyMovesFast = true;
			return true;
		}
		return false;
	}

	// Enemy is standing or walking, direction of its speed does not matter
	if( IsEscapingFromStandingEntity( self, selectedEnemies.Ent(), botVelocitySqLen ) ) {
		*botMovesFast = true;
		*enemyMovesFast = false;
		return true;
	}
	return false;
}

int BotWeaponSelector::SuggestHitEscapingEnemyWeapon( const WorldState &worldState, bool botMovesFast, bool enemyMovesFast ) {
	if( worldState.DistanceToEnemy() < CLOSE_RANGE ) {
		constexpr const char *format = "(hit escaping) too small distance %.1f to change weapon, too risky\n";
		Debug( format, worldState.DistanceToEnemy() );
		return WEAP_NONE;
	}

	// If target will be lost out of sight, its worth to do a fast weapon switching

	constexpr float predictionSeconds = 0.8f;

	edict_t *self = game.edicts + bot->EntNum();
	const auto &selectedEnemies = bot->selectedEnemies;

	// Extrapolate bot origin
	Vec3 extrapolatedBotOrigin( self->velocity );
	extrapolatedBotOrigin *= predictionSeconds;
	extrapolatedBotOrigin += self->s.origin;
	Vec3 predictedBotOrigin( extrapolatedBotOrigin );

	// Extrapolate enemy origin
	Vec3 initialEnemyOrigin( selectedEnemies.LastSeenOrigin() );
	Vec3 extrapolatedEnemyOrigin( selectedEnemies.LastSeenVelocity() );
	extrapolatedEnemyOrigin *= predictionSeconds;
	extrapolatedEnemyOrigin += initialEnemyOrigin;
	Vec3 predictedEnemyOrigin( extrapolatedEnemyOrigin );

	const float *const mins = playerbox_stand_mins;
	const float *const maxs = playerbox_stand_maxs;

	trace_t trace;
	// Get a coarse predicted bot origin
	G_Trace( &trace, self->s.origin, mins, maxs, extrapolatedBotOrigin.Data(), self, MASK_AISOLID );
	if( trace.fraction != 1.0f ) {
		predictedBotOrigin.Set( trace.endpos );
		// Compensate Z for ground trace hit point
		if( trace.endpos[2] > extrapolatedBotOrigin.Z() ) {
			predictedBotOrigin.Z() += -playerbox_stand_mins[2];
		}
	}

	// Get a coarse predicted enemy origin
	auto *skip = const_cast<edict_t *>( selectedEnemies.Ent() );
	G_Trace( &trace, initialEnemyOrigin.Data(), mins, maxs, extrapolatedEnemyOrigin.Data(), skip, MASK_AISOLID );
	if( trace.fraction != 1.0f ) {
		predictedEnemyOrigin.Set( trace.endpos );
		if( trace.endpos[2] > extrapolatedEnemyOrigin.Z() ) {
			predictedEnemyOrigin.Z() += -playerbox_stand_mins[2];
		}
	}

	// Check whether the bot is likely to be able to hit the enemy
	if( trap_inPVS( predictedBotOrigin.Data(), predictedBotOrigin.Data() ) ) {
		G_Trace( &trace, predictedBotOrigin.Data(), nullptr, nullptr, predictedEnemyOrigin.Data(), self, MASK_AISOLID );
		// Still may hit, keep using current weapon
		if( trace.fraction == 1.0f || selectedEnemies.Ent() == game.edicts + trace.ent ) {
			return WEAP_NONE;
		}
	}

	const float lgRange = GetLaserRange();
	const float distance = worldState.DistanceToEnemy();
	// Only switch to EB on far range. Keep using the current weapon.
	if( distance > lgRange ) {
		if( distance > 1.5f * lgRange ) {
			if( BoltsReadyToFireCount() ) {
				return WEAP_ELECTROBOLT;
			}
		} else if( BlastsReadyToFireCount() ) {
			return WEAP_GUNBLADE;
		}
		return WEAP_NONE;
	}

	if( botMovesFast && !enemyMovesFast ) {
		TestTargetEnvironment( Vec3( self->s.origin ), selectedEnemies.LastSeenOrigin(), selectedEnemies.Ent() );
		if( targetEnvironment.factor > 0.5f ) {
			if( RocketsReadyToFireCount() ) {
				return WEAP_ROCKETLAUNCHER;
			}
		}
	}

	if( BlastsReadyToFireCount() && worldState.DamageToKill() < 75 ) {
		return WEAP_GUNBLADE;
	}

	return WEAP_NONE;
}

int BotWeaponSelector::SuggestQuadBearerWeapon( const WorldState &worldState ) {
	float distance = ( bot->selectedEnemies.LastSeenOrigin() - bot->Origin() ).LengthFast();
	auto lgDef = GS_GetWeaponDef( WEAP_LASERGUN );
	auto lgRange = ( lgDef->firedef.timeout + lgDef->firedef_weak.timeout ) / 2.0f;
	int lasersCount = 0;
	if( Inventory()[WEAP_LASERGUN] && distance < lgRange ) {
		lasersCount = Inventory()[AMMO_LASERS] + Inventory()[AMMO_WEAK_LASERS];
		if( lasersCount > 7 ) {
			return WEAP_LASERGUN;
		}
	}
	int bulletsCount = BulletsReadyToFireCount();
	if( bulletsCount > 10 ) {
		return WEAP_MACHINEGUN;
	}
	int plasmasCount = PlasmasReadyToFireCount();
	if( plasmasCount > 10 ) {
		return WEAP_PLASMAGUN;
	}
	if( ShellsReadyToFireCount() ) {
		return WEAP_RIOTGUN;
	}
	if( Inventory()[WEAP_SHOCKWAVE] ) {
		if( WavesReadyToFireCount() > 0 && distance > 128.0f && distance < lgRange * 1.25f ) {
			return WEAP_SHOCKWAVE;
		}
	}
	if( Inventory()[WEAP_ROCKETLAUNCHER] ) {
		if( RocketsReadyToFireCount() > 0 && distance > CLOSE_RANGE && distance < lgRange * 1.25f ) {
			return WEAP_ROCKETLAUNCHER;
		}
	}
	if( lasersCount > 0 && distance < lgRange ) {
		return WEAP_LASERGUN;
	}
	if( bulletsCount > 0 ) {
		return WEAP_MACHINEGUN;
	}
	if( plasmasCount > 0 ) {
		return WEAP_PLASMAGUN;
	}
	if( Inventory()[WEAP_GRENADELAUNCHER] ) {
		if( GrenadesReadyToFireCount() > 0 && distance > CLOSE_RANGE && distance < lgRange ) {
			float deltaZ = bot->Origin()[2] - bot->selectedEnemies.LastSeenOrigin().Z();
			if( deltaZ < -250.0f && random() > 0.5f ) {
				return WEAP_GRENADELAUNCHER;
			}
		}
	}
	return WEAP_GUNBLADE;
}

int BotWeaponSelector::SuggestInstagibWeapon( const WorldState &worldState ) {
	// Prefer hitscan weapons
	if( BulletsReadyToFireCount() ) {
		return WEAP_MACHINEGUN;
	}
	if( Inventory()[WEAP_LASERGUN] ) {
		auto lgDef = GS_GetWeaponDef( WEAP_LASERGUN );
		float squaredDistance = ( bot->selectedEnemies.LastSeenOrigin() - bot->Origin() ).SquaredLength();
		if( Inventory()[AMMO_LASERS] && squaredDistance < lgDef->firedef.timeout * lgDef->firedef.timeout ) {
			return WEAP_LASERGUN;
		}
		if( Inventory()[AMMO_WEAK_LASERS] && squaredDistance < lgDef->firedef_weak.timeout * lgDef->firedef_weak.timeout ) {
			return WEAP_LASERGUN;
		}
	}
	if( ShellsReadyToFireCount() ) {
		return WEAP_RIOTGUN;
	}

	if( Inventory()[WEAP_PLASMAGUN] ) {
		float squaredDistance = ( bot->selectedEnemies.LastSeenOrigin() - bot->Origin() ).SquaredLength();
		if( squaredDistance < 1000 && PlasmasReadyToFireCount() ) {
			return WEAP_PLASMAGUN;
		}
	}
	if( Inventory()[WEAP_INSTAGUN] ) {
		return WEAP_INSTAGUN;
	}
	if( BoltsReadyToFireCount() ) {
		return WEAP_ELECTROBOLT;
	}
	if( WavesReadyToFireCount() ) {
		return WEAP_SHOCKWAVE;
	}
	return WEAP_GUNBLADE;
}

const float BotWeaponSelector::TargetEnvironment::TRACE_DEPTH = 250.0f;

void BotWeaponSelector::TestTargetEnvironment( const Vec3 &botOrigin, const Vec3 &targetOrigin, const edict_t *traceKey ) {
	Vec3 forward = targetOrigin - botOrigin;
	forward.Z() = 0;
	float frontSquareLen = forward.SquaredLength();
	if( frontSquareLen > 1 ) {
		// Normalize
		forward *= Q_RSqrt( frontSquareLen );
	} else {
		// Pick dummy horizontal direction
		forward = Vec3( &axis_identity[AXIS_FORWARD] );
	}
	Vec3 right = Vec3( &axis_identity[AXIS_UP] ).Cross( forward );

	// Now botToTarget is a normalized horizontal part of botToTarget - botOrigin

	edict_t *passedict = const_cast<edict_t*>( traceKey );
	float *start = const_cast<float*>( botOrigin.Data() );
	trace_t *traces = targetEnvironment.sideTraces;

	const float TRACE_DEPTH = TargetEnvironment::TRACE_DEPTH;

	vec_t offsets[6 * 3];
	VectorSet( offsets + 3 * TargetEnvironment::TOP, 0, 0, +TRACE_DEPTH );
	VectorSet( offsets + 3 * TargetEnvironment::BOTTOM, 0, 0, -TRACE_DEPTH );
	VectorSet( offsets + 3 * TargetEnvironment::FRONT, -TRACE_DEPTH, 0, 0 );
	VectorSet( offsets + 3 * TargetEnvironment::BACK, +TRACE_DEPTH, 0, 0 );
	VectorSet( offsets + 3 * TargetEnvironment::LEFT, 0, -TRACE_DEPTH, 0 );
	VectorSet( offsets + 3 * TargetEnvironment::RIGHT, 0, +TRACE_DEPTH, 0 );

	vec3_t mins = { -32, -32, -32 };
	vec3_t maxs = { +32, +32, +32 };

	float factor = 0.0f;
	for( int i = 0; i < 6; ++i ) {
		vec3_t end;
		trace_t *trace = traces + i;
		G_ProjectSource( start, offsets + 3 * i, forward.Data(), right.Data(), end );
		G_Trace( trace, start, mins, maxs, end, passedict, MASK_AISOLID );
		// Give some non-zero score by the fact that trace is detected a hit itself
		if( trace->fraction < 1.0f ) {
			factor += 1.0f / 6.0f;
		}
		// Compute a dot product between a bot-to-target direction and trace-point-to-target direction
		// If the dot product is close to 1, a bot may shoot almost perpendicular to the traced point.
		// If trace point is itself close to a target, bot may inflict enemy lots of damage by rockets.
		Vec3 hitPointToTarget = targetOrigin - trace->endpos;
		if( trace->fraction > 0.01f ) {
			// Normalize
			hitPointToTarget *= 1.0f / ( TRACE_DEPTH * trace->fraction );
			factor += 1.5f * ( 1.0f - trace->fraction ) * fabsf( hitPointToTarget.Dot( forward ) );
		} else {
			// The target is very close to a traced solid surface anyway, use 1 instead of a dot product.
			factor += 1.5f * ( 1.0f - trace->fraction );
		}
	}

	targetEnvironment.factor = std::min( 1.0f, factor / 6.0f );
}
