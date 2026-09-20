// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TYWaterfallSettingsComponent.generated.h"

class UMaterialInterface;

/** Authoring settings shared by all paths generated for one waterfall actor. */
UCLASS(ClassGroup = (Waterfall), meta = (DisplayName = "TY Waterfall Settings"))
class TYWATERFALLTOOLSRUNTIME_API UTYWaterfallSettingsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTYWaterfallSettingsComponent();

	int32 GetNumPaths() const { return FMath::Max(NumPaths, 1); }
	int32 GetSeed() const { return Seed; }
	FVector2D GetSpawnRange() const;
	float GetSpawnJitterDegrees() const { return FMath::Max(SpawnJitterDegrees, 0.0f); }
	bool ShouldReverseFlowDirection() const { return bReverseFlowDirection; }
	int32 GetSimulationStepsPerFrame() const { return FMath::Max(SimulationStepsPerFrame, 1); }
	float GetInitialSpeed() const { return FMath::Max(InitialSpeed, 0.0f); }
	FVector GetGravity() const { return Gravity; }
	float GetDrag() const { return FMath::Clamp(Drag, 0.0f, 1.0f); }
	float GetFixedDeltaTime() const { return FMath::Max(FixedDeltaTime, 0.001f); }
	int32 GetMaxSteps() const { return FMath::Max(MaxSteps, 1); }
	float GetTerminationHeight() const { return TerminationHeight; }
	float GetRibbonWidth() const { return FMath::Max(RibbonWidth, 1.0f); }
	float GetMeshSampleSpacing() const { return FMath::Max(MeshSampleSpacing, 1.0f); }
	float GetMeshUVLength() const { return FMath::Max(MeshUVLength, 1.0f); }
	UMaterialInterface* GetWaterfallMaterial() const { return WaterfallMaterial; }

protected:
	/** Number of paths distributed across the selected part of the top spline. */
	UPROPERTY(EditAnywhere, Category = "Paths", meta = (ClampMin = "1", ClampMax = "256"))
	int32 NumPaths = 8;

	/** Seed used for deterministic spawn-direction variation. */
	UPROPERTY(EditAnywhere, Category = "Paths")
	int32 Seed = 1337;

	/** Normalized start and end positions along the top spline. */
	UPROPERTY(EditAnywhere, Category = "Paths")
	FVector2D SpawnRange = FVector2D(0.0f, 1.0f);

	/** Maximum random yaw applied to each path's initial direction. */
	UPROPERTY(EditAnywhere, Category = "Paths", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float SpawnJitterDegrees = 2.0f;

	/** Flips the flow to the opposite side of the top spline. */
	UPROPERTY(EditAnywhere, Category = "Paths")
	bool bReverseFlowDirection = false;

	UPROPERTY(EditAnywhere, Category = "Simulation", meta = (ClampMin = "0.0"))
	float InitialSpeed = 800.0f;

	UPROPERTY(EditAnywhere, Category = "Simulation")
	FVector Gravity = FVector(0.0f, 0.0f, -980.0f);

	UPROPERTY(EditAnywhere, Category = "Simulation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Drag = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Simulation", meta = (ClampMin = "0.001", ClampMax = "0.1"))
	float FixedDeltaTime = 0.016f;

	UPROPERTY(EditAnywhere, Category = "Simulation", meta = (ClampMin = "1"))
	int32 MaxSteps = 600;

	UPROPERTY(EditAnywhere, Category = "Simulation")
	float TerminationHeight = -1000.0f;

	/** Total width of every generated ribbon, measured in Unreal units. */
	UPROPERTY(EditAnywhere, Category = "Mesh", meta = (ClampMin = "1.0"))
	float RibbonWidth = 100.0f;

	/** Target distance between adjacent ribbon rows. Smaller values create denser meshes. */
	UPROPERTY(EditAnywhere, Category = "Mesh", meta = (ClampMin = "1.0"))
	float MeshSampleSpacing = 25.0f;

	/** World-space distance represented by one repeat along the material's V axis. */
	UPROPERTY(EditAnywhere, Category = "Mesh", meta = (ClampMin = "1.0"))
	float MeshUVLength = 200.0f;

	/** Optional material assigned to slot 0 after the ribbons are generated. */
	UPROPERTY(EditAnywhere, Category = "Mesh")
	TObjectPtr<UMaterialInterface> WaterfallMaterial;

	/** Total simulation steps processed across all paths during one editor frame. */
	UPROPERTY(EditAnywhere, Category = "Performance", meta = (ClampMin = "1", ClampMax = "10000"))
	int32 SimulationStepsPerFrame = 256;
};
