// Fill out your copyright notice in the Description page of Project Settings.


#include "DrawManager.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"


class USceneComponent;

ADrawManager::ADrawManager()
{
	NS_DrawSolver = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
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
	DrawingPositionsAndRadius.Empty();
	DrawingDirections.Empty();
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
	DrawingDirections.Empty();
	PrevLocation.SetNum(DrawingSpheres.Num());
}

void ADrawManager::SetGridLocation(FVector NewGridLocation)
{
	GridLocation = NewGridLocation;
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
				TArray<FVector> Points;
				FVector Direction;
				FindPointsBetweenLocationsWithDistance(DrawingSpheres[i]->GetComponentLocation(),
					PrevLocation[i], DistanceBetweenDraws, Points, Direction);

				for (int j=0; j < Points.Num(); j++)
				{
					DrawingPositionsAndRadius.Add( FVector4(
						Points[j].X, Points[j].Y, Points[j].Z,DrawingSpheres[i]->GetUnscaledSphereRadius()));

					DrawingDirections.Add(Direction);
				}

				PrevLocation[i] = DrawingSpheres[i]->GetComponentLocation();
			}
		}
	}

	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(NS_DrawSolver,
		"DrawingLocationsAndRadiuses", DrawingPositionsAndRadius);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NS_DrawSolver,
	"DrawingDirections", DrawingDirections);

	NS_DrawSolver->SetVectorParameter("GridLocation", GridLocation);
	NS_DrawSolver->SetFloatParameter("EraseStrength", EraseStrength);

	DrawingPositionsAndRadius.Empty();
	DrawingDirections.Empty();
}

void ADrawManager::FindPointsBetweenLocationsWithDistance(FVector End, FVector Start, float DistanceBetween,
	TArray<FVector>& OutPoints, FVector& OutDirection)
{
	OutPoints.Empty();
	
	FVector Shift = End - Start;
	float Length = UKismetMathLibrary::VSize(Shift);
	int InteractionsCount = UKismetMathLibrary::FFloor( Length / DistanceBetween );
	OutDirection = FVector(Shift.X / Length, Shift.Y / Length, Shift.Z / Length);

	for (int i = 0; i <= InteractionsCount; i++)
	{
		OutPoints.Add(Start + i * OutDirection * DistanceBetween);
	}
}
