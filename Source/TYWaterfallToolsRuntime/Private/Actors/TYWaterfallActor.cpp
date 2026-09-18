// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/TYWaterfallActor.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"

#if WITH_EDITORONLY_DATA
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#endif

ATYWaterfallActor::ATYWaterfallActor()
{
	PrimaryActorTick.bCanEverTick = false;

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
}
