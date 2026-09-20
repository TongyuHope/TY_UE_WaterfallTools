// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/TYWaterfallMeshComponent.h"

#include "Components/TYWaterfallPathComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

using namespace UE::Geometry;

UTYWaterfallMeshComponent::UTYWaterfallMeshComponent(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetTangentsType(EDynamicMeshComponentTangentsMode::AutoCalculated);
	SetVisibility(false);
}

#if WITH_EDITOR
bool UTYWaterfallMeshComponent::BuildPerPathRibbons(
	const TArray<TObjectPtr<UTYWaterfallPathComponent>>& Paths,
	FVector WorldWidthAxis,
	float RibbonWidth,
	float SampleSpacing,
	float UVLength)
{
	const float SafeWidth = FMath::Max(RibbonWidth, 1.0f);
	const float SafeSpacing = FMath::Max(SampleSpacing, 1.0f);
	const float SafeUVLength = FMath::Max(UVLength, 1.0f);
	WorldWidthAxis = WorldWidthAxis.GetSafeNormal();
	if (WorldWidthAxis.IsNearlyZero())
	{
		return false;
	}

	FDynamicMesh3 NewMesh;
	NewMesh.EnableAttributes();
	FDynamicMeshNormalOverlay* NormalOverlay = NewMesh.Attributes()->PrimaryNormals();
	FDynamicMeshUVOverlay* UVOverlay = NewMesh.Attributes()->PrimaryUV();
	const FTransform WorldToMesh = GetComponentTransform().Inverse();
	int32 BuiltRibbonCount = 0;

	for (const UTYWaterfallPathComponent* Path : Paths)
	{
		if (!IsValid(Path) || Path->GetNumberOfSplinePoints() < 2)
		{
			continue;
		}

		const float PathLength = Path->GetSplineLength();
		if (PathLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		// Distance-based sampling produces stable geometry density even when the
		// simulation itself generated unevenly spaced points after collisions.
		const int32 SegmentCount = FMath::Max(FMath::CeilToInt(PathLength / SafeSpacing), 1);
		TArray<int32> VertexIDs;
		TArray<int32> NormalIDs;
		TArray<int32> UVIDs;
		VertexIDs.Reserve((SegmentCount + 1) * 2);
		NormalIDs.Reserve((SegmentCount + 1) * 2);
		UVIDs.Reserve((SegmentCount + 1) * 2);

		for (int32 SampleIndex = 0; SampleIndex <= SegmentCount; ++SampleIndex)
		{
			const float Alpha = static_cast<float>(SampleIndex) / static_cast<float>(SegmentCount);
			const float Distance = PathLength * Alpha;
			const FVector WorldCenter = Path->GetLocationAtDistanceAlongSpline(
				Distance, ESplineCoordinateSpace::World);
			const FVector WorldTangent = Path->GetDirectionAtDistanceAlongSpline(
				Distance, ESplineCoordinateSpace::World).GetSafeNormal();

			// Project the waterfall width axis onto the plane perpendicular to flow.
			// This keeps the ribbon stable as a path turns from horizontal to vertical.
			FVector WorldAcross = FVector::VectorPlaneProject(WorldWidthAxis, WorldTangent).GetSafeNormal();
			if (WorldAcross.IsNearlyZero())
			{
				WorldAcross = FVector::CrossProduct(WorldTangent, FVector::UpVector).GetSafeNormal();
			}
			if (WorldAcross.IsNearlyZero())
			{
				WorldAcross = FVector::RightVector;
			}

			const FVector WorldNormal = FVector::CrossProduct(WorldAcross, WorldTangent).GetSafeNormal();
			const FVector HalfWidthOffset = WorldAcross * (SafeWidth * 0.5f);
			const FVector LocalLeft = WorldToMesh.TransformPosition(WorldCenter - HalfWidthOffset);
			const FVector LocalRight = WorldToMesh.TransformPosition(WorldCenter + HalfWidthOffset);
			const FVector3f LocalNormal = FVector3f(
				WorldToMesh.TransformVectorNoScale(WorldNormal).GetSafeNormal());

			VertexIDs.Add(NewMesh.AppendVertex(FVector3d(LocalLeft)));
			VertexIDs.Add(NewMesh.AppendVertex(FVector3d(LocalRight)));
			NormalIDs.Add(NormalOverlay->AppendElement(LocalNormal));
			NormalIDs.Add(NormalOverlay->AppendElement(LocalNormal));
			UVIDs.Add(UVOverlay->AppendElement(FVector2f(0.0f, Distance / SafeUVLength)));
			UVIDs.Add(UVOverlay->AppendElement(FVector2f(1.0f, Distance / SafeUVLength)));
		}

		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const int32 Left0 = SegmentIndex * 2;
			const int32 Right0 = Left0 + 1;
			const int32 Left1 = Left0 + 2;
			const int32 Right1 = Left0 + 3;

			// Unreal renders clockwise winding as the front face. Keep the authored
			// normal pointing above the water surface, but wind both triangles so
			// that same side is also the visible side for one-sided materials.
			const FIndex3i TriangleA(
				VertexIDs[Left0], VertexIDs[Left1], VertexIDs[Right0]);
			const FIndex3i TriangleB(
				VertexIDs[Right0], VertexIDs[Left1], VertexIDs[Right1]);
			const int32 TriangleAID = NewMesh.AppendTriangle(TriangleA);
			const int32 TriangleBID = NewMesh.AppendTriangle(TriangleB);

			if (TriangleAID >= 0)
			{
				NormalOverlay->SetTriangle(TriangleAID, FIndex3i(
					NormalIDs[Left0], NormalIDs[Left1], NormalIDs[Right0]));
				UVOverlay->SetTriangle(TriangleAID, FIndex3i(
					UVIDs[Left0], UVIDs[Left1], UVIDs[Right0]));
			}
			if (TriangleBID >= 0)
			{
				NormalOverlay->SetTriangle(TriangleBID, FIndex3i(
					NormalIDs[Right0], NormalIDs[Left1], NormalIDs[Right1]));
				UVOverlay->SetTriangle(TriangleBID, FIndex3i(
					UVIDs[Right0], UVIDs[Left1], UVIDs[Right1]));
			}
		}

		++BuiltRibbonCount;
	}

	SetMesh(MoveTemp(NewMesh));
	SetVisibility(BuiltRibbonCount > 0);
	MarkPackageDirty();
	return BuiltRibbonCount > 0;
}

void UTYWaterfallMeshComponent::ClearWaterfallMesh()
{
	FDynamicMesh3 EmptyMesh;
	SetMesh(MoveTemp(EmptyMesh));
	SetVisibility(false);
	MarkPackageDirty();
}
#endif
