// Fill out your copyright notice in the Description page of Project Settings.


#include "DrawManager.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"


class USceneComponent;

ADrawManager::ADrawManager()
{
	PrimaryActorTick.bCanEverTick = true;
	
	NS_DrawSolver = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
}

void ADrawManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DrawFromBuffer();
}

void ADrawManager::Draw()
{
	if (IsErasingMod)
	{
		NS_DrawSolver->SetVariableBool("Eraser", false);
		IsErasingMod = false;
	}

	Drawing();
}

void ADrawManager::Erase()
{
	if (!IsErasingMod)
	{
		NS_DrawSolver->SetVariableBool("Eraser", true);
		IsErasingMod = true;
	}

	Drawing();
}

void ADrawManager::SetSphereComponentsToFollow(TArray<USphereComponent*> NewDrawingSpheres)
{
	DrawingSpheres = NewDrawingSpheres;
	BufferDrawingPositionsAndRadius.Empty();
	BufferDrawingDirections.Empty();
	PrevLocation.SetNum(NewDrawingSpheres.Num());
}

void ADrawManager::NewStart()
{
	PrevLocation.Empty();
	for (USphereComponent* Sphere : DrawingSpheres)
	{
		PrevLocation.Add(Sphere->GetComponentLocation());
	}
}

void ADrawManager::ResetNiagara()
{
	NS_DrawSolver->ResetSystem();
}

void ADrawManager::ClearCanvas()
{
	NS_DrawSolver->SetBoolParameter("ClearCanvas", true);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(NS_DrawSolver,
	"DrawingLocationsAndRadiuses", TArray<FVector4>());
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NS_DrawSolver,
	"DrawingDirections", TArray<FVector>());
	BufferDrawingPositionsAndRadius.Empty();
	BufferDrawingDirections.Empty();
	PrevLocation.SetNum(DrawingSpheres.Num());
}

void ADrawManager::SetGridLocation(FVector NewGridLocation)
{
	NS_DrawSolver->SetVectorParameter("GridLocation", NewGridLocation);
}

void ADrawManager::Drawing()
{
	for (int i=0; i < DrawingSpheres.Num(); i++)
	{
		if (PrevLocation[i].IsZero())
		{
			PrevLocation[i] = DrawingSpheres[i]->GetComponentLocation();
		}
		else
		{
			if(UKismetMathLibrary::VSizeSquared(DrawingSpheres[i]->GetComponentLocation() - PrevLocation[i]) > MinMovement)
			{
				AsyncFindPointsBetweenLocationsWithDistance(DrawingSpheres[i]->GetComponentLocation(),
					PrevLocation[i], DistanceBetweenDraws, DrawingSpheres[i]->GetUnscaledSphereRadius());
				
				PrevLocation[i] = DrawingSpheres[i]->GetComponentLocation();
			}
		}
	}
}

void ADrawManager::DrawFromBuffer()
{
	int DrawsCount = BufferDrawingPositionsAndRadius.Num() > NiagaraDrawsPerTick ?
		NiagaraDrawsPerTick : BufferDrawingPositionsAndRadius.Num();

	if (DrawsCount == 0)
	{
		return;
	}
	
	TArray<FVector4> DrawingPositions;
	TArray<FVector> Directions;

	for (int i = 0; i < DrawsCount; i++)
	{
		DrawingPositions.Add(BufferDrawingPositionsAndRadius[i]);
		Directions.Add(BufferDrawingDirections[i]);
	}

	BufferDrawingPositionsAndRadius.RemoveAtSwap(0,DrawsCount, true);
	BufferDrawingDirections.RemoveAtSwap(0,DrawsCount, true);
	
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(NS_DrawSolver,
	"DrawingLocationsAndRadiuses", DrawingPositions);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NS_DrawSolver,
	"DrawingDirections", Directions);

	NS_DrawSolver->SetFloatParameter("EraseStrength", EraseStrength);
}


void ADrawManager::AsyncFindPointsBetweenLocationsWithDistance(FVector End, FVector Start, float DistanceBetween, float DrawRadius)
{
	AsyncTask(ENamedThreads::AnyThread, [this, End, Start, DistanceBetween, DrawRadius]()
	{
		TArray<FVector> Points;
		FVector Direction;
		FindPointsBetweenLocationsWithDistance(End, Start, DistanceBetween, Points, Direction);
					
		AsyncTask(ENamedThreads::GameThread, [this, Points, Direction, DrawRadius]()
		{
			for (int j=0; j < Points.Num(); j++)
			{
				BufferDrawingPositionsAndRadius.Add( FVector4(Points[j].X, Points[j].Y, Points[j].Z,DrawRadius));
				BufferDrawingDirections.Add(Direction);
			}
		});
	});
}

void ADrawManager::FindPointsBetweenLocationsWithDistance(const FVector& End, const FVector& Start, float DistanceBetween,
                                                          TArray<FVector>& OutPoints, FVector& OutDirection)
{
	OutPoints.Empty();

	const FVector Shift = End - Start;
	const float Length = UKismetMathLibrary::VSize(Shift);
	const int InteractionsCount = UKismetMathLibrary::FFloor( Length / DistanceBetween );
	OutDirection = FVector(Shift.X / Length, Shift.Y / Length, Shift.Z / Length);

	for (int i = 0; i <= InteractionsCount; i++)
	{
		OutPoints.Add(Start + i * OutDirection * DistanceBetween);
	}
}
