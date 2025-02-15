// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "Containers/SortedMap.h"
#include "DrawManager.generated.h"

struct FPointInfo
{
	FVector4 LocationAndRadius;
	FVector Direction;
};

struct FDrawInfo
{
	TArray<FPointInfo> PointInfos;
};


UCLASS()
class DRAWNIAGARA_API ADrawManager : public AActor
{
	GENERATED_BODY()
	
public:
	ADrawManager();

	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNiagaraComponent* NS_DrawSolver;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DrawManager|Settings")
	float DistanceBetweenDraws = 5;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DrawManager|Settings")
	float EraseStrength = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DrawManager|Settings")
	float NiagaraDrawsPerTick = 100;
	
	UFUNCTION(BlueprintCallable)
	void Draw();

	UFUNCTION(BlueprintCallable)
	void Erase();

	UFUNCTION(BlueprintCallable)
	void SetSphereComponentsToFollow(TArray<USphereComponent*> NewDrawingSpheres);

	UFUNCTION(BlueprintCallable)
	void NewStart();

	UFUNCTION(BlueprintCallable)
	void ResetNiagara();

	UFUNCTION(BlueprintCallable)
	void ClearCanvas();

	UFUNCTION(BlueprintCallable)
	void SetGridLocation(FVector NewGridLocation);
	
	UFUNCTION(BlueprintCallable)
	void SwitchRT();

protected:
	UPROPERTY()
	TArray<USphereComponent*>  DrawingSpheres;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinMovement = 0.1f;
	
	bool IsErasingMod = false ;
	bool IsSecondRTUsed = false ;

	TArray<FVector> PrevLocation;
	TArray<FVector4> BufferDrawingPositionsAndRadius;
	TArray<FVector> BufferDrawingDirections;
	TMap<float, FDrawInfo> AsyncData;
	TQueue<float> TimeOfAsyncData;
	
	void Drawing();
	void DrawFromBuffer();
	void TryAddToBuffer();
	void AsyncFindPointsBetweenLocationsWithDistance(FVector End, FVector Start, float DistanceBetween, float DrawRadius,
		float Time);
	void FindPointsBetweenLocationsWithDistance(const FVector& End, const FVector& Start, float DistanceBetween,
	                                                   TArray<FVector>& OutPoints, FVector& OutDirection);
	
};
