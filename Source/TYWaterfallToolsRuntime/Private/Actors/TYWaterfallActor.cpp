// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/TYWaterfallActor.h"

#include "Components/TYWaterfallMeshComponent.h"
#include "Components/TYWaterfallPathComponent.h"
#include "Components/TYWaterfallSettingsComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"

#if WITH_EDITORONLY_DATA
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#endif

#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

ATYWaterfallActor::ATYWaterfallActor()
{
	// Tick is available for editor-time generation but remains disabled until a
	// builder has work. This keeps placed waterfalls free of idle tick cost.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);

	TopSpline = CreateDefaultSubobject<USplineComponent>(TEXT("TopSpline"));
	TopSpline->SetupAttachment(RootComp);
	TopSpline->SetMobility(EComponentMobility::Movable);
	TopSpline->SetClosedLoop(false);
	TopSpline->ClearSplinePoints(false);
	TopSpline->AddSplinePoint(FVector(-300.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	TopSpline->AddSplinePoint(FVector(300.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	TopSpline->SetSplinePointType(0, ESplinePointType::Curve, false);
	TopSpline->SetSplinePointType(1, ESplinePointType::Curve, false);
	TopSpline->UpdateSpline();

	BakedMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BakedMesh"));
	BakedMeshComponent->SetupAttachment(RootComp);
	BakedMeshComponent->SetMobility(EComponentMobility::Movable);
	BakedMeshComponent->SetVisibility(false);
	BakedMeshComponent->SetHiddenInGame(true);
	BakedMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DynamicMeshComponent = CreateDefaultSubobject<UTYWaterfallMeshComponent>(TEXT("DynamicMesh"));
	DynamicMeshComponent->SetupAttachment(RootComp);
	DynamicMeshComponent->SetMobility(EComponentMobility::Movable);

#if WITH_EDITORONLY_DATA
	WaterfallSettings = CreateEditorOnlyDefaultSubobject<UTYWaterfallSettingsComponent>(
		TEXT("WaterfallSettings"));
#endif

#if WITH_EDITORONLY_DATA
	KillPlaneComponent = CreateEditorOnlyDefaultSubobject<UStaticMeshComponent>(TEXT("KillPlane"));
	if (KillPlaneComponent)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(
			TEXT("/Engine/BasicShapes/Plane"));
		if (PlaneMeshFinder.Succeeded())
		{
			KillPlaneComponent->SetStaticMesh(PlaneMeshFinder.Object);
		}

		KillPlaneComponent->SetupAttachment(RootComp);
		KillPlaneComponent->SetRelativeScale3D(FVector(20.0f));
		KillPlaneComponent->SetMobility(EComponentMobility::Movable);
		KillPlaneComponent->SetVisibility(false);
		KillPlaneComponent->SetHiddenInGame(true);
		KillPlaneComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
#endif

#if WITH_EDITOR
	PathBuilder.Initialize(this);
	MeshBuilder.Initialize(this);
#endif
}

void ATYWaterfallActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if WITH_EDITOR
	if (PathBuilder.IsGenerating())
	{
		PathBuilder.TickGeneration();
	}
#endif
}

#if WITH_EDITOR
void ATYWaterfallActor::GeneratePaths()
{
	PathBuilder.StartGeneration();
}

void ATYWaterfallActor::CancelPathGeneration()
{
	PathBuilder.CancelGeneration();
}

void ATYWaterfallActor::ClearGeneratedPaths()
{
	if (PathBuilder.IsGenerating())
	{
		PathBuilder.CancelGeneration();
		return;
	}

	const FScopedTransaction Transaction(NSLOCTEXT(
		"TYWaterfallTools", "ClearWaterfallPaths", "Clear Waterfall Paths"));
	Modify();
	PathBuilder.ClearGeneratedPaths();
}

void ATYWaterfallActor::GeneratePerPathMesh()
{
	MeshBuilder.BuildPerPathMesh();
}

void ATYWaterfallActor::ClearDynamicMesh()
{
	const FScopedTransaction Transaction(NSLOCTEXT(
		"TYWaterfallTools", "ClearWaterfallMesh", "Clear Waterfall Mesh"));
	Modify();
	MeshBuilder.ClearMesh();
}

void ATYWaterfallActor::SetPathDebugVisible(bool bVisible)
{
	for (UTYWaterfallPathComponent* Path : GeneratedPaths)
	{
		if (IsValid(Path))
		{
			Path->SetDrawDebug(bVisible);
			Path->MarkRenderStateDirty();
		}
	}
}
#endif
