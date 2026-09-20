// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/DynamicMeshComponent.h"
#include "TYWaterfallMeshComponent.generated.h"

class UTYWaterfallPathComponent;

/** Dynamic mesh component that stores the generated per-path waterfall ribbons. */
UCLASS(ClassGroup = (Waterfall), meta = (DisplayName = "TY Waterfall Mesh"))
class TYWATERFALLTOOLSRUNTIME_API UTYWaterfallMeshComponent : public UDynamicMeshComponent
{
	GENERATED_BODY()

public:
	UTYWaterfallMeshComponent(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	/** Replaces the current mesh with one ribbon for every valid path. */
	bool BuildPerPathRibbons(
		const TArray<TObjectPtr<UTYWaterfallPathComponent>>& Paths,
		FVector WorldWidthAxis,
		float RibbonWidth,
		float SampleSpacing,
		float UVLength);

	void ClearWaterfallMesh();
#endif
};
