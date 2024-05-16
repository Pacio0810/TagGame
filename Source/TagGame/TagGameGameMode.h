// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Ball.h"
#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/GameModeBase.h"
#include "TagGameGameMode.generated.h"

UCLASS(minimalapi)
class ATagGameGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATagGameGameMode();
	void BeginPlay() override;
	void Tick(const float DeltaTime) override;

protected:
	TArray<ATargetPoint*> TargetPointArray;
	TArray<ABall*> GameBallArray;
	void ResetMatch();

public:
	const TArray<ABall*>& GetBalls() const;
};



