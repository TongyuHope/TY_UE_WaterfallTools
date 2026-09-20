// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/TYWaterfallSettingsComponent.h"

UTYWaterfallSettingsComponent::UTYWaterfallSettingsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FVector2D UTYWaterfallSettingsComponent::GetSpawnRange() const
{
	// Accept reversed ranges in Details while always returning a sorted,
	// normalized interval to the generation code.
	return FVector2D(
		FMath::Clamp(FMath::Min(SpawnRange.X, SpawnRange.Y), 0.0f, 1.0f),
		FMath::Clamp(FMath::Max(SpawnRange.X, SpawnRange.Y), 0.0f, 1.0f));
}
