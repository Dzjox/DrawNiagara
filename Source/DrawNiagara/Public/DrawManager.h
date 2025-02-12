// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "DrawManager.generated.h"

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

protected:
	UPROPERTY()
	bool IsErasingMod = false ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinMovement = 0.1f;

	UPROPERTY()
	TArray<USphereComponent*>  DrawingSpheres;

	UPROPERTY()
	TArray<FVector> PrevLocation;

	UPROPERTY()
	TArray<FVector4> BufferDrawingPositionsAndRadius;

	UPROPERTY()
	TArray<FVector> BufferDrawingDirections;
	
	UFUNCTION()
	void Drawing();

	UFUNCTION()
	void DrawFromBuffer();

	UFUNCTION()
	void AsyncFindPointsBetweenLocationsWithDistance(FVector End, FVector Start, float DistanceBetween, float DrawRadius);
	
	UFUNCTION()
	static void FindPointsBetweenLocationsWithDistance(const FVector& End, const FVector& Start, float DistanceBetween,
	                                                   TArray<FVector>& OutPoints, FVector& OutDirection);
	
};
