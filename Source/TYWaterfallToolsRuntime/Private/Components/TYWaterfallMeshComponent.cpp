// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/TYWaterfallMeshComponent.h"

#include "Components/TYWaterfallPathComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

using namespace UE::Geometry;

namespace
{
struct FWaterfallMeshAttributes
{
	FDynamicMeshNormalOverlay* Normals = nullptr;
	FDynamicMeshUVOverlay* UV0 = nullptr;
	FDynamicMeshUVOverlay* UV1 = nullptr;
	FDynamicMeshUVOverlay* UV2 = nullptr;
	FDynamicMeshColorOverlay* Colors = nullptr;
};

FWaterfallMeshAttributes InitializeAttributes(FDynamicMesh3& Mesh)
{
	Mesh.EnableAttributes();
	Mesh.Attributes()->SetNumUVLayers(3);
	Mesh.Attributes()->EnablePrimaryColors();
	return {
		Mesh.Attributes()->PrimaryNormals(),
		Mesh.Attributes()->GetUVLayer(0),
		Mesh.Attributes()->GetUVLayer(1),
		Mesh.Attributes()->GetUVLayer(2),
		Mesh.Attributes()->PrimaryColors()
	};
}

void SetTriangleAttributes(
	const FWaterfallMeshAttributes& Attributes,
	int32 TriangleID,
	const FIndex3i& Corners,
	const TArray<int32>& NormalIDs,
	const TArray<int32>& UV0IDs,
	const TArray<int32>& UV1IDs,
	const TArray<int32>& UV2IDs,
	const TArray<int32>& ColorIDs)
{
	if (TriangleID < 0)
	{
		return;
	}

	auto Remap = [&Corners](const TArray<int32>& IDs)
	{
		return FIndex3i(IDs[Corners.A], IDs[Corners.B], IDs[Corners.C]);
	};

	Attributes.Normals->SetTriangle(TriangleID, Remap(NormalIDs));
	Attributes.UV0->SetTriangle(TriangleID, Remap(UV0IDs));
	Attributes.UV1->SetTriangle(TriangleID, Remap(UV1IDs));
	Attributes.UV2->SetTriangle(TriangleID, Remap(UV2IDs));
	Attributes.Colors->SetTriangle(TriangleID, Remap(ColorIDs));
}

bool AppendRibbon(
	FDynamicMesh3& Mesh,
	const FWaterfallMeshAttributes& Attributes,
	const TArray<FTYWaterfallSample>& Samples,
	const FTransform& WorldToMesh,
	FVector WorldWidthAxis,
	float Width,
	float UVLength,
	float RotationDegrees)
{
	if (Samples.Num() < 2 || Samples.Last().Distance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const int32 SegmentCount = Samples.Num() - 1;
	TArray<int32> VertexIDs;
	TArray<int32> NormalIDs;
	TArray<int32> UV0IDs;
	TArray<int32> UV1IDs;
	TArray<int32> UV2IDs;
	TArray<int32> ColorIDs;
	VertexIDs.Reserve(Samples.Num() * 2);
	NormalIDs.Reserve(Samples.Num() * 2);
	UV0IDs.Reserve(Samples.Num() * 2);
	UV1IDs.Reserve(Samples.Num() * 2);
	UV2IDs.Reserve(Samples.Num() * 2);
	ColorIDs.Reserve(Samples.Num() * 2);

	for (const FTYWaterfallSample& Sample : Samples)
	{
		const FVector WorldTangent = Sample.Tangent.GetSafeNormal();
		FVector WorldAcross = FVector::VectorPlaneProject(
			WorldWidthAxis, WorldTangent).GetSafeNormal();
		if (WorldAcross.IsNearlyZero())
		{
			WorldAcross = FVector::CrossProduct(
				WorldTangent, FVector::UpVector).GetSafeNormal();
		}
		if (WorldAcross.IsNearlyZero())
		{
			WorldAcross = FVector::RightVector;
		}

		// Cross mode rotates a second copy around the local flow direction. Both
		// copies retain independent vertices so each plane has a stable normal.
		WorldAcross = WorldAcross.RotateAngleAxis(RotationDegrees, WorldTangent);
		const FVector WorldNormal = FVector::CrossProduct(
			WorldAcross, WorldTangent).GetSafeNormal();
		const FVector HalfWidthOffset = WorldAcross * (Width * 0.5f);
		const FVector LocalLeft = WorldToMesh.TransformPosition(
			Sample.Position - HalfWidthOffset);
		const FVector LocalRight = WorldToMesh.TransformPosition(
			Sample.Position + HalfWidthOffset);
		const FVector3f LocalNormal = FVector3f(
			WorldToMesh.TransformVectorNoScale(WorldNormal).GetSafeNormal());

		VertexIDs.Add(Mesh.AppendVertex(FVector3d(LocalLeft)));
		VertexIDs.Add(Mesh.AppendVertex(FVector3d(LocalRight)));
		NormalIDs.Add(Attributes.Normals->AppendElement(LocalNormal));
		NormalIDs.Add(Attributes.Normals->AppendElement(LocalNormal));
		UV0IDs.Add(Attributes.UV0->AppendElement(FVector2f(
			0.0f, Sample.Distance / UVLength)));
		UV0IDs.Add(Attributes.UV0->AppendElement(FVector2f(
			1.0f, Sample.Distance / UVLength)));

		const FVector2f UV1(Sample.Distance, Sample.NormalizedDistance);
		const FVector2f UV2(Sample.Speed / 1000.0f, Sample.Turbulence);
		const FVector4f Color(
			Sample.Turbulence, Sample.Impact, Sample.RandomValue, 1.0f);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			UV1IDs.Add(Attributes.UV1->AppendElement(UV1));
			UV2IDs.Add(Attributes.UV2->AppendElement(UV2));
			ColorIDs.Add(Attributes.Colors->AppendElement(Color));
		}
	}

	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const int32 Left0 = SegmentIndex * 2;
		const int32 Right0 = Left0 + 1;
		const int32 Left1 = Left0 + 2;
		const int32 Right1 = Left0 + 3;
		const FIndex3i CornersA(Left0, Left1, Right0);
		const FIndex3i CornersB(Right0, Left1, Right1);
		const int32 TriangleA = Mesh.AppendTriangle(
			VertexIDs[CornersA.A], VertexIDs[CornersA.B], VertexIDs[CornersA.C]);
		const int32 TriangleB = Mesh.AppendTriangle(
			VertexIDs[CornersB.A], VertexIDs[CornersB.B], VertexIDs[CornersB.C]);
		SetTriangleAttributes(Attributes, TriangleA, CornersA,
			NormalIDs, UV0IDs, UV1IDs, UV2IDs, ColorIDs);
		SetTriangleAttributes(Attributes, TriangleB, CornersB,
			NormalIDs, UV0IDs, UV1IDs, UV2IDs, ColorIDs);
	}

	return true;
}

bool AppendSplash(
	FDynamicMesh3& Mesh,
	const FWaterfallMeshAttributes& Attributes,
	const FTYWaterfallSample& Endpoint,
	const FTransform& WorldToMesh,
	FVector WorldWidthAxis,
	float FrontRadius,
	float BackRadius,
	int32 RadialSegments,
	int32 Rings)
{
	FVector WorldNormal = Endpoint.Normal.GetSafeNormal();
	if (WorldNormal.IsNearlyZero())
	{
		WorldNormal = FVector::UpVector;
	}

	// The terminal tangent determines the long axis of the splash. Projecting it
	// onto the contact plane keeps the mesh flat even when the path ends steeply.
	FVector WorldForward = FVector::VectorPlaneProject(
		Endpoint.Tangent, WorldNormal).GetSafeNormal();
	if (WorldForward.IsNearlyZero())
	{
		WorldForward = FVector::VectorPlaneProject(
			WorldWidthAxis, WorldNormal).GetSafeNormal();
	}
	if (WorldForward.IsNearlyZero())
	{
		WorldForward = FVector::ForwardVector;
	}
	FVector WorldAcross = FVector::CrossProduct(
		WorldNormal, WorldForward).GetSafeNormal();
	if (WorldAcross.IsNearlyZero())
	{
		return false;
	}
	WorldForward = FVector::CrossProduct(WorldAcross, WorldNormal).GetSafeNormal();

	const float HalfLongRadius = (FrontRadius + BackRadius) * 0.5f;
	const float CenterOffset = (FrontRadius - BackRadius) * 0.5f;
	const float SideRadius = HalfLongRadius;
	const FVector WorldCenter = Endpoint.Position + WorldNormal;
	const FVector3f LocalNormal = FVector3f(
		WorldToMesh.TransformVectorNoScale(WorldNormal).GetSafeNormal());
	const FVector2f SharedUV1(Endpoint.Distance, Endpoint.NormalizedDistance);
	const FVector2f SharedUV2(Endpoint.Speed / 1000.0f, Endpoint.Turbulence);
	const FVector4f SharedColor(
		Endpoint.Turbulence, Endpoint.Impact, Endpoint.RandomValue, 1.0f);

	TArray<int32> VertexIDs;
	TArray<int32> NormalIDs;
	TArray<int32> UV0IDs;
	TArray<int32> UV1IDs;
	TArray<int32> UV2IDs;
	TArray<int32> ColorIDs;
	const int32 VertexCount = 1 + Rings * RadialSegments;
	VertexIDs.Reserve(VertexCount);
	NormalIDs.Reserve(VertexCount);
	UV0IDs.Reserve(VertexCount);
	UV1IDs.Reserve(VertexCount);
	UV2IDs.Reserve(VertexCount);
	ColorIDs.Reserve(VertexCount);

	auto AppendVertex = [&](const FVector& WorldPosition, const FVector2f& UV0)
	{
		VertexIDs.Add(Mesh.AppendVertex(FVector3d(
			WorldToMesh.TransformPosition(WorldPosition))));
		NormalIDs.Add(Attributes.Normals->AppendElement(LocalNormal));
		UV0IDs.Add(Attributes.UV0->AppendElement(UV0));
		UV1IDs.Add(Attributes.UV1->AppendElement(SharedUV1));
		UV2IDs.Add(Attributes.UV2->AppendElement(SharedUV2));
		ColorIDs.Add(Attributes.Colors->AppendElement(SharedColor));
	};

	AppendVertex(WorldCenter, FVector2f(0.5f, 0.5f));
	for (int32 RingIndex = 1; RingIndex <= Rings; ++RingIndex)
	{
		const float RingAlpha = static_cast<float>(RingIndex) / Rings;
		for (int32 SegmentIndex = 0; SegmentIndex < RadialSegments; ++SegmentIndex)
		{
			const float Angle = UE_TWO_PI * static_cast<float>(SegmentIndex)
				/ RadialSegments;
			const float CosAngle = FMath::Cos(Angle);
			const float SinAngle = FMath::Sin(Angle);
			const FVector RadialOffset =
				WorldForward * (CenterOffset + CosAngle * HalfLongRadius)
				+ WorldAcross * (SinAngle * SideRadius);
			const FVector2f UV0(
				0.5f + CosAngle * RingAlpha * 0.5f,
				0.5f + SinAngle * RingAlpha * 0.5f);
			AppendVertex(WorldCenter + RadialOffset * RingAlpha, UV0);
		}
	}

	// Match the visible winding already validated for the per-path ribbons while
	// keeping the authored overlay normal pointed away from the contact surface.
	for (int32 SegmentIndex = 0; SegmentIndex < RadialSegments; ++SegmentIndex)
	{
		const int32 Current = 1 + SegmentIndex;
		const int32 Next = 1 + (SegmentIndex + 1) % RadialSegments;
		const FIndex3i Corners(0, Next, Current);
		const int32 TriangleID = Mesh.AppendTriangle(
			VertexIDs[Corners.A], VertexIDs[Corners.B], VertexIDs[Corners.C]);
		SetTriangleAttributes(Attributes, TriangleID, Corners,
			NormalIDs, UV0IDs, UV1IDs, UV2IDs, ColorIDs);
	}

	for (int32 RingIndex = 1; RingIndex < Rings; ++RingIndex)
	{
		const int32 InnerStart = 1 + (RingIndex - 1) * RadialSegments;
		const int32 OuterStart = 1 + RingIndex * RadialSegments;
		for (int32 SegmentIndex = 0; SegmentIndex < RadialSegments; ++SegmentIndex)
		{
			const int32 NextSegment = (SegmentIndex + 1) % RadialSegments;
			const int32 InnerCurrent = InnerStart + SegmentIndex;
			const int32 InnerNext = InnerStart + NextSegment;
			const int32 OuterCurrent = OuterStart + SegmentIndex;
			const int32 OuterNext = OuterStart + NextSegment;
			const FIndex3i CornersA(InnerCurrent, OuterNext, OuterCurrent);
			const FIndex3i CornersB(InnerCurrent, InnerNext, OuterNext);
			const int32 TriangleA = Mesh.AppendTriangle(
				VertexIDs[CornersA.A], VertexIDs[CornersA.B], VertexIDs[CornersA.C]);
			const int32 TriangleB = Mesh.AppendTriangle(
				VertexIDs[CornersB.A], VertexIDs[CornersB.B], VertexIDs[CornersB.C]);
			SetTriangleAttributes(Attributes, TriangleA, CornersA,
				NormalIDs, UV0IDs, UV1IDs, UV2IDs, ColorIDs);
			SetTriangleAttributes(Attributes, TriangleB, CornersB,
				NormalIDs, UV0IDs, UV1IDs, UV2IDs, ColorIDs);
		}
	}

	return true;
}
}

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
bool UTYWaterfallMeshComponent::BuildCombinedMesh(
	const TArray<TObjectPtr<UTYWaterfallPathComponent>>& Paths,
	FVector WorldWidthAxis,
	float RibbonWidth,
	float CrossWidth,
	float UVLength,
	float FrontRadius,
	float BackRadius,
	int32 RadialSegments,
	int32 Rings)
{
	WorldWidthAxis = WorldWidthAxis.GetSafeNormal();
	if (WorldWidthAxis.IsNearlyZero())
	{
		return false;
	}

	FDynamicMesh3 NewMesh;
	const FWaterfallMeshAttributes Attributes = InitializeAttributes(NewMesh);
	const FTransform WorldToMesh = GetComponentTransform().Inverse();
	const float SafeRibbonWidth = FMath::Max(RibbonWidth, 1.0f);
	const float SafeCrossWidth = FMath::Max(CrossWidth, 1.0f);
	const float SafeUVLength = FMath::Max(UVLength, 1.0f);
	const float SafeFrontRadius = FMath::Max(FrontRadius, 1.0f);
	const float SafeBackRadius = FMath::Max(BackRadius, 1.0f);
	const int32 SafeRadialSegments = FMath::Clamp(RadialSegments, 3, 128);
	const int32 SafeRings = FMath::Clamp(Rings, 1, 32);
	int32 BuiltSurfaceCount = 0;

	// Pass 1 builds the validated water-surface ribbon for every path.
	for (const UTYWaterfallPathComponent* Path : Paths)
	{
		if (!IsValid(Path))
		{
			continue;
		}

		const TArray<FTYWaterfallSample>& Samples = Path->GetResampledSamples();
		BuiltSurfaceCount += AppendRibbon(NewMesh, Attributes, Samples,
			WorldToMesh, WorldWidthAxis, SafeRibbonWidth, SafeUVLength, 0.0f) ? 1 : 0;
	}

	// Pass 2 adds only the perpendicular plane. Re-adding the zero-degree ribbon
	// here would overlap Per Path exactly and cause depth flicker.
	for (const UTYWaterfallPathComponent* Path : Paths)
	{
		if (IsValid(Path))
		{
			BuiltSurfaceCount += AppendRibbon(NewMesh, Attributes,
				Path->GetResampledSamples(), WorldToMesh, WorldWidthAxis,
				SafeCrossWidth, SafeUVLength, 90.0f) ? 1 : 0;
		}
	}

	// Pass 3 finishes the combined mesh with one radial splash per endpoint.
	for (const UTYWaterfallPathComponent* Path : Paths)
	{
		if (!IsValid(Path) || Path->GetResampledSamples().IsEmpty())
		{
			continue;
		}

		BuiltSurfaceCount += AppendSplash(NewMesh, Attributes,
			Path->GetResampledSamples().Last(), WorldToMesh, WorldWidthAxis,
			SafeFrontRadius, SafeBackRadius, SafeRadialSegments, SafeRings) ? 1 : 0;
	}

	SetMesh(MoveTemp(NewMesh));
	SetVisibility(BuiltSurfaceCount > 0);
	MarkPackageDirty();
	return BuiltSurfaceCount > 0;
}

void UTYWaterfallMeshComponent::ClearWaterfallMesh()
{
	FDynamicMesh3 EmptyMesh;
	SetMesh(MoveTemp(EmptyMesh));
	SetVisibility(false);
	MarkPackageDirty();
}
#endif
