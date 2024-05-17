// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "TagGameGameMode.h"

AEnemyAIController::AEnemyAIController()
{
	BlackboardAsset = NewObject<UBlackboardData>();
	BlackboardAsset->UpdatePersistentKey<UBlackboardKeyType_Object>(FName("Target"));

	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	BlackboardComponent->SetValueAsObject("Target", this->GetWorld()->GetFirstPlayerController()->GetPawn());

	GoToPlayer = MakeShared<FAivState>(
		[this](AAIController* AIController) {

			AActor* Player = Cast<AActor>(BlackboardComponent->GetValueAsObject("Target"));
			AIController->MoveToActor(Player, 50.0f);
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
		[this](AAIController* AIController) 
		{

			AGameModeBase* GameMode = AIController->GetWorld()->GetAuthGameMode();
			const ATagGameGameMode* AIGameMode = Cast<ATagGameGameMode>(GameMode);

			const TArray<ABall*>& BallsList = AIGameMode->GetBalls();

			float MaxDistance = INT_MAX;

			for (ABall* Ball : BallsList)
			{
				if (!Ball->GetAttachParentActor())
				{
					const float Distance = FVector::Distance(AIController->GetPawn()->GetActorLocation(), Ball->GetActorLocation());

					if (Distance < MaxDistance)
					{
						BestBall = Ball;
						MaxDistance = Distance;
					}
				}
			}
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
			AIController->MoveToActor(BestBall, 50.0f);
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

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	BlackboardComponent->InitializeBlackboard(*BlackboardAsset);
}
