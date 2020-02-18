#ifndef QFUSION_HAZARDSSELECTOR_H
#define QFUSION_HAZARDSSELECTOR_H

#include "../planning/Planner.h"
#include "HazardsDetector.h"

struct Hazard : public PoolItem {
	static constexpr unsigned TIMEOUT = 750;

	explicit Hazard( PoolBase *pool_ ) : PoolItem( pool_ ) {}

	// Sorting by this operator is fast but should be used only
	// to prepare most dangerous entities of the same type.
	// Ai decisions should be made by more sophisticated code.
	bool operator<( const Hazard &that ) const { return this->damage < that.damage; }

	bool IsValid() const { return timeoutAt > level.time; }

	Vec3 hitPoint { 0, 0, 0 };
	Vec3 direction { 0, 0, 0 };
	float damage { 0.0f };
	float splashRadius { 0.0f };
	int64_t timeoutAt { 0 };
	const edict_t *attacker { nullptr };

	bool IsSplashLike() const { return splashRadius > 0; };

	bool SupportsImpactTests() const { return IsSplashLike(); }

	bool HasImpactOnPoint( const Vec3 &point ) const {
		return HasImpactOnPoint( point.Data() );
	}

	bool HasImpactOnPoint( const vec3_t point ) const {
		// Currently only splash-like hazards are supported
		return IsSplashLike() && hitPoint.SquareDistanceTo( point ) <= splashRadius * splashRadius;
	}
};

class HazardsSelector {
	friend class BotAwarenessModule;

	Bot *const bot;

	static constexpr auto MAX_CLASS_HAZARDS = 1;
	typedef Pool<Hazard, MAX_CLASS_HAZARDS> HazardsPool;

	// Currently there is no more than a single active hazard. It might be changed in future.
	Hazard *primaryHazard { nullptr };

	// We need a bit more space for intermediate results
	Pool<Hazard, 3> hazardsPool { "hazardsPool" };

	void FindProjectileHazards( const EntNumsVector &entNums );
	void FindWaveHazards( const EntNumsVector &entNums );
	void FindPlasmaHazards( const EntNumsVector &entNums );
	void FindLaserHazards( const EntNumsVector &entNums );
public:
	explicit HazardsSelector( Bot *bot_ ): bot( bot_ ) {}

	void BeginUpdate();
	void EndUpdate();

	const Hazard *PrimaryHazard() const { return primaryHazard; }

	bool TryAddHazard( float damageScore,
					   const vec3_t hitPoint,
					   const vec3_t direction,
					   const edict_t *owner,
					   float splashRadius = 0.0f );
};

class HazardsSelectorCache {
	friend class SameDirBeamsList;

	template <typename, unsigned> friend class StaticVector;

	uint8_t *storageMem;
	class CachingAllocator *sortedProjectilesAllocator;
	class CachingAllocator *plasmaBeamsAllocator;

	HazardsSelectorCache();
	~HazardsSelectorCache();

	static HazardsSelectorCache *instance;
public:
	static HazardsSelectorCache *Instance() {
		assert( instance );
		return instance;
	}

	static void Init();
	static void Shutdown();
};

#endif
