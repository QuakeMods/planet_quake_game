#ifndef QFUSION_BUNNYTOBESTCLUSTERPOINTACTION_H
#define QFUSION_BUNNYTOBESTCLUSTERPOINTACTION_H

#include "BunnyTestingMultipleLookDirsAction.h"

class BunnyToBestFloorClusterPointAction final : public BunnyTestingMultipleLookDirsAction {
	using Super = BunnyTestingMultipleLookDirsAction;

	Vec3 localDirStorage { 0, 0, 0 };

	bool hasTestedSameCluster { false };
	bool hasTestedNextCluster { false };

	static constexpr const char *NAME = "BunnyToBestFloorClusterPointAction";

	void OnApplicationSequenceStarted( MovementPredictionContext *context ) override;

	void OnApplicationSequenceFailed( MovementPredictionContext *context, unsigned ) override;
public:
	explicit BunnyToBestFloorClusterPointAction( BotMovementModule *module_ );

	void BeforePlanning() override {
		Super::BeforePlanning();
		hasTestedSameCluster = false;
		hasTestedNextCluster = false;
	}
};

#endif
