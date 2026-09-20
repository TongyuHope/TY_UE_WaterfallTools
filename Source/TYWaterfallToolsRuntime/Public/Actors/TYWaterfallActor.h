// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Generation/TYWaterfallPathBuilder.h"
#include "TYWaterfallActor.generated.h"

class USceneComponent;
class USplineComponent;
class UStaticMeshComponent;
class UTYWaterfallPathComponent;
class UTYWaterfallSettingsComponent;

/** Base actor that owns the editable waterfall authoring components. */
UCLASS(Blueprintable, meta = (DisplayName = "TY Waterfall"))
class TYWATERFALLTOOLSRUNTIME_API ATYWaterfallActor : public AActor
{
	GENERATED_BODY()

public:
	ATYWaterfallActor();
	virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#endif

	UFUNCTION(BlueprintPure, Category = "Waterfall|Components")
	USplineComponent* GetTopSpline() const { return TopSpline; }

	UFUNCTION(BlueprintPure, Category = "Waterfall|Components")
	UStaticMeshComponent* GetBakedMeshComponent() const { return BakedMeshComponent; }

#if WITH_EDITOR
	/** Creates and starts frame-budgeted generation of all configured paths. */
	UFUNCTION(CallInEditor, Category = "Waterfall|Simulation")
	void GeneratePaths();

	/** Cancels generation and removes incomplete paths. */
	UFUNCTION(CallInEditor, Category = "Waterfall|Simulation")
	void CancelPathGeneration();

	/** Removes all generated path components. */
	UFUNCTION(CallInEditor, Category = "Waterfall|Simulation")
	void ClearGeneratedPaths();

	UFUNCTION(BlueprintPure, Category = "Waterfall|Simulation")
	bool IsGeneratingPaths() const { return PathBuilder.IsGenerating(); }

	UFUNCTION(BlueprintPure, Category = "Waterfall|Simulation")
	float GetPathGenerationProgress() const { return PathBuilder.GetProgress(); }
#endif

#if WITH_EDITOR
	UFUNCTION(BlueprintPure, Category = "Waterfall|Components")
	UStaticMeshComponent* GetKillPlaneComponent() const { return KillPlaneComponent; }
#endif

protected:
	friend struct FTYWaterfallPathBuilder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> RootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> TopSpline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BakedMeshComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTYWaterfallSettingsComponent> WaterfallSettings;

	UPROPERTY(VisibleAnywhere, Instanced, Category = "Generated Paths", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UTYWaterfallPathComponent>> GeneratedPaths;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Editor", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> KillPlaneComponent;
#endif

#if WITH_EDITOR
	FTYWaterfallPathBuilder PathBuilder;
#endif
};
