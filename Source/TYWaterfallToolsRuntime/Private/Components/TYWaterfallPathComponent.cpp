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
	if (!InitializeSimulation(0.5f))
	{
		return false;
	}

	// This compatibility entry point intentionally runs synchronously. The
	// multi-path builder uses AdvanceSimulation directly with a frame budget.
	while (!bSimulationComplete)
	{
		AdvanceSimulation(MaxSteps);
	}
	return bSimulationComplete;
}

bool UTYWaterfallPathComponent::InitializeSimulation(
	float SplineTime,
	float DirectionJitterDegrees,
	bool bReverseFlowDirection)
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
		SplineTime, ESplineCoordinateSpace::World, true);
	const FVector UpDirection = TopSpline->GetUpVectorAtTime(
		SplineTime, ESplineCoordinateSpace::World, true).GetSafeNormal();
	// The top spline describes the waterfall's width. Water must leave the
	// cliff perpendicular to that width, producing a T-shaped top/path layout.
	// The right vector supplies that perpendicular direction while preserving
	// the spline's authored up vector and rotation.
	FVector StartDirection = TopSpline->GetRightVectorAtTime(
		SplineTime, ESplineCoordinateSpace::World, true)
		.GetSafeNormal()
		.RotateAngleAxis(DirectionJitterDegrees, UpDirection);
	if (bReverseFlowDirection)
	{
		StartDirection *= -1.0f;
	}

	if (StartDirection.IsNearlyZero())
	{
		return false;
	}

	CurrentPoint.Position = StartPosition;
	CurrentPoint.Velocity = StartDirection * FMath::Max(InitialSpeed, 0.0f);
	CurrentPoint.State = ETYWaterfallPointState::Start;
	SimulatedPoints.Add(CurrentPoint);
	StepsCompleted = 0;
	WorldTerminationHeight = Waterfall->GetActorLocation().Z + TerminationHeight;
	bSimulationInitialized = true;
	bSimulationComplete = false;
	WriteSimulationToSpline();
	return true;
}

void UTYWaterfallPathComponent::ConfigureSimulation(
	float InInitialSpeed,
	FVector InGravity,
	float InDrag,
	float InFixedDeltaTime,
	int32 InMaxSteps,
	float InTerminationHeight)
{
	InitialSpeed = FMath::Max(InInitialSpeed, 0.0f);
	Gravity = InGravity;
	Drag = FMath::Clamp(InDrag, 0.0f, 1.0f);
	FixedDeltaTime = FMath::Max(InFixedDeltaTime, 0.001f);
	MaxSteps = FMath::Max(InMaxSteps, 1);
	TerminationHeight = InTerminationHeight;
}

int32 UTYWaterfallPathComponent::AdvanceSimulation(int32 StepBudget)
{
	if (!bSimulationInitialized || bSimulationComplete || StepBudget <= 0)
	{
		return 0;
	}

	ATYWaterfallActor* Waterfall = GetWaterfallOwner();
	UWorld* World = GetWorld();
	if (!IsValid(Waterfall) || !IsValid(World))
	{
		bSimulationComplete = true;
		return 0;
	}

	const float Step = FMath::Max(FixedDeltaTime, KINDA_SMALL_NUMBER);
	const int32 StepLimit = FMath::Max(MaxSteps, 1);
	const float DragFactor = 1.0f - FMath::Clamp(Drag * Step, 0.0f, 1.0f);
	int32 StepsUsed = 0;

	// Each iteration advances exactly one fixed step. A line trace covers the
	// whole segment, so fast-moving points cannot skip a thin collision surface.
	while (StepsUsed < StepBudget && StepsCompleted < StepLimit)
	{
		++StepsUsed;
		++StepsCompleted;
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
			|| CurrentPoint.Velocity.IsNearlyZero()
			|| StepsCompleted >= StepLimit)
		{
			SimulatedPoints.Last().State = ETYWaterfallPointState::Terminated;
			bSimulationComplete = true;
			break;
		}
	}

	WriteSimulationToSpline();
	MarkRenderStateDirty();
	if (bSimulationComplete)
	{
		MarkPackageDirty();
	}
	return StepsUsed;
}

void UTYWaterfallPathComponent::ClearPreviewPath()
{
	Modify();
	SimulatedPoints.Reset();
	bSimulationComplete = false;
	bSimulationInitialized = false;
	StepsCompleted = 0;
	CurrentPoint = FTYWaterfallSimPoint();
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
	if (GetNumberOfSplinePoints() > 0)
	{
		SetSplinePointType(0, ESplinePointType::Linear, false);
	}
	for (int32 PointIndex = 1; PointIndex < GetNumberOfSplinePoints(); ++PointIndex)
	{
		SetSplinePointType(PointIndex, ESplinePointType::CurveClamped, false);
	}
	UpdateSpline();
}
#endif
