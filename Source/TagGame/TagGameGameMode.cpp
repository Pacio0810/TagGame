// Copyright Epic Games, Inc. All Rights Reserved.

#include "TagGameGameMode.h"
#include "TagGameCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

ATagGameGameMode::ATagGameGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
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
		if (!It->ActorHasTag("Cover"))
		{
			TargetPointArray.Add(*It);
		}
	}

	GameBallArray.Empty();

	for (TActorIterator<ABall> It(GetWorld()); It; ++It)
	{
		if (It->GetAttachParentActor())
		{
			It->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
		GameBallArray.Add(*It);
	}

	TArray<ATargetPoint*> RandomTargetPoints = TargetPointArray;

	for (int32 i = 0; i < GameBallArray.Num(); i++)
	{
		const int32 RandomIndex = FMath::RandRange(0, RandomTargetPoints.Num() - 1);
		GameBallArray[i]->SetActorLocation(RandomTargetPoints[RandomIndex]->GetActorLocation());
		RandomTargetPoints.RemoveAt(RandomIndex);
	}

	CoverPoints.Empty();

	for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag("Cover"))
		{
			CoverPoints.Add(*It);
		}
	}
}

const TArray<ABall*>& ATagGameGameMode::GetBalls() const
{
	return GameBallArray;
}

const TArray<ATargetPoint*>& ATagGameGameMode::GetCoverPoints() const
{
	return CoverPoints;
}

void ATagGameGameMode::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	for (int32 i = 0; i < GameBallArray.Num(); i++)
	{
		if (GameBallArray[i]->GetAttachParentActor() != GetWorld()->GetFirstPlayerController()->GetPawn())
		{
			return;
		}
	}

	ResetMatch();
}
