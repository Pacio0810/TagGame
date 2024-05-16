// Copyright Epic Games, Inc. All Rights Reserved.

#include "TagGameGameMode.h"
#include "TagGameCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

ATagGameGameMode::ATagGameGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void ATagGameGameMode::BeginPlay()
{
	Super::BeginPlay();

	ResetMatch();
}

void ATagGameGameMode::ResetMatch()
{
	TargetPointArray.Empty();

	for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It)
	{
		TargetPointArray.Add(*It);
	}

	GameBallArray.Empty();

	for (TActorIterator<ABall> It(GetWorld()); It; ++It)
	{
		GameBallArray.Add(*It);
	}

	TArray<ATargetPoint*> RandomTargetPoints = TargetPointArray;

	for (int32 i = 0; i < GameBallArray.Num(); i++)
	{
		const int32 RandomIndex = FMath::RandRange(0, RandomTargetPoints.Num() - 1);
		GameBallArray[i]->SetActorLocation(RandomTargetPoints[RandomIndex]->GetActorLocation());
		RandomTargetPoints.RemoveAt(RandomIndex);
	}
}

const TArray<ABall*>& ATagGameGameMode::GetBalls() const
{
	return GameBallArray;
}
