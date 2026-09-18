// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/TYWaterfallPathComponent.h"

#include "Actors/TYWaterfallActor.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"

UTYWaterfallPathComponent::UTYWaterfallPathComponent()
{
	// The actor or a generation builder owns the simulation cadence. The component
	// itself must not tick, otherwise every path would create its own editor tick.
	PrimaryComponentTick.bCanEverTick = false;
	SetMobility(EComponentMobility::Movable);
}

#if WITH_EDITOR
ATYWaterfallActor* UTYWaterfallPathComponent::GetWaterfallOwner() const
{
	return GetOwner<ATYWaterfallActor>();
}

bool UTYWaterfallPathComponent::GeneratePreviewPath()
{
	ATYWaterfallActor* Waterfall = GetWaterfallOwner();
	if (!IsValid(Waterfall) || !IsValid(Waterfall->GetTopSpline()))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	Modify();
	ClearPreviewPath();

	USplineComponent* TopSpline = Waterfall->GetTopSpline();
	const FVector StartPosition = TopSpline->GetLocationAtTime(
		0.5f, ESplineCoordinateSpace::World, true);
	const FVector StartDirection = TopSpline->GetDirectionAtTime(
		0.5f, ESplineCoordinateSpace::World, true).GetSafeNormal();

	if (StartDirection.IsNearlyZero())
	{
		return false;
	}

	const float Step = FMath::Max(FixedDeltaTime, KINDA_SMALL_NUMBER);
	const int32 StepLimit = FMath::Max(MaxSteps, 1);
	const float DragFactor = 1.0f - FMath::Clamp(Drag * Step, 0.0f, 1.0f);
	const float WorldTerminationHeight = Waterfall->GetActorLocation().Z + TerminationHeight;

	FTYWaterfallSimPoint CurrentPoint;
	CurrentPoint.Position = StartPosition;
	CurrentPoint.Velocity = StartDirection * FMath::Max(InitialSpeed, 0.0f);
	CurrentPoint.State = ETYWaterfallPointState::Start;
	SimulatedPoints.Add(CurrentPoint);

	// Each iteration advances exactly one fixed step. A line trace covers the
	// whole segment, so fast-moving points cannot skip a thin collision surface.
	for (int32 StepIndex = 0; StepIndex < StepLimit; ++StepIndex)
	{
		const FVector PreviousPosition = CurrentPoint.Position;
		CurrentPoint.Velocity += Gravity * Step;
		CurrentPoint.Velocity *= DragFactor;
		CurrentPoint.State = ETYWaterfallPointState::Airborne;

		const FVector CandidatePosition = PreviousPosition + CurrentPoint.Velocity * Step;
		FHitResult Hit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TYWaterfallPath), true, Waterfall);
		const bool bHit = World->LineTraceSingleByChannel(
			Hit, PreviousPosition, CandidatePosition, ECC_Visibility, QueryParams);

		if (bHit)
		{
			CurrentPoint.Position = Hit.ImpactPoint + Hit.ImpactNormal * 2.0f;
			CurrentPoint.HitNormal = Hit.ImpactNormal;

			// Remove the velocity component pointing into the surface. The
			// remaining component becomes the sliding direction.
			CurrentPoint.Velocity = FVector::VectorPlaneProject(
				CurrentPoint.Velocity, Hit.ImpactNormal);
			CurrentPoint.State = ETYWaterfallPointState::Sliding;

			if (CurrentPoint.Velocity.SizeSquared() < FMath::Square(1.0f))
			{
				CurrentPoint.Velocity = FVector::ZeroVector;
				CurrentPoint.State = ETYWaterfallPointState::Stopped;
			}
		}
		else
		{
			CurrentPoint.Position = CandidatePosition;
			CurrentPoint.HitNormal = FVector::UpVector;
		}

		SimulatedPoints.Add(CurrentPoint);

		if (CurrentPoint.State == ETYWaterfallPointState::Stopped
			|| CurrentPoint.Position.Z <= WorldTerminationHeight
			|| CurrentPoint.Velocity.IsNearlyZero())
		{
			SimulatedPoints.Last().State = ETYWaterfallPointState::Terminated;
			break;
		}

	}

	bSimulationComplete = SimulatedPoints.Num() > 1;
	WriteSimulationToSpline();
	MarkRenderStateDirty();
	MarkPackageDirty();
	return bSimulationComplete;
}

void UTYWaterfallPathComponent::ClearPreviewPath()
{
	Modify();
	SimulatedPoints.Reset();
	bSimulationComplete = false;
	ClearSplinePoints(false);
	UpdateSpline();
}

void UTYWaterfallPathComponent::WriteSimulationToSpline()
{
	ClearSplinePoints(false);
	for (const FTYWaterfallSimPoint& Point : SimulatedPoints)
	{
		AddSplinePoint(Point.Position, ESplineCoordinateSpace::World, false);
	}
	SetSplinePointType(0, ESplinePointType::Linear, false);
	for (int32 PointIndex = 1; PointIndex < GetNumberOfSplinePoints(); ++PointIndex)
	{
		SetSplinePointType(PointIndex, ESplinePointType::CurveClamped, false);
	}
	UpdateSpline();
}
#endif
