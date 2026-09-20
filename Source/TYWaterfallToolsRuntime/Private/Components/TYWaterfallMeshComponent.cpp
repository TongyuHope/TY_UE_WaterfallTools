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
	const float SafeUVLength = FMath::Max(UVLength, 1.0f);
	WorldWidthAxis = WorldWidthAxis.GetSafeNormal();
	if (WorldWidthAxis.IsNearlyZero())
	{
		return false;
	}

	FDynamicMesh3 NewMesh;
	NewMesh.EnableAttributes();
	NewMesh.Attributes()->SetNumUVLayers(3);
	NewMesh.Attributes()->EnablePrimaryColors();
	FDynamicMeshNormalOverlay* NormalOverlay = NewMesh.Attributes()->PrimaryNormals();
	FDynamicMeshUVOverlay* UV0Overlay = NewMesh.Attributes()->GetUVLayer(0);
	FDynamicMeshUVOverlay* UV1Overlay = NewMesh.Attributes()->GetUVLayer(1);
	FDynamicMeshUVOverlay* UV2Overlay = NewMesh.Attributes()->GetUVLayer(2);
	FDynamicMeshColorOverlay* ColorOverlay = NewMesh.Attributes()->PrimaryColors();
	const FTransform WorldToMesh = GetComponentTransform().Inverse();
	int32 BuiltRibbonCount = 0;

	for (const UTYWaterfallPathComponent* Path : Paths)
	{
		if (!IsValid(Path) || Path->GetResampledSamples().Num() < 2)
		{
			continue;
		}

		const TArray<FTYWaterfallSample>& Samples = Path->GetResampledSamples();
		const float PathLength = Samples.Last().Distance;
		if (PathLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		// Distance-based sampling produces stable geometry density even when the
		// simulation itself generated unevenly spaced points after collisions.
		const int32 SegmentCount = Samples.Num() - 1;
		TArray<int32> VertexIDs;
		TArray<int32> NormalIDs;
		TArray<int32> UV0IDs;
		TArray<int32> UV1IDs;
		TArray<int32> UV2IDs;
		TArray<int32> ColorIDs;
		VertexIDs.Reserve((SegmentCount + 1) * 2);
		NormalIDs.Reserve((SegmentCount + 1) * 2);
		UV0IDs.Reserve((SegmentCount + 1) * 2);
		UV1IDs.Reserve((SegmentCount + 1) * 2);
		UV2IDs.Reserve((SegmentCount + 1) * 2);
		ColorIDs.Reserve((SegmentCount + 1) * 2);

		for (int32 SampleIndex = 0; SampleIndex <= SegmentCount; ++SampleIndex)
		{
			const FTYWaterfallSample& Sample = Samples[SampleIndex];
			const float Distance = Sample.Distance;
			const FVector WorldCenter = Sample.Position;
			const FVector WorldTangent = Sample.Tangent.GetSafeNormal();

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
			const FVector2f UV0Left(0.0f, Distance / SafeUVLength);
			const FVector2f UV0Right(1.0f, Distance / SafeUVLength);
			const FVector2f UV1(Distance, Sample.NormalizedDistance);
			const FVector2f UV2(Sample.Speed / 1000.0f, Sample.Turbulence);
			const FVector4f Color(Sample.Turbulence, Sample.Impact, Sample.RandomValue, 1.0f);
			UV0IDs.Add(UV0Overlay->AppendElement(UV0Left));
			UV0IDs.Add(UV0Overlay->AppendElement(UV0Right));
			const int32 UV1Left = UV1Overlay->AppendElement(UV1);
			const int32 UV1Right = UV1Overlay->AppendElement(UV1);
			const int32 UV2Left = UV2Overlay->AppendElement(UV2);
			const int32 UV2Right = UV2Overlay->AppendElement(UV2);
			const int32 ColorLeft = ColorOverlay->AppendElement(Color);
			const int32 ColorRight = ColorOverlay->AppendElement(Color);
			UV1IDs.Add(UV1Left);
			UV1IDs.Add(UV1Right);
			UV2IDs.Add(UV2Left);
			UV2IDs.Add(UV2Right);
			ColorIDs.Add(ColorLeft);
			ColorIDs.Add(ColorRight);
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
				UV0Overlay->SetTriangle(TriangleAID, FIndex3i(
					UV0IDs[Left0], UV0IDs[Left1], UV0IDs[Right0]));
				UV1Overlay->SetTriangle(TriangleAID, FIndex3i(
					UV1IDs[Left0], UV1IDs[Left1], UV1IDs[Right0]));
				UV2Overlay->SetTriangle(TriangleAID, FIndex3i(
					UV2IDs[Left0], UV2IDs[Left1], UV2IDs[Right0]));
				ColorOverlay->SetTriangle(TriangleAID, FIndex3i(
					ColorIDs[Left0], ColorIDs[Left1], ColorIDs[Right0]));
			}
			if (TriangleBID >= 0)
			{
				NormalOverlay->SetTriangle(TriangleBID, FIndex3i(
					NormalIDs[Right0], NormalIDs[Left1], NormalIDs[Right1]));
				UV0Overlay->SetTriangle(TriangleBID, FIndex3i(
					UV0IDs[Right0], UV0IDs[Left1], UV0IDs[Right1]));
				UV1Overlay->SetTriangle(TriangleBID, FIndex3i(
					UV1IDs[Right0], UV1IDs[Left1], UV1IDs[Right1]));
				UV2Overlay->SetTriangle(TriangleBID, FIndex3i(
					UV2IDs[Right0], UV2IDs[Left1], UV2IDs[Right1]));
				ColorOverlay->SetTriangle(TriangleBID, FIndex3i(
					ColorIDs[Right0], ColorIDs[Left1], ColorIDs[Right1]));
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
