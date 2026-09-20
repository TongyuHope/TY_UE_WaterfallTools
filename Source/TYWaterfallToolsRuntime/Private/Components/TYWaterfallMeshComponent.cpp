// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/TYWaterfallMeshComponent.h"

#include "Components/TYWaterfallPathComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

using namespace UE::Geometry;

namespace
{
enum class EWaterfallMaterialSlot : int32
{
	Singular = 0,
	PerPath = 1,
	Cross = 2,
	Splash = 3
};

struct FWaterfallMeshAttributes
{
	FDynamicMeshNormalOverlay* Normals = nullptr;
	FDynamicMeshUVOverlay* UV0 = nullptr;
	FDynamicMeshUVOverlay* UV1 = nullptr;
	FDynamicMeshUVOverlay* UV2 = nullptr;
	FDynamicMeshUVOverlay* UV3 = nullptr;
	FDynamicMeshColorOverlay* Colors = nullptr;
	FDynamicMeshMaterialAttribute* MaterialIDs = nullptr;
};

FWaterfallMeshAttributes InitializeAttributes(FDynamicMesh3& Mesh)
{
	// Every surface must populate all four UV overlays, including triangle IDs.
	Mesh.EnableAttributes();
	Mesh.Attributes()->SetNumUVLayers(4);
	Mesh.Attributes()->EnablePrimaryColors();
	Mesh.Attributes()->EnableMaterialID();
	return {
		Mesh.Attributes()->PrimaryNormals(),
		Mesh.Attributes()->GetUVLayer(0),
		Mesh.Attributes()->GetUVLayer(1),
		Mesh.Attributes()->GetUVLayer(2),
		Mesh.Attributes()->GetUVLayer(3),
		Mesh.Attributes()->PrimaryColors(),
		Mesh.Attributes()->GetMaterialID()
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
	const TArray<int32>& UV3IDs,
	const TArray<int32>& ColorIDs,
	EWaterfallMaterialSlot MaterialSlot)
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
	Attributes.UV3->SetTriangle(TriangleID, Remap(UV3IDs));
	Attributes.Colors->SetTriangle(TriangleID, Remap(ColorIDs));
	Attributes.MaterialIDs->SetValue(TriangleID, static_cast<int32>(MaterialSlot));
}

FVector4f MakeWaterfallVertexColor(FVector Direction, float Turbulence)
{
	Direction = Direction.GetSafeNormal();
	const FVector EncodedDirection = (Direction + FVector::OneVector) * 0.5;
	return FVector4f(
		static_cast<float>(EncodedDirection.X),
		static_cast<float>(EncodedDirection.Y),
		static_cast<float>(EncodedDirection.Z),
		Turbulence);
}

FTYWaterfallSample InterpolateSampleAtNormalizedDistance(
	const TArray<FTYWaterfallSample>& Samples,
	float NormalizedDistance)
{
	const float TargetDistance = FMath::Clamp(NormalizedDistance, 0.0f, 1.0f);
	if (Samples.Num() == 1 || TargetDistance <= Samples[0].NormalizedDistance)
	{
		return Samples[0];
	}
	if (TargetDistance >= Samples.Last().NormalizedDistance)
	{
		return Samples.Last();
	}

	int32 UpperIndex = 1;
	while (UpperIndex < Samples.Num()
		&& Samples[UpperIndex].NormalizedDistance < TargetDistance)
	{
		++UpperIndex;
	}

	const int32 LowerIndex = FMath::Max(UpperIndex - 1, 0);
	const FTYWaterfallSample& Lower = Samples[LowerIndex];
	const FTYWaterfallSample& Upper = Samples[FMath::Min(UpperIndex, Samples.Num() - 1)];
	const float DistanceRange = Upper.NormalizedDistance - Lower.NormalizedDistance;
	const float Alpha = DistanceRange > KINDA_SMALL_NUMBER
		? (TargetDistance - Lower.NormalizedDistance) / DistanceRange
		: 0.0f;

	FTYWaterfallSample Result;
	Result.Position = FMath::Lerp(Lower.Position, Upper.Position, Alpha);
	Result.Tangent = FMath::Lerp(Lower.Tangent, Upper.Tangent, Alpha).GetSafeNormal();
	Result.Normal = FMath::Lerp(Lower.Normal, Upper.Normal, Alpha).GetSafeNormal();
	Result.Velocity = FMath::Lerp(Lower.Velocity, Upper.Velocity, Alpha);
	Result.Distance = FMath::Lerp(Lower.Distance, Upper.Distance, Alpha);
	Result.NormalizedDistance = TargetDistance;
	Result.Speed = FMath::Lerp(Lower.Speed, Upper.Speed, Alpha);
	Result.Flow = FMath::Lerp(Lower.Flow, Upper.Flow, Alpha);
	Result.Impact = FMath::Lerp(Lower.Impact, Upper.Impact, Alpha);
	Result.Turbulence = FMath::Lerp(Lower.Turbulence, Upper.Turbulence, Alpha);
	Result.RandomValue = FMath::Lerp(Lower.RandomValue, Upper.RandomValue, Alpha);
	return Result;
}

bool AppendSingular(
	FDynamicMesh3& Mesh,
	const FWaterfallMeshAttributes& Attributes,
	const TArray<TObjectPtr<UTYWaterfallPathComponent>>& Paths,
	const FTransform& WorldToMesh,
	FVector WorldWidthAxis,
	FVector2D BaseUVScale)
{
	struct FSortedPath
	{
		const TArray<FTYWaterfallSample>* Samples = nullptr;
		float UVSeed = 0.0f;
		float TopSplinePosition = 0.0f;
		float TopSplineDistance = 0.0f;
		double WidthPosition = 0.0;
	};

	TArray<FSortedPath> SortedPaths;
	int32 RowCount = 0;
	for (const UTYWaterfallPathComponent* Path : Paths)
	{
		if (!IsValid(Path))
		{
			continue;
		}

		const TArray<FTYWaterfallSample>& Samples = Path->GetResampledSamples();
		if (Samples.Num() < 2 || Samples.Last().Distance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		SortedPaths.Add({ &Samples, Path->GetUVSeed(),
			Path->GetNormalizedTopSplinePosition(), Path->GetTopSplineDistance(),
			FVector::DotProduct(Samples[0].Position, WorldWidthAxis) });
		RowCount = FMath::Max(RowCount, Samples.Num());
	}

	if (SortedPaths.Num() < 2 || RowCount < 2)
	{
		return false;
	}

	// The reference plugin remaps paths to a shared longitudinal resolution before
	// joining them. Sorting once at the lip preserves path identity on every row,
	// which makes the topology deterministic and avoids row-to-row index swaps.
	SortedPaths.StableSort([](const FSortedPath& A, const FSortedPath& B)
	{
		return A.WidthPosition < B.WidthPosition;
	});

	const int32 PathCount = SortedPaths.Num();
	TArray<FTYWaterfallSample> GridSamples;
	GridSamples.SetNum(PathCount * RowCount);
	for (int32 PathIndex = 0; PathIndex < PathCount; ++PathIndex)
	{
		for (int32 RowIndex = 0; RowIndex < RowCount; ++RowIndex)
		{
			const float RowAlpha = static_cast<float>(RowIndex) / (RowCount - 1);
			GridSamples[PathIndex * RowCount + RowIndex] =
				InterpolateSampleAtNormalizedDistance(*SortedPaths[PathIndex].Samples, RowAlpha);
		}
	}

	const int32 VertexCount = PathCount * RowCount;
	TArray<int32> VertexIDs;
	TArray<int32> NormalIDs;
	TArray<int32> UV0IDs;
	TArray<int32> UV1IDs;
	TArray<int32> UV2IDs;
	TArray<int32> UV3IDs;
	TArray<int32> ColorIDs;
	VertexIDs.Reserve(VertexCount);
	NormalIDs.Reserve(VertexCount);
	UV0IDs.Reserve(VertexCount);
	UV1IDs.Reserve(VertexCount);
	UV2IDs.Reserve(VertexCount);
	UV3IDs.Reserve(VertexCount);
	ColorIDs.Reserve(VertexCount);

	for (int32 PathIndex = 0; PathIndex < PathCount; ++PathIndex)
	{
		const float PathAlpha = static_cast<float>(PathIndex) / (PathCount - 1);
		for (int32 RowIndex = 0; RowIndex < RowCount; ++RowIndex)
		{
			const int32 GridIndex = PathIndex * RowCount + RowIndex;
			const FTYWaterfallSample& Sample = GridSamples[GridIndex];
			const int32 PreviousPath = FMath::Max(PathIndex - 1, 0);
			const int32 NextPath = FMath::Min(PathIndex + 1, PathCount - 1);
			FVector WorldAcross = (
				GridSamples[NextPath * RowCount + RowIndex].Position
				- GridSamples[PreviousPath * RowCount + RowIndex].Position).GetSafeNormal();
			if (WorldAcross.IsNearlyZero())
			{
				WorldAcross = WorldWidthAxis;
			}

			FVector WorldTangent = Sample.Tangent.GetSafeNormal();
			if (WorldTangent.IsNearlyZero())
			{
				WorldTangent = FVector::DownVector;
			}
			FVector WorldNormal = FVector::CrossProduct(WorldAcross, WorldTangent).GetSafeNormal();
			if (WorldNormal.IsNearlyZero())
			{
				WorldNormal = Sample.Normal.GetSafeNormal();
			}
			if (WorldNormal.IsNearlyZero())
			{
				WorldNormal = FVector::UpVector;
			}

			VertexIDs.Add(Mesh.AppendVertex(FVector3d(
				WorldToMesh.TransformPosition(Sample.Position))));
			NormalIDs.Add(Attributes.Normals->AppendElement(FVector3f(
				WorldToMesh.TransformVectorNoScale(WorldNormal).GetSafeNormal())));
			UV0IDs.Add(Attributes.UV0->AppendElement(FVector2f(
				PathAlpha * BaseUVScale.X,
				Sample.NormalizedDistance * BaseUVScale.Y)));
			UV1IDs.Add(Attributes.UV1->AppendElement(FVector2f(
				SortedPaths[PathIndex].TopSplineDistance * BaseUVScale.X,
				Sample.Distance * BaseUVScale.Y)));
			UV2IDs.Add(Attributes.UV2->AppendElement(FVector2f(
				Sample.Speed, Sample.Flow * BaseUVScale.Y)));
			UV3IDs.Add(Attributes.UV3->AppendElement(FVector2f(
				SortedPaths[PathIndex].UVSeed, SortedPaths[PathIndex].TopSplinePosition)));
			ColorIDs.Add(Attributes.Colors->AppendElement(
				MakeWaterfallVertexColor(Sample.Velocity, Sample.Turbulence)));
		}
	}

	// Use the same winding as the validated Per-Path ribbon: increasing path
	// index is left-to-right, while increasing row index follows the water flow.
	for (int32 PathIndex = 0; PathIndex < PathCount - 1; ++PathIndex)
	{
		for (int32 RowIndex = 0; RowIndex < RowCount - 1; ++RowIndex)
		{
			const int32 Left0 = PathIndex * RowCount + RowIndex;
			const int32 Left1 = Left0 + 1;
			const int32 Right0 = (PathIndex + 1) * RowCount + RowIndex;
			const int32 Right1 = Right0 + 1;
			const FIndex3i CornersA(Left0, Left1, Right0);
			const FIndex3i CornersB(Right0, Left1, Right1);
			const int32 TriangleA = Mesh.AppendTriangle(
				VertexIDs[CornersA.A], VertexIDs[CornersA.B], VertexIDs[CornersA.C]);
			const int32 TriangleB = Mesh.AppendTriangle(
				VertexIDs[CornersB.A], VertexIDs[CornersB.B], VertexIDs[CornersB.C]);
			SetTriangleAttributes(Attributes, TriangleA, CornersA,
				NormalIDs, UV0IDs, UV1IDs, UV2IDs, UV3IDs, ColorIDs,
				EWaterfallMaterialSlot::Singular);
			SetTriangleAttributes(Attributes, TriangleB, CornersB,
				NormalIDs, UV0IDs, UV1IDs, UV2IDs, UV3IDs, ColorIDs,
				EWaterfallMaterialSlot::Singular);
		}
	}

	return true;
}

bool AppendRibbon(
	FDynamicMesh3& Mesh,
	const FWaterfallMeshAttributes& Attributes,
	const TArray<FTYWaterfallSample>& Samples,
	const FTransform& WorldToMesh,
	FVector WorldWidthAxis,
	float Width,
	int32 Subdivisions,
	FVector2D BaseUVScale,
	float RotationDegrees,
	float UVSeed,
	float TopSplinePosition,
	EWaterfallMaterialSlot MaterialSlot)
{
	if (Samples.Num() < 2 || Samples.Last().Distance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const int32 SegmentCount = Samples.Num() - 1;
	const int32 SafeSubdivisions = FMath::Clamp(Subdivisions, 0, 32);
	const int32 RowVertexCount = SafeSubdivisions + 2;
	TArray<int32> VertexIDs;
	TArray<int32> NormalIDs;
	TArray<int32> UV0IDs;
	TArray<int32> UV1IDs;
	TArray<int32> UV2IDs;
	TArray<int32> UV3IDs;
	TArray<int32> ColorIDs;
	const int32 VertexCount = Samples.Num() * RowVertexCount;
	VertexIDs.Reserve(VertexCount);
	NormalIDs.Reserve(VertexCount);
	UV0IDs.Reserve(VertexCount);
	UV1IDs.Reserve(VertexCount);
	UV2IDs.Reserve(VertexCount);
	UV3IDs.Reserve(VertexCount);
	ColorIDs.Reserve(VertexCount);

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
		// WaterfallTools treats Per-Path width as the total width, while Cross
		// width is the distance from its centre to either side.
		const float HalfWidth = MaterialSlot == EWaterfallMaterialSlot::Cross
			? Width : Width * 0.5f;
		const FVector HalfWidthOffset = WorldAcross * HalfWidth;
		const FVector WorldLeft = Sample.Position - HalfWidthOffset;
		const FVector WorldRight = Sample.Position + HalfWidthOffset;
		const FVector3f LocalNormal = FVector3f(
			WorldToMesh.TransformVectorNoScale(WorldNormal).GetSafeNormal());

		const FVector2f UV2(Sample.Speed, Sample.Flow * BaseUVScale.Y);
		const FVector2f UV3(UVSeed, TopSplinePosition);
		const FVector4f Color = MakeWaterfallVertexColor(
			Sample.Velocity, Sample.Turbulence);
		for (int32 ColumnIndex = 0; ColumnIndex < RowVertexCount; ++ColumnIndex)
		{
			const float ColumnAlpha = static_cast<float>(ColumnIndex)
				/ static_cast<float>(RowVertexCount - 1);
			const FVector WorldPosition = FMath::Lerp(WorldLeft, WorldRight, ColumnAlpha);
			const float Across = FMath::Lerp(-HalfWidth, HalfWidth, ColumnAlpha);
			VertexIDs.Add(Mesh.AppendVertex(FVector3d(
				WorldToMesh.TransformPosition(WorldPosition))));
			NormalIDs.Add(Attributes.Normals->AppendElement(LocalNormal));
			UV0IDs.Add(Attributes.UV0->AppendElement(FVector2f(
				ColumnAlpha * BaseUVScale.X,
				Sample.NormalizedDistance * BaseUVScale.Y)));
			UV1IDs.Add(Attributes.UV1->AppendElement(FVector2f(
				Across * BaseUVScale.X, Sample.Distance * BaseUVScale.Y)));
			UV2IDs.Add(Attributes.UV2->AppendElement(UV2));
			UV3IDs.Add(Attributes.UV3->AppendElement(UV3));
			ColorIDs.Add(Attributes.Colors->AppendElement(Color));
		}
	}

	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		for (int32 ColumnIndex = 0; ColumnIndex < RowVertexCount - 1; ++ColumnIndex)
		{
			const int32 Left0 = SegmentIndex * RowVertexCount + ColumnIndex;
			const int32 Right0 = Left0 + 1;
			const int32 Left1 = Left0 + RowVertexCount;
			const int32 Right1 = Left1 + 1;
			const FIndex3i CornersA(Left0, Left1, Right0);
			const FIndex3i CornersB(Right0, Left1, Right1);
			const int32 TriangleA = Mesh.AppendTriangle(
				VertexIDs[CornersA.A], VertexIDs[CornersA.B], VertexIDs[CornersA.C]);
			const int32 TriangleB = Mesh.AppendTriangle(
				VertexIDs[CornersB.A], VertexIDs[CornersB.B], VertexIDs[CornersB.C]);
			SetTriangleAttributes(Attributes, TriangleA, CornersA,
				NormalIDs, UV0IDs, UV1IDs, UV2IDs, UV3IDs, ColorIDs, MaterialSlot);
			SetTriangleAttributes(Attributes, TriangleB, CornersB,
				NormalIDs, UV0IDs, UV1IDs, UV2IDs, UV3IDs, ColorIDs, MaterialSlot);
		}
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
	int32 Rings,
	float UVSeed,
	float TopSplinePosition)
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
	const FVector2f SharedUV2(Endpoint.Speed, Endpoint.Flow);
	const FVector2f SharedUV3(UVSeed, TopSplinePosition);

	TArray<int32> VertexIDs;
	TArray<int32> NormalIDs;
	TArray<int32> UV0IDs;
	TArray<int32> UV1IDs;
	TArray<int32> UV2IDs;
	TArray<int32> UV3IDs;
	TArray<int32> ColorIDs;
	const int32 VertexCount = 1 + Rings * RadialSegments;
	VertexIDs.Reserve(VertexCount);
	NormalIDs.Reserve(VertexCount);
	UV0IDs.Reserve(VertexCount);
	UV1IDs.Reserve(VertexCount);
	UV2IDs.Reserve(VertexCount);
	UV3IDs.Reserve(VertexCount);
	ColorIDs.Reserve(VertexCount);

	auto AppendVertex = [&](const FVector& WorldPosition, const FVector2f& UV0)
	{
		VertexIDs.Add(Mesh.AppendVertex(FVector3d(
			WorldToMesh.TransformPosition(WorldPosition))));
		NormalIDs.Add(Attributes.Normals->AppendElement(LocalNormal));
		UV0IDs.Add(Attributes.UV0->AppendElement(UV0));
		const FVector Offset = WorldPosition - WorldCenter;
		UV1IDs.Add(Attributes.UV1->AppendElement(FVector2f(
			FVector::DotProduct(Offset, WorldAcross),
			FVector::DotProduct(Offset, WorldForward))));
		UV2IDs.Add(Attributes.UV2->AppendElement(SharedUV2));
		UV3IDs.Add(Attributes.UV3->AppendElement(SharedUV3));
		const FVector ColorDirection = Offset.IsNearlyZero()
			? Endpoint.Velocity : Offset;
		ColorIDs.Add(Attributes.Colors->AppendElement(
			MakeWaterfallVertexColor(ColorDirection, Endpoint.Turbulence)));
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
			NormalIDs, UV0IDs, UV1IDs, UV2IDs, UV3IDs, ColorIDs,
			EWaterfallMaterialSlot::Splash);
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
				NormalIDs, UV0IDs, UV1IDs, UV2IDs, UV3IDs, ColorIDs,
				EWaterfallMaterialSlot::Splash);
			SetTriangleAttributes(Attributes, TriangleB, CornersB,
				NormalIDs, UV0IDs, UV1IDs, UV2IDs, UV3IDs, ColorIDs,
				EWaterfallMaterialSlot::Splash);
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
	bool bGenerateSingular,
	bool bGeneratePerPath,
	bool bGenerateCross,
	bool bGenerateSplash,
	float RibbonWidth,
	float CrossWidth,
	int32 PerPathSubdivisions,
	int32 CrossSubdivisions,
	FVector2D BaseUVScale,
	float FrontRadius,
	float BackRadius,
	int32 RadialSegments,
	int32 Rings)
{
	FDynamicMesh3 NewMesh;
	WorldWidthAxis = WorldWidthAxis.GetSafeNormal();
	if (WorldWidthAxis.IsNearlyZero())
	{
		// A failed rebuild must not leave geometry from an earlier successful run.
		SetMesh(MoveTemp(NewMesh));
		SetVisibility(false);
		MarkPackageDirty();
		return false;
	}

	const FWaterfallMeshAttributes Attributes = InitializeAttributes(NewMesh);
	const FTransform WorldToMesh = GetComponentTransform().Inverse();
	const float SafeRibbonWidth = FMath::Max(RibbonWidth, 1.0f);
	const float SafeCrossWidth = FMath::Max(CrossWidth, 1.0f);
	const float SafeFrontRadius = FMath::Max(FrontRadius, 1.0f);
	const float SafeBackRadius = FMath::Max(BackRadius, 1.0f);
	const int32 SafeRadialSegments = FMath::Clamp(RadialSegments, 3, 128);
	const int32 SafeRings = FMath::Clamp(Rings, 1, 32);
	int32 BuiltSurfaceCount = 0;

	// Singular joins all valid paths into one sheet after remapping their samples
	// to a shared row count. It can coexist with any of the per-path modes.
	if (bGenerateSingular)
	{
		BuiltSurfaceCount += AppendSingular(NewMesh, Attributes, Paths,
			WorldToMesh, WorldWidthAxis, BaseUVScale) ? 1 : 0;
	}

	if (bGeneratePerPath)
	{
		for (const UTYWaterfallPathComponent* Path : Paths)
		{
			if (!IsValid(Path))
			{
				continue;
			}

			BuiltSurfaceCount += AppendRibbon(NewMesh, Attributes,
				Path->GetResampledSamples(), WorldToMesh, WorldWidthAxis,
				SafeRibbonWidth, PerPathSubdivisions, BaseUVScale, 0.0f, Path->GetUVSeed(),
				Path->GetNormalizedTopSplinePosition(),
				EWaterfallMaterialSlot::PerPath) ? 1 : 0;
		}
	}

	if (bGenerateCross)
	{
		for (const UTYWaterfallPathComponent* Path : Paths)
		{
			if (IsValid(Path))
			{
				BuiltSurfaceCount += AppendRibbon(NewMesh, Attributes,
					Path->GetResampledSamples(), WorldToMesh, WorldWidthAxis,
					SafeCrossWidth, CrossSubdivisions, BaseUVScale, 90.0f, Path->GetUVSeed(),
					Path->GetNormalizedTopSplinePosition(),
					EWaterfallMaterialSlot::Cross) ? 1 : 0;
			}
		}
	}

	if (bGenerateSplash)
	{
		for (const UTYWaterfallPathComponent* Path : Paths)
		{
			if (!IsValid(Path) || Path->GetResampledSamples().IsEmpty())
			{
				continue;
			}

			BuiltSurfaceCount += AppendSplash(NewMesh, Attributes,
				Path->GetResampledSamples().Last(), WorldToMesh, WorldWidthAxis,
				SafeFrontRadius, SafeBackRadius, SafeRadialSegments,
				SafeRings, Path->GetUVSeed(), Path->GetNormalizedTopSplinePosition()) ? 1 : 0;
		}
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
