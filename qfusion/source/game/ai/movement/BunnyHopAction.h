#ifndef QFUSION_BUNNYHOPACTION_H
#define QFUSION_BUNNYHOPACTION_H

#include "BaseMovementAction.h"

class BunnyHopAction : public BaseMovementAction {
	friend class MovementPredictionContext;
protected:
	int travelTimeAtSequenceStart { 0 };
	int reachAtSequenceStart { 0 };
	int groundedAreaAtSequenceStart { 0 };
	float groundZAtSequenceStart { 0.0f };
	// Best results so far achieved in the action application sequence
	int minTravelTimeToNavTargetSoFar { 0 };
	int minTravelTimeAreaNumSoFar { 0 };

	float distanceToReachAtStart { std::numeric_limits<float>::infinity() };

	// A fraction of speed gain per frame time.
	// Might be negative, in this case it limits allowed speed loss
	float minDesiredSpeedGainPerSecond { 0.0f };
	unsigned currentSpeedLossSequentialMillis { 0 };
	unsigned tolerableSpeedLossSequentialMillis { 300 };

	// When bot bunnies over a gap, its target either becomes unreachable
	// or travel time is calculated from the bottom of the pit.
	// These timers allow to temporarily skip targer reachability/travel time tests.
	unsigned currentUnreachableTargetSequentialMillis { 0 };
	unsigned tolerableUnreachableTargetSequentialMillis { 700 };

	// Allow increased final travel time if the min travel time area is reachable by walking
	// from the final area and walking travel time is lower than this limit.
	// It allows to follow the reachability chain less strictly while still being close to it.
	unsigned tolerableWalkableIncreasedTravelTimeMillis { 3000 };

	// There is a mechanism for completely disabling an action for further planning by setting isDisabledForPlanning flag.
	// However we need a more flexible way of disabling an action after an failed application sequence.
	// A sequence started from different frame that the failed one might succeed.
	// An application sequence will not start at the frame indexed by this value.
	unsigned disabledForApplicationFrameIndex { std::numeric_limits<unsigned>::max() };

	// This should be set if we want to continue prediction
	// but give a path built by a current sequence an additional penalty
	// accounted by MovementPredictionContext::CompleteOrSaveGoodEnoughPath()
	unsigned sequencePathPenalty { 0 };

	bool hasEnteredNavTargetArea { false };
	bool hasTouchedNavTarget { false };

	void SetupCommonBunnyHopInput( MovementPredictionContext *context );
	// TODO: Mark as virtual in base class and mark as final here to avoid a warning about hiding parent member?
	bool GenericCheckIsActionEnabled( MovementPredictionContext *context, BaseMovementAction *suggestedAction );
	bool CheckCommonBunnyHopPreconditions( MovementPredictionContext *context );
	bool SetupBunnyHopping( const Vec3 &intendedLookVec, MovementPredictionContext *context );
	bool CanFlyAboveGroundRelaxed( const MovementPredictionContext *context ) const;
	bool CanSetWalljump( MovementPredictionContext *context ) const;
	void TrySetWalljump( MovementPredictionContext *context );

	// Can be overridden for finer control over tests
	virtual bool CheckStepSpeedGainOrLoss( MovementPredictionContext *context );

	bool CheckNavTargetAreaTransition( MovementPredictionContext *context );

	inline bool IsSkimmingInAGivenState( const MovementPredictionContext *context ) const;

	bool TryHandlingSkimmingState( MovementPredictionContext *context );

	bool TryHandlingUnreachableTarget( MovementPredictionContext *context );

	bool TryHandlingWorseTravelTimeToTarget( MovementPredictionContext *context,
		                                     int currTravelTimeToTarget,
		                                     int groundedAreaNum );

	inline bool WasOnGroundThisFrame( const MovementPredictionContext *context ) const;

	inline bool HasSubstantiallyChangedZ( const AiEntityPhysicsState &entityPhysicsState ) const;

	void EnsurePathPenalty( unsigned penalty ) {
		assert( penalty < 30000 );
		sequencePathPenalty = std::max( sequencePathPenalty, penalty );
	}

	void CompleteOrSaveGoodEnoughPath( MovementPredictionContext *context, unsigned additionalPenalty = 0 );

	bool CheckDirectReachWalkingOrFallingShort( int fromAreaNum, int toAreaNum );
public:
	BunnyHopAction( BotMovementModule *module_, const char *name_, int debugColor_ = 0 )
		: BaseMovementAction( module_, name_, debugColor_ ) {
		// Do NOT stop prediction on this! We have to check where the bot is going to land!
		BaseMovementAction::stopPredictionOnTouchingNavEntity = false;
	}

	void CheckPredictionStepResults( MovementPredictionContext *context ) override;
	void OnApplicationSequenceStarted( MovementPredictionContext *context ) override;
	void OnApplicationSequenceStopped( MovementPredictionContext *context,
									   SequenceStopReason reason,
									   unsigned stoppedAtFrameIndex ) override;
	void BeforePlanning() override;
};

#endif
