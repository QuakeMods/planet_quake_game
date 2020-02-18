#ifndef QFUSION_USEWALKABLENODEFALLBACK_H
#define QFUSION_USEWALKABLENODEFALLBACK_H

#include "GenericGroundMovementScript.h"

class UseWalkableNodeScript: public GenericGroundMovementScript {
protected:
	vec3_t nodeOrigin { 0, 0, 0 };
	int nodeAasAreaNum { 0 };
	float reachRadius { 0.0f };
	unsigned timeout { 0 };

	void GetSteeringTarget( vec3_t target ) override {
		VectorCopy( nodeOrigin, target );
	}
public:
	explicit UseWalkableNodeScript( const Bot *bot_, BotMovementModule *module_ )
		: GenericGroundMovementScript( bot_, module_, COLOR_RGB( 0, 192, 0 ) ) {}

	const vec3_t &NodeOrigin() const { return nodeOrigin; }
	int NodeAreaNum() const { return nodeAasAreaNum; }

	void Activate( const vec3_t nodeOrigin_, float reachRadius_, int nodeAasAreaNum_ = 0, unsigned timeout_ = 750 );

	bool TryDeactivate( MovementPredictionContext *context = nullptr ) override;
};

#endif
