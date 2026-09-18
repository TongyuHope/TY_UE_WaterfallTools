// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TYWaterfallActor.generated.h"

class USceneComponent;
class USplineComponent;
class UStaticMeshComponent;

/** Base actor that owns the editable waterfall authoring components. */
UCLASS(Blueprintable, meta = (DisplayName = "TY Waterfall"))
class TYWATERFALLTOOLSRUNTIME_API ATYWaterfallActor : public AActor
{
	GENERATED_BODY()

public:
	ATYWaterfallActor();

	UFUNCTION(BlueprintPure, Category = "Waterfall|Components")
	USplineComponent* GetTopSpline() const { return TopSpline; }

	UFUNCTION(BlueprintPure, Category = "Waterfall|Components")
	UStaticMeshComponent* GetBakedMeshComponent() const { return BakedMeshComponent; }

#if WITH_EDITOR
	UFUNCTION(BlueprintPure, Category = "Waterfall|Components")
	UStaticMeshComponent* GetKillPlaneComponent() const { return KillPlaneComponent; }
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> RootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> TopSpline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BakedMeshComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Editor", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> KillPlaneComponent;
#endif
};
