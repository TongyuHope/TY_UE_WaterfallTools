// Copyright Epic Games, Inc. All Rights Reserved.

#include "Generation/TYWaterfallMeshBuilder.h"

#include "Actors/TYWaterfallActor.h"
#include "Components/TYWaterfallMeshComponent.h"
#include "Components/TYWaterfallSettingsComponent.h"
#include "Components/SplineComponent.h"
#include "Materials/MaterialInterface.h"

#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

#if WITH_EDITOR
void FTYWaterfallMeshBuilder::Initialize(ATYWaterfallActor* InOwner)
{
	Owner = InOwner;
}

bool FTYWaterfallMeshBuilder::BuildPerPathMesh()
{
	ATYWaterfallActor* Waterfall = Owner.Get();
	if (!IsValid(Waterfall) || !IsValid(Waterfall->DynamicMeshComponent)
		|| !IsValid(Waterfall->WaterfallSettings) || Waterfall->GeneratedPaths.IsEmpty())
	{
		return false;
	}

	const FScopedTransaction Transaction(NSLOCTEXT(
		"TYWaterfallTools", "GenerateWaterfallMesh", "Generate Waterfall Mesh"));
	Waterfall->Modify();
	Waterfall->DynamicMeshComponent->Modify();

	const FVector WidthAxis = Waterfall->TopSpline->GetDirectionAtTime(
		0.5f, ESplineCoordinateSpace::World, true).GetSafeNormal();
	const bool bBuilt = Waterfall->DynamicMeshComponent->BuildPerPathRibbons(
		Waterfall->GeneratedPaths,
		WidthAxis,
		Waterfall->WaterfallSettings->GetRibbonWidth(),
		Waterfall->WaterfallSettings->GetMeshSampleSpacing(),
		Waterfall->WaterfallSettings->GetMeshUVLength());

	if (bBuilt)
	{
		Waterfall->DynamicMeshComponent->SetMaterial(
			0, Waterfall->WaterfallSettings->GetWaterfallMaterial());
		Waterfall->MarkPackageDirty();
	}
	return bBuilt;
}

void FTYWaterfallMeshBuilder::ClearMesh()
{
	if (ATYWaterfallActor* Waterfall = Owner.Get())
	{
		if (IsValid(Waterfall->DynamicMeshComponent))
		{
			Waterfall->DynamicMeshComponent->Modify();
			Waterfall->DynamicMeshComponent->ClearWaterfallMesh();
		}
	}
}
#endif
