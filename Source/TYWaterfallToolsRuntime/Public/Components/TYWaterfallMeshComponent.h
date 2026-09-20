// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/DynamicMeshComponent.h"
#include "TYWaterfallMeshComponent.generated.h"

class UTYWaterfallPathComponent;

/** Dynamic mesh component that stores the combined waterfall representation. */
UCLASS(ClassGroup = (Waterfall), meta = (DisplayName = "TY Waterfall Mesh"))
class TYWATERFALLTOOLSRUNTIME_API UTYWaterfallMeshComponent : public UDynamicMeshComponent
{
	GENERATED_BODY()

public:
	UTYWaterfallMeshComponent(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	/** Replaces the mesh with Per Path ribbons, Cross planes and endpoint splashes. */
	bool BuildCombinedMesh(
		const TArray<TObjectPtr<UTYWaterfallPathComponent>>& Paths,
		FVector WorldWidthAxis,
		float RibbonWidth,
		float CrossWidth,
		float UVLength,
		float FrontRadius,
		float BackRadius,
		int32 RadialSegments,
		int32 Rings);

	void ClearWaterfallMesh();
#endif
};
