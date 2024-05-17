// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "TagGameGameMode.h"

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	GoToPlayer = MakeShared<FAivState>(
		[](AAIController* AIController) {
			AIController->MoveToActor(AIController->GetWorld()->GetFirstPlayerController()->GetPawn(), 100.0f);
		},
		nullptr,
		[this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> {
			EPathFollowingStatus::Type State = AIController->GetMoveStatus();

			if (State == EPathFollowingStatus::Moving)
			{
				return nullptr;
			}

			if (BestBall)
			{
				BestBall->AttachToActor(AIController->GetWorld()->GetFirstPlayerController()->GetPawn(), FAttachmentTransformRules::KeepRelativeTransform);
				BestBall->SetActorRelativeLocation(FVector(0, 0, 0));
				BestBall = nullptr;
			}

			return SearchForBall;
		}
	);

	SearchForBall = MakeShared<FAivState>(
		[this](AAIController* AIController) {
			AGameModeBase* GameMode = AIController->GetWorld()->GetAuthGameMode();
			ATagGameGameMode* AIGameMode = Cast<ATagGameGameMode>(GameMode);
			const TArray<ABall*>& BallsList = AIGameMode->GetBalls();

			ABall* NearestBall = nullptr;

			for (int32 i = 0; i < BallsList.Num(); i++)
			{
				if (!BallsList[i]->GetAttachParentActor() &&
					(!NearestBall ||
						FVector::Distance(AIController->GetPawn()->GetActorLocation(), BallsList[i]->GetActorLocation()) <
						FVector::Distance(AIController->GetPawn()->GetActorLocation(), NearestBall->GetActorLocation())))
				{
					NearestBall = BallsList[i];
				}
			}

			BestBall = NearestBall;
		},
		[this](AAIController* AIController)
		{
			CurrentState->CallEnter(this);
		},
		[this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> {
			if (BestBall)
			{
				return GoToBall;
			}
			CurrentState->CallExit(this);
			return SearchForBall;
			
		}
	);

	GoToBall = MakeShared<FAivState>(
		[this](AAIController* AIController) {
			AIController->MoveToActor(BestBall, 100.0f);
		},
		nullptr,
		[this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> {
			EPathFollowingStatus::Type State = AIController->GetMoveStatus();

			if (State == EPathFollowingStatus::Moving)
			{
				return nullptr;
			}
			if (BestBall->GetAttachParentActor())
			{
				return SearchForBall;
			}
			return GrabBall;
		}
	);

	GrabBall = MakeShared<FAivState>(
		[this](AAIController* AIController)
		{
			if (BestBall->GetAttachParentActor())
			{
				BestBall = nullptr;
			}
		},
		nullptr,
		[this](AAIController* AIController, const float DeltaTime) -> TSharedPtr<FAivState> {

			if (!BestBall)
			{
				return SearchForBall;
			}

			BestBall->AttachToActor(AIController->GetPawn(), FAttachmentTransformRules::KeepRelativeTransform);
			BestBall->SetActorRelativeLocation(FVector(0, 0, 0));

			if (BestBall->GetAttachParentActor() != this->GetPawn())
			{
				return SearchForBall;
			}

			return GoToPlayer;
		}
	);

	CurrentState = SearchForBall;
	CurrentState->CallEnter(this);
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (CurrentState)
	{
		CurrentState = CurrentState->CallTick(this, DeltaTime);
	}
}