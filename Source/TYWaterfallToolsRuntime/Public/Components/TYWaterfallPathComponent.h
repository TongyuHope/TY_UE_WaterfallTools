// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "TYWaterfallPathComponent.generated.h"

class ATYWaterfallActor;

/** State recorded for each point produced by the editor-time path simulation. */
UENUM()
enum class ETYWaterfallPointState : uint8
{
	Start,
	Airborne,
	Sliding,
	Stopped,
	Terminated
};

/** A debug-friendly snapshot of one simulated fluid point. */
USTRUCT()
struct TYWATERFALLTOOLSRUNTIME_API FTYWaterfallSimPoint
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Position = FVector::ZeroVector;

	UPROPERTY()
	FVector Velocity = FVector::ZeroVector;

	UPROPERTY()
	FVector HitNormal = FVector::UpVector;

	UPROPERTY()
	ETYWaterfallPointState State = ETYWaterfallPointState::Start;
};

/** Spline that stores one simulated waterfall path. */
UCLASS(ClassGroup = (Waterfall), meta = (DisplayName = "TY Waterfall Path"))
class TYWATERFALLTOOLSRUNTIME_API UTYWaterfallPathComponent : public USplineComponent
{
	GENERATED_BODY()

public:
	UTYWaterfallPathComponent();

#if WITH_EDITOR
	/** Runs one deterministic path simulation and writes the result into this spline. */
	UFUNCTION(CallInEditor, Category = "Waterfall|Simulation")
	bool GeneratePreviewPath();

	/** Removes all simulated points and spline points created by GeneratePreviewPath. */
	UFUNCTION(CallInEditor, Category = "Waterfall|Simulation")
	void ClearPreviewPath();

	UFUNCTION(BlueprintPure, Category = "Waterfall|Simulation")
	int32 GetNumSimulatedPoints() const { return SimulatedPoints.Num(); }

	UFUNCTION(BlueprintPure, Category = "Waterfall|Simulation")
	bool HasCompletedSimulation() const { return bSimulationComplete; }
#endif

protected:
	/** Initial speed along the top spline's forward direction, in cm/s. */
	UPROPERTY(EditAnywhere, Category = "Simulation", meta = (ClampMin = "0.0"))
	float InitialSpeed = 800.0f;

	/** World-space acceleration applied every fixed simulation step, in cm/s^2. */
	UPROPERTY(EditAnywhere, Category = "Simulation")
	FVector Gravity = FVector(0.0f, 0.0f, -980.0f);

	/** Fraction of velocity removed per second. Values are clamped to [0, 1]. */
	UPROPERTY(EditAnywhere, Category = "Simulation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Drag = 0.1f;

	/** Fixed time step makes repeated runs with the same inputs deterministic. */
	UPROPERTY(EditAnywhere, Category = "Simulation", meta = (ClampMin = "0.001", ClampMax = "0.1"))
	float FixedDeltaTime = 0.016f;

	/** Safety limit that prevents a malformed scene from simulating forever. */
	UPROPERTY(EditAnywhere, Category = "Simulation", meta = (ClampMin = "1"))
	int32 MaxSteps = 600;

	/** Simulation ends when the point reaches this world-Z offset from its owner. */
	UPROPERTY(EditAnywhere, Category = "Simulation")
	float TerminationHeight = -1000.0f;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Simulation")
	TArray<FTYWaterfallSimPoint> SimulatedPoints;

	UPROPERTY(VisibleAnywhere, Category = "Simulation")
	bool bSimulationComplete = false;
#endif

private:
#if WITH_EDITOR
	ATYWaterfallActor* GetWaterfallOwner() const;
	void WriteSimulationToSpline();
#endif
};
