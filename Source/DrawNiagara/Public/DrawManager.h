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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNiagaraComponent* NS_DrawSolver;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DistanceBetweenDraws = 5;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EraseStrength = 100;
	
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
	FVector GridLocation = FVector();

	UPROPERTY()
	TArray<USphereComponent*>  DrawingSpheres;

	UPROPERTY()
	TArray<FVector> PrevLocation;

	UPROPERTY()
	TArray<FVector4> DrawingPositionsAndRadius;

	UPROPERTY()
	TArray<FVector> DrawingDirections;

	UFUNCTION()
	void Drawing();

	UFUNCTION()
	void  FindPointsBetweenLocationsWithDistance(FVector End, FVector Start, float DistanceBetween,
		TArray<FVector>& OutPoints, FVector& OutDirection);
};
