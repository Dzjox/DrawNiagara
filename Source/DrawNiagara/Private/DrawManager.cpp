// Fill out your copyright notice in the Description page of Project Settings.


#include "DrawManager.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

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
	NS_DrawSolver->SetBoolParameter("ClearCanvas", false);
	
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
				float TimeSinceStart = UGameplayStatics::GetTimeSeconds(GetWorld());

				TimeOfAsyncData.Enqueue(TimeSinceStart);
				AsyncData.Add(TimeSinceStart, FDrawInfo());
				
				AsyncFindPointsBetweenLocationsWithDistance(DrawingSpheres[i]->GetComponentLocation(),
					PrevLocation[i], DistanceBetweenDraws, DrawingSpheres[i]->GetUnscaledSphereRadius(), TimeSinceStart);
				
				PrevLocation[i] = DrawingSpheres[i]->GetComponentLocation();
			}
		}
	}
}

void ADrawManager::DrawFromBuffer()
{
	int DrawsCount = BufferDrawingPositionsAndRadius.Num() > NiagaraDrawsPerTick ?
		NiagaraDrawsPerTick : BufferDrawingPositionsAndRadius.Num();

	if (DrawsCount != 0)
	{
		TArray<FVector4> DrawingPositions;
		TArray<FVector> Directions;

		for (int i = 0; i < DrawsCount; i++)
		{
			DrawingPositions.Add(BufferDrawingPositionsAndRadius[i]);
			Directions.Add(BufferDrawingDirections[i]);
		}

		BufferDrawingPositionsAndRadius.RemoveAt(0,DrawsCount, true);
		BufferDrawingDirections.RemoveAt(0,DrawsCount, true);

		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector4(NS_DrawSolver,
		"DrawingLocationsAndRadiuses", DrawingPositions);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(NS_DrawSolver,
		"DrawingDirections", Directions);

		NS_DrawSolver->SetFloatParameter("EraseStrength", EraseStrength);
	}

	TryAddToBuffer();
}

void ADrawManager::TryAddToBuffer()
{
	if (AsyncData.Num() == 0)
	{
		return;
	}
	
	int AddedPoints = 0;
	while(true)
	{
		if (AsyncData.Num() == 0)
		{
			return;
		}

		if (!AsyncData.Find(*TimeOfAsyncData.Peek()))
		{
			return;
		}

		if (AsyncData.Find(*TimeOfAsyncData.Peek())->PointInfos.Num() == 0)
		{
			return;
		}

		float Time;
		TimeOfAsyncData.Dequeue(Time);
		FDrawInfo DrawInfo = AsyncData.FindAndRemoveChecked(Time);
		for (FPointInfo PointInfo : DrawInfo.PointInfos)
		{
			BufferDrawingPositionsAndRadius.Add(PointInfo.LocationAndRadius);
			BufferDrawingDirections.Add(PointInfo.Direction);
			AddedPoints ++;
		}
		
		if (AddedPoints > NiagaraDrawsPerTick)
		{
			return;
		}
	}
}


void ADrawManager::AsyncFindPointsBetweenLocationsWithDistance(FVector End, FVector Start, float DistanceBetween,
                                                               float DrawRadius, float Time)
{
	AsyncTask(ENamedThreads::AnyThread, [this, End, Start, DistanceBetween, DrawRadius, Time]()
	{
		TArray<FVector> Points;
		FVector Direction;
		FindPointsBetweenLocationsWithDistance(End, Start, DistanceBetween, Points, Direction);
					
		AsyncTask(ENamedThreads::GameThread, [this, Points, Direction, DrawRadius, Time]()
		{
			AsyncData.Find(Time)->PointInfos.SetNum(Points.Num());
			for (int j=0; j < Points.Num(); j++)
			{
				AsyncData.Find(Time)->PointInfos[j].LocationAndRadius = FVector4(Points[j].X, Points[j].Y, Points[j].Z,DrawRadius);
				AsyncData.Find(Time)->PointInfos[j].Direction = Direction;
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
