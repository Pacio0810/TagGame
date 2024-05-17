// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Ball.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnemyAIController.generated.h"

struct FAivState : public TSharedFromThis<FAivState>
{
private:
	TFunction<void()> Enter;
	TFunction<void()> Exit;
	TFunction<TSharedPtr<FAivState>(const float)> Tick;
public:

	FAivState()
	{
		Enter = nullptr;
		Exit = nullptr;
		Tick = nullptr;
	}

	FAivState(TFunction<void()> InEnter = nullptr, TFunction<void()> InExit = nullptr, TFunction<TSharedPtr<FAivState>(const float)> InTick = nullptr)
	{
		Enter = InEnter;
		Exit = InExit;
		Tick = InTick;
	}

	FAivState(const FAivState& Other) = delete;
	FAivState& operator=(const FAivState& Other) = delete;
	FAivState(FAivState&& Other) = delete;
	FAivState& operator=(FAivState&& Other) = delete;

	void CallEnter()
	{
		if (Enter)
		{
			Enter();
		}
	}

	void CallExit()
	{
		if (Exit)
		{
			Exit();
		}
	}

	TSharedPtr<FAivState> CallTick(const float DeltaTime)
	{
		if (Tick)
		{
			TSharedPtr<FAivState> NewState = Tick(DeltaTime);

			if (NewState != nullptr && NewState != AsShared())
			{
				CallExit();
				NewState->CallEnter();
				return NewState;
			}
		}

		return AsShared();
	}
};

/**
 *
 */
UCLASS()
class TAGGAME_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
public:
	AEnemyAIController();

protected:
	TSharedPtr<FAivState> CurrentState;
	TSharedPtr<FAivState> GoToPlayer;
	TSharedPtr<FAivState> GoToBall;
	TSharedPtr<FAivState> GrabBall;
	TSharedPtr<FAivState> SearchForBall;

	void BeginPlay() override;
	void Tick(float DeltaTime) override;

	//ABall* BestBall;

	UBlackboardComponent* BlackboardComponent;
	UBlackboardData* BlackboardAsset;

	void OnPossess(APawn* InPawn) override;
};
